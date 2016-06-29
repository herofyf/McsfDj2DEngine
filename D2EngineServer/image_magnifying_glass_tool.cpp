#include "stdafx.h"
#include "image_magnifying_glass_tool.h"
#include "study_image.h"
#include "study_series_command_request_args.h"
#include "graphics_imager.h"
#include <boost/make_shared.hpp>

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageMagnifyingGlassTool::ImageMagnifyingGlassTool(StudyImage *pStudyImage) :
	ImageDrawingItem(pStudyImage)
{
	m_centerPointF.X = m_centerPointF.Y = 0;

	m_bEnabled = false;
}


ImageMagnifyingGlassTool::~ImageMagnifyingGlassTool(void)
{
}


bool ImageMagnifyingGlassTool::Draw(GraphicsImager *pImager) const
{
	if (m_bEnabled)
	{
		const GraphicsImager *pGraphicsImager = m_pStudyImage->PeekBackground();
		if (pGraphicsImager == NULL) return false;

		pImager->DrawMagnifyGlass(pGraphicsImager, m_centerPointF, m_pStudyImage->ImageWidth() / 10, 2);
		return true;
	}
	return false;
}

int ImageMagnifyingGlassTool::OnImageSizeChanged(float width, float height)
{
	return 0;
}

int ImageMagnifyingGlassTool::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvtArgs)
{
	const int nMinOffset = 12;

	int iRet = 0;
	
	if (IsInterestMouseEvt(pMouseEvtArgs) == false)
		return iRet;

	bool isMouseLeftDown = pMouseEvtArgs->leftDown;
	if (isMouseLeftDown)
	{
		bool isPtInDicomImageRegion = m_pStudyImage->IsPtInDicomImageRegion(pMouseEvtArgs->pointX, pMouseEvtArgs->pointY);
		bool isMoveAway = false;

		if (fabs(m_centerPointF.X - pMouseEvtArgs->pointX) > nMinOffset || 
			fabs(m_centerPointF.Y - pMouseEvtArgs->pointY) > nMinOffset)
		{
			m_centerPointF.X = pMouseEvtArgs->pointX;
			m_centerPointF.Y = pMouseEvtArgs->pointY;

			isMoveAway = true;
		}

		if (isPtInDicomImageRegion && (isMoveAway || m_bEnabled == false))
		{
			m_bEnabled = true;
			iRet = 1;
		}
		else if (isPtInDicomImageRegion == false && m_bEnabled)
		{
			m_bEnabled = false;
			iRet = 1;
		}
		
	}
	else if (m_bEnabled)
	{
		m_bEnabled = false;
		iRet = 1;
	}

	return iRet;
}

int ImageMagnifyingGlassTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

MCSF_DJ2DENGINE_END_NAMESPACE