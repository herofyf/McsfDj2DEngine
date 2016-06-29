#include "d2engine_netfile_operator.h"
#include <Winnetwk.h>
#include "d2engine_netfile_utility.h"
#include <sstream>

D2ENGINE_FILEWORKER_BEG_NS

NetFileOperator *NetFileOperator::m_pInstance = NULL;

NetFileOperator * NetFileOperator::GetInstance()
{
	if (m_pInstance == NULL)
		m_pInstance = new NetFileOperator();

	return m_pInstance;
}

NetFileOperator::NetFileOperator(void)
{
	m_pLockedFilesMutex = new boost::recursive_mutex();

	m_bInited = false;
	m_bTaskRunning = true;
	m_pReactor = NULL;
}


NetFileOperator::~NetFileOperator(void)
{
	if (m_pLockedFilesMutex)
	{
		delete m_pLockedFilesMutex;
		m_pLockedFilesMutex = NULL;
	}

	m_bInited = false;
}

void NetFileOperator::AddNetDriver(std::string strLocalDriver, std::string strRemotePath)
{
	NETRESOURCE NetResource ;
	memset(&NetResource, 0, sizeof(NetResource)) ;
	NetResource.lpProvider		= NULL ;
	NetResource.dwType			= RESOURCETYPE_ANY;

	std::wstring wstr1(strLocalDriver.begin(), strLocalDriver.end()), wstr2(strRemotePath.begin(), strRemotePath.end());
	NetResource.lpLocalName = const_cast<LPTSTR>(wstr1.c_str());
	NetResource.lpRemoteName = const_cast<LPTSTR>(wstr2.c_str());
	DWORD dwLastError = WNetAddConnection2(&NetResource, NULL, NULL, FALSE) ;
}

void NetFileOperator::FindNetDrivers()
{
	m_netDrivers.clear();

#define DEFAULT_PATH_LEN 512
	char  remotePath[DEFAULT_PATH_LEN];
	DWORD remotelen = DEFAULT_PATH_LEN;
	DWORD dwRet = 0;

	std::string strDriver;
	for (char ch ='A'; ch <= 'Z'; ch += 1)
	{
		strDriver = ch;
		strDriver += ':';
		memset(remotePath, 0x0, DEFAULT_PATH_LEN);

		dwRet = WNetGetConnectionA(strDriver.c_str(),remotePath,&remotelen);
		if (remotePath[0] != '\0')
		{
			m_netDrivers.insert(strDriver);

			strDriver.at(0) = strDriver.at(0) + 32;
			m_netDrivers.insert(strDriver);

			if (dwRet != NO_ERROR)
			{
				AddNetDriver(strDriver, remotePath);
			}
		}
	}
}

bool NetFileOperator::IsNetDriver(const std::string &strDriver)
{
	// no lock
	StringSetIt it = m_netDrivers.find(strDriver);
	
	return (it != m_netDrivers.end());
}

void NetFileOperator::Initialize(std::string strCacheDir)
{
	m_strCacheDir = strCacheDir;

	if (m_bInited == false)
	{
		m_bInited = true;

		m_bTaskRunning = true;
		this->activate(THR_NEW_LWP /*| THR_DETACHED*/ );

		FindNetDrivers();
	}
}

void NetFileOperator::Release()
{
	CloseMyTask();

	this->wait();

	delete this;
}

const std::string &NetFileOperator::GetCacheDir()
{
	return m_strCacheDir;
}

bool NetFileOperator::LockCacheFileBySeries(const SeriesFilesCache *pSfCache, const std::string &strCacheFileName)
{
	if (m_pLockedFilesMutex == NULL) return false;

	boost::recursive_mutex::scoped_lock lock(*m_pLockedFilesMutex);

	FileLockedByCachesMapIt it = m_CachesLockedFiles.find(strCacheFileName);
	if (it == m_CachesLockedFiles.end()) 
	{
		SfCachesSet cacheSet;
		cacheSet.insert(pSfCache);
		m_CachesLockedFiles[strCacheFileName] = cacheSet;
	}
	else
	{
		SfCachesSet &cachesSet = it->second;
		cachesSet.insert(pSfCache);
	}
	
	return true;
}

bool NetFileOperator::UnLockCacheFileBySeries(const SeriesFilesCache *pSfCache, const std::string &strCacheFileName)
{
	if (m_pLockedFilesMutex == NULL) return false;

	boost::recursive_mutex::scoped_lock lock(*m_pLockedFilesMutex);

	FileLockedByCachesMapIt it = m_CachesLockedFiles.find(strCacheFileName);
	if (it != m_CachesLockedFiles.end())
	{
		SfCachesSet &cacheSet = (it->second);

		SfCachesSetIt cacheIt = cacheSet.find(pSfCache);
		if (cacheIt != cacheSet.end())
			cacheSet.erase(cacheIt);
	}

	return true;
}

std::string NetFileOperator::DumpTime(pt::ptime ptVal)
{
	using namespace boost::posix_time;
	static std::locale loc(std::cout.getloc(), new time_facet("%Y%m%d_%H%M%S"));
	
	std::basic_stringstream<char> ss;
	ss.imbue(loc);
	ss << ptVal;
	return ss.str();

}

void NetFileOperator::DeleteOlderFiles(const std::string &strDir, const pt::ptime  &now_dt)
{
#ifndef TEST_CASES
#define THREE_DAYS 60 * 60 * 24 * 3
#else
#define THREE_DAYS 10
#endif
	int dirLen = strDir.length();

	if (m_pLockedFilesMutex == NULL ||  dirLen <= 0) 
		return;
	
	WIN32_FIND_DATAA fda;
	std::string strNormDir = strDir, strDirFiles, strTempFileName;

	if (strNormDir.at(dirLen - 1) != '\\') 
	{
		strNormDir  += "\\"; 
	}

	strDirFiles = strNormDir + "*.*";

	HANDLE handle = FindFirstFileA(strDirFiles.c_str(), &fda);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	while (FindNextFileA(handle, &fda))
	{
		// . or ..
		if (fda.cFileName[0] == '.')
		{
			continue;
		}

		if (fda.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strTempFileName = strNormDir;
			strTempFileName += fda.cFileName;
			DeleteOlderFiles(strTempFileName, now_dt);
		}
		else
		{
			strTempFileName = strNormDir;
			strTempFileName += fda.cFileName;

			FILETIME ftCreate, ftAccess, ftWrite;

			if (false == GetFileTimesAttribFt(strTempFileName, ftCreate, ftAccess, ftWrite))
				continue;

			pt::ptime fileLastAccess = pt::from_ftime<pt::ptime>(ftAccess);
			/*std::string st1 = DumpTime(fileLastAccess);
			std::string st2 = DumpTime(now_dt);*/
			pt::time_duration td = now_dt - fileLastAccess;

			if (td.total_seconds() > THREE_DAYS)
			{
				if (m_pLockedFilesMutex)
				{
					boost::recursive_mutex::scoped_lock lock(*m_pLockedFilesMutex);

					FileLockedByCachesMapIt it = m_CachesLockedFiles.find(strTempFileName);
					if (it != m_CachesLockedFiles.end())
					{
						SfCachesSet &caches = it->second;
						if (caches.size() > 0)
							continue;
					}
				}

				DeleteFileA(strTempFileName.c_str());
			}
		}
	}

	FindClose(handle);
	return ;
}

void NetFileOperator::DeleteEmptyDir(const std::string &strDir)
{
	int dirLen = strDir.length();

	if (dirLen <= 0) 
		return;

	WIN32_FIND_DATAA fda;
	std::string strNormDir = strDir, strDirFiles, strTempFileName;

	if (strNormDir.at(dirLen - 1) != '\\') 
	{
		strNormDir  += "\\"; 
	}

	strDirFiles = strNormDir + "*.*";

	HANDLE handle = FindFirstFileA(strDirFiles.c_str(), &fda);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	while (FindNextFileA(handle, &fda))
	{
		if (fda.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (fda.cFileName[0] != '.' )
			{
				std::string strChildDir = strNormDir + fda.cFileName;
				DeleteEmptyDir(strChildDir);
				RemoveDirectoryA(strChildDir.c_str());
			}
		}
	}

	FindClose(handle);
}

int NetFileOperator::handle_timeout(const ACE_Time_Value &current_time, const void *act /* = 0 */)
{
	// to check the time and delete the file exceeds the dt
	// in 

	// calculate the current time
	//ACE_Time_Value now_dt = ACE_OS::gettimeofday();
	pt::ptime  now_dt = pt::second_clock::local_time();
#ifndef TEST_CASES
	if (now_dt.time_of_day().hours() >= 0 && now_dt.time_of_day().hours() < 2)
#endif
	{
		DeleteOlderFiles(m_strCacheDir, now_dt);

		DeleteEmptyDir(m_strCacheDir);
	}

	return 0;
}

int NetFileOperator::close(u_long flags )
{
	// clear resource used by thread
	if (m_pReactor)
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
	
	return 0;
}

int NetFileOperator::svc(void)
{
	if (m_pReactor == NULL)
	{
		m_pReactor = new ACE_Reactor();
	}

#ifndef TEST_CASES
	ACE_Time_Value tv_1hour ( 60 * 60, 0 );
#else
	ACE_Time_Value tv_1hour ( 10, 0 );
#endif
	ACE_Time_Value tv_now ( 0, 0 );
	m_pReactor->schedule_timer(this, 0, tv_now, tv_1hour);

	while (m_bTaskRunning)
	{
		m_pReactor->run_reactor_event_loop();
	}
	
	return 0;
}

void NetFileOperator::CloseMyTask()
{
	m_bTaskRunning = false;

	if (m_pReactor)
	{
		m_pReactor->end_reactor_event_loop();
	}
}

D2ENGINE_FILEWORKER_END_NS