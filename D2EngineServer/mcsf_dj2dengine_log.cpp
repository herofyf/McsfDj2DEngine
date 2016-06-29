#include "stdafx.h"
#include "mcsf_dj2dengine_log.h"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include <boost/date_time/time_facet.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

LoggerManager g_LoggerManager;

LogFile::LogFile()
{
	_fileHandle = INVALID_HANDLE_VALUE;
}

LogFile::~LogFile()
{
	close();
}

void LogFile::close()
{
	if (_fileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_fileHandle);
		_fileHandle = INVALID_HANDLE_VALUE;
	}
}

void LogFile::open(const char *szFile, int flag)
{
	close();

	_fileHandle = CreateFileA(szFile, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

}

bool LogFile::is_open()
{
	return (_fileHandle != INVALID_HANDLE_VALUE);
}

void LogFile::flush()
{

}

void LogFile::write(const char *sz, int size)
{
	if (is_open() == false || sz == NULL || size <= 0)
		return;

	DWORD dwBytesWritten = 0;

	WriteFile(_fileHandle, sz, size, &dwBytesWritten, NULL);
}

LoggerManager::LoggerManager() : m_dwOption(0), 
	m_bEnabled(false), m_nCurFileSize(0), m_nFileIndex(-1)
{

}

void LoggerManager::Enable()
{
	if (m_bEnabled == false)
	{
		this->activate(THR_NEW_LWP | THR_JOINABLE);
		m_bEnabled = true;
	}
}


void LoggerManager::Uninit()
{
	if (m_bEnabled)
	{
		StopTask();
		m_bEnabled = false;
	}
}

void LoggerManager::ResetLogFile()
{
	m_nCurFileSize = 0;

	std::string sdate = boost::gregorian::to_iso_extended_string( 
		boost::gregorian::day_clock::local_day());

	std::string dir = "log/";
	boost::filesystem::path filePath(dir + sdate + "/");

	if(!boost::filesystem::exists(filePath))
		boost::filesystem::create_directories(filePath);

	if (m_nFileIndex == -1)
	{
		boost::filesystem::directory_iterator dirIter(filePath), dirEnd;
		boost::regex e(".+\\.*log$");
		for(; dirIter != dirEnd; ++dirIter)
		{
			std::string sPath = dirIter->path().leaf().string();
			if(boost::filesystem::is_regular_file(*dirIter)
				&& boost::regex_match(sPath, e))
			{
				int iTmp = 0;
				sscanf(sPath.c_str(), "%d", &iTmp);
				if(iTmp > m_nFileIndex)
					m_nFileIndex = iTmp;
			}
		}
	}

	++ m_nFileIndex;

	filePath /= (boost::lexical_cast<std::string>(m_nFileIndex) + ".log");
	m_file.close();

	m_file.open(filePath.string().c_str(), std::ofstream::trunc);
	
	
}
void LoggerManager::WriteLogFile(const std::string &log)
{
	try
	{
		if (m_file.is_open()== false)
			ResetLogFile();

		if (m_file.is_open())
		{
			m_file.write(log.c_str(), log.length());
			m_file.write("\r\n", 2);
			m_file.flush();

			m_nCurFileSize += log.length();
		}

		if (m_nCurFileSize > LOG_MAX_FILE_SIZE)
			ResetLogFile();
	}
	catch(...)
	{
		std::cout << "Log exception happends" << std::endl;
	}
}



int LoggerManager::svc(void)
{
	LogMessage *pLogMsg = NULL;
	while(true)
	{
		ACE_Message_Block *mb = NULL;
		if( getq(mb) == -1 )
		{
			break;
		}

		if( mb->msg_type() == ACE_Message_Block::MB_STOP )
		{
			mb->release();
			break;
		}

		if (NULL == mb)
		{
			continue;
		}

		memcpy(&pLogMsg, mb->rd_ptr(), sizeof(pLogMsg));

		WriteLogFile(pLogMsg->LogText);
		delete pLogMsg;
		
		mb->release();

	}
	return 0;
}

int LoggerManager::close(u_long flags /* = 0 */)
{
	return 0;
}

std::string LoggerManager::GetCurTimeText()
{
	boost::posix_time::ptime now = (boost::posix_time::microsec_clock::local_time());

	/*std::stringstream ss;
	ss << now.date().year() << "-" << now.date().month() << "-" << now.date().day() << " "
	   << now.time_of_day().hours() << ":" << now.time_of_day().minutes() << ":" << now.time_of_day().seconds() << " "
	   << now.time_of_day().*/
	static std::locale loc(std::cout.getloc(), new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S.%f"));
	
	std::basic_stringstream<char> ss;
	ss.imbue(loc);
	ss << now;
	return ss.str();
		
}

std::string LoggerManager::GetCurThreadInf()
{

	std::string strThreadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
	/*unsigned long threadNumber = 0;
	sscanf(threadId.c_str(), "%lx", &threadNumber);
	return threadNumber;*/
	return strThreadId;
}

std::string LoggerManager::GetLogHeader(const char *szTag)
{
	std::stringstream ssLog;

	ssLog << GetCurTimeText() << "-" << GetCurThreadInf() << "-< " << szTag << " >:";
	return ssLog.str();
}

void LoggerManager::LogInfo(std::string funName, std::string log)
{
	if (m_dwOption < LOG_LEVEL_INFO) return;

	std::stringstream ssLog;

	ssLog	<< GetLogHeader("INFO") << funName << " >> " <<  log;

	LogMsg(ssLog.str());
}

void LoggerManager::LogError(std::string funName, std::string log)
{
	if (m_dwOption < LOG_LEVEL_ERROR) return;

	std::stringstream ssLog;

	ssLog	<< GetLogHeader("ERROR") << funName << " >> " <<  log;
	
	LogMsg(ssLog.str());
		
}

void LoggerManager::LogDebug(std::string funName, std::string log)
{
	if (m_dwOption < LOG_LEVEL_DEBUG) return;

	std::stringstream ssLog;

	ssLog	<< GetLogHeader("DEBUG") << funName << " >> " <<  log;

	LogMsg(ssLog.str());
}

void LoggerManager::LogMsg(const std::string &log)
{
	LogMessage *pLogMsg = new LogMessage;
	pLogMsg->LogText = log;

	ACE_Message_Block *mb = new ACE_Message_Block(sizeof(pLogMsg));

	memcpy(mb->wr_ptr(), &pLogMsg, sizeof(pLogMsg));

	putq(mb);
}

void LoggerManager::StopTask()
{
	ACE_Message_Block *mb = new ACE_Message_Block();
	mb->msg_type( ACE_Message_Block::MB_STOP );
	this->putq( mb );
}