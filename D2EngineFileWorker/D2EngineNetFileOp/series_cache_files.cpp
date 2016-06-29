#include "series_cache_files.h"
#include "d2engine_netfile_operator.h"
#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include "d2engine_netfile_utility.h"

D2ENGINE_FILEWORKER_BEG_NS

SeriesFilesCache::SeriesFilesCache(void)
{
}


SeriesFilesCache::~SeriesFilesCache(void)
{
	UnlockAllSeriesFiles();
}

void SeriesFilesCache::LockFile(const std::string &strFileName, const std::string &strCachedFileName)
{
	NetFileOperator *pNetFileOp = NetFileOperator::GetInstance();

	if (m_lockedFilesMap.find(strFileName) == m_lockedFilesMap.end())
	{
		pNetFileOp->LockCacheFileBySeries(this, strCachedFileName);

		m_lockedFilesMap[strFileName] = strCachedFileName;
	}
}

void SeriesFilesCache::UnLockFile(const std::string &strFileName, const std::string &strCachedFileName)
{
	NetFileOperator *pNetFileOp = NetFileOperator::GetInstance();

	if (m_lockedFilesMap.find(strFileName) != m_lockedFilesMap.end())
	{
		pNetFileOp->UnLockCacheFileBySeries(this, strCachedFileName);

		m_lockedFilesMap.erase(strFileName);
	}
}

bool SeriesFilesCache::CacheFile(const std::string &strNetFileName, std::string &strCachedFileName)
{
	// such as e:\, \\x
	if (strNetFileName.length() <= 2)
		return false;

	SeriesOrgCacheFileNameMapIt orgCacheFileNameIt = m_lockedFilesMap.find(strNetFileName);
	if (orgCacheFileNameIt != m_lockedFilesMap.end())
	{
		strCachedFileName = orgCacheFileNameIt->second;
		bool b = IsFileModifyTimeSame(strNetFileName, strCachedFileName);
		if (b){
			return true;
		}
	}
	else
	{
		strCachedFileName = GenerateCachedFileName(strNetFileName);
		LockFile(strNetFileName, strCachedFileName);
	}

	// found
	if (IsNetMapDriverFile(strNetFileName))
	{
		bool b = CopyFileToCache(strNetFileName, strCachedFileName);
		if (b == false)
		{
			UnLockFile(strNetFileName, strCachedFileName);
			return false;
		}
	}

	return true;
}

std::string SeriesFilesCache::GenerateCachedFileName(const std::string &strFileName)
{
	std::string strCacheDir = NetFileOperator::GetInstance()->GetCacheDir();

	if (IsNetMapDriverFile(strFileName) == false)
	{
		return strFileName;
	}
	else
	{
		//  
		std::string strConvertToSubDir = strFileName;

		boost::replace_all(strConvertToSubDir, ":", "");
		boost::replace_all(strConvertToSubDir, "\\\\", "");

		std::stringstream ss;
	
		ss << strCacheDir << "\\" << strConvertToSubDir;
		return ss.str();
	}
	
	
}

void SeriesFilesCache::onNewSeriesLoad()
{
	UnlockAllSeriesFiles();
}

bool SeriesFilesCache::UnlockAllSeriesFiles()
{
	SeriesOrgCacheFileNameMapIt it;
	NetFileOperator *pNetFileOp = NetFileOperator::GetInstance();

	for (it = m_lockedFilesMap.begin(); it != m_lockedFilesMap.end(); it ++)
	{
		pNetFileOp->UnLockCacheFileBySeries(this, it->second);
	}

	m_lockedFilesMap.clear();

	return true;
}


bool SeriesFilesCache::CopyFileToCache(const std::string &strSrcFileName, const std::string &strCachedFileName)
{
	if (strCachedFileName.length() <= 0 ||
		strSrcFileName.length() <= 0) 
		return false;

	boost::filesystem::path cachedFileFullPath(strCachedFileName.c_str());
	boost::filesystem::path cachedFileDir = cachedFileFullPath.parent_path();
	std::string s1 = cachedFileDir.string();

	if (boost::filesystem::exists(cachedFileDir) == false)
	{
		boost::filesystem::create_directories(s1);
	}

	/*
	boost::filesystem::path srcPath(strSrcFileName.c_str());
	boost::system::error_code error_code;
	boost::filesystem::copy_file(srcPath, cachedFileFullPath, error_code);
	*/

	if (IsFileExists(strCachedFileName))
	{
		bool sameFile = IsFileModifyTimeSame(strSrcFileName, strCachedFileName);
		if (sameFile == true)
			return true;

		BOOL b = DeleteFileA(strCachedFileName.c_str());
		if (b == FALSE)
		{
			// someone in use, try next time
			return true;
		}
	}
	
	BOOL cancel = FALSE;
	BOOL bRet = CopyFileExA(strSrcFileName.c_str(), strCachedFileName.c_str(), NULL, NULL, &cancel, COPY_FILE_NO_BUFFERING );
	return (bRet == TRUE);
}


bool SeriesFilesCache::IsNetMapDriverFile(const std::string &strNetFileName)
{
	if (strNetFileName.length() <= 0)
		return false;

	char c1 = strNetFileName.at(0);
	char c2 = strNetFileName.at(1);
	if (c1 == c2 && c1 == '\\')
		return true;

	std::string strDriver(strNetFileName.begin(), strNetFileName.begin()+2);
	NetFileOperator *pNetFileOp = NetFileOperator::GetInstance();
	
	return pNetFileOp->IsNetDriver(strDriver);

}
D2ENGINE_FILEWORKER_END_NS