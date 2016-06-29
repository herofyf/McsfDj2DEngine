#include "system_global_configuration.h"
#include "graphics_imager.h"
#include "mcsf_dj2dengine_log.h"
#include "image_drawing_theme_template.h"
#include "dicom_image_helper.h"
#include "mcsf_dj2dengine_containee.h"
#include "McsfContainer\mcsf_constant_fetchor.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

SystemGlobalResource g_SystemGlobalResource;

#ifdef USE_MCSF_MYSQL_TEST
Mcsf::IDatabasePtr SystemGlobalResource::m_pDatabase;
Mcsf::IDatabaseFactory::IDBPointer SystemGlobalResource::m_pDBPointer;

Mcsf::IDatabasePtr SystemGlobalResource::GetDatabasePtr()
{
	return m_pDatabase;
}

#else
SqlServerWrapper *SystemGlobalResource::m_pDatabase;

SqlServerWrapper *SystemGlobalResource::GetDatabasePtr()
{
	return m_pDatabase;
}

#endif

NetFileOperator *SystemGlobalResource::m_pNetFileOperator = NULL;

NetFileOperator *SystemGlobalResource::GetNetFileOperatorPtr()
{
	return m_pNetFileOperator;
}

std::string SystemGlobalResource::m_strDbIp;
std::string SystemGlobalResource::m_strDbName;
std::string SystemGlobalResource::m_strDbUser;
std::string SystemGlobalResource::m_strDbPassword;

int SystemGlobalResource::m_nDbPort;

CConfigXmlParse SystemGlobalResource::m_configXmlParse;

int SystemGlobalResource::m_dwDumpImageOption = -1;
int SystemGlobalResource::m_dwLogOption = 1;
GraphicsImager *SystemGlobalResource::m_pDefBmpGraphicsImager = NULL;
FontsSizeInformationCache *SystemGlobalResource::m_pFontsSizeCache = NULL;
Gdiplus::PixelFormat SystemGlobalResource::m_MediateImagePixelFormat = PixelFormat16bppRGB555;

bool SystemGlobalResource::Init()
{
	// enable log thread
	g_LoggerManager.Enable();

	std::string strCustConfigFileName = Mcsf2DEngineContainee::GetInstance()->GetCustConfigFileName();

	std::string strRepositoryPathFile = Mcsf::ConstantFetchor::GetAppPath() + strCustConfigFileName;

	if (LoadAppConfig(strRepositoryPathFile) == false)
	{
		LoadAppConfig(strCustConfigFileName);
	}

	g_LoggerManager.SetLogOption(m_dwLogOption);

	if (m_pDefBmpGraphicsImager == NULL)
	{
		m_pDefBmpGraphicsImager = new GraphicsImager(1, 1, m_MediateImagePixelFormat);
		Color color(0, 0, 0);
		m_pDefBmpGraphicsImager->Clear(color);
	}

	m_pFontsSizeCache = FontsSizeInformationCache::Instance();
	if (m_pFontsSizeCache)
	{
		// insert some defined template
	}
	
	InitDb();

	InitNetFileOp();

	DicomImageHelper::InitDecoder();
	// to set 
	return true;
}

bool SystemGlobalResource::IsDumpImageOn()
{
	return (m_dwDumpImageOption > 0);
}
bool SystemGlobalResource::LoadAppConfig(std::string strConfigFile)
{
	bool b = m_configXmlParse.LoadConfigXmlFile(strConfigFile);
	if (b)
	{
		std::string strTag, strVal;

		strTag ="LogLevel";
		b = m_configXmlParse.GetContentByXmlTag(strTag, strVal);

		if (b && strVal.length() > 0)
		{
			m_dwLogOption = atoi(strVal.c_str());
		}

		strTag = "DumpImage";
		b = m_configXmlParse.GetContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			m_dwDumpImageOption = atoi(strVal.c_str());			
		}

		int dwMediateImageQuality = 0;
		strTag = "MedImageQuality";
		b = m_configXmlParse.GetContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			dwMediateImageQuality = atoi(strVal.c_str());			

			if (dwMediateImageQuality == 0)
				m_MediateImagePixelFormat = PixelFormat24bppRGB;
			else if (dwMediateImageQuality > 0)
				m_MediateImagePixelFormat = PixelFormat32bppRGB;
			else if (dwMediateImageQuality < 0)
				m_MediateImagePixelFormat = PixelFormat16bppRGB555;
		}

		strTag = "DbServerIp";
		b = m_configXmlParse.FindContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			m_strDbIp = strVal;		
		}

		strTag = "DbName";
		b = m_configXmlParse.FindContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			m_strDbName = strVal;		
		}

		strTag = "DbUser";
		b = m_configXmlParse.FindContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			m_strDbUser = strVal;		
		}

		strTag = "DbPassword";
		b = m_configXmlParse.FindContentByXmlTag(strTag, strVal);
		if (b && strVal.length() > 0)
		{
			m_strDbPassword = strVal;		
		}
		return true;
	}

	return false;
}

Gdiplus::PixelFormat SystemGlobalResource::MediateImagePixelFormat()
{
	return m_MediateImagePixelFormat;
}
GraphicsImager *SystemGlobalResource::DefBmpGraphicsImager()
{
	return m_pDefBmpGraphicsImager;
}

bool SystemGlobalResource::Fini()
{
	if (m_pFontsSizeCache)
	{
		m_pFontsSizeCache->Release();
	}

	if (m_pDefBmpGraphicsImager)
	{
		delete m_pDefBmpGraphicsImager;
		m_pDefBmpGraphicsImager = NULL;
	}
	// close log thread
	g_LoggerManager.Uninit();

	FinalDb();

	FinaNetFileOp();

	DicomImageHelper::FiniDecoder();
	return true;
}

bool SystemGlobalResource::InitDb()
{
#ifdef USE_MCSF_MYSQL_TEST
	m_pDBPointer = Mcsf::IDatabaseFactory::Instance();
	m_pDatabase = m_pDBPointer->CreateDBWrapper();
	// appdata/sysconfig/McsfDatabaseConfig.xml
	if (m_pDatabase == NULL || !m_pDatabase->Initialize())
	{
		LOG_CONTAINEE_ERROR( "Initial DB Wrapper failed!" );
		assert(0);
		exit(0);
	}

#else
	m_pDatabase = SqlServerWrapper::Instance();
	if (m_pDatabase->Initialize(m_strDbIp, m_strDbName, m_strDbUser, m_strDbPassword) == false)
	{
		assert(0);
		exit(0);
	}
#endif
	return true;
}

bool SystemGlobalResource::FinalDb()
{
#ifdef USE_MCSF_MYSQL_TEST
	if (m_pDatabase)
	{
		m_pDatabase->Finalize();
	}
#else
	if (m_pDatabase)
	{
		m_pDatabase->Release();
	}
	
#endif

	return true;
}

bool SystemGlobalResource::InitNetFileOp()
{
	m_pNetFileOperator = NetFileOperator::GetInstance();

	m_pNetFileOperator->Initialize(TEMP_CACHED_DIR_NETIMAGES);

	return true;
}

bool SystemGlobalResource::FinaNetFileOp()
{
	if (m_pNetFileOperator)
	{
		m_pNetFileOperator->Release();
	}
	return true;
}

MCSF_DJ2DENGINE_END_NAMESPACE
