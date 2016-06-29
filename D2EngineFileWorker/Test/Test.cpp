// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ace/ACE.h>
#include "d2engine_netfile_comm.h"
#include "d2engine_netfile_operator.h"

using namespace D2ENGINE_FILEWORKER_NS;
int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
	int n;
	NetFileOperator *pNetFileOp = NetFileOperator::GetInstance();
	pNetFileOp->Initialize("..\\..\\..\\..\\..\\UIH\\bin_debug\\Temp\\CachedNetFiles");

	SeriesFilesCache scacheFiles;
	scacheFiles.onNewSeriesLoad();

	std::string strFileName = "Z:\\ACE64d.lib";
	std::string strNewFileName ;
	scacheFiles.CacheFile(strFileName, strNewFileName);
	//scacheFiles.onNewSeriesLoad();
	//strFileName = "E:\\temp\\ddd\ssdf\TraceManager.h";
	strFileName = "Z:\\ACE64d.lib";
	scacheFiles.CacheFile(strFileName, strNewFileName);
	
	std::cin >> n;
	
	scacheFiles.onNewSeriesLoad();
	

	pNetFileOp->Release();

	std::cin >> n;
	return 0;
}

