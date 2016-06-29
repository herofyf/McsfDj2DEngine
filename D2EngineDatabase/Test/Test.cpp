// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <deque>
#include <string>
#include "d2engine_dbwrapper.h"
#include "series_rec.h"
int _tmain(int argc, _TCHAR* argv[])
{
	std::string str1 ="192.168.130.244";
	std::string str2 = "DicomDB";
	std::string str3 ="admin";
	std::string str4 ="admin";
	SqlServerWrapper *pDb = SqlServerWrapper::Instance();
	pDb->Initialize(str1, str2, str3, str4);
	
	SeriesRecPtr sr = pDb->GetSeriesByName(str1);
	ImageRecPtrList images = pDb->GetImagesBySeriesId(729);
	pDb->Release();
	return 0;
}

