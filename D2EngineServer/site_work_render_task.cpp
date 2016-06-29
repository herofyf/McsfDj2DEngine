#include "stdafx.h"
#include "site_work_render_task.h"
#include "study_image_interface.h"
#include "site_work_task.h"
#include "mcsf_dj2dengine_containee.h"
#include "trace_performance_func.h"
#include "mcsf_dj2dengine_log.h"
#include "image_msg_notifier.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

SiteWorkRenderTask::SiteWorkRenderTask(const SiteWorkTask *pMyWorkTask) :
	_pMyWorkTask(pMyWorkTask), m_bStopped(false), pTaskContexts(NULL),
	_ThreadsCount(0)
{
	std::stringstream ssLog;

	SYSTEM_INFO si;  
	GetSystemInfo(&si);  
	int count = si.dwNumberOfProcessors;

	_ThreadsCount = (count <= 0) ? 1 : count;

	pTaskContexts = new RenderTaskContext[_ThreadsCount];
	if (pTaskContexts != NULL)
	{
		ACE_thread_t threadId;

		for (int i = 0; i < _ThreadsCount; i ++)
		{
			pTaskContexts[i].Owner = this;
			pTaskContexts[i].Queue = &_msgQueue;
			pTaskContexts[i].hStartTaskEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
			pTaskContexts[i].hTaskCompEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
			pTaskContexts[i].bValid = true;
			int iRet = ACE_Thread::spawn((ACE_THR_FUNC)onTask, &(pTaskContexts[i]), THR_JOINABLE |THR_NEW_LWP, &threadId, 
				&(pTaskContexts[i].threadHandle));
			if (iRet < 0)
			{
				pTaskContexts[i].bValid = false;
			}
		}
	}
}

void SiteWorkRenderTask::StopTask()
{
	m_bStopped = true;

	if (pTaskContexts != NULL)
	{
		for (int i = 0; i < _ThreadsCount; i ++)
		{
			SetEvent(pTaskContexts[i].hStartTaskEvt);

			if (pTaskContexts[i].bValid)
			{
				ACE_Thread::join(pTaskContexts[i].threadHandle);
			}
			CloseHandle(pTaskContexts[i].hTaskCompEvt);
			CloseHandle(pTaskContexts[i].hStartTaskEvt);
		}
		delete []pTaskContexts;
	}
}

SiteWorkRenderTask::~SiteWorkRenderTask(void)
{
}

void SiteWorkRenderTask::ProcessQueue(ACE_Message_Queue<ACE_MT_SYNCH> *pQueue)
{
	if (pQueue == NULL)
		return;

	ACE_Time_Value cur;;
	ACE_Time_Value secds(0);
	ACE_Message_Block *mb = NULL;
	const IStudyImage *pIStudyImage = NULL;
	ImageRenderTaskArgs *pImageRenderTaskArgs = NULL;

	while (pQueue->is_empty() == false)
	{
		cur = ACE_OS::gettimeofday();
		cur += secds;

		int res = pQueue->dequeue_head(mb, &cur);
		if (res >= 0 && mb)
		{
			memcpy(&pImageRenderTaskArgs, mb->rd_ptr(), sizeof(pImageRenderTaskArgs));

			pIStudyImage = pImageRenderTaskArgs->pImageOwner;
			
			if (pIStudyImage)
			{
				if ((pImageRenderTaskArgs->taskFlags) & OPTYPE_RENDER)
				{
					pIStudyImage->SyncRenderImageOut();
				}
				
				if ((pImageRenderTaskArgs->taskFlags) & OPTYPE_REPORT_POSTRENDER_MSG)
				{
					IStudyImage *pModStudyImage = const_cast<IStudyImage *>(pIStudyImage);
					if (pModStudyImage)
					{
						ImageStatusMsgNotifier *pMsgNotifier = dynamic_cast<ImageStatusMsgNotifier *>(pModStudyImage);
						if (pMsgNotifier)
						{
							pMsgNotifier->SendPostRenderStatusMsgs();
						}
					}
					
				}
			}
			
			DEL_PTR(pImageRenderTaskArgs);

			mb->release();
		}
		else
		{
			break;
		}
	}
}

void *SiteWorkRenderTask::onTask(void *pParam)
{
	RenderTaskContext *pContext = (RenderTaskContext *)(pParam);
	if (pContext == NULL || pContext->Owner == NULL)
	{
		LOG_ERROR("MGD, No render context.");
		return 0;
	}

	SiteWorkRenderTask *pRenderTask = pContext->Owner;

	ACE_Message_Queue<ACE_MT_SYNCH> *pQueue = pContext->Queue;
	
	while (true)
	{
		// wait task signal
		WaitForSingleObject(pContext->hStartTaskEvt, INFINITE);

		if (pContext->bValid)
		{
			ProcessQueue(pQueue);
		}

		if (pRenderTask->IsStopped())
			break;

		// notify task completed 
		SetEvent(pContext->hTaskCompEvt);
	}

	return 0;
}

void SiteWorkRenderTask::HandleRenderImageOut(const IStudyImage *pStudyImage)
{
	if (pStudyImage == NULL)
		return;

	ImageRenderOpMapIt it = m_renderTaskMap.find(pStudyImage);
	if (it != m_renderTaskMap.end())
	{
		it->second->taskFlags |= OPTYPE_RENDER;
		return;
	}
	else
	{
		ImageRenderTaskArgs *pRenderTaskArgs = new ImageRenderTaskArgs;
		pRenderTaskArgs->pImageOwner = pStudyImage;
		pRenderTaskArgs->taskFlags = OPTYPE_RENDER;
		m_renderTaskMap[pStudyImage] = pRenderTaskArgs;
	}
}

void SiteWorkRenderTask::HandlePostRenderStatusMsgs(const IStudyImage *pStudyImage)
{
	if (pStudyImage == NULL)
		return;

	ImageRenderOpMapIt it = m_renderTaskMap.find(pStudyImage);
	if (it != m_renderTaskMap.end())
	{
		it->second->taskFlags |= OPTYPE_REPORT_POSTRENDER_MSG;
		return;
	}
	else
	{
		ImageRenderTaskArgs *pRenderTaskArgs = new ImageRenderTaskArgs;
		pRenderTaskArgs->pImageOwner = pStudyImage;
		pRenderTaskArgs->taskFlags = OPTYPE_REPORT_POSTRENDER_MSG;
		m_renderTaskMap[pStudyImage] = pRenderTaskArgs;
	}
}


void SiteWorkRenderTask::Process()
{
	const ImageRenderTaskArgs *pRenderTaskArgs = NULL;

	for (ImageRenderOpMapCIt cit = m_renderTaskMap.begin(); cit != m_renderTaskMap.end();
		 cit ++)
	{
		pRenderTaskArgs = cit->second;

		ACE_Message_Block *mb = new ACE_Message_Block(sizeof(pRenderTaskArgs));
		if (mb)
		{
			memcpy(mb->wr_ptr(), &pRenderTaskArgs, sizeof(pRenderTaskArgs));
			_msgQueue.enqueue_tail(mb);
		}
	}

	m_renderTaskMap.clear();

	if (_msgQueue.is_empty() == false)
	{
		for (int i = 0; i < _ThreadsCount; i ++)
		{
			ResetEvent(pTaskContexts[i].hTaskCompEvt);

			if (pTaskContexts[i].bValid)
			{
				SetEvent(pTaskContexts[i].hStartTaskEvt);
			}
		}

		for (int i = 0; i < _ThreadsCount; i ++)
		{
			if (pTaskContexts[i].bValid)
			{
				WaitForSingleObject(pTaskContexts[i].hTaskCompEvt, INFINITE);
			}
		}

		// retry again, just afraid thread creating failed.
		ProcessQueue(&_msgQueue);
	}
	
}

MCSF_DJ2DENGINE_END_NAMESPACE