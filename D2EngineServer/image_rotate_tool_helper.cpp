#include "stdafx.h"
#include "image_rotate_tool_helper.h"
#include "study_image.h"
#include "study_series_command_request_args.h"
#include "math_line.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageRotateToolHelper::ImageRotateToolHelper(StudyImage *pImage, ImageToolType toolType) :
	TransformationTool(pImage, toolType)
{
}


ImageRotateToolHelper::~ImageRotateToolHelper(void)
{
}

int ImageRotateToolHelper::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	int iRet = 0;
	float minAngle = 10;
	bool isClockWise = false;

	if (IsInterestMouseEvt(pMouseEvt) == false)
		return iRet;

	bool canTransformation = IsMouseAngleTrigTransformation(pMouseEvt, isClockWise, minAngle);
	if (pMouseEvt->leftDown && canTransformation)
	{
		float angle = isClockWise ? minAngle : -minAngle;

		iRet = m_pStudyImage->NotifySeriesRotate(angle);
	}
	

	return iRet;
}
MCSF_DJ2DENGINE_END_NAMESPACE
