
#include "mcsf_dj2dengine_containee.h"
#include "CommunicationMessage.pb.h"
#include <Windows.h>
#include "system_global_configuration.h"
#include "series_image_id.h"
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

IMPLEMENT_CONTAINEE(Mcsf2DEngineContainee);

Mcsf2DEngineContainee *Mcsf2DEngineContainee::m_p2DEngineContainee = NULL;

Mcsf2DEngineContainee::Mcsf2DEngineContainee(void) :
		m_sName(""), m_bExit(false), m_p2DEngineCmdHandler(NULL), m_p2DEngineEvtHandler(NULL),
		m_p2DEngineDataHandler(NULL), m_bHasEvtSendChannel(false)
{
	Mcsf2DEngineContainee::m_p2DEngineContainee = this;
}

Mcsf2DEngineContainee *Mcsf2DEngineContainee::GetInstance()
{
	return m_p2DEngineContainee;
}

Mcsf2DEngineContainee::~Mcsf2DEngineContainee(void)
{
	ReleaseResource();
}

Mcsf::ICommunicationProxy * Mcsf2DEngineContainee::m_pICommProxy = NULL;

Mcsf::ICommunicationProxy * Mcsf2DEngineContainee::GetCommProxy()
{
	return m_pICommProxy;

}
void Mcsf2DEngineContainee::SetCommunicationProxy(Mcsf::ICommunicationProxy* pContainer)
{
	if (NULL == pContainer)
	{
		LOG_CONTAINEE_ERROR("Error:the param is null! in SetContainer(*pC)");
		return;
	}

	Mcsf2DEngineContainee::m_pICommProxy = pContainer;
	LOG_CONTAINEE_INFO( "Info:Set Database container ok!" );
}

void Mcsf2DEngineContainee::NotifyClient_ReportSeriesStatus(const PageSeriesId *pSeriesId, const StudySeries *pStudySeries)
{
	if (pStudySeries == NULL || pSeriesId == NULL)
		return;

	McsfCommunication::ReportStatusInformation reportStatusInf;

	reportStatusInf.set_reporttype(McsfCommunication::ReportSeriesStatus);

	McsfCommunication::ImagePosId *pImagePosId = reportStatusInf.mutable_imageposid();
	if (pImagePosId == NULL)
		return;

	McsfCommunication::ReportSeriesStatusArgs *pRepSeriesStatusArgs = reportStatusInf.mutable_seriesstatus();
	if (pRepSeriesStatusArgs == NULL)
		return;
	
	pSeriesId->BuildValue(pImagePosId);
	pRepSeriesStatusArgs->set_firstshownimageindex(pStudySeries->GetFirstCellImageIndex());
	pRepSeriesStatusArgs->set_cellsnum(pStudySeries->GetCellsCount());
	pRepSeriesStatusArgs->set_imagescount(pStudySeries->GetSeriesImagesCount());

	std::string strResult;
	bool b = reportStatusInf.SerializeToString(&strResult);
	if (b && strResult.length() >0)
	{
		Mcsf::CommandContext  cmdTxt;
		cmdTxt.iCommandId = McsfCommunication::ReportStatus;
		cmdTxt.sReceiver = pStudySeries->GetCommReceiver();
		cmdTxt.sSender = GetName();
		cmdTxt.sSerializeObject = strResult;
		m_pICommProxy->AsyncSendCommand(&cmdTxt);
	}
}

void Mcsf2DEngineContainee::NotifyClient_ReportImageStatusMsg(const std::string &strReceiver, const std::string &strMsg) const
{
	if (strMsg.length() <= 0)
		return;

	Mcsf::CommandContext  cmdTxt;
	cmdTxt.iCommandId = McsfCommunication::ReportStatus;
	cmdTxt.sReceiver = strReceiver;
	cmdTxt.sSender = GetName();
	cmdTxt.sSerializeObject = strMsg;
	m_pICommProxy->AsyncSendCommand(&cmdTxt);
}

void Mcsf2DEngineContainee::NotifyClient_ChangeCellImageMouseCursor(const SeriesImageId *pSeriesImageId, MouseCursorType mouseCursorType)
{
	if (pSeriesImageId)
	{
		try
		{
			McsfCommunication::ReportStatusInformation reportStatusInf;

			McsfCommunication::ImagePosId *pImagePosId = reportStatusInf.mutable_imageposid();
			if (pImagePosId == NULL)
				return;

			McsfCommunication::ReportMouseCursorArgs *pReportMouseArgs = reportStatusInf.mutable_mousecursor();
			if (pReportMouseArgs == NULL)
				return;

			pSeriesImageId->BuildValue(pImagePosId);
			McsfCommunication::MouseCursorType mcsfMouseCursorType = McsfCommunication::MouseCursorDefault;
			switch(mouseCursorType)
			{
			case MouseCursorWait:
				mcsfMouseCursorType = McsfCommunication::MouseCursorWait;
				break;
			case MouseCursorMagnifyGlass:
				mcsfMouseCursorType = McsfCommunication::MouseCursorMagnifyGlass;
				break;
			default:
				mcsfMouseCursorType = McsfCommunication::MouseCursorDefault;
				break;
			}

			pReportMouseArgs->set_mousetype(mcsfMouseCursorType);
		}
		catch(...)
		{
			LOG_CONTAINEE_ERROR( "NotifyEvent_ChangeCellImageMouseCursor exception." );
		}
	}
}


void Mcsf2DEngineContainee::Startup()
{
	try
	{
		CreateDirectoryA(TEMP_DIR_NAME, NULL);
		CreateDirectoryA(TEMP_IMAGE_DIR_NAME, NULL);
		CreateDirectoryA(TEMP_CACHED_DIR_NETIMAGES, NULL);
		// check graphic option
		SYS_GLOBAL_RES.Init();

		Mcsf::ICommunicationProxy *pICommProxy = Mcsf2DEngineContainee::m_pICommProxy;
		if (NULL == pICommProxy)
		{
			LOG_CONTAINEE_ERROR( "Initial Communication Proxy failed!" );
			return;
		}

		//m_pDatabase->SetAutoNotifyOn(pCommProxy);

		//register
		LOG_INFO( "Info:Communication Proxy initialized ok!" );
		
		m_p2DEngineCmdHandler = new Mcsf2DEngineCommandHandler(3);
		m_p2DEngineEvtHandler = new Mcsf2DEngineEventHandler();
		m_p2DEngineDataHandler = new Mcsf2DEngineDataHandler();

		int commands[] = {
				McsfCommunication::OpenImage,
				McsfCommunication::LoadSeries,
				McsfCommunication::CloseSeries,
				McsfCommunication::ChangeToolType,
				McsfCommunication::Mouse,
				McsfCommunication::Keyboard, 
				McsfCommunication::SeriesSettings,
				McsfCommunication::SiteSettings,
				McsfCommunication::ResetImage,
		};
		// to register command which id 20000
		for (int i =0; i < sizeof(commands)/ sizeof(commands[0]); i ++)
		{
			pICommProxy->RegisterCommandHandler(commands[i], m_p2DEngineCmdHandler);
		}
		// to register event which id 2002
		//m_pCommProxy->RegisterEventHandler(0, InstanceChanged, m_p2DEngineEvtHandler);

		// to register data handle while there have new big data
		pICommProxy->RegisterDataHandler(m_p2DEngineDataHandler);

		Mcsf::CommunicationProxy *pCommunicationProxy = dynamic_cast<Mcsf::CommunicationProxy *>(pICommProxy);
		if (pCommunicationProxy && pCommunicationProxy->GetSenderChannelId() > 0)
		{
			m_bHasEvtSendChannel = true;
		}
	}
	catch (...)
	{
		LOG_CONTAINEE_ERROR("Startup() error!");
	}
}

void Mcsf2DEngineContainee::ReleaseResource()
{
	Mcsf::ICommunicationProxy *pCommProxy = Mcsf2DEngineContainee::m_pICommProxy;
	if (pCommProxy == NULL)
		return;

	if (m_p2DEngineDataHandler)
	{
		pCommProxy->UnRegisterDataHandler();
		delete m_p2DEngineDataHandler;
		m_p2DEngineDataHandler = NULL;
	}

	if (m_p2DEngineEvtHandler )
	{
		pCommProxy->UnRegisterAllEventHandlers();
		delete m_p2DEngineEvtHandler;
		m_p2DEngineEvtHandler = 0;
	}

	if (m_p2DEngineCmdHandler)
	{
		pCommProxy->UnRegisterAllCommandHandlers();
		delete m_p2DEngineCmdHandler;
		m_p2DEngineCmdHandler = NULL;
	}

	SYS_GLOBAL_RES.Fini();

	//google::protobuf::ShutdownProtobufLibrary();
}

/*
  There has stop command handler to process shutdown command from dispatch server.
  m_Event signal. then this method to be called. and last CommunicationProxy to be called
  while CommunicationProxy to be deleted, all of reactor thread && clients && server task object all
  be deleted

*/
bool Mcsf2DEngineContainee::Shutdown(bool bReboot)
{
	// to quit DoWork if there has loop procedure
	m_bExit = true;

	ReleaseResource();

	return false;
}

void Mcsf2DEngineContainee::SetName(const std::string& sName)
{
	m_sName = sName;
}

const std::string& Mcsf2DEngineContainee::GetName() const
{
	return m_sName;
}

const std::string &Mcsf2DEngineContainee::GetCustConfigFileName()
{
	return m_sCustConfigFileName;
}
void Mcsf2DEngineContainee::SetCustomConfigFile(const std::string& sConfigFile)
{
	m_sCustConfigFileName = sConfigFile;
}

void Mcsf2DEngineContainee::DoWork()
{
	LOG_CONTAINEE_INFO(
		"Info:McsfDatabaseBEContainee::DoWork()! start the Database provider Thread!");

	try
	{
		Mcsf::ICommunicationProxy *pCommProxy = Mcsf2DEngineContainee::m_pICommProxy;
		if (NULL == pCommProxy)
		{
			LOG_CONTAINEE_ERROR("NULL == m_pCommProxy");
			return;
		}

		if(0 != pCommProxy->SendSystemEvent(
			"",
			Mcsf::SYSTEM_COMMAND_EVENT_ID_COMPONENT_READY, 
			m_pICommProxy->GetName()))
		{
			LOG_CONTAINEE_ERROR(
				"The event send to System manager fail,Please restart the DatabaseBEContainee");
		}
	}
	catch (...)
	{
		LOG_CONTAINEE_ERROR("Exception at DoWork");
	}

	return;
}

MCSF_DJ2DENGINE_END_NAMESPACE
