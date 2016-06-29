#include "stdafx.h"
#include "site_work_task.h"
#include "study_series.h"
#include "dispatch_command_task.h"
#include <sstream>
#include "mcsf_dj2dengine_log.h"
#include "site_page.h"
#include "site_work_render_task.h"
#include "image_tool_interface.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

SiteWorkTask::SiteWorkTask(CommandsDispatcher *pCmdDispatcher, const std::string &siteId) :
		m_pCmdDispatcher(pCmdDispatcher), m_strSiteId(siteId),
		m_bStopped(false)
{
	this->activate(THR_NEW_LWP | THR_JOINABLE);

	_pSiteRenderTask = new SiteWorkRenderTask(this);
}

int SiteWorkTask::close(u_long flags /* = 0 */)
{
	delete this;

	return 0;
}

SiteWorkTask::~SiteWorkTask(void)
{
	
}

void SiteWorkTask::StopTask()
{
	if (_pSiteRenderTask)
	{
		_pSiteRenderTask->StopTask();

		DEL_PTR(_pSiteRenderTask);
	}

	m_bStopped = true;

	ACE_Message_Block *mb = new ACE_Message_Block();
	mb->msg_type( ACE_Message_Block::MB_STOP );
	this->putq( mb );

}
/*
 TO RELEASE ALL OF RESOURCE BELONG TO THIS THREAD
*/
void SiteWorkTask::Release()
{
	// release all of information
	for (SitePageMapIt it = m_sitePageMap.begin(); it != m_sitePageMap.end(); it ++)
	{
		it->second->Release();
	}

	m_sitePageMap.clear();

	StopTask();

	// can't put in message process, avoid dead lock
	//this->wait();
}

void SiteWorkTask::onDispatchImageMsg(const ImageCommRequestArgs *pImageCommRequestArgs)
{
	if (m_bStopped)
	{
		delete pImageCommRequestArgs;
		return;
	}

	if (pImageCommRequestArgs)
	{
		ACE_Message_Block *mb = new ACE_Message_Block(sizeof(pImageCommRequestArgs));

		memcpy(mb->wr_ptr(), &pImageCommRequestArgs, sizeof(pImageCommRequestArgs));

		this->putq(mb);
	}
}

int SiteWorkTask::svc()
{
	ImageCommRequestArgs *pImageCommRequestArgs = NULL;

	SetupDefaultComments();

	while(true)
	{
		try
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

			memcpy(&pImageCommRequestArgs, mb->rd_ptr(), sizeof(pImageCommRequestArgs));
			onCommandRequest(pImageCommRequestArgs);

			delete pImageCommRequestArgs;
			mb->release();

			if (_pSiteRenderTask)
			{
				_pSiteRenderTask->Process();
			}
		}
		catch(...)
		{
			//throw "work task crashed";
			LOG_ERROR("catch exception.");
		}
	}

	return 0;
}


void SiteWorkTask::onPageClose(const SitePageKey &pageId)
{
	SitePageMapIt it = m_sitePageMap.find(pageId);
	if (it != m_sitePageMap.end())
	{
		it->second->Release();
		m_sitePageMap.erase(it);
	}

	if (m_sitePageMap.size() == 0)
	{
		if (m_pCmdDispatcher)
		{
			m_pCmdDispatcher->onNotifyRemoveSiteTask(m_strSiteId);
		}

		StopTask();
	}
}

void SiteWorkTask::onCommandRequest(const ImageCommRequestArgs *pImageCommRequestArgs)
{
	if (pImageCommRequestArgs == NULL)
		return;
	
	// if all of series closed, this site work task need to close
	// otherwise to find the right series window
	switch (pImageCommRequestArgs->iCommandId)
	{
	case LoadSeries:
	case CloseSeries:
	case OpenImage:
	case ChangeToolType:
	case Mouse:
	case Keyboard:
	case SeriesSettings:
	case MessageCommandType::ResetImage:
		{
			const SitePageKey &sitePageId = pImageCommRequestArgs->imagePosId.pageId;
			SitePageMapIt it = m_sitePageMap.find(sitePageId);

			SitePage *pSitePage = NULL;
			if (it != m_sitePageMap.end())
			{
				pSitePage = it->second;
			}
			else
			{
				if (pImageCommRequestArgs->iCommandId == LoadSeries)
				{
					pSitePage = new SitePage(this, m_strSiteId, sitePageId);
					m_sitePageMap[sitePageId] = pSitePage;
				}
			}

			if (pSitePage != NULL)
			{
				pSitePage->onCommandRequest(pImageCommRequestArgs);
			}
		}
		break;
	case SiteSettings:
		{
			const SiteCustSettingsRequestArgs *pSiteCustReqArgs = dynamic_cast<const SiteCustSettingsRequestArgs *>(pImageCommRequestArgs);
			if (pSiteCustReqArgs)
			{
				if (pSiteCustReqArgs->GetCustomSettingType() == CUSTOM_TYPE_COMMENT)
				{
					const SiteCustComSettingsRequestArgs *pSiteCustCommReqArgs = dynamic_cast<const SiteCustComSettingsRequestArgs *>(pSiteCustReqArgs);
					if (pSiteCustReqArgs)
					{
						onSiteCommentTagsRequest(pSiteCustCommReqArgs);
					}
				}
				
			}
		}
		break;
	}
}

void SiteWorkTask::SetupDefaultComments()
{
	/*CustomizedPosDcmTagComment comm;
	comm.SetDcmTagComment(VP_LEFT_TOP, 0x0010, 0x0010);
	m_customizedComments.push_back(comm);*/
}

const CUSTOMIZED_COMMENTS_DEQ *SiteWorkTask::GetCustomizedComments()
{
	return &m_customizedComments;
}

void SiteWorkTask::onSiteCommentTagsRequest(const SiteCustComSettingsRequestArgs *pSiteCustCommReqArgs)
{
	if (pSiteCustCommReqArgs == NULL) 
		return;

	m_customizedComments = *(pSiteCustCommReqArgs->GetComments());
	for (SitePageMapIt it = m_sitePageMap.begin(); it != m_sitePageMap.end(); it ++)
	{
		it->second->onSiteCommentTagsChanged();
	}
}

bool SiteWorkTask::HandleRenderImageOut(const IStudyImage *pImageObj)
{
	_pSiteRenderTask->HandleRenderImageOut(pImageObj);

	return true;
}

bool SiteWorkTask::HandlePostRenderStatusMsgs(const IStudyImage *pImageObj)
{
	_pSiteRenderTask->HandlePostRenderStatusMsgs(pImageObj);

	return true;
}
MCSF_DJ2DENGINE_END_NAMESPACE
