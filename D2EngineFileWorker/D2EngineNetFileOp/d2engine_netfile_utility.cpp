#include "d2engine_netfile_utility.h"

D2ENGINE_FILEWORKER_BEG_NS

bool GetFileTimesAttribFt(const std::string &strFile1, FILETIME &ftCreate, FILETIME &ftAccess, FILETIME &ftWrite)
{
	HANDLE hFile;
	DWORD dwRet;

	hFile = CreateFileA(strFile1.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	FILETIME ftUtcCreate, ftUtcAccess, ftUtcWrite;

	BOOL bRet = GetFileTime(hFile, &ftUtcCreate, &ftUtcAccess, &ftUtcWrite);
	if (bRet == FALSE)
		goto ERR_HANDLE;

	FileTimeToLocalFileTime(&ftUtcCreate, &ftCreate);
	FileTimeToLocalFileTime(&ftUtcAccess, &ftAccess);
	FileTimeToLocalFileTime(&ftUtcWrite, &ftWrite);

	CloseHandle(hFile);
	return true;

ERR_HANDLE:
	CloseHandle(hFile);
	return false;
}

bool GetFileTimesAttrib(const std::string &strFile1, SYSTEMTIME &stLocCreate, SYSTEMTIME &stLocAccess, SYSTEMTIME &stLocWrite)
{
	HANDLE hFile;
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stUTC, stLocal;
	DWORD dwRet;

	hFile = CreateFileA(strFile1.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	BOOL bRet = GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);
	if (bRet == FALSE)
		goto ERR_HANDLE;

	FileTimeToSystemTime(&ftCreate, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocCreate);

	FileTimeToSystemTime(&ftAccess, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocAccess);

	FileTimeToSystemTime(&ftWrite, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocWrite);

	CloseHandle(hFile);
	return true;

ERR_HANDLE:
	CloseHandle(hFile);
	return false;
}

bool IsFileModifyTimeSame(const std::string &strFile1, const std::string &strFile2)
{
	SYSTEMTIME st1LocCreate, st2LocCreate;
	SYSTEMTIME st1LocAccess, st2LocAccess;
	SYSTEMTIME st1LocWrite, st2LocWrite;

	bool b1 = GetFileTimesAttrib(strFile1, st1LocCreate, st1LocAccess, st1LocWrite);
	if (b1)
	{
		bool b2 = GetFileTimesAttrib(strFile2, st2LocCreate, st2LocAccess, st2LocWrite);
		if (b2)
		{
			bool b3 = (memcmp(&st2LocWrite, &st1LocWrite, sizeof(SYSTEMTIME)) == 0);
			return b3;
		}
	}
	
	return true;
}

bool IsFileExists(const std::string &strFile1)
{
	WIN32_FIND_DATAA FindFileData;
	HANDLE handle = FindFirstFileA(strFile1.c_str(), &FindFileData) ;
	bool found = (handle != INVALID_HANDLE_VALUE);
	if(found) 
	{
		FindClose(handle);
	}
	return found;
}

D2ENGINE_FILEWORKER_END_NS