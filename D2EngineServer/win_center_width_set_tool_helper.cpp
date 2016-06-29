#include "stdafx.h"
#include "win_center_width_set_tool_helper.h"
#include "study_image.h"
#include "study_series_command_request_args.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

WinCenterWidthSetToolHelper::WinCenterWidthSetToolHelper(StudyImage *pImage, ImageToolType toolType) :
	TransformationTool(pImage, toolType)
{
}


WinCenterWidthSetToolHelper::~WinCenterWidthSetToolHelper(void)
{
}

int WinCenterWidthSetToolHelper::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	int iRet = 0;

	if (IsInterestMouseEvt(pMouseEvt) == false)
		return iRet;

	bool canTransformation = IsMouseOffsetTrigTransformation(pMouseEvt, 0.05);
	if (pMouseEvt->leftDown && canTransformation)
	{
		float absY10 = fabs(m_MousePosParams[1].Y - m_MousePosParams[0].Y);
		float absX10 = fabs(m_MousePosParams[1].X - m_MousePosParams[0].X);

		float scaleFactor = 0.1;
		// vertical move
		if (absY10 > absX10)
		{
			scaleFactor = (m_MousePosParams[1].Y > m_MousePosParams[0].Y) ? -1 * scaleFactor : 1 * scaleFactor;
		}
		else // horizontal move
		{
			scaleFactor = (m_MousePosParams[1].X > m_MousePosParams[0].X) ? 1 * scaleFactor : -1 * scaleFactor;
		}

		m_pStudyImage->NotifySeriesSetWinWidthCenter(scaleFactor, scaleFactor, false);
		iRet = 1;
	}
	
	return iRet;
}
MCSF_DJ2DENGINE_END_NAMESPACE
