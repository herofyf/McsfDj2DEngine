// This is the main DLL file.
#include "Stdafx.h"
#include <sstream>
#include "d2engine_dbwrapper.h"
#include "ado2.h"
#include "image_rec_operator.h"
#include "series_rec_operator.h"

SqlServerWrapper *SqlServerWrapper::m_pInstance = NULL;

SqlServerWrapper::SqlServerWrapper()
{
	m_pDbMutext = new boost::recursive_mutex();

	m_pDatabase = NULL;

	m_bInit = false;
}

SqlServerWrapper::~SqlServerWrapper()
{
	if (m_pDatabase)
	{
		m_pDatabase->Close();
		delete m_pDatabase;
		m_pDatabase = NULL;
	}

	if (m_pDbMutext)
	{
		delete m_pDbMutext;
		m_pDbMutext = NULL;
	}
}

SqlServerWrapper *SqlServerWrapper::Instance()
{
	if (m_pInstance == NULL)
		m_pInstance = new SqlServerWrapper();

	return m_pInstance;
}

bool SqlServerWrapper::Initialize(const std::string &strIp, const std::string &strDbName, const std::string &strUserName, const std::string &strUserPwd)
{
	try
	{
		if (m_pDatabase == NULL)
			m_pDatabase = new CADODatabase();

		if (m_bInit)
		{
			if (m_strIp == strIp && m_strDbName == strDbName && m_strUserName == strUserName && m_strUserPwd == strUserPwd)
				return false;
		}

		std::stringstream ssConnect;
		ssConnect << "Provider=MSDASQL;Driver={SQL Server};Server=" << strIp 
			<< ";Database=" << strDbName 
			<< ";Uid=" << strUserName 
			<< ";Pwd=" << strUserPwd;

		std::string strConnect = ssConnect.str();
		std::wstring wstrConnect(strConnect.begin(), strConnect.end());
		m_sConnectionString = wstrConnect;
		m_bInit = m_pDatabase->Open(m_sConnectionString.c_str());
		if (m_bInit == true)
		{
			m_strIp = strIp;
			m_strDbName = strDbName;
			m_strUserName = strUserName;
			m_strUserPwd = strUserPwd;
			return true;
		}

	}
	catch(...)	
	{

	}

	return false;
}

bool SqlServerWrapper::QueryRecord(const std::string &strQuery, boost::shared_ptr<CADORecordset> &retRecordPtr)
{
	if (m_bInit == false)
		return false;

	boost::recursive_mutex::scoped_lock lock(*m_pDbMutext);
	try
	{
		if (m_pDatabase->IsOpen() == FALSE)
		{
			m_pDatabase->Open(m_sConnectionString.c_str());
		}

		retRecordPtr = boost::make_shared<CADORecordset>(m_pDatabase);
		CString cstrQuery = strQuery.c_str();
		return retRecordPtr->Open(cstrQuery);
	}
	catch(...)
	{
	}

	return false;
}

SeriesRecPtr SqlServerWrapper::GetSeriesByName(const std::string &strSeriesName)
{
	// to 
	SeriesRecOperator sop;

	return sop.GetSeriesByName(strSeriesName);
}

ImageRecPtrList SqlServerWrapper::GetImagesBySeriesId(int seriesId)
{
	ImageRecOperator iop;

	return iop.GetImagesBySeriesId(seriesId);
}

bool SqlServerWrapper::Release()
{
	delete this;
	return true;
}
