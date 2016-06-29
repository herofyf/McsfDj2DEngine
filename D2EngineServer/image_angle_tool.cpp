#include "stdafx.h"
#include "image_angle_tool.h"
#include "study_image.h"
#include "graphics_imager.h"
#include "math_point.h"
#include <sstream>
#include<float.h>
#include "mcsf_dj2dengine_log.h"
#include "geometry_math.h"
#include "mcsf_dj2dengine_utility.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageAngleTool::ImageAngleTool(StudyImage *pStudyImage, ImageToolType toolType) :
	MeasureToolDrawing(pStudyImage, toolType), m_eToolState(TR_TS_INIT), 
	m_pCurMouseEvt(NULL), m_fAngleValue(0), m_nTextHeight(0), m_nTextWidth(0),
	m_isArcCW(false)
{
	m_pMeasureToolAppearance = &(m_pStudyImage->GetDrawingAppearance()->angleToolAppearance);
	for (int i = 0; i < TRI_TOOL_MAXP; i ++)
	{
		m_points[i].X = m_points[i].Y = -1;
	}
	
	m_textStartPoint.X = m_textStartPoint.Y = 0;

	m_nTextHeight = m_nTextWidth = m_nHlTextHeight = m_nHlTextWidth = 0;
	
	// first to attach to the last one
	m_textAttachPointIndex = 2;
}


ImageAngleTool::~ImageAngleTool(void)
{

}

int ImageAngleTool::OnToolActiveStateEvent(MeasureToolDrawing *pSender, bool bActive)
{
	int iRet = 0;

	if (pSender != this && bActive)
	{
		if (IsShapeCompleted())
		{
			if (IsShapeActive())
			{
				m_eToolState = TR_TS_COMP;
				iRet = 1;
			}
		}
		else
		{
			delete this;
		}
	}

	return iRet;
}

bool ImageAngleTool::IsShapeCompleted() const
{
	return (m_eToolState == TR_TS_COMP	  || m_eToolState == TR_TS_ACTP1	||
		    m_eToolState == TR_TS_ACTP2   || m_eToolState == TR_TS_ACTTRI   ||
			m_eToolState == TR_TS_TRIMOVE || m_eToolState == TR_TS_ACTTXT);
}

bool ImageAngleTool::IsShapeActive() const
{	
	return (m_eToolState == TR_TS_CENTERP_PEND	||  m_eToolState == TR_TS_CENTERP_COMP	||
			m_eToolState == TR_TS_P1_PEND	||  m_eToolState == TR_TS_P1_COMP   ||
			m_eToolState == TR_TS_P2_PEND   ||  m_eToolState == TR_TS_ACTP1		||
			m_eToolState == TR_TS_ACTP2		||  m_eToolState == TR_TS_ACTTRI    ||
			m_eToolState == TR_TS_TRIMOVE   ||  m_eToolState == TR_TS_ACTTXT);	
}

bool ImageAngleTool::CheckPoint1Active(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_points[1].X, m_points[1].Y);
	if (b)
	{
		m_points[1].X = x;
		m_points[1].Y = y;
		m_eToolState = TR_TS_ACTP1;
	}
	return b;
}

bool ImageAngleTool::CheckPoint2Active(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_points[2].X, m_points[2].Y);
	if (b)
	{
		m_points[2].X = x;
		m_points[2].Y = y;
		m_eToolState = TR_TS_ACTP2;
	}
	return b;
}

bool ImageAngleTool::CheckAngleActive(float x, float y)
{
	bool b = IsPointInLine(m_points[1].X, m_points[1].Y, m_points[0].X, m_points[0].Y, x, y);
	if (b == false)
	{
		b = IsPointInLine(m_points[2].X, m_points[2].Y, m_points[0].X, m_points[0].Y, x, y);
		if (b == false)
		{
			return false;
		}
	}

	m_eToolState = TR_TS_TRIMOVE;
	m_newestMousePoint.X = x;
	m_newestMousePoint.Y = y;
	return true;
}

bool ImageAngleTool::CheckAngleTextActive(float x , float y)
{
	bool b = IsPointInViewPort(m_textStartPoint.X, m_textStartPoint.Y, m_nTextWidth, m_nTextHeight, x, y);
	if (b)
	{
		/*m_textStartPoint.X = x;
		m_textStartPoint.Y = y;*/
		m_eToolState = TR_TS_ACTTXT;
	}
	return b;
}

bool ImageAngleTool::OnCompClick()
{
	bool bSelected = false;

	if (m_pCurMouseEvt->leftDown)
	{
		bSelected = true;
		bool b = CheckPoint1Active(m_pCurMouseEvt->pointX, m_pCurMouseEvt->pointY);
		if (b == false)
		{
			b = CheckPoint2Active(m_pCurMouseEvt->pointX, m_pCurMouseEvt->pointY);
			if (b == false)
			{
				b = CheckAngleActive(m_pCurMouseEvt->pointX, m_pCurMouseEvt->pointY);
				if (b == false)
				{
					b = CheckAngleTextActive(m_pCurMouseEvt->pointX, m_pCurMouseEvt->pointY);
					if ( b == false )
					{
						m_eToolState = TR_TS_COMP;
						bSelected = false;
					}
					else
					{
						m_activeLastMousePoint = m_newestMousePoint;
					}
				}
				else
				{
					m_activeLastMousePoint = m_newestMousePoint;
				}
			}
		}

	}
	return bSelected;
}

int ImageAngleTool::OnImageSizeChanged(float width, float height)
{
	return 0;
}
bool ImageAngleTool::onInit()
{
	if (m_pCurMouseEvt->leftDown)
	{
		m_points[0] = m_newestMousePoint;
		m_points[1] = m_points[0];
		m_points[2] = m_points[0];
		m_eToolState = TR_TS_CENTERP_PEND;
	}
	return false;
}

bool ImageAngleTool::onCenterPPending()
{
	bool bChanged = false;
	
	if (m_pCurMouseEvt->leftDown)
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_points[1], m_newestMousePoint, ANGLE_MOUSE_MOVE_MIN_OFFSET))
		{
			m_points[1] = m_newestMousePoint;
			m_eToolState = TR_TS_P1_PEND;
		}
	}
	else
	{
		m_eToolState = TR_TS_CENTERP_COMP;
	}

	return bChanged;
}

bool ImageAngleTool::onCenterPComp()
{
	bool bChanged = false;

	if (m_pCurMouseEvt->leftDown)
	{
		m_eToolState = TR_TS_P1_PEND;
	}

	if (GdiPlusTypeOperator::IsPointFGreater(m_points[1], m_newestMousePoint, ANGLE_MOUSE_MOVE_MIN_OFFSET))
	{
		m_points[1] = m_newestMousePoint;
		bChanged =  true;
	}
	
	return bChanged;
}

/*
 when p1 click left has down, and waiting for click up 
*/
bool ImageAngleTool::onP1Pending()
{
	bool bChanged = false;
	if (m_pCurMouseEvt->leftDown == false)
	{
		m_points[1] = m_newestMousePoint;
		m_textStartPoint = m_points[1];
		m_eToolState = TR_TS_P1_COMP;
		bChanged = true;
	}
	else
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_points[1], m_newestMousePoint, ANGLE_MOUSE_MOVE_MIN_OFFSET))
		{
			m_points[1] = m_newestMousePoint;
			bChanged =  true;
		}
	}

	return bChanged;
}

/*
 when p2 is moving while click up, and if waitted for click down. then p2 is locationed. to 
*/
bool ImageAngleTool::onP1Comp()
{
	bool bChanged = false;
	if (m_pCurMouseEvt->leftDown)
	{
		m_points[2] = m_newestMousePoint;
		CalArcAppendix();
		m_eToolState = TR_TS_P2_PEND;
		bChanged = true;
	}
	else
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_points[2], m_newestMousePoint, 3))
		{
			m_points[2] = m_newestMousePoint;
			CalArcAppendix();
			bChanged =  true;
		}
	}
	return bChanged;
}

void ImageAngleTool::CalTextPosition()
{
	if (m_eToolState <= TR_TS_COMP)
	{
		m_textStartPoint = m_points[2] + PointF(m_pMeasureToolAppearance->textXOffset, 0);
	}
	else
	{
		float minLength = 0x8000000;
		
		for (int i =0; i < 3; i ++)
		{
			float len = CalLineLen(m_points[i].X, m_points[i].Y, m_textStartPoint.X, m_textStartPoint.Y);
			if (len < minLength)
			{
				minLength = len;
				m_textAttachPointIndex = i;
			}
		}
	}
}

float ImageAngleTool::CalAngleValue(bool &isClockWise)
{
	float angle = CalTriangleAngle(m_points[0], m_points[1], m_points[2], isClockWise);
	return angle;
}

void ImageAngleTool::CalArcPointsPostionValue()
{
	// to calculate the point3,4
	float lenP21 = CalLineLen(m_points[1].X, m_points[1].Y, m_points[0].X, m_points[0].Y);
	float lenP31 = CalLineLen(m_points[2].X, m_points[2].Y, m_points[0].X, m_points[0].Y);

	float dis = (lenP21 > lenP31) ? lenP31 : lenP21;
	dis = dis / 2;
	CalLinePoint(m_points[0].X, m_points[0].Y, m_points[1].X, m_points[1].Y, dis, &(m_points[3].X), &(m_points[3].Y));
	CalLinePoint(m_points[0].X, m_points[0].Y, m_points[2].X, m_points[2].Y, dis, &(m_points[4].X), &(m_points[4].Y));

	m_fAngleValue = CalAngleValue(m_isArcCW);

	std::stringstream ssAngle;
	ssAngle << "Angle:";
	ssAngle << StringConverter::FloatToString(round1f(m_fAngleValue), 1);
	
	
	std::string strValue = ssAngle.str();
	if (m_strAngleValue.length() != strValue.length())
	{
		Gdiplus::RectF textRectF = m_pMeasureToolAppearance->CalNormTextSize(strValue);
		m_nTextWidth  = textRectF.Width;
		m_nTextHeight = textRectF.Height;

		textRectF = m_pMeasureToolAppearance->CalHlTextSize(strValue);
		m_nHlTextWidth = textRectF.Width;
		m_nHlTextHeight = textRectF.Height;
	}

	m_strAngleValue = strValue;
}

void ImageAngleTool::CalArcAppendix()
{
	CalTextPosition();
	CalArcPointsPostionValue();
}

bool ImageAngleTool::onP2Pending()
{
	if (m_pCurMouseEvt->leftDown == false)
	{
		m_points[2] = m_newestMousePoint;
		m_eToolState = TR_TS_COMP;
		CalArcAppendix();
		return true;
	}

	return false;
}

/*
	1. to equal two line length
	2. triangle/ p1/p3 active
	3.

*/
bool ImageAngleTool::onComp()
{
	if (m_pCurMouseEvt->leftDown)
	{
		return OnCompClick();
	}

	return false;
}


bool ImageAngleTool::onActP1()
{
	bool bChanged = false;
	// to move point1
	if (m_pCurMouseEvt->leftDown)
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_points[1], m_newestMousePoint, 3))
		{
			m_points[1] = m_newestMousePoint;
			CalArcAppendix();
			bChanged = true;
		}
	}
	else
	{
		bChanged = true;
		m_eToolState = TR_TS_ACTTRI;
	}

	return bChanged;
}

bool ImageAngleTool::onActP2()
{
	bool bChanged = false;
	// to move point1
	if (m_pCurMouseEvt->leftDown)
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_points[2], m_newestMousePoint, 3))
		{
			m_points[2] = m_newestMousePoint;
			CalArcAppendix();
			bChanged = true;
		}
	}
	else
	{
		bChanged = true;
		m_eToolState = TR_TS_ACTTRI;
	}

	return bChanged;
}

bool ImageAngleTool::onActText()
{
	bool bChanged = false;

	if (m_pCurMouseEvt->leftDown)
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_textStartPoint, m_newestMousePoint, ANGLE_MOUSE_MOVE_MIN_OFFSET))
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(m_newestMousePoint.X - m_activeLastMousePoint.X),
				(m_newestMousePoint.Y - m_activeLastMousePoint.Y),
				&m_textStartPoint,
				1);

			m_activeLastMousePoint = m_newestMousePoint;
			CalTextPosition();
			bChanged = true;
		}
	}
	else
	{
		bChanged = true;
		m_eToolState = TR_TS_ACTTRI;
	}
	return bChanged;
}

/*
	after move p1/p2/angle. this angle is in active
*/
bool ImageAngleTool::onActTri()
{
	bool bChanged = false;

	if (m_pCurMouseEvt->leftDown)
	{
		OnCompClick();

		bChanged = (m_eToolState == TR_TS_COMP);
	}

	return bChanged;
}

/*
	when triangle active and to move mouse
*/
bool ImageAngleTool::onTriMove()
{
	bool bChanged = false;
	if (m_pCurMouseEvt->leftDown)
	{
		if (GdiPlusTypeOperator::IsPointFGreater(m_activeLastMousePoint, m_newestMousePoint, 3))
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(m_newestMousePoint.X - m_activeLastMousePoint.X),
				(m_newestMousePoint.Y - m_activeLastMousePoint.Y),
				m_points,
				sizeof(m_points)/ sizeof(m_points[0]));

			m_activeLastMousePoint = m_newestMousePoint;
			CalTextPosition();
			bChanged = true;
		}
		
	}
	else
	{
		m_eToolState = TR_TS_ACTTRI;
	}

	return bChanged;
}


int ImageAngleTool::OnMouseEvent(const MouseEvtRequestArgs *mouseEventInfo)
{
	bool bChanged = false;

	if (IsInterestMouseEvt(mouseEventInfo) == false)
		return bChanged;

	m_pCurMouseEvt = mouseEventInfo;

	m_newestMousePoint.X = m_pCurMouseEvt->pointX;
	m_newestMousePoint.Y = m_pCurMouseEvt->pointY;

	if (m_eToolState == TR_TS_INIT)
	{
		bChanged = onInit();
	}
	else if (m_eToolState == TR_TS_CENTERP_PEND)
	{
		bChanged = onCenterPPending();
	}
	else if (m_eToolState == TR_TS_CENTERP_COMP)
	{
		bChanged = onCenterPComp();
	}
	else if (m_eToolState == TR_TS_P1_PEND)
	{
		bChanged = onP1Pending();
	}
	else if (m_eToolState == TR_TS_P1_COMP)
	{
		bChanged = onP1Comp();
	}
	else if (m_eToolState == TR_TS_P2_PEND)
	{
		bChanged = onP2Pending();
	}
	else if (m_eToolState == TR_TS_COMP)
	{
		bChanged = onComp();
	}
	else if (m_eToolState == TR_TS_ACTP1)
	{
		bChanged = onActP1();
	}
	else if (m_eToolState == TR_TS_ACTP2)
	{
		bChanged = onActP2();
	}
	else if (m_eToolState == TR_TS_ACTTXT)
	{
		bChanged = onActText();
	}
	else if (m_eToolState == TR_TS_ACTTRI)
	{
		bChanged = onActTri();
	}
	else if (m_eToolState == TR_TS_TRIMOVE)
	{
		bChanged = onTriMove();
	}
	else 
	{
		bChanged = false;
	}

	if (bChanged)
	{
		bool bActive = IsShapeActive();
		NotifyMyActiveState(bActive);

		//DumpObject();
	}
	
	m_pCurMouseEvt = NULL;
	
	return bChanged;
}

int ImageAngleTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{

	return 0;
}


bool ImageAngleTool::Draw(GraphicsImager *pImager) const
{
	if (pImager == NULL) return false;

	if (m_eToolState > TR_TS_INIT)
	{
		LinePropMetaInfo linePropInfo = IsShapeActive() ? m_pMeasureToolAppearance->lineHlProp : m_pMeasureToolAppearance->lineNormProp;
		Color lineDrawingColor =  linePropInfo.lineColor;
		int   lineDrawingWidth = linePropInfo.lineWidth;
		
		if (m_eToolState >= TR_TS_CENTERP_COMP)
		{
			pImager->DrawStartCapLine(lineDrawingColor, lineDrawingWidth, m_points[1], m_points[0], LineCapRoundAnchor, true);
		}

		if (m_eToolState >= TR_TS_P1_COMP)
		{
			pImager->DrawStartCapLine(lineDrawingColor, lineDrawingWidth, m_points[2], m_points[0], LineCapRoundAnchor, true);
			
			pImager->DrawCircleArc(lineDrawingColor, lineDrawingWidth, m_points[0], m_points[3], m_points[4], m_isArcCW);

			FontPropMetaInfo fontProp = IsShapeActive() ? m_pMeasureToolAppearance->textFontHlProp : m_pMeasureToolAppearance->textFontProp;

			pImager->DrawString(m_strAngleValue, m_textStartPoint.X, m_textStartPoint.Y, 
				fontProp.sFontName, fontProp.fontSize, fontProp.fontStyle, fontProp.fontColor);
				
			if (m_strAngleValue.length() > 0)
			{
				int textWidth = IsShapeActive() ? m_nHlTextWidth : m_nTextWidth;
				int angleSymFontSize = fontProp.fontSize / 2;
				angleSymFontSize = angleSymFontSize > 0 ? angleSymFontSize : 2;
				pImager->DrawString("o", m_textStartPoint.X + textWidth + angleSymFontSize, m_textStartPoint.Y, fontProp.sFontName,
					angleSymFontSize, fontProp.fontStyle, fontProp.fontColor);
			}

			if (m_textAttachPointIndex >= 0 && m_textAttachPointIndex < 3)
			{
				pImager->DrawDashLine(m_pMeasureToolAppearance->connectorLineNormProp.lineColor, 
					m_pMeasureToolAppearance->connectorLineNormProp.lineWidth, 
					m_points[m_textAttachPointIndex].X, m_points[m_textAttachPointIndex].Y, 
					(INT)m_textStartPoint.X, (INT)m_textStartPoint.Y);
			}
			
		}
	}

	return true;
}

bool ImageAngleTool::onTransformation(const TransformationArgs *curTransformationArgs)
{
	bool bResult = false;

	bool b1 = TransformPointsByCellCoord(curTransformationArgs, m_points, (sizeof(m_points)/ sizeof(m_points[0])));
	bool b2 = TransformPointsByCellCoord(curTransformationArgs, &m_textStartPoint, 1);
	if (b1 && b2)
	{
		TransformableDrawing::onTransformation(curTransformationArgs);
		bResult = true;
	}

	return bResult;

}

void ImageAngleTool::DumpObject()
{
	std::stringstream log;
	log << "<" << this << ">-";
	log << "m_eToolState = " << m_eToolState << "," << "m_lastMousePoint=" << "(" << m_newestMousePoint.X
		<< "," << m_newestMousePoint.Y << "), {";

	for (int i = 0; i < TRI_TOOL_MAXP; i ++)
	{
		log << "(" << m_points[i].X << "," << m_points[i].Y << ") , ";
	}

	log << "}, ";
	log << "m_textStartPoint=" << "(" << m_textStartPoint.X << "," << m_textStartPoint.Y << ")";
	LOG_INFO(log.str());
}
MCSF_DJ2DENGINE_END_NAMESPACE
