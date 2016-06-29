#include "stdafx.h"
#include "image_tool_interface.h"
#include "study_image.h"
#include "math_line.h"
#include "coordinate_converter.h"
#include "image_drawing_theme_template.h"
#include "site_work_task.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE


int ImageStreamOperator::SaveToFile(GraphicsImager *pImager, const std::string &fileName, EncoderClsidType type) 
{
	if (pImager == NULL) return -1;

	int iRet = pImager->SaveToFile(fileName, type);
	return iRet;
}

int ImageStreamOperator::SaveToBuffer(GraphicsImager *pImager, EncoderClsidType type, boost::shared_array<char> &bufPtr, const std::string &strTag) 
{
	if (pImager == NULL) return 0;

	int iRet = pImager->SaveToBuffer(type, bufPtr, strTag);

	return iRet;
}

ImageDrawingItem::ImageDrawingItem(const ImageDrawingItem &a)
{
	this->operator=(a);
}

ImageDrawingItem &ImageDrawingItem::operator=(const ImageDrawingItem &a)
{
	this->m_pStudyImage = a.m_pStudyImage;
	return *this;
}

int ImageDrawingItem::NormalizeTextFontSize(int txtFontSize)
{
	if (m_pStudyImage == NULL) return txtFontSize;

	int minSize = m_pStudyImage->ImageWidth() / 50;
	int maxSize = m_pStudyImage->ImageWidth() / 26;
	
	int resultSize = txtFontSize;
	if (txtFontSize < minSize || txtFontSize > maxSize)
	{
		resultSize = (maxSize / 4) * 4;
	}

	return resultSize;
}

bool ImageDrawingItem::ResetDrawing(bool bReset)
{
	m_bResetDrawing = bReset;

	return m_bResetDrawing;
}

bool ImageDrawingItem::NeedResetDrawing()
{
	return m_bResetDrawing;
}

bool ImageDrawingItem::IsInterestMouseEvt(const MouseEvtRequestArgs *pMouseEvt)
{
	if (pMouseEvt == NULL)
		return false;

	return (pMouseEvt->mouseBehavior & m_InterestMouseBehavior);
}

void ImageDrawingItem::SetInterestMouseBehavior(long val)
{
	m_InterestMouseBehavior = val;
}

MeasureToolDrawing::MeasureToolDrawing(StudyImage *pImage, ImageToolType toolType) : 
	TransformableDrawing(pImage), m_toolType(toolType)
{
	SubscribeTool();
}

MeasureToolDrawing::~MeasureToolDrawing()
{
	UnsubscribeTool();

	NotifyMyActiveState(false, false);
	
}

StudyImage *ImageDrawingItem::OwnerImage()
{
	return m_pStudyImage;
}

void MeasureToolDrawing::SubscribeTool()
{
	if (m_pStudyImage)
	{
		m_pStudyImage->AddTool(this);
	}
}

void MeasureToolDrawing::UnsubscribeTool()
{
	if (m_pStudyImage)
	{
		m_pStudyImage->RemoveTool(this);
	}
}

int MeasureToolDrawing::NotifyMyActiveState(bool bActive, bool bDispatchTools)
{
	if (m_pStudyImage)
	{
		m_pStudyImage->HandleToolActiveStateChanged(this, bActive, bDispatchTools);
		return 0;
	}

	return -1;
}

TransformationTool::TransformationTool(StudyImage *pImage, ImageToolType toolType) :
	m_pStudyImage(pImage), m_toolType(toolType), m_nMouseParamState(0),
	m_InterestMouseBehavior(MouseBehaviorMouseMove | MouseBehaviorMouseDown)
{

}

void TransformationTool::SetInterestMouseBehavior(long val)
{
	m_InterestMouseBehavior = val;
}

bool TransformationTool::IsInterestMouseEvt(const MouseEvtRequestArgs *pMouseEvt)
{
	if (pMouseEvt == NULL)
		return false;

	return (pMouseEvt->mouseBehavior & m_InterestMouseBehavior);
}

TransformationTool::~TransformationTool()
{

}

bool TransformationTool::IsMouseAngleTrigTransformation(const MouseEvtRequestArgs *pMouseEvt, bool &isClockWise, float minAngle)
{
	bool bTrigger = false;

	if (pMouseEvt->leftDown)
	{
		if (m_nMouseParamState == 0)
		{
			m_MousePosParams[0].X = pMouseEvt->pointX;
			m_MousePosParams[0].Y = pMouseEvt->pointY;
			m_MousePosParams[1].X = pMouseEvt->pointX;
			m_MousePosParams[1].Y = pMouseEvt->pointY;
			m_nMouseParamState = 1;
		}
		else if (m_nMouseParamState == 1)
		{
			CoordinateTranslator *pCoord = CO_TR_OBJ(m_pStudyImage);
			PointF center(pCoord->GetCenterX(), pCoord->GetCenterY());
			PointF p1(m_MousePosParams[1].X, m_MousePosParams[1].Y);
			PointF p2(pMouseEvt->pointX, pMouseEvt->pointY);

			float angle = CalTriangleAngle(center, p2, p1, isClockWise);
			if (angle > minAngle)
			{
				m_MousePosParams[0].X = m_MousePosParams[1].X;
				m_MousePosParams[0].Y = m_MousePosParams[1].Y;
				m_MousePosParams[1].X = pMouseEvt->pointX;
				m_MousePosParams[1].Y = pMouseEvt->pointY;
				bTrigger = true;
			}
		}
	}
	else
	{
		m_nMouseParamState = 0;
	}

	return bTrigger;
}

bool TransformationTool::IsMouseOffsetTrigTransformation(const MouseEvtRequestArgs *pMouseEvt, float fFactor)
{
	int iRet = 0;
	if (pMouseEvt->leftDown)
	{
		if (m_nMouseParamState == 0)
		{
			m_MousePosParams[0].X = pMouseEvt->pointX;
			m_MousePosParams[0].Y = pMouseEvt->pointY;
			m_MousePosParams[1].X = pMouseEvt->pointX;
			m_MousePosParams[1].Y = pMouseEvt->pointY;
			m_nMouseParamState = 1;
			iRet = 0;
		}
		else if (m_nMouseParamState == 1)
		{
			float minOffset = m_pStudyImage->ImageWidth() * fFactor;

			if (abs(m_MousePosParams[1].X - pMouseEvt->pointX) > minOffset ||
				abs(m_MousePosParams[1].Y - pMouseEvt->pointY) > minOffset)
			{
				m_MousePosParams[0].X = m_MousePosParams[1].X;
				m_MousePosParams[0].Y = m_MousePosParams[1].Y;
				m_MousePosParams[1].X = pMouseEvt->pointX;
				m_MousePosParams[1].Y = pMouseEvt->pointY;
				iRet =  1;
			}	
		}
	}
	else
	{
		m_nMouseParamState = 0;
	}
	return iRet;
}

SiteWorkTask *TransformableDrawing::OwnerSite()
{
	if (m_pStudyImage)
	{
		return m_pStudyImage->OwnerSite();
	}
	return NULL;
}

bool TransformableDrawing::onApplyTransformationState(const TransformationArgs *args)
{
	return true;
}

bool TransformableDrawing::TransformPointsByCellCoord(const TransformationArgs *pTransformationArgs, PointF *pPoints, int size)
{
	bool result = false;

	CoordinateTranslator *pCoordHelper = CO_TR_OBJ(OwnerImage());
	if (pCoordHelper == NULL) return result;

	if (pTransformationArgs->transformationType == TRANS_TRANLATE)
	{
		float translateX = pTransformationArgs->args.translateArgs.offsetX;
		float translateY = pTransformationArgs->args.translateArgs.offsetY;

		pCoordHelper->TranslateShape(translateX, translateY, pPoints, size);
		result = true;
	}

	if (pTransformationArgs->transformationType == TRANS_SCALE)
	{
		float scaleX = pTransformationArgs->args.scaleArgs.scaleX;
		float scaleY = pTransformationArgs->args.scaleArgs.scaleY;

		result = pCoordHelper->ScaleShape(scaleX, scaleY, pPoints, size);
	}

	if (pTransformationArgs->transformationType == TRANS_ROTATE)
	{
		float angle = pTransformationArgs->args.rotateArg.angle;
		
		pCoordHelper->RotateShape(angle, pPoints, size);

		result = true;
	}

	if (pTransformationArgs->transformationType == TRANS_FLIPX)
	{
		// reverse just same do it again.
		pCoordHelper->FlipX(pPoints, size);
		result = true;
	}

	if (pTransformationArgs->transformationType == TRANS_FLIPY)
	{
		pCoordHelper->FlipY(pPoints, size);
		result = true;
	}
	return result;
}


RectF MeasureToolDrawing::CalNormTextSize(const std::string &str) const
{
	if (m_pMeasureToolAppearance)
	{
		return m_pMeasureToolAppearance->CalNormTextSize(str);
	}
}

RectF MeasureToolDrawing::CalHlTextSize(const std::string &str) const
{
	if (m_pMeasureToolAppearance)
	{
		return m_pMeasureToolAppearance->CalHlTextSize(str);
	}
}


MCSF_DJ2DENGINE_END_NAMESPACE
