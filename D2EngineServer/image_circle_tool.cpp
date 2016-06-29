#include "stdafx.h"
#include "image_circle_tool.h"
#include "study_image.h"
#include "geometry_math.h"
#include <sstream>
#include "mcsf_dj2dengine_utility.h"
#include "image_region_pixels_drawing.h"
#include "dicom_region_pixel_helper.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageCircleTool::ImageCircleTool(StudyImage *pImage, ImageToolType toolType) :
	MeasureToolDrawing(pImage, toolType)
{
	m_pMeasureToolAppearance = &(m_pStudyImage->GetDrawingAppearance()->circleToolAppearance);

	m_centerPointF.X = m_centerPointF.Y = 0;
	m_attachedCirclePf = m_textStartPointF = m_lastMousePointF = m_centerPointF;

	m_initedTextPointF = false;
	m_fRadius =0;
	m_fHitTolerance = 10;
	m_nState = CIR_TS_INIT;
	
	m_pRegionPixelsStDrawing = new ImageDicomRegPixelsStDrawing(this);
}


ImageCircleTool::~ImageCircleTool(void)
{
	DEL_PTR(m_pRegionPixelsStDrawing);
}

void ImageCircleTool::RecalHitTolerance()
{
	float fTolerance = m_fRadius / 10;
	if (fTolerance < 3)
		fTolerance = 3;

	if (fTolerance > 10)
		fTolerance = 10;
	m_fHitTolerance = fTolerance;
}

/*
	cross flat part 1 and point1 to move.
	I use point1 as anchor
*/
void ImageCircleTool::RecalCircleComponents_OnCFP1Point(int movedPointI)
{
	if (movedPointI < 0 || movedPointI >= 2) return;

	// to calculate the center point
	m_fRadius = CalLineLen(m_crossFlatPart1PointFs[0].X, m_crossFlatPart1PointFs[0].Y, m_crossFlatPart1PointFs[1].X, m_crossFlatPart1PointFs[1].Y) /2;
	RecalHitTolerance();
	m_centerPointF.X = (m_crossFlatPart1PointFs[0].X + m_crossFlatPart1PointFs[1].X) / 2;
	m_centerPointF.Y = (m_crossFlatPart1PointFs[0].Y + m_crossFlatPart1PointFs[1].Y) / 2;

	int anchorPointI = (movedPointI == 0) ? 1 : 0;
	int part2PointI = anchorPointI;

	/*
	 should know, counter clock-wise is opposite
	*/
	// to calculate
	CalCircleRotatedPoint(m_centerPointF.X, m_centerPointF.Y, m_crossFlatPart1PointFs[anchorPointI].X, 
		m_crossFlatPart1PointFs[anchorPointI].Y, 90, &(m_crossFlatPart2PointFs[part2PointI].X), &(m_crossFlatPart2PointFs[part2PointI].Y));

	part2PointI = (anchorPointI + 1) % 2;

	CalCircleRotatedPoint(m_centerPointF.X, m_centerPointF.Y, m_crossFlatPart1PointFs[anchorPointI].X, 
		m_crossFlatPart1PointFs[anchorPointI].Y, 270, &(m_crossFlatPart2PointFs[part2PointI].X), &(m_crossFlatPart2PointFs[part2PointI].Y));

	RecalCircleCommentText();
}



void ImageCircleTool::RecalCircleComponents_OnCFP2Point(int movedPointI)
{
	if (movedPointI < 0 || movedPointI >= 2) return;

	// to calculate the center point
	m_fRadius = CalLineLen(m_crossFlatPart2PointFs[0].X, m_crossFlatPart2PointFs[0].Y, m_crossFlatPart2PointFs[1].X, m_crossFlatPart2PointFs[1].Y) /2;

	RecalHitTolerance();
	m_centerPointF.X = (m_crossFlatPart2PointFs[0].X + m_crossFlatPart2PointFs[1].X) / 2;
	m_centerPointF.Y = (m_crossFlatPart2PointFs[0].Y + m_crossFlatPart2PointFs[1].Y) / 2;

	int anchorPointI = (movedPointI == 0) ? 1 : 0;
	int part1PointI = anchorPointI;

	/*
	 should know, counter clock-wise is opposite
	*/
	// to calculate
	CalCircleRotatedPoint(m_centerPointF.X, m_centerPointF.Y, m_crossFlatPart2PointFs[anchorPointI].X, 
		m_crossFlatPart2PointFs[anchorPointI].Y, 90, &(m_crossFlatPart1PointFs[part1PointI].X), &(m_crossFlatPart1PointFs[part1PointI].Y));

	part1PointI = (anchorPointI + 1) % 2;

	CalCircleRotatedPoint(m_centerPointF.X, m_centerPointF.Y, m_crossFlatPart2PointFs[anchorPointI].X, 
		m_crossFlatPart2PointFs[anchorPointI].Y, 270, &(m_crossFlatPart1PointFs[part1PointI].X), &(m_crossFlatPart1PointFs[part1PointI].Y));

	RecalCircleCommentText();
}

void ImageCircleTool::RecalCircleCommentTextPos()
{
	if (m_initedTextPointF == false)
	{
		m_initedTextPointF = true;
		m_attachedCirclePf = m_crossFlatPart1PointFs[0];
		m_textStartPointF = m_attachedCirclePf;
		m_textStartPointF.X = m_attachedCirclePf.X + m_pMeasureToolAppearance->textXOffset;
	}
	else 
	{
		float fMin = 1e9;
		PointF pointf;

		for (int i = 0; i < P_ALLPART_PS; i ++)
		{
			if (i == P_PART1_P0)
				pointf = m_crossFlatPart1PointFs[0];
			else if (i == P_PART1_P1)
				pointf = m_crossFlatPart1PointFs[1];
			else if (i == P_PART2_P0)
				pointf = m_crossFlatPart2PointFs[0];
			else if (i == P_PART2_P1)
				pointf = m_crossFlatPart2PointFs[1];
			else
				continue;
			float fVal = CalLineLen(m_textStartPointF.X, m_textStartPointF.Y, pointf.X, pointf.Y);
			if (fVal < fMin)
			{
				fMin = fVal;
				m_attachedCirclePf = pointf;
			}
		}
	}
	
	if (m_pRegionPixelsStDrawing)
	{
		m_pRegionPixelsStDrawing->UpdateTextStartPointf(m_textStartPointF);
	}
}

void ImageCircleTool::RecalCircleCommentText()
{
	if (m_pStudyImage == NULL || m_pRegionPixelsStDrawing == NULL) return;

	DicomRegionPixelsStatistics st;
	
	m_pStudyImage->StatisticsCirclePixels(m_centerPointF, m_fRadius, &st);

	RecalCircleCommentTextPos();
	
	m_pRegionPixelsStDrawing->UpdateStValues(m_textStartPointF, &st);
}

bool ImageCircleTool::CheckCircleLine()
{
	float validRange_InnerCircleRadius = m_fRadius - 10;
	float validRange_OutterCircleRadius = m_fRadius + 10;

	float center_to_p = CalLineLen(m_centerPointF.X, m_centerPointF.Y, m_lastMousePointF.X, m_lastMousePointF.Y);
	if (center_to_p >= validRange_InnerCircleRadius && center_to_p <= validRange_OutterCircleRadius)
	{
		return true;
	}

	return false;
}

bool ImageCircleTool::CheckCircleAnchorPoints(CircleToolState &state)
{
	float anchorp_to_p = CalLineLen(m_crossFlatPart1PointFs[0].X, m_crossFlatPart1PointFs[0].Y, m_lastMousePointF.X, m_lastMousePointF.Y);
	if (anchorp_to_p < m_fHitTolerance)
	{
		state = CIR_TS_CIRCLE_MOVE_CFP1_P0;
		return true;
	}

	anchorp_to_p = CalLineLen(m_crossFlatPart1PointFs[1].X, m_crossFlatPart1PointFs[1].Y, m_lastMousePointF.X, m_lastMousePointF.Y);
	if (anchorp_to_p < m_fHitTolerance)
	{
		state = CIR_TS_CIRCLE_MOVE_CFP1_P1;
		return true;
	}

	anchorp_to_p = CalLineLen(m_crossFlatPart2PointFs[0].X, m_crossFlatPart2PointFs[0].Y, m_lastMousePointF.X, m_lastMousePointF.Y);
	if (anchorp_to_p < m_fHitTolerance)
	{
		state = CIR_TS_CIRCLE_MOVE_CFP2_P0;
		return true;
	}

	anchorp_to_p = CalLineLen(m_crossFlatPart2PointFs[1].X, m_crossFlatPart2PointFs[1].Y, m_lastMousePointF.X, m_lastMousePointF.Y);
	if (anchorp_to_p < m_fHitTolerance)
	{
		state = CIR_TS_CIRCLE_MOVE_CFP2_P1;
		return true;
	}

	return false;
}

bool ImageCircleTool::CheckCommentText()
{
	if (m_pRegionPixelsStDrawing)
	{
		return m_pRegionPixelsStDrawing->IsWithinTextRegion(m_lastMousePointF.X, m_lastMousePointF.Y);
	}
	return false;
}

bool ImageCircleTool::OnCircleClick()
{
	bool bRes = false;
	CircleToolState curState;

	bRes = CheckCircleAnchorPoints(curState);
	if (bRes == false)
	{
		bRes = CheckCircleLine();
		if (bRes == false)
		{
			bRes = CheckCommentText();
			if (bRes)
			{
				m_nState = CIR_TS_CIRCLE_ACTTXTMOV;
			}
			else
			{
				m_nState = CIR_TS_CIRCLE_COMP;
			}
		}
		else
		{
			m_nState = CIR_TS_CIRCLE_MOVE;
		}
	}
	else
	{
		m_nState = curState;
	}
	
	return bRes;
}



/*
	0: not redraw
	1: redraw the picture
*/
int ImageCircleTool::OnMouseEvent(const MouseEvtRequestArgs *pMouseEventInfo)
{
	int bRedraw = 0;

	if (IsInterestMouseEvt(pMouseEventInfo) == false)
		return bRedraw;

	bool leftDown = pMouseEventInfo->leftDown;
	int x = pMouseEventInfo->pointX;
	int y = pMouseEventInfo->pointY;

	if (m_nState == CIR_TS_INIT)
	{
		if (leftDown)
		{
			m_lastMousePointF.X = x;
			m_lastMousePointF.Y = y;
			m_crossFlatPart1PointFs[0] = m_lastMousePointF;
			m_nState = CIR_TS_STARTP;
		}
	}
	else if (m_nState == CIR_TS_STARTP)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;

		if (leftDown)
		{
			if (GdiPlusTypeOperator::IsPointFGreater(m_crossFlatPart1PointFs[0], m_lastMousePointF, CIRCLE_MOUSE_MOVE_MIN_OFFSET))
			{
				m_crossFlatPart1PointFs[1] = m_lastMousePointF;
				m_nState = CIR_TS_CIRCLE_PENDING;
				RecalCircleComponents_OnCFP1Point(1);
				bRedraw = 1;
			}		
		}
		else
		{
			m_nState = CIR_TS_ABORT;
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_PENDING)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;

		m_crossFlatPart1PointFs[1] = m_lastMousePointF;
		RecalCircleComponents_OnCFP1Point(1);

		if (leftDown)
		{
			if (GdiPlusTypeOperator::IsPointFGreater(m_crossFlatPart1PointFs[0], m_lastMousePointF, CIRCLE_MOUSE_MOVE_MIN_OFFSET))
			{
				bRedraw = 1;
			}
		}
		else
		{
			m_nState = CIR_TS_CIRCLE_COMP;
			bRedraw = 1;
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_COMP)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;

		if (leftDown)
		{
			OnCircleClick();

			if (m_nState != CIR_TS_CIRCLE_COMP)
			{
				bRedraw = 1;
			}
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_ACT)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;

		if (leftDown)
		{
			OnCircleClick();
			if (m_nState == CIR_TS_CIRCLE_COMP)
			{
				bRedraw = 1;
			}
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_ACTTXTMOV)
	{
		if (leftDown)
		{
			PointF curPointF(x, y);
			if (GdiPlusTypeOperator::IsPointFGreater(curPointF, m_lastMousePointF, CIRCLE_MOUSE_MOVE_MIN_OFFSET))
			{
				bRedraw = 1;

				float offsetX = (x - m_lastMousePointF.X);
				float offsetY = (y - m_lastMousePointF.Y);
				CO_TR_OBJ(OwnerImage())->TranslateShape(
					offsetX, offsetY, &m_textStartPointF, 1);

				RecalCircleCommentTextPos();
				m_lastMousePointF = curPointF;
			}
		}
		else
		{
			m_nState = CIR_TS_CIRCLE_ACT;
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_MOVE)
	{
		if (leftDown == false)
		{
			m_nState = CIR_TS_CIRCLE_ACT;
			bRedraw = 1;
		}
		else
		{
			PointF curPointF(x, y);
			if (GdiPlusTypeOperator::IsPointFGreater(curPointF, m_lastMousePointF, CIRCLE_MOUSE_MOVE_MIN_OFFSET))
			{
				bRedraw = 1;

				float offsetX = (x - m_lastMousePointF.X);
				float offsetY = (y - m_lastMousePointF.Y);
				CO_TR_OBJ(OwnerImage())->TranslateShape(
					offsetX, offsetY, &m_centerPointF, 1);

				CO_TR_OBJ(OwnerImage())->TranslateShape(
					offsetX, offsetY, m_crossFlatPart1PointFs, 2);
				CO_TR_OBJ(OwnerImage())->TranslateShape(
					offsetX, offsetY, m_crossFlatPart2PointFs, 2);

				RecalCircleCommentText();

				m_lastMousePointF = curPointF;
			}
		}
	}
	else if (m_nState == CIR_TS_CIRCLE_MOVE_CFP1_P0 ||
			 m_nState == CIR_TS_CIRCLE_MOVE_CFP1_P1 ||
			 m_nState == CIR_TS_CIRCLE_MOVE_CFP2_P0 ||
			 m_nState == CIR_TS_CIRCLE_MOVE_CFP2_P1)
	{
		if (leftDown == false)
		{
			m_nState = CIR_TS_CIRCLE_ACT;
			bRedraw = 1;
		}
		else
		{
			PointF curPointF(x, y);
			if (GdiPlusTypeOperator::IsPointFGreater(curPointF, m_lastMousePointF, CIRCLE_MOUSE_MOVE_MIN_OFFSET))
			{
				bRedraw = 1;
				if (m_nState == CIR_TS_CIRCLE_MOVE_CFP1_P0)
				{
					m_crossFlatPart1PointFs[0] = curPointF;
					RecalCircleComponents_OnCFP1Point(0);
				}
				else if (m_nState == CIR_TS_CIRCLE_MOVE_CFP1_P1)
				{
					m_crossFlatPart1PointFs[1] = curPointF;
					RecalCircleComponents_OnCFP1Point(1);
				}
				else if (m_nState == CIR_TS_CIRCLE_MOVE_CFP2_P0)
				{
					m_crossFlatPart2PointFs[0] = curPointF;
					RecalCircleComponents_OnCFP2Point(0);
				}
				else if (m_nState == CIR_TS_CIRCLE_MOVE_CFP2_P1)
				{
					m_crossFlatPart2PointFs[1] = curPointF;
					RecalCircleComponents_OnCFP2Point(1);
				}
				m_lastMousePointF = curPointF;
			}
		}
	}
	

	if (m_nState == CIR_TS_ABORT)
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

int ImageCircleTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

int ImageCircleTool::OnImageSizeChanged(float width, float height)
{
	return 0;
}

bool ImageCircleTool::Draw(GraphicsImager *pImager) const
{
	if (pImager && m_nState >= CIR_TS_CIRCLE_PENDING)
	{
		bool bActive = IsShapeActive();
		bool bCompleted = IsShapeCompleted();

		Color lineDrawingColor = bActive ? m_pMeasureToolAppearance->lineHlProp.lineColor : m_pMeasureToolAppearance->lineNormProp.lineColor;
		int   lineDrawingWidth = bActive ? m_pMeasureToolAppearance->lineHlProp.lineWidth : m_pMeasureToolAppearance->lineNormProp.lineWidth;

		RectF circleRectF;
		circleRectF.X = m_centerPointF.X - m_fRadius;
		circleRectF.Y = m_centerPointF.Y - m_fRadius;
		circleRectF.Height = circleRectF.Width = 2 * m_fRadius;

		pImager->DrawEllipse(lineDrawingColor, lineDrawingWidth, circleRectF, true);
	
		Rect circleRect((int)circleRectF.X, (int)circleRectF.Y, (int)circleRectF.Width, (int)circleRectF.Height);
		
		if (bActive)
		{
			// to show anchor 4 points
			PointF points[] = {
				m_crossFlatPart1PointFs[0],
				m_crossFlatPart1PointFs[1],
				m_crossFlatPart2PointFs[0],
				m_crossFlatPart2PointFs[1]
			};

			int points_count = sizeof(points) / sizeof(points[0]);
			for (int i = 0; i < points_count; i ++)
			{
				const int nSpace = lineDrawingWidth * 2;
				pImager->FillEllipse(lineDrawingColor, points[i].X - nSpace, points[i].Y - nSpace,
					nSpace * 2, nSpace * 2);
			}
		}

		if (m_pRegionPixelsStDrawing)
		{
			m_pRegionPixelsStDrawing->Draw(pImager, &(m_pMeasureToolAppearance->textFontProp), &(m_pMeasureToolAppearance->textFontHlProp));
		}

		pImager->DrawDashLine(m_pMeasureToolAppearance->connectorLineNormProp.lineColor, 
			m_pMeasureToolAppearance->connectorLineNormProp.lineWidth, 
			(INT)m_textStartPointF.X,(INT)m_textStartPointF.Y, (INT)m_attachedCirclePf.X, (INT)m_attachedCirclePf.Y);

		return true;
	}

	return false;
}

int ImageCircleTool::OnToolActiveStateEvent(MeasureToolDrawing *pSender, bool bActive)
{
	int iRet = 0;

	if (bActive && pSender != this)
	{
		if (IsShapeCompleted())
		{
			if (IsShapeActive())
			{
				m_nState = CIR_TS_CIRCLE_COMP;
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

bool ImageCircleTool::IsShapeActive() const
{
	return (m_nState != CIR_TS_INIT) && (m_nState != CIR_TS_ABORT) && (m_nState != CIR_TS_CIRCLE_COMP);
}

bool ImageCircleTool::IsShapeCompleted() const
{
	return (m_nState >= CIR_TS_CIRCLE_COMP);
}

bool ImageCircleTool::onTransformation(const TransformationArgs *curTransformationArgs)
{
	bool bResult = false;

	PointF circlePointFs[] = {
		m_centerPointF,
		PointF(m_centerPointF.X + m_fRadius, m_centerPointF.Y)
	};

	bool b1 = TransformPointsByCellCoord(curTransformationArgs, circlePointFs, sizeof(circlePointFs) / sizeof(circlePointFs[0]));

	if (b1)
	{
		TransformPointsByCellCoord(curTransformationArgs, &m_attachedCirclePf, 1);
		TransformPointsByCellCoord(curTransformationArgs, &m_textStartPointF, 1);
		TransformPointsByCellCoord(curTransformationArgs, m_crossFlatPart1PointFs, 2);
		TransformPointsByCellCoord(curTransformationArgs, m_crossFlatPart2PointFs, 2);

		m_pRegionPixelsStDrawing->UpdateTextStartPointf(m_textStartPointF);

		m_centerPointF = circlePointFs[0];

		m_fRadius = CalLineLen(circlePointFs[0].X, circlePointFs[0].Y, circlePointFs[1].X, circlePointFs[1].Y);

		TransformableDrawing::onTransformation(curTransformationArgs);
		bResult = true;
	}

	return bResult;
}

MCSF_DJ2DENGINE_END_NAMESPACE