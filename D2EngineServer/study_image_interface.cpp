#include "stdafx.h"
#include "study_image_interface.h"
#include "study_series.h"
#include "dicom_component_information.h"
#include <sstream>
#include "mcsf_dj2dengine_containee.h"
#include "mcsf_dj2dengine_log.h"
#include "trace_performance_func.h"
#include "system_global_configuration.h"
#include "site_work_task.h"
#include "site_page.h"
#include "site_work_render_task.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

IStudyImage::IStudyImage(StudySeries *pParent, const SeriesImageId &seriesImageId) :
     ImageStatusMsgNotifier(this), m_pStudySeries(pParent), m_seriesImageId(seriesImageId),
	 m_strImageUidTag(""), m_nImageWidth(0), m_nImageHeight(0), m_pImageHostCell(NULL)
{
	UpdateImageUidTag();
}


IStudyImage::~IStudyImage(void)
{
}

void IStudyImage::UpdateImageUidTag()
{
	std::stringstream ssTag;

	ssTag << "?";
	ssTag << m_seriesImageId.siteSeriesId.sSiteId <<  "?"
		<< m_seriesImageId.siteSeriesId.sPageId << "?"
		<< m_seriesImageId.siteSeriesId.nSeriesPagePos << "?"
		<< m_seriesImageId.siteSeriesId.sSeriesId << "?"
		<< m_seriesImageId.imageCellPos << "?";

	m_strImageUidTag = ssTag.str();
}

void IStudyImage::ChangeImageCellPos(int cellPos, IStudySeriesImageCell *pImageHostCell)
{
	m_seriesImageId.imageCellPos = cellPos;

	UpdateImageUidTag();

	m_pImageHostCell = pImageHostCell;
}

void IStudyImage::OnShowActive()
{

}

void IStudyImage::OnShowDeactive()
{

}

bool IStudyImage::SyncRenderImageOut() const
{
	int size = 0;
	boost::shared_array<char> imageBuf = CompositeImage(size);

	const StudySeries *pSeries = (const StudySeries *)OwnerSeries();
	if (size > 0 && pSeries)
	{
		Mcsf::SendDataContext context;
		context.sReceiver = pSeries->GetCommReceiver();
		context.iLen = size;
		context.pRawData = imageBuf.get();

		int iRet = Mcsf2DEngineContainee::GetCommProxy()->AsyncSendData( &context );
		return iRet;
	}

	return false;
}

void IStudyImage::NotifyClient_ChangeMouseCursor(MouseCursorType cursorType)
{
	Mcsf2DEngineContainee::GetInstance()->NotifyClient_ChangeCellImageMouseCursor(&m_seriesImageId, cursorType);
}

bool IStudyImage::AsyncRenderImageOut() const
{
	const SitePage *pSitePage = (const SitePage *)OwnerPage();
	if (pSitePage)
	{
		pSitePage->HandleRenderImageOut(this);
		return true;
	}
	else
	{
		LOG_ERROR("My god, allocate render task args failed.");
		return false;
	}
}

bool IStudyImage::SeriesMsgToCommString(ImageMsgType statusType, void *pParam, std::string &strResult) const
{
	switch(statusType)
	{
	case ImageNoteStatusMsg:
		{
			NoteStatusInformation *pNoteStatusInfo = static_cast<NoteStatusInformation *>(pParam);
			if (pNoteStatusInfo)
			{
				McsfCommunication::ReportStatusInformation mcsReportStatus;
				mcsReportStatus.set_reporttype(McsfCommunication::ReportStatusType::ReportImageNoteStatus);
				McsfCommunication::ImagePosId *pImagePosId = mcsReportStatus.mutable_imageposid();
				m_seriesImageId.BuildValue(pImagePosId);

				McsfCommunication::ReportNoteStatusArgs *pReportNoteStArgs = mcsReportStatus.mutable_notestatus();
				bool b = pNoteStatusInfo->BuildValue(pReportNoteStArgs);
				if (b == false)
					return false;

				b = mcsReportStatus.SerializeToString(&strResult);
				return b;
			}
		}
		break;
	}

	return false;
}

bool IStudyImage::RefreshRenderImage()
{
	AsyncRenderImageOut();

	return true;
}

void IStudyImage::SetImageSizeValue(unsigned long width, unsigned long height)
{
	if (m_nImageWidth != width || m_nImageHeight != height)
	{
		m_nImageWidth = width;
		m_nImageHeight = height;
	}
}

IStudySeriesImageCell *IStudyImage::OwnerCell() const
{ 
	return m_pImageHostCell;
}

StudySeries *IStudyImage::OwnerSeries() const
{
	return m_pStudySeries;
}

SitePage *IStudyImage::OwnerPage() const
{
	if (m_pStudySeries != NULL)
	{
		return m_pStudySeries->OwnerPage();
	}
	return NULL;
}

SiteWorkTask *IStudyImage::OwnerSite() const
{
	SitePage *ownerPage = OwnerPage();
	if (ownerPage != NULL)
	{
		return ownerPage->OwnerSite();
	}
	return NULL;
}


MCSF_DJ2DENGINE_END_NAMESPACE