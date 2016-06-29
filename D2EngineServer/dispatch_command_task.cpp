#include <assert.h>
#include "dispatch_command_task.h"
#include "mcsf_dj2dengine_command_handler.h"
#include "CommunicationMessage.pb.h"
#include "site_work_task.h"
#include "study_series_command_request_args.h"
#include "mcsf_config_xml_parse.h"
#include "study_series_command_request_args_note.h"
#include "mcsf_charset_converter.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

CommandsDispatcher *CommandsDispatcher::m_pInstance = NULL;

CommandsDispatcher *CommandsDispatcher::GetInstance()
{
	if (m_pInstance == NULL)	
	{
		m_pInstance = new CommandsDispatcher();
	}

	return m_pInstance;
}

CommandsDispatcher::CommandsDispatcher() 
{
	m_bStopDispatch = false;

	ACE_Thread::spawn_n(3, &CommandsDispatcher::svc, this);
}


CommandsDispatcher::~CommandsDispatcher(void)
{
	m_bStopDispatch = true;
}

#define SET_IMAGE_REQUEST_IMAGEUID(a, b)												\
	p##a->iCommandId = b;																\
	p##a->sReceiver = commandInfo->sReceiver;											\
	p##a->sSender   = commandInfo->sSender;												\
	p##a->imagePosId.siteId = pImageRequest->imageposid().siteid();						\
	p##a->imagePosId.pageId = pImageRequest->imageposid().pageid();						\
	p##a->imagePosId.seriesPagePos = pImageRequest->imageposid().seriespagepos();		\
	p##a->imagePosId.seriesId      = pImageRequest->imageposid().seriesid();				\
	p##a->imagePosId.imageCellPos  = pImageRequest->imageposid().imagecellpos();

void CommandsDispatcher::DispatchSiteTaskCommunicationRequest(const Mcsf::CommandContext *commandInfo)
{
	bool validCmd = false;
	// to find the site task. 
	McsfCommunication::ImageRequest *pImageRequest = new McsfCommunication::ImageRequest();
	bool b =(*pImageRequest).ParseFromString(commandInfo->sSerializeObject);
	if (b == false) return;

	SiteWorkTask *pSiteWorkTask = NULL;

	if (pImageRequest->has_imageposid() == false)
		return;
	
	std::string siteId = pImageRequest->imageposid().siteid();

	if (pSiteWorkTask == NULL)
	{
		ACE_Guard<ACE_Thread_Mutex> lock(m_siteWorkTaskMutex);

		SiteWorkTaskMapItor findIt = m_siteWorkTaskMap.find(siteId);
		if (findIt == m_siteWorkTaskMap.end())
		{
			pSiteWorkTask = new SiteWorkTask(this, siteId);
			m_siteWorkTaskMap[siteId] = pSiteWorkTask;
		}
		else
		{
			pSiteWorkTask = m_siteWorkTaskMap[siteId];
		}
	}
	
	ImageCommRequestArgs *pImageCommRequestArgs = NULL;

	switch(commandInfo->iCommandId)
	{
	case McsfCommunication::LoadSeries:
		{
			pImageCommRequestArgs = new ImageCommRequestArgs();
			SET_IMAGE_REQUEST_IMAGEUID(ImageCommRequestArgs, LoadSeries)
		}
		break;
	case McsfCommunication::OpenImage:
		{
			if (pImageRequest->has_openimageinformation() == false) return;

			OpenImageRequestArgs *pOpenImageRequestArgs = new OpenImageRequestArgs();
			
			SET_IMAGE_REQUEST_IMAGEUID(OpenImageRequestArgs, OpenImage)
			pOpenImageRequestArgs->offset = pImageRequest->openimageinformation().offset();
			pOpenImageRequestArgs->imageHeight = pImageRequest->openimageinformation().imageheight();
			pOpenImageRequestArgs->imageWidth = pImageRequest->openimageinformation().imagewidth();
			pOpenImageRequestArgs->isAbsOffset = pImageRequest->openimageinformation().has_isabsoffset() ?
									pImageRequest->openimageinformation().isabsoffset() : false;
			pOpenImageRequestArgs->cellsNum = pImageRequest->openimageinformation().has_cellsnum() ? 
				pImageRequest->openimageinformation().cellsnum() : 1;

			pImageCommRequestArgs = pOpenImageRequestArgs;
		}

		break;
	case McsfCommunication::CloseSeries:
		{
			pImageCommRequestArgs = new ImageCommRequestArgs();
			SET_IMAGE_REQUEST_IMAGEUID(ImageCommRequestArgs, CloseSeries)
		}
		break;
	case McsfCommunication::ResetImage:
		{
			pImageCommRequestArgs = new ImageCommRequestArgs();
			SET_IMAGE_REQUEST_IMAGEUID(ImageCommRequestArgs, ResetImage)
		}
		break;
	case McsfCommunication::ChangeToolType:
		{
			if (pImageRequest->has_toolinformation() == false) return;

			const McsfCommunication::ToolInformation &toolInformation = pImageRequest->toolinformation();
			if (toolInformation.tooltype() == McsfCommunication::LineTool  ||
				toolInformation.tooltype() == McsfCommunication::AngleTool ||
				toolInformation.tooltype() == McsfCommunication::CircleTool ||
				toolInformation.tooltype() == McsfCommunication::FreeHand)
			{
				ToolChgRequestArgs *pToolChgRequestArgs = new ToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(ToolChgRequestArgs, ChangeToolType)
				
				if (toolInformation.tooltype() == McsfCommunication::LineTool)
					pToolChgRequestArgs->toolType = LineToolType;
				else if (toolInformation.tooltype() == McsfCommunication::AngleTool)
					pToolChgRequestArgs->toolType = AngleToolType;
				else if (toolInformation.tooltype() == McsfCommunication::CircleTool)
					pToolChgRequestArgs->toolType = CircleToolType;
				else if (toolInformation.tooltype() == McsfCommunication::FreeHand)
					pToolChgRequestArgs->toolType = FreeHandType;
				else
				{
					delete pToolChgRequestArgs;
					return;
				}
				pImageCommRequestArgs = pToolChgRequestArgs;
			}
			else if (toolInformation.tooltype() == McsfCommunication::ScaleTool)
			{
				ScaleToolChgRequestArgs *pScaleToolChgRequestArgs = new ScaleToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(ScaleToolChgRequestArgs, ChangeToolType)
				pScaleToolChgRequestArgs->toolType = ScaleToolType;
				pScaleToolChgRequestArgs->scaleXFactor = toolInformation.has_scaleinformation() ? 
														 toolInformation.scaleinformation().scalexfactor() : 1;
				pScaleToolChgRequestArgs->scaleYFactor = toolInformation.has_scaleinformation() ? 
														 toolInformation.scaleinformation().scaleyfactor() : 1;
				pImageCommRequestArgs = pScaleToolChgRequestArgs;
			}
			else if (toolInformation.tooltype() == McsfCommunication::TranslateTool ||
				toolInformation.tooltype() == McsfCommunication::HandModeTool)
			{
				ToolChgRequestArgs *pToolChgRequestArgs = new ToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(ToolChgRequestArgs, ChangeToolType)

				if (toolInformation.tooltype() == McsfCommunication::TranslateTool)
					pToolChgRequestArgs->toolType = TranslateToolType;
				else if (toolInformation.tooltype() == McsfCommunication::HandModeTool)
					pToolChgRequestArgs->toolType = HandModeToolType;

				pImageCommRequestArgs = pToolChgRequestArgs;

			}
			else if (toolInformation.tooltype() == McsfCommunication::RotateTool)
			{
				RotateToolChgRequestArgs *pRotateToolChgRequestArgs = new RotateToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(RotateToolChgRequestArgs, ChangeToolType)
				pRotateToolChgRequestArgs->toolType = RotateToolType;
				pRotateToolChgRequestArgs->angle = toolInformation.has_rotateinformation() ?
												   toolInformation.rotateinformation().angle() : 0;
				pImageCommRequestArgs = pRotateToolChgRequestArgs;

			}
			else if (toolInformation.tooltype() == McsfCommunication::SetWinCenterWidth)
			{
				WinWidthCenterRequestArgs *pWinWidthCenterRequestArgs = new WinWidthCenterRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(WinWidthCenterRequestArgs, ChangeToolType)
				pWinWidthCenterRequestArgs->toolType = SetWinCenterWidthType;
				if (toolInformation.has_winwidthcenterinformation())
				{
					if (toolInformation.winwidthcenterinformation().has_wincenter() &&
						toolInformation.winwidthcenterinformation().has_winwidth())
					{
						pWinWidthCenterRequestArgs->SetValue(toolInformation.winwidthcenterinformation().winwidth(),
							toolInformation.winwidthcenterinformation().wincenter());
					}
				}

				pImageCommRequestArgs = pWinWidthCenterRequestArgs;
			}
			else if (toolInformation.tooltype() == McsfCommunication::MagnifyGlassTool)
			{
				ToolChgRequestArgs *pToolChgRequestArgs = new ToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(ToolChgRequestArgs, ChangeToolType)
					pToolChgRequestArgs->toolType = MagnifyGlassType;
				pImageCommRequestArgs = pToolChgRequestArgs;
				
			}
			else if (toolInformation.tooltype() == McsfCommunication::FlipX ||
				toolInformation.tooltype() == McsfCommunication::FlipY ||
				toolInformation.tooltype() == McsfCommunication::ColorInvert)
			{
				ToolChgRequestArgs *pToolChgRequestArgs = new ToolChgRequestArgs();
				SET_IMAGE_REQUEST_IMAGEUID(ToolChgRequestArgs, ChangeToolType)
				
				if (toolInformation.tooltype() == McsfCommunication::FlipX)
				{
					pToolChgRequestArgs->toolType = FlipXType;
				}
				else if (toolInformation.tooltype() == McsfCommunication::FlipY)
				{
					pToolChgRequestArgs->toolType = FlipYType;
				}
				else if (toolInformation.tooltype() == McsfCommunication::ColorInvert)
				{
					pToolChgRequestArgs->toolType = ColorInvertType;
				}
				else
				{
					delete pToolChgRequestArgs;
					return;
				}
				
				pImageCommRequestArgs = pToolChgRequestArgs;
			}
			else if (toolInformation.tooltype() == McsfCommunication::LocalizerLines)
			{
				if (toolInformation.has_localizerlinesresource())
				{
					LocalizerLinesRequestArgs *pLocalizerLinesReqArgs = new LocalizerLinesRequestArgs();
					SET_IMAGE_REQUEST_IMAGEUID(LocalizerLinesReqArgs, ChangeToolType)
					pLocalizerLinesReqArgs->toolType = LocalizerLines;

					pLocalizerLinesReqArgs->ResourceId(toolInformation.localizerlinesresource().resourceid());
					pLocalizerLinesReqArgs->IsReferringSide(toolInformation.localizerlinesresource().isreferringside());

					pLocalizerLinesReqArgs->Operation(0);
					if (toolInformation.localizerlinesresource().has_operation())
					{
						pLocalizerLinesReqArgs->Operation(toolInformation.localizerlinesresource().operation());
					}

					pImageCommRequestArgs = pLocalizerLinesReqArgs;
				}
			}
			else if (toolInformation.tooltype() == McsfCommunication::NoteTool ||
					 toolInformation.tooltype() == McsfCommunication::SetNoteProp)
			{
				if (toolInformation.has_noteobjectinformation())
				{
					const McsfCommunication::NoteObjectInformation &noteObjInf = toolInformation.noteobjectinformation();
					
					NoteToolRequestArgs *pNoteToolReqArgs = new NoteToolRequestArgs();
					SET_IMAGE_REQUEST_IMAGEUID(NoteToolReqArgs, ChangeToolType)

					NoteMetaInformation &noteMetaInfo = pNoteToolReqArgs->noteMetaInformation;
					if (toolInformation.tooltype() == McsfCommunication::NoteTool)
					{
						pNoteToolReqArgs->toolType = MCSF_DJ2DENGINE_NAMESPACE::ImageToolType::NoteToolType;
						if (noteObjInf.has_notetype())
						{
							if (noteObjInf.notetype() == McsfCommunication::NoteToolType::NoteToolTextNote)
							{
								noteMetaInfo.noteToolType = MCSF_DJ2DENGINE_NAMESPACE::NoteToolTextNote;
								validCmd = true;
							}
							else if (noteObjInf.notetype() == McsfCommunication::NoteToolType::NoteToolArrowNote)
							{
								noteMetaInfo.noteToolType = MCSF_DJ2DENGINE_NAMESPACE::NoteToolArrowNote;
								validCmd = true;
							}
						}
					}
					else if (toolInformation.tooltype() == McsfCommunication::SetNoteProp)
					{
						validCmd = true;

						pNoteToolReqArgs->toolType = MCSF_DJ2DENGINE_NAMESPACE::ImageToolType::SetNotePropType;
						if (noteObjInf.has_notetextinf())
						{
							const McsfCommunication::MetaTextInformation &metaText = noteObjInf.notetextinf();
							if (metaText.has_text())
							{
								std::string strText;
								UTF8TOMB(metaText.text(), strText);
								noteMetaInfo.noteTextInf.sText = strText;
							}
							else
								validCmd = false;

							if (metaText.has_fontsize())
								noteMetaInfo.noteTextInf.fontSize = metaText.fontsize();
							else
								validCmd = false;

							if (metaText.has_fontname())
							{
								std::string strFontName;
								UTF8TOMB(metaText.fontname(), strFontName);
								noteMetaInfo.noteTextInf.sFontName = strFontName;
							}
							else
								validCmd = false;

							if (metaText.has_fontcolor())
								noteMetaInfo.noteTextInf.fontColor.SetValue(metaText.fontcolor());
							else
								validCmd = false;

							if (metaText.has_fontstyle())
								noteMetaInfo.noteTextInf.fontStyle = metaText.fontstyle();
							else
								validCmd = false;
						}

						if (noteObjInf.has_arrownotelineinf())
						{
							const McsfCommunication::MetaLineInformation &metaLine = noteObjInf.arrownotelineinf();
							if (metaLine.has_linewidth())
								noteMetaInfo.arrowNoteLineInf.lineWidth = metaLine.linewidth();
							else
								validCmd = false;

							if (metaLine.has_linecolor())
								noteMetaInfo.arrowNoteLineInf.lineColor.SetValue(metaLine.linecolor());
							else
								validCmd = false;
						}
					}

					if (validCmd == false)
					{
						DEL_PTR(pNoteToolReqArgs);
						return;
					}
					pImageCommRequestArgs = pNoteToolReqArgs;
				}
			}
			else
			{
				assert(0);
			}
		}
		break;
	case McsfCommunication::Mouse:
		{
			if (pImageRequest->has_mouseinformation() == false) return;

			MouseEvtRequestArgs *pMouseEvtRequestArgs = new MouseEvtRequestArgs();
			SET_IMAGE_REQUEST_IMAGEUID(MouseEvtRequestArgs, Mouse)
			pMouseEvtRequestArgs->pointX = pImageRequest->mouseinformation().pointx();
			pMouseEvtRequestArgs->pointY = pImageRequest->mouseinformation().pointy();
			pMouseEvtRequestArgs->leftDown = pImageRequest->mouseinformation().has_leftdown() ?
											pImageRequest->mouseinformation().leftdown() : false;
			pMouseEvtRequestArgs->rightDown = pImageRequest->mouseinformation().has_rightdown() ?
											pImageRequest->mouseinformation().rightdown() : false;
			pMouseEvtRequestArgs->midRollerDown = pImageRequest->mouseinformation().has_midrollerdown() ?
											pImageRequest->mouseinformation().midrollerdown() : false;
			pMouseEvtRequestArgs->midRollerMove = pImageRequest->mouseinformation().has_midrollermove() ?
												pImageRequest->mouseinformation().midrollermove() : false;
			
			if (pImageRequest->mouseinformation().has_behaviortype())
			{
				long mouseBehaviorType = pImageRequest->mouseinformation().behaviortype();

				if (mouseBehaviorType == 1)
				{
					pMouseEvtRequestArgs->mouseBehavior = MouseBehaviorMouseMove;
				}
				else if (mouseBehaviorType == 2)
				{
					pMouseEvtRequestArgs->mouseBehavior = MouseBehaviorMouseDown;
				}
				else if (mouseBehaviorType == 3)
				{
					pMouseEvtRequestArgs->mouseBehavior = MouseBehaviorDoubleClick;
				}
				else
				{
					DEL_PTR(pMouseEvtRequestArgs);
					return;
				}
			}

			pImageCommRequestArgs = pMouseEvtRequestArgs;
		}
		break;
	case McsfCommunication::Keyboard:
		{
			if (pImageRequest->has_keyboardinformation() == false) return;

			KeyboardEvtRequestArgs *pKeyboardRequestArgs = new KeyboardEvtRequestArgs();
			SET_IMAGE_REQUEST_IMAGEUID(KeyboardRequestArgs, Keyboard)

				if (pImageRequest->has_keyboardinformation())
				{
					pKeyboardRequestArgs->keyVal = pImageRequest->keyboardinformation().keyval();
					pKeyboardRequestArgs->altPressed = pImageRequest->keyboardinformation().has_altpressed() ?
												pImageRequest->keyboardinformation().altpressed() : false;
					pKeyboardRequestArgs->ctrlPressed = pImageRequest->keyboardinformation().has_ctrlpressed() ?
												pImageRequest->keyboardinformation().ctrlpressed() : false;
					pKeyboardRequestArgs->shiftpressed = pImageRequest->keyboardinformation().has_shiftpressed() ?
												pImageRequest->keyboardinformation().shiftpressed() : false;

					pKeyboardRequestArgs->featureKey = MCSF_DJ2DENGINE_NAMESPACE::FK_None;
					if (pImageRequest->keyboardinformation().has_featurekey())
					{
						switch(pImageRequest->keyboardinformation().featurekey())
						{
						case McsfCommunication::FK_DEL:
							{
								pKeyboardRequestArgs->featureKey = MCSF_DJ2DENGINE_NAMESPACE::FK_DEL;
							}
							break;
						}
					}
				}
			pImageCommRequestArgs = pKeyboardRequestArgs;
		}
		break;
	case McsfCommunication::SeriesSettings:
		{
			if (pImageRequest->has_seriessettinginformation() == false) return;

			SeriesSettingsRequestArgs *pSeriesSettingRequestArgs = new SeriesSettingsRequestArgs();
			if (pSeriesSettingRequestArgs == NULL) return;

			SET_IMAGE_REQUEST_IMAGEUID(SeriesSettingRequestArgs, SeriesSettings)
			
			if (pImageRequest->seriessettinginformation().has_enableseriespersist())
			{
				pSeriesSettingRequestArgs->enableSeriesPersist = pImageRequest->seriessettinginformation().enableseriespersist();
				pSeriesSettingRequestArgs->enableSeriesPersistChanged = true;
			}
													
			if (pImageRequest->seriessettinginformation().has_enabletransfsync())
			{
				pSeriesSettingRequestArgs->enableSyncTransf = pImageRequest->seriessettinginformation().enabletransfsync();
				pSeriesSettingRequestArgs->enableSyncTransfChanged = true;
			}

			pImageCommRequestArgs = pSeriesSettingRequestArgs;
		}
		break;
	case McsfCommunication::SiteSettings:
		{
			if (pImageRequest->has_sitesettinginformation() == false) return;

			if (pImageRequest->sitesettinginformation().has_sitesettingtype() == false) return;

			SiteCustComSettingsRequestArgs *pCustCommentsReqArgs = NULL;
			
			if (pImageRequest->sitesettinginformation().sitesettingtype() == McsfCommunication::SiteCommentSettingsType)
			{
				if (pImageRequest->sitesettinginformation().has_sitesettingcontent() == false) return;

				pCustCommentsReqArgs =  new SiteCustComSettingsRequestArgs();
				
				SET_IMAGE_REQUEST_IMAGEUID(CustCommentsReqArgs, SiteSettings)

				bool b = CSiteCommentsConfig::BuildSiteCommentsRequestArgs(pCustCommentsReqArgs, 
							pImageRequest->sitesettinginformation().sitesettingcontent().messagecontent());
				if (b == false)
				{
					DEL_PTR(pCustCommentsReqArgs);
				}
			}

			if (pCustCommentsReqArgs == NULL) return;

			pImageCommRequestArgs = pCustCommentsReqArgs;
		}
		break;
	default:
		assert(0);
		break;

	}
	
	pSiteWorkTask->onDispatchImageMsg(pImageCommRequestArgs);
}

ACE_THR_FUNC_RETURN CommandsDispatcher::svc(void *p)
{
	int res = 0;
	ACE_Message_Block *mb = NULL;
	ACE_Time_Value cur = ACE_OS::gettimeofday();
	ACE_Time_Value secds(5);

	CommandsDispatcher *own = static_cast<CommandsDispatcher *>(p);
	assert(p != NULL);

	SyncMsgQueue &msgQueue = own->m_msgQueue;
	while (own->m_bStopDispatch == false)
	{
		cur = ACE_OS::gettimeofday();
		cur += secds;
		res = msgQueue.dequeue_head(mb, &cur);

		if (res >= 0 && mb)
		{
			Mcsf::CommandContext *pCommand = NULL;
			memcpy(&pCommand, mb->rd_ptr(), sizeof(pCommand));

			if (pCommand == NULL) continue;

			// process
			own->DispatchSiteTaskCommunicationRequest(pCommand);

			delete pCommand;
			mb->release();
		}
		
	}

	return 0;
}

int CommandsDispatcher::Release()
{
	m_bStopDispatch = true;

	ACE_Guard<ACE_Thread_Mutex> lock(m_siteWorkTaskMutex);
	// close all of task
	for (SiteWorkTaskMapItor it = m_siteWorkTaskMap.begin(); it != m_siteWorkTaskMap.end(); it ++)
	{
		it->second->Release();
	}

	m_siteWorkTaskMap.clear();

	delete this;

	return 0;
}

int CommandsDispatcher::ReceiveMessage(ACE_Message_Block *mb)
{
	SyncMsgQueue &pMsgQueueu = m_msgQueue;
	if (mb)
	{
		pMsgQueueu.enqueue_tail(mb);
	}
	return 0;
}

void CommandsDispatcher::onNotifyRemoveSiteTask(const std::string &siteId)
{
	ACE_Guard<ACE_Thread_Mutex> lock(m_siteWorkTaskMutex);

	SiteWorkTaskMapItor it = m_siteWorkTaskMap.find(siteId);
	if (it != m_siteWorkTaskMap.end())
	{
		m_siteWorkTaskMap.erase(it);
	}
}
MCSF_DJ2DENGINE_END_NAMESPACE
