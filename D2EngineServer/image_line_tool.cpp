#include "stdafx.h"
#include "image_line_tool.h"
#include "system_global_configuration.h"
#include <sstream>
#include "geometry_math.h"
#include "image_property_state.h"
#include "image_tool_interface.h"
#include "study_image.h"
#include "image_logic_unit_helper.h"
#include "trace_performance_func.h"
#include "image_drawing_theme_template.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageLineTool::ImageLineTool(StudyImage *pImage, ImageToolType toolType) : 
	MeasureToolDrawing(pImage, toolType), m_bTxtPtfInited(false)
{
	m_pMeasureToolAppearance = &(m_pStudyImage->GetDrawingAppearance()->lineToolAppearance);

	m_textPointF.X = 0;
	m_textPointF.Y = 0;

	m_points[0].X = 0;
	m_points[0].Y = 0;
	m_points[1].X = 0;
	m_points[1].Y = 0;

	m_textAttachedPointF = m_textPointF;

	m_nTextWidth = m_nTextHeight = 0;
	
	m_nState = LI_TS_INIT;
}


ImageLineTool::~ImageLineTool(void)
{
}

bool ImageLineTool::onTransformation(const TransformationArgs *curTransformationArgs)
{
	bool bResult = false;

	bool b1 = TransformPointsByCellCoord(curTransformationArgs, m_points, (sizeof(m_points)/ sizeof(m_points[0])));
	bool b2 = TransformPointsByCellCoord(curTransformationArgs, &m_textPointF, 1);
	bool b3 = TransformPointsByCellCoord(curTransformationArgs, &m_textAttachedPointF, 1);
	if (b1 && b2)
	{
		TransformableDrawing::onTransformation(curTransformationArgs);
		bResult = true;
	}
	
	return bResult;
}

void ImageLineTool::RecalLineCommentText()
{
	LogicUnitValuef logicUnitValue = LOGIC_U_OBJ(OwnerImage())->CalLineLogicLength(m_points, 2);

	std::string strValue = logicUnitValue.toString();
	
	if (m_strText.length() != strValue.length())
	{
		RectF textRectF = m_pMeasureToolAppearance->CalNormTextSize(strValue);

		m_nTextWidth  = textRectF.Width;
		m_nTextHeight = textRectF.Height;
	}
	
	m_strText = strValue;
}

void ImageLineTool::RecalAttachedPt()
{
	if (m_bTxtPtfInited == false)
	{
		float lefty = 0;
		float leftx = 0;
		
		if (m_points[1].X < m_points[0].X) 
		{
			leftx = m_points[1].X;
			lefty = m_points[1].Y;
		}
		else
		{
			leftx = m_points[0].X;
			lefty = m_points[0].Y;
		}
		m_textPointF.X = leftx - m_pMeasureToolAppearance->textXOffset;
		m_textPointF.Y = lefty;

		m_bTxtPtfInited = true;
	}

	FindLinePointShortestDist(m_points[0].X, 
		m_points[0].Y, 
		m_points[1].X, 
		m_points[1].Y, 
		m_textPointF.X, 
		m_textPointF.Y, 
		&m_textAttachedPointF.X, 
		&m_textAttachedPointF.Y);
}

void ImageLineTool::RecalLineComment()
{
	RecalAttachedPt();

	RecalLineCommentText();
}

bool ImageLineTool::checkPoint1Active(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_points[0].X, m_points[0].Y);
	if (b)
	{
		m_points[0].X = x;
		m_points[0].Y = y;
		m_lastMousePoint = m_points[0];
		m_nState = LI_TS_ACT1;
	}
	return b;
}

bool ImageLineTool::checkPoint2Active(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_points[1].X, m_points[1].Y);
	if (b)
	{
		m_points[1].X = x;
		m_points[1].Y = y;
		m_lastMousePoint = m_points[1];
		m_nState = LI_TS_ACT2;
	}
	return b;
}

bool ImageLineTool::checkLineActive(float x, float y)
{
	bool b = IsPointInLine(m_points[0].X, m_points[0].Y, m_points[1].X, m_points[1].Y, x, y);
	if (b)
	{
		m_nState = LI_TS_ACTLINMOVE;
		m_lastMousePoint.X = x;
		m_lastMousePoint.Y = y;
	}
	return b;
}

bool ImageLineTool::checkLineTextActive(float x, float y)
{
	bool b = IsPointInViewPort(m_textPointF.X, m_textPointF.Y, m_nTextWidth, m_nTextHeight, x, y);
	if (b)
	{
		m_lastMousePoint.X = x;
		m_lastMousePoint.Y = y;
		m_nState = LI_TS_ACTTXT;
	}

	return b;
}

/*
	whether there has object selected
*/
bool ImageLineTool::onLineClick(float x, float y)
{
	bool bResult = true;

	bool b = checkPoint1Active(x, y);
	if ( !b )
	{
		b = checkPoint2Active(x, y);
		if(!b)
		{
			b = checkLineActive(x, y);
			if (!b)
			{
				b = checkLineTextActive(x, y);
				if (!b)
				{
					m_nState = LI_TS_COMP;
					bResult = false;
				}
			}
		}
	}
	return bResult;
}


int ImageLineTool::OnMouseEvent(const MouseEvtRequestArgs *pMouseEventInfo)
{
	int bRedraw = 0;

	if (IsInterestMouseEvt(pMouseEventInfo) == false)
		return bRedraw;

	bool leftDown = pMouseEventInfo->leftDown;
	int x = pMouseEventInfo->pointX;
	int y = pMouseEventInfo->pointY;

	if (m_nState == LI_TS_INIT)
	{
		if (leftDown)
		{
			m_points[0].X = x;
			m_points[0].Y = y;
			m_lastMousePoint = m_points[0];
			m_points[1] = m_points[0];
			m_nState = LI_TS_P1;
		}
	}
	else if (m_nState == LI_TS_P1)
	{
		m_lastMousePoint.X = x;
		m_lastMousePoint.Y = y;
		
		if (leftDown && false == m_points[0].Equals(m_lastMousePoint))
		{
			m_points[1] = m_lastMousePoint;
			m_nState = LI_TS_P2PENDING;
			bRedraw = 1;
		}
		else
		{
			m_nState = LI_TS_ABORT;
		}
	}
	else if (m_nState == LI_TS_P2PENDING)
	{
		m_lastMousePoint.X = x;
		m_lastMousePoint.Y = y;

		if (leftDown)
		{
			m_points[1] = m_lastMousePoint;
			bRedraw = 1;
		}
		else
		{
			// there should have some position changed
			if (false == m_points[0].Equals(m_points[1]))
			{
				m_points[1] = m_lastMousePoint;
				
				RecalLineComment();
				m_nState = LI_TS_COMP;
				bRedraw = 1;
			}		
		}
	}
	else if (m_nState == LI_TS_COMP)
	{
		// check 
		if (leftDown)
		{
			onLineClick(x, y);

			if (m_nState != LI_TS_COMP)
			{
				bRedraw = 1;
			}
		}
	}
	else if (m_nState == LI_TS_ACTLINMOVE)
	{
		if (leftDown)
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(x - m_lastMousePoint.X),
				(y - m_lastMousePoint.Y),
				m_points,
				sizeof(m_points)/ sizeof(m_points[0]));
	
			m_lastMousePoint.X = x;
			m_lastMousePoint.Y = y;
			RecalAttachedPt();
		}
		else
		{
			m_nState = LI_TS_ACTL;
		}
		bRedraw = 1;
	}
	else if (m_nState == LI_TS_ACTL)
	{
		if (leftDown)
		{
			onLineClick(x, y);

			bRedraw = 1;
		}
	}
	else if (m_nState == LI_TS_ACT1)
	{
		if (leftDown)
		{
			m_points[0].X = x;
			m_points[0].Y = y;
			m_lastMousePoint = m_points[0];
			RecalLineComment();
		}
		else
		{
			m_nState = LI_TS_ACTL;
		}
		bRedraw = 1;
	}
	else if (m_nState == LI_TS_ACT2)
	{
		if (leftDown)
		{
			m_points[1].X = x;
			m_points[1].Y = y;
			m_lastMousePoint = m_points[1];
			RecalLineComment();
		}
		else
		{
			m_nState = LI_TS_ACTL;
		}
		
		bRedraw = 1;
	}
	else if (m_nState == LI_TS_ACTTXT)
	{
		if (leftDown)
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(x - m_lastMousePoint.X),
				(y - m_lastMousePoint.Y),
				&m_textPointF, 1);

			m_lastMousePoint.X = x;
			m_lastMousePoint.Y = y;
			RecalAttachedPt();
		}
		else
		{
			m_nState = LI_TS_ACTL;
		}
		bRedraw = 1;
	}

	if (m_nState == LI_TS_ABORT)
	{
		bRedraw = 0;
		delete this;
	}
	else if (bRedraw > 0)
	{
		bool bActive = IsShapeActive();
		NotifyMyActiveState(bActive);
	}
	return bRedraw;
}

int ImageLineTool::OnToolActiveStateEvent(MeasureToolDrawing *pSender, bool bActive)
{
	int iRet = 0;

	if (bActive && pSender != this)
	{
		if (IsShapeCompleted())
		{
			if (IsShapeActive())
			{
				m_nState = LI_TS_COMP;
				iRet =  1;
			}
		}
		else
		{
			delete this;
		}
	}
	return iRet;
}

int ImageLineTool::OnImageSizeChanged(float width, float height)
{
	return 0;
}

int ImageLineTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	
	return 0;
}

bool ImageLineTool::IsShapeCompleted() const
{
	return (m_nState == LI_TS_COMP || m_nState == LI_TS_ACT1 ||
		    m_nState == LI_TS_ACT2 || m_nState == LI_TS_ACTLINMOVE ||
			m_nState == LI_TS_ACTL || m_nState == LI_TS_ACTTXT);
}


bool ImageLineTool::IsShapeActive() const
{
	return (m_nState == LI_TS_P1   || m_nState == LI_TS_P2PENDING  ||
			m_nState == LI_TS_ACT2 || m_nState == LI_TS_ACTLINMOVE ||
			m_nState == LI_TS_ACTL || m_nState == LI_TS_ACTTXT ||
			m_nState == LI_TS_ACT1);
}

bool ImageLineTool::Draw(GraphicsImager *pImager) const
{
	if (pImager == NULL)
		return false;

	//CPU_PERF_CAL cal("ImageLineTool::Draw");
	
	bool bActive = IsShapeActive();
	bool bCompleted = IsShapeCompleted();

	Color lineDrawingColor = bActive ? m_pMeasureToolAppearance->lineHlProp.lineColor : m_pMeasureToolAppearance->lineNormProp.lineColor;
	int   lineDrawingWidth = bActive ? m_pMeasureToolAppearance->lineHlProp.lineWidth : m_pMeasureToolAppearance->lineNormProp.lineWidth;
	pImager->DrawCapLine(lineDrawingColor, lineDrawingWidth, m_points[0].X, m_points[0].Y, m_points[1].X, m_points[1].Y, 
			 LineCapRoundAnchor, true);

	if (bCompleted)
	{
		FontPropMetaInfo fontProp = bActive ? m_pMeasureToolAppearance->textFontHlProp : m_pMeasureToolAppearance->textFontProp;
		pImager->DrawString(m_strText, m_textPointF.X, m_textPointF.Y, 
			fontProp.sFontName, fontProp.fontSize, fontProp.fontStyle, fontProp.fontColor, true);
			

		pImager->DrawDashLine(m_pMeasureToolAppearance->connectorLineNormProp.lineColor, 
			m_pMeasureToolAppearance->connectorLineNormProp.lineWidth, 
			m_textAttachedPointF.X, m_textAttachedPointF.Y, (INT)m_textPointF.X, (INT)m_textPointF.Y);
	}
	
	return true;
}

MCSF_DJ2DENGINE_END_NAMESPACE
