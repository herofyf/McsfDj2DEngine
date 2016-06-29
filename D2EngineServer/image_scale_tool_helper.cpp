#include "stdafx.h"
#include "image_scale_tool_helper.h"
#include "study_image.h"
#include "study_series_command_request_args.h"
#include "math_float_type.h"
#include "math_line.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageScaleToolHelper::ImageScaleToolHelper(StudyImage *pImage, ImageToolType toolType) :
	TransformationTool(pImage, toolType), m_fScaleXFactor(1), m_fScaleYFactor(1),
	m_nScaleDir(0)
{
	
}


ImageScaleToolHelper::~ImageScaleToolHelper(void)
{
}

int ImageScaleToolHelper::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	int iRet = 0;
	float minFactor = 0.05;

	if (IsInterestMouseEvt(pMouseEvt) == false)
		return iRet;

	bool canTransformation =  IsMouseOffsetTrigTransformation(pMouseEvt, minFactor);
	if (pMouseEvt->leftDown && canTransformation)
	{
		float x10 = m_MousePosParams[1].X - m_MousePosParams[0].X;
		float y10 = m_MousePosParams[1].Y - m_MousePosParams[0].Y;
		if (x10 > 0)
		{
			m_nScaleDir = 1;
		}
		else if (x10 < 0)
		{
			m_nScaleDir = -1;
		}
		else
		{
			return iRet;
		}
	
		float scaleX = m_nScaleDir * minFactor * m_fScaleXFactor ;
		float scaleY = m_nScaleDir * minFactor * m_fScaleYFactor ;
		/*std::cout << "m_MousePosParams[1].X=" << m_MousePosParams[1].X << ",";
		std::cout << "m_MousePosParams[0].X=" << m_MousePosParams[0].X << ",";
		std::cout << "m_MousePosParams[1].Y=" << m_MousePosParams[1].Y << ",";
		std::cout << "m_MousePosParams[0].Y=" << m_MousePosParams[0].Y << ",";
		std::cout << "scale " << m_nScaleDir << std::endl;*/

		iRet = m_pStudyImage->NotifySeriesScale(scaleX, scaleY);
	}

	if (pMouseEvt->leftDown == false)
	{
		m_nScaleDir = 0;
	}
	return iRet;
}
MCSF_DJ2DENGINE_END_NAMESPACE
