#include "stdafx.h"
#include "image_translate_tool_helper.h"
#include "study_image.h"
#include "study_series_command_request_args.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageTranslateToolHelper::ImageTranslateToolHelper(StudyImage *pImage, ImageToolType toolType) :
	TransformationTool(pImage, toolType)
{
}


ImageTranslateToolHelper::~ImageTranslateToolHelper(void)
{
}

int ImageTranslateToolHelper::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	int iRet = 0;

	if (IsInterestMouseEvt(pMouseEvt) == false)
		return iRet;

	bool canTransformation = IsMouseOffsetTrigTransformation(pMouseEvt, 0.03);
	
	if (pMouseEvt->leftDown && canTransformation)
	{
		int offsetx = m_MousePosParams[1].X - m_MousePosParams[0].X;
		int offsety = m_MousePosParams[1].Y - m_MousePosParams[0].Y;
		iRet = m_pStudyImage->NotifySeriesTranslation(offsetx, offsety);
	}
	
	return iRet;
}
MCSF_DJ2DENGINE_END_NAMESPACE
