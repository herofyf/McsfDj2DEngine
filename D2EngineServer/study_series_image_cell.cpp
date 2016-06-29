#include "stdafx.h"
#include "study_series_image_cell.h"
#include "study_series.h"
#include "study_image.h"
#include "study_series_command_request_args.h"
#include "image_property_state.h"
#include "dicom_component_information.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

IStudySeriesImageCell::IStudySeriesImageCell(StudySeries *pStudySeries, int cellNum) :
	m_pStudySeries(pStudySeries), m_nCellNum(cellNum), m_nWidth(0), m_nHeight(0),
	m_pCurrentStudyImage(NULL), m_transformReqs(), m_bColorInverted(false),
	m_nWinWidth(-1), m_nWinCenter(-1), m_pCurImageDesc(NULL)
{
	
}


IStudySeriesImageCell::~IStudySeriesImageCell(void)
{
}

void IStudySeriesImageCell::ResetToDefault()
{
	m_nWinWidth = m_nWinCenter = -1;

	m_transformReqs.ClearReqs();

	m_bColorInverted = false;
}


const IStudyImage * IStudySeriesImageCell::AssignNewStudyImage(IStudyImage *pNewStudyImage, DicomImageDescription *pDicomAttrib, int width, int height)
{
	IStudyImage *pOldImage = m_pCurrentStudyImage;
	if (m_pCurrentStudyImage != pNewStudyImage)
	{
		m_pCurrentStudyImage = pNewStudyImage;
	}

	m_nWidth = width;
	m_nHeight = height;
	m_pCurImageDesc = pDicomAttrib;

	m_pCurrentStudyImage->ChangeImageCellPos(m_nCellNum, this);
	m_pCurrentStudyImage->LoadImageFile(pDicomAttrib, width, height);
	
	return pOldImage;
}

const DicomImageDescription *IStudySeriesImageCell::GetCurImageDesc()
{
	return m_pCurImageDesc;
}

void  IStudySeriesImageCell::RefeshImage()
{
	if (m_pCurrentStudyImage)
	{
		m_pCurrentStudyImage->RefreshRenderImage();
	}
}

void IStudySeriesImageCell::HandleSyncImageTransformation(const TransformationArgs *pTransformationArgs)
{
	if (m_pCurrentStudyImage)
	{
		if (m_pStudySeries->IsTransfSyncEnabled() || (pTransformationArgs->pSender == m_pCurrentStudyImage))
		{
			m_pCurrentStudyImage->onSyncTransformation(pTransformationArgs);
		}
	}
}

void IStudySeriesImageCell::HandleSyncImageInvertColor(void *pOwner)
{
	if (m_pCurrentStudyImage)
	{
		if (m_pStudySeries->IsTransfSyncEnabled() || (pOwner == m_pCurrentStudyImage))
		{
			m_pCurrentStudyImage->onSyncInvertImageColor(pOwner);
		}
	}
}

void IStudySeriesImageCell::HandleSyncImageSetWinWidthCenter(float fWinWidth, float fWinCenter, bool isAbsoluteVal, void *pOwner)
{
	if (m_pCurrentStudyImage)
	{
		if (m_pStudySeries->IsTransfSyncEnabled() || (pOwner == m_pCurrentStudyImage))
		{
			m_pCurrentStudyImage->onSyncSetImageWinWidthCenter(fWinWidth, fWinCenter, isAbsoluteVal, pOwner);
		}
	}
}

int IStudySeriesImageCell::onMouseEvtCommandRequest(const MouseEvtRequestArgs *requestArgs)
{
	if (m_pCurrentStudyImage)
	{
		return m_pCurrentStudyImage->onMouseEvtCommandRequest(requestArgs);
	}
	return false;
}

int IStudySeriesImageCell::onToolTypeChangedCommandRequest(const ToolChgRequestArgs *toolInformation)
{
	if (m_pCurrentStudyImage)
	{
		return m_pCurrentStudyImage->onToolTypeChangedCommandRequest(toolInformation);
	}
	return false;
}

int IStudySeriesImageCell::onKeyboardEvtCommandRequest(const KeyboardEvtRequestArgs *keyboard)
{
	if (m_pCurrentStudyImage)
	{
		return m_pCurrentStudyImage->onKeyboardEvtCommandRequest(keyboard);
	}
	return false;
}

bool IStudySeriesImageCell::IsImageTransfSyncEnabled()
{
	if (m_pStudySeries)
	{
		return m_pStudySeries->IsTransfSyncEnabled();
	}
	return false;
}

bool IStudySeriesImageCell::onImageWinWidthCenterChanged(int curWinWidth, int curWinCenter)
{
	return false;
}

bool IStudySeriesImageCell::onImageColorInverted(bool bInverted)
{
	return false;
}
bool IStudySeriesImageCell::onLoadDicomFile()
{
	return true;
}

StudySeriesImageCell::StudySeriesImageCell(StudySeries *pStudySeries, int cellNum) :
	IStudySeriesImageCell(pStudySeries, cellNum)
{

}

StudySeriesImageCell::~StudySeriesImageCell()
{

}

bool StudySeriesImageCell::onTransformation(const TransformationArgs *args)
{
	m_transformReqs.RecordTransformReq(args);
	
	return true;
}

bool StudySeriesImageCell::onImageWinWidthCenterChanged(int curWinWidth, int curWinCenter)
{
	m_nWinWidth = curWinWidth;
	m_nWinCenter = curWinCenter;
	return true;
}

bool StudySeriesImageCell::onImageColorInverted(bool bInverted)
{
	m_bColorInverted = bInverted;
	return true;
}

void StudySeriesImageCell::RefreshLocalizerLines(const DicomSeriesDescription *pDicomSeriesDesc)
{
	StudyImage *pStudyImage = dynamic_cast<StudyImage *>(m_pCurrentStudyImage);
	if (pStudyImage) 
	{
		pStudyImage->ShowLocalizerLinesForReferredSeries(pDicomSeriesDesc);
	}
}

void StudySeriesImageCell::onSitePageToolTypeChanged(const ImageToolType &newToolType)
{
	StudyImage *pStudyImage = dynamic_cast<StudyImage *>(m_pCurrentStudyImage);
	if (pStudyImage) 
	{
		pStudyImage->onSitePageToolTypeChanged(newToolType);
	}
}

bool StudySeriesImageCell::ResetCellDrawing()
{
	StudyImage *pStudyImage = dynamic_cast<StudyImage *>(m_pCurrentStudyImage);
	if (pStudyImage) 
	{
		ResetToDefault();

		pStudyImage->ResetDrawing();
		
		pStudyImage->ReloadImage();

		pStudyImage->RefreshRenderImage();

		return true;
	}

	return false;
}

MCSF_DJ2DENGINE_END_NAMESPACE