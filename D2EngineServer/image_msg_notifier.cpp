#include "stdafx.h"
#include "image_msg_notifier.h"
#include "study_image_interface.h"
#include "site_page.h"
#include "study_image.h"
#include "mcsf_dj2dengine_containee.h"
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageStatusMsgNotifier::PostRenderStatusMsgs::PostRenderStatusMsgs(const ImageStatusMsgNotifier *pMsgNotifier) :
	m_pMsgNotifier(pMsgNotifier)
{

}

ImageStatusMsgNotifier::PostRenderStatusMsgs::~PostRenderStatusMsgs()
{

}


bool ImageStatusMsgNotifier::PostRenderStatusMsgs::HandleStatusMsgs()
{
	// for each msg to call send
	std::string strReceiver = m_pMsgNotifier->m_pIStudyImage->OwnerSeries()->GetCommReceiver();

	std::deque<std::string>::iterator it;
	for (it = m_StatusMsgs.begin(); it != m_StatusMsgs.end(); it ++)
	{
		Mcsf2DEngineContainee::GetInstance()->NotifyClient_ReportImageStatusMsg(strReceiver, *it);
	}

	m_StatusMsgs.clear();
	return false;
}

bool ImageStatusMsgNotifier::PostRenderStatusMsgs::ReportStatusMsg(const std::string &strMsg)
{
	if (strMsg.length() <= 0)
		return false;

	m_StatusMsgs.push_back(strMsg);

	return true;
}

ImageStatusMsgNotifier::ImageStatusMsgNotifier(const IStudyImage *pIStudyImage) :
m_pIStudyImage(pIStudyImage), m_PrStatusMsgs(this)
{

}


bool ImageStatusMsgNotifier::SendPostRenderStatusMsgs() 
{
	return m_PrStatusMsgs.HandleStatusMsgs();
}

bool ImageStatusMsgNotifier::ReportPostRenderStatusMsgs(ImageMsgType statusType, void *pParam) 
{
	std::string strResult;
	const SitePage *pSitePage = (const SitePage *)m_pIStudyImage->OwnerPage();
	if (pSitePage == NULL)
		return false;

	bool b = SeriesMsgToCommString(statusType, pParam, strResult);
	if (b == true)
	{
		m_PrStatusMsgs.ReportStatusMsg(strResult);

		return pSitePage->HandlePostRenderStatusMsgs(m_pIStudyImage);
	}

	return false;
}

MCSF_DJ2DENGINE_END_NAMESPACE