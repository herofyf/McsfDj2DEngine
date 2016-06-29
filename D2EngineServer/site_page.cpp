#include "stdafx.h"
#include "site_page.h"
#include <string>
#include <sstream>
#include "study_series.h"
#include "dicom_component_information.h"
#include "mcsf_dj2dengine_log.h"
#include "site_work_render_task.h"
#include "study_series_command_request_args_note.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

SitePage::SitePage(SiteWorkTask *pSiteTask, const std::string &sSiteId, const SitePageKey &pageId) : 
	m_pSiteWorkTask(pSiteTask)
{
	m_sitePageId.sSiteId = sSiteId;
	m_sitePageId.sPageId = pageId;
}


SitePage::~SitePage(void)
{
	for (PageSeriesMapIt it = m_pageSeriesMap.begin(); it != m_pageSeriesMap.end(); it ++)
	{
		it->second->Release();
	}

	m_pageSeriesMap.clear();
}


void SitePage::Release()
{
	delete this;
}

bool SitePage::HandleRenderImageOut(const IStudyImage *pStudyImage) const
{
	m_pSiteWorkTask->HandleRenderImageOut(pStudyImage);

	return true;
}

bool SitePage::HandlePostRenderStatusMsgs(const IStudyImage *pStudyImage) const
{
	m_pSiteWorkTask->HandlePostRenderStatusMsgs(pStudyImage);

	return true;
}

void SitePage::onSiteCommentTagsChanged()
{
	for (PageSeriesMapIt it = m_pageSeriesMap.begin(); it != m_pageSeriesMap.end(); it++)
	{
		it->second->onSiteCommentTagsChanged();
	}
}

bool SitePage::IsPageScopeToolType(const ImageToolType &newToolType) const
{
	bool bRet = (newToolType == LineToolType || 
		newToolType == AngleToolType		 || 
		newToolType == CircleToolType		 ||
		newToolType == FreeHandType			 ||
		newToolType == ScaleToolType		 ||
		newToolType == TranslateToolType	 ||
		newToolType == RotateToolType		 ||
		newToolType == SetWinCenterWidthType ||
		newToolType == MagnifyGlassType		 ||
		newToolType == HandModeToolType		 ||
		newToolType == ImageToolType::NoteToolType
		);

	return bRet;
}

bool SitePage::IsImageScopeToolType(const ImageToolType &newToolType) const
{
	bool bRet = (newToolType == FlipXType || 
		newToolType == FlipYType || 
		newToolType == ColorInvertType ||
		newToolType == SetNotePropType);

	return bRet;
}

bool SitePage::IsTransfomationToolUsed() const
{
	ImageToolType curToolType = m_toolTypeContext.ImageToolTypeVal();
	
	return IsTransformationTool(curToolType);
}

bool SitePage::IsMeasureToolUsed() const
{
	ImageToolType curToolType = m_toolTypeContext.ImageToolTypeVal();
	
	return IsMeasureTool(curToolType);
}

bool SitePage::IsMeasureTool(const ImageToolType &newToolType) const
{
	bool isMeasureTool = 
		(newToolType == LineToolType          || 
		 newToolType == AngleToolType		  ||
		 newToolType == CircleToolType		  ||
		 newToolType == FreeHandType		  ||
		 newToolType == ImageToolType::NoteToolType);
	return isMeasureTool;
}

bool SitePage::IsHandModeToolUsed() const
{
	ImageToolType curToolType = m_toolTypeContext.ImageToolTypeVal();
	return (curToolType == HandModeToolType);
}

bool SitePage::IsTransformationTool(const ImageToolType &newToolType) const
{
	bool isTransformationTool = 
		(newToolType == TranslateToolType    || 
		newToolType == ScaleToolType         ||
		newToolType == RotateToolType        ||
		newToolType == SetWinCenterWidthType ||
		newToolType == MagnifyGlassType);

	return isTransformationTool;
}

void SitePage::onToolTypeChangedRequest(const ImageCommRequestArgs *pImageCommRequestArgs)
{
	const ToolChgRequestArgs *pToolInformation = dynamic_cast<const ToolChgRequestArgs *>(pImageCommRequestArgs);
	if (pToolInformation == NULL)
		return;
	
	ImageToolType newToolType = pToolInformation->toolType;
	
	if (IsPageScopeToolType(newToolType) == false)
		return;
	
	if (newToolType != m_toolTypeContext.ImageToolTypeVal())
	{
		if (IsMeasureTool(newToolType))
		{
			onSitePageToolTypeChanged(newToolType);
		}
		
		if (newToolType == ScaleToolType)
		{
			const ScaleToolChgRequestArgs *pScaleToolChgRequestArgs = dynamic_cast<const ScaleToolChgRequestArgs*>(pToolInformation);
			if (pScaleToolChgRequestArgs && pScaleToolChgRequestArgs->scaleXFactor > 0 && pScaleToolChgRequestArgs->scaleYFactor > 0) 
			{
				m_toolTypeContext.ScaleFactorX(pScaleToolChgRequestArgs->scaleXFactor);
				m_toolTypeContext.ScaleFactorY(pScaleToolChgRequestArgs->scaleYFactor);
			}
		}
		else if (newToolType == ImageToolType::NoteToolType)
		{
			const NoteToolRequestArgs *pNoteToolReqArgs = dynamic_cast<const NoteToolRequestArgs *>(pToolInformation);
			if (pNoteToolReqArgs)
			{
				m_toolTypeContext.UseNoteType(pNoteToolReqArgs->noteMetaInformation.noteToolType);
			}
		}

		m_toolTypeContext.ImageToolTypeVal(newToolType);
	}
}

void SitePage::onSitePageToolTypeChanged(const ImageToolType &newToolType)
{
	for (PageSeriesMapIt it = m_pageSeriesMap.begin(); it != m_pageSeriesMap.end();
		it ++)
	{
		it->second->onSitePageToolTypeChanged(newToolType);
	}
}


void SitePage::onCommandRequest(const ImageCommRequestArgs *pImageCommRequestArgs)
{
	if (pImageCommRequestArgs == NULL)
		return;

	// if all of series closed, this site work task need to close
	// otherwise to find the right series window
	switch (pImageCommRequestArgs->iCommandId)
	{
	case LoadSeries:
		{
			onLoadSeries(pImageCommRequestArgs->imagePosId.seriesPagePos,
						pImageCommRequestArgs->imagePosId.seriesId);
		}
		break;
	case CloseSeries:
		{
			onSeriesRemove(pImageCommRequestArgs->imagePosId.seriesPagePos);
		}
		break;
	case OpenImage:
	case ChangeToolType:
	case Mouse:
	case Keyboard:
	case SeriesSettings:
	case MessageCommandType::ResetImage:
		{
			if (pImageCommRequestArgs->iCommandId == ChangeToolType)
			{
				onToolTypeChangedRequest(pImageCommRequestArgs);
			}

			StudySeries *pStudySeries = NULL;
			PageSeriesMapIt it = m_pageSeriesMap.find(pImageCommRequestArgs->imagePosId.seriesPagePos);
			if (it != m_pageSeriesMap.end())
			{
				pStudySeries = it->second;
				if (pStudySeries)
				{
					pStudySeries->onCommunicationCommandRequest(pImageCommRequestArgs);
				}
			}
		}
		break;
	
	}
}

SiteWorkTask *SitePage::OwnerSite()
{
	return m_pSiteWorkTask;
}


void SitePage::onSeriesRemove(const PageSeriesKey seriesPagePos)
{
	PageSeriesMapIt it = m_pageSeriesMap.find(seriesPagePos);
	if (it != m_pageSeriesMap.end())
	{
		it->second->Release();
		m_pageSeriesMap.erase(it);
	}

	if (m_pageSeriesMap.size() == 0)
	{
		m_pSiteWorkTask->onPageClose(m_sitePageId.sPageId);
	}
}

/*
	
*/
void SitePage::onLoadSeries(PageSeriesKey pagePos, const std::string &seriesName)
{
	StudySeries *pStudySeries = NULL;
	std::stringstream log;

	PageSeriesMapIt it = m_pageSeriesMap.find(pagePos);
	if (it != m_pageSeriesMap.end())
	{
		pStudySeries = it->second;
		
		// if series name is changed
		if (seriesName != pStudySeries->GetSeriesName())
		{
			log << "replace:" << pStudySeries->GetSeriesName() << " in page" << m_sitePageId.toString() << "-" << pagePos;
			pStudySeries->LoadSeries(seriesName);
		}
		else
		{
			log << "has same series name:" << seriesName << " in page " << m_sitePageId.toString() << "-" << pagePos;
		}
		LOG_INFO(log.str());
	}
	else
	{
		PageSeriesId pageSeriesId;
		pageSeriesId.sSiteId = m_sitePageId.sSiteId;
		pageSeriesId.sPageId = m_sitePageId.sPageId;
		pageSeriesId.nSeriesPagePos =  pagePos;
		pStudySeries = new StudySeries(this, pageSeriesId);
		m_pageSeriesMap[pagePos] = pStudySeries;
		pStudySeries->LoadSeries(seriesName);
	}
}

bool SitePage::IsLocalizerLinesEnabled(const LocalizerLinesResourceId *pLlResId) const
{
	return m_LlRefSidesRes.IsValidReferSides(pLlResId);
}

bool SitePage::IsLocalizerLinesEnabled(const DicomSeriesDescription *pSeriesDesc) const
{
	const StudySeries *pStudySeries = pSeriesDesc->MyStudySeries();
	if (pStudySeries)
	{
		const LocalizerLinesResourceId *pLlResId = pStudySeries->GetLocalizerLinesResId();
		if (pLlResId)
		{
			return IsLocalizerLinesEnabled(pLlResId);
		}
	}
}

void SitePage::NotifyRefSeriesRedrawLocalizerLine(const DicomSeriesDescription *pSeriesDesc)
{
	if (IsLocalizerLinesEnabled(pSeriesDesc))
	{
		m_LlRefSidesRes.NotifyRefSeriesRedrawLocalizerLine(pSeriesDesc);
	}
}

void SitePage::onLlReferSidesChanged(const DicomSeriesDescription *pSeriesDesc)
{
	m_LlRefSidesRes.onLlReferSidesChanged(pSeriesDesc);	
}

void SitePage::onLocalizerLineMove(const DicomSeriesDescription *pRefSeriesDesc, 
	const DicomImageDescription *pImageDesc, int nRefedSeriesCellNum, PointF refImagePixel)
{
	m_LlRefSidesRes.onLocalizerLineMove(pRefSeriesDesc, pImageDesc, nRefedSeriesCellNum, refImagePixel);
}

void SitePage::ClearLlRefResource(const DicomSeriesDescription *pSeriesDesc)
{
	m_LlRefSidesRes.ClearLlRefResource(pSeriesDesc);
}

MCSF_DJ2DENGINE_END_NAMESPACE