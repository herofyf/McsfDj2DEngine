#include "stdafx.h"
#include "image_free_hand_tool.h"
#include "graphics_imager.h"
#include "study_image.h"
#include "image_free_hand_tool.h"
#include "image_region_pixels_drawing.h"
#include "dicom_region_pixel_helper.h"
#include "image_free_hand_tool_impl.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE


ImageFreeHandTool::ImageFreeHandTool(StudyImage *pImage, ImageToolType toolType) :
	MeasureToolDrawing(pImage, toolType)
{
	SetInterestMouseBehavior(MouseBehaviorDoubleClick | MouseBehaviorMouseMove | MouseBehaviorMouseDown);

	m_pMeasureToolAppearance = &(APPEAR_OBJ(m_pStudyImage)->freeHandAppearance);

	m_state = FH_TS_INIT;
	m_textBegPointf.X = m_textBegPointf.Y = 0;

	m_txtAttachedPointf = m_lastRedrawMousePos = m_curMousePos = m_textBegPointf;

	m_pRegionPixelsStDrawing = new ImageDicomRegPixelsStDrawing(this);
	m_pFreeHandPathImpl = new ImageFreeHandPathImpl(this);
	m_bInitTextPos = false;
}


ImageFreeHandTool::~ImageFreeHandTool(void)
{
	DEL_PTR(m_pRegionPixelsStDrawing);

	DEL_PTR(m_pFreeHandPathImpl);
}

void ImageFreeHandTool::RecalCommentText()
{
	if (m_pStudyImage == NULL || m_pRegionPixelsStDrawing == NULL) 
		return;

	RecalCommentTextAttachedPos();

	DicomRegionPixelsStatistics st;
	int count = 0;
	PointF *pPointFs = m_pFreeHandPathImpl->CloneWalkPathPoints(count);
	if (pPointFs)
	{
		m_pStudyImage->StatisticsFreeHandPixels(pPointFs, count, &st);

		DEL_PTR_ARRAY(pPointFs);
		m_pRegionPixelsStDrawing->UpdateStValues(m_textBegPointf, &st);
	}
}

void ImageFreeHandTool::RecalCommentTextAttachedPos()
{
	if (m_bInitTextPos == false)
	{
		m_bInitTextPos = true;
		const PointF *firstWalk = m_pFreeHandPathImpl->GetWalkPathIndexPointF(0);
		if (firstWalk)
		{
			m_txtAttachedPointf = *(firstWalk);
			m_textBegPointf.X += m_txtAttachedPointf.X + m_pMeasureToolAppearance->textXOffset;
			m_textBegPointf.Y = m_txtAttachedPointf.Y;
		}
	}
	else
	{
		m_txtAttachedPointf = *(m_pFreeHandPathImpl->GetNearestDistPoint(m_textBegPointf));
	}

	m_pRegionPixelsStDrawing->UpdateTextStartPointf(m_textBegPointf);
}

int ImageFreeHandTool::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	bool bRedraw = false;

	if (IsInterestMouseEvt(pMouseEvt) == false)
		return bRedraw;

	PointF curp;
	m_curMousePos.X = pMouseEvt->pointX;
	m_curMousePos.Y = pMouseEvt->pointY;
	
	if (m_state == FH_TS_INIT)
	{
		if (pMouseEvt->leftDown)
		{
			m_state = FH_TS_START_SEG_DRAW;

			m_pFreeHandPathImpl->BeginEditPath();

			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;
			m_pFreeHandPathImpl->AddWalkPathPoint(curp);

			m_lastRedrawMousePos = m_curMousePos;
			m_lastAddMousePos = curp;
		}
	}
	else if (m_state == FH_TS_START_SEG_DRAW)
	{
		curp.X = m_curMousePos.X;
		curp.Y = m_curMousePos.Y;
		
		if (pMouseEvt->leftDown)
		{
			m_pFreeHandPathImpl->AddWalkPathPoint(curp);
			m_lastAddMousePos = curp;
			m_state = FH_TS_DRAWING_SEG_MOVING;
		}
		else
		{
			// add new line another point as temporary
			if (m_lastAddMousePos.Equals(m_curMousePos) == false)
			{
				m_pFreeHandPathImpl->AddWalkPathPoint(curp);
				m_lastAddMousePos = curp;
				m_state = FH_TS_DRAWING_SEG_LINING;
			}
		}

		if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
		{
			m_lastRedrawMousePos = m_curMousePos;
			bRedraw = true;
		}
	}
	else if (m_state == FH_TS_DRAWING_SEG_MOVING)
	{
		curp.X = m_curMousePos.X;
		curp.Y = m_curMousePos.Y;

		if (pMouseEvt->leftDown == false)
		{
			if (m_lastAddMousePos.Equals(m_curMousePos) == false)
			{
				m_lastAddMousePos = m_curMousePos;
				m_state = FH_TS_DRAWING_SEG_LINING;
				m_pFreeHandPathImpl->AddWalkPathPoint(curp);
			}
		}
		else
		{
			m_pFreeHandPathImpl->AddWalkPathPoint(curp);
			m_lastAddMousePos = curp;
		}
		
		if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
		{
			m_lastRedrawMousePos = m_curMousePos;
			bRedraw = true;
		}

		if (pMouseEvt->IsMouseDoubleClick())
		{
			m_pFreeHandPathImpl->EndWalkPath();
			RecalCommentText();
			m_state = FH_TS_END_DRAW;
			m_lastRedrawMousePos = m_curMousePos;
			bRedraw = true;
		}

	}
	else if (m_state == FH_TS_DRAWING_SEG_LINING)
	{
		curp.X = m_curMousePos.X;
		curp.Y = m_curMousePos.Y;

		if (pMouseEvt->leftDown)
		{
			m_state = FH_TS_DRAWING_SEG_MOVING;
			m_pFreeHandPathImpl->AddWalkPathPoint(curp);
			bRedraw = true;
			m_lastAddMousePos = m_curMousePos;
		}
		else
		{
			m_pFreeHandPathImpl->ChangeWalkPathLastPoint(curp);
		}

		if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
		{
			m_lastRedrawMousePos = m_curMousePos;
			bRedraw = true;
		}
	}
	else if (m_state == FH_TS_END_DRAW)
	{
		if (pMouseEvt->leftDown)			
		{
			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;
		
			if (m_pFreeHandPathImpl->IsHit(curp))
			{
				m_state = FH_TS_EDITING;
				m_pFreeHandPathImpl->BeginEditPath();
				m_pFreeHandPathImpl->AddEditPath(curp);
			}
			else if (m_pFreeHandPathImpl->IsWithin(curp))
			{
				m_state = FH_TS_MOV;
			}
			else if (m_pRegionPixelsStDrawing->IsWithinTextRegion(m_curMousePos.X, m_curMousePos.Y))
			{
				m_state = FH_TS_ACTTXTMOV;
			}

			if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
			{
				m_lastRedrawMousePos = m_curMousePos;
				bRedraw = true;
			}
		}
	}
	else if (m_state == FH_TS_EDITING)
	{
		if (pMouseEvt->leftDown)
		{
			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;
			
			m_pFreeHandPathImpl->AddEditPath(curp);

			if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
			{
				m_lastRedrawMousePos = m_curMousePos;
				bRedraw = true;
			}
		}
		else
		{
			m_state = FH_TS_ACT;
			m_pFreeHandPathImpl->EndEditPath();
			RecalCommentText();
			m_lastRedrawMousePos = m_curMousePos;
			bRedraw = true;
		}
	}

	else if (m_state == FH_TS_ACT)
	{
		if (pMouseEvt->leftDown)	
		{
			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;

			if (m_pFreeHandPathImpl->IsHit(curp))
			{
				m_state = FH_TS_EDITING;
				m_pFreeHandPathImpl->BeginEditPath();
				m_pFreeHandPathImpl->AddEditPath(curp);
			}
			else if (m_pFreeHandPathImpl->IsWithin(curp))
			{
				m_state = FH_TS_MOV;
			}
			else
			{
				if (m_pRegionPixelsStDrawing->IsWithinTextRegion(m_curMousePos.X, m_curMousePos.Y))
				{
					m_state = FH_TS_ACTTXTMOV;
				}
				else
				{
					m_state = FH_TS_END_DRAW;
					bRedraw = true;		
				}
			}

			if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
			{
				m_lastRedrawMousePos = m_curMousePos;
				bRedraw = true;
			}
			
		}
	}
	else if (m_state == FH_TS_MOV)
	{
		if (pMouseEvt->leftDown)
		{
			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;
			if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
			{
				float offsetx = m_curMousePos.X - m_lastRedrawMousePos.X;
				float offsety = m_curMousePos.Y - m_lastRedrawMousePos.Y;

				MoveWalkPathShape(offsetx, offsety);
				
				m_lastRedrawMousePos = m_curMousePos;
				bRedraw = true;
			}

		}
		else
		{
			m_state = FH_TS_ACT;
			bRedraw = true;
		}
	}
	else if (m_state == FH_TS_ACTTXTMOV)
	{
		if (pMouseEvt->leftDown)
		{
			curp.X = m_curMousePos.X;
			curp.Y = m_curMousePos.Y;
			if (GdiPlusTypeOperator::IsPointFGreater(m_lastRedrawMousePos, m_curMousePos, FREEHAND_MOV_MIN_OFFSET))
			{
				float offsetX = m_curMousePos.X - m_lastRedrawMousePos.X;
				float offsetY = m_curMousePos.Y - m_lastRedrawMousePos.Y;

				CO_TR_OBJ(OwnerImage())->TranslateShape(offsetX, offsetY, &m_textBegPointf, 1);

				RecalCommentTextAttachedPos();
				bRedraw = true;
				m_lastRedrawMousePos = m_curMousePos;
			}
		}
		else
		{
			m_state = FH_TS_ACT;
		}
	}

	if (bRedraw)
	{
		bool bActive = IsShapeActive();
		NotifyMyActiveState(bActive);
	}
	return bRedraw;
}

// when you move the free hand, the attached point and the text should be changed
void ImageFreeHandTool::MoveWalkPathShape(float offsetX, float offsetY)
{
	TransformationArgs translate;
	translate.transformationType = TRANS_TRANLATE;
	translate.args.translateArgs.offsetX = offsetX;
	translate.args.translateArgs.offsetY = offsetY;
	m_pFreeHandPathImpl->onTransformation(&translate);

	RecalCommentText();
}

int ImageFreeHandTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return false;
}

int ImageFreeHandTool::OnImageSizeChanged(float width, float height)
{
	return false;
}

void ImageFreeHandTool::showPathPoints(const POINTFS_V *pPoints, GraphicsImager *pImager) const
{
	if (pPoints == NULL || pImager == NULL)	 return;

	POINTFS_V_CIT nextIt;
	Point p1, p2;
	
	bool bActive = IsShapeActive();
	Color drawingColor = bActive ? m_pMeasureToolAppearance->lineHlProp.lineColor : m_pMeasureToolAppearance->lineNormProp.lineColor;
	int   drawingWidth = bActive ? m_pMeasureToolAppearance->lineHlProp.lineWidth : m_pMeasureToolAppearance->lineNormProp.lineWidth;

	for (POINTFS_V_CIT it = pPoints->begin(); it != pPoints->end(); it ++)
	{
		nextIt = it + 1;
		if (nextIt == pPoints->end()) 
		{
			break;
		}
		pImager->DrawLine(drawingColor, drawingWidth, it->X, it->Y, nextIt->X, nextIt->Y, true);
	}

	pImager->DrawDashLine(m_pMeasureToolAppearance->connectorLineNormProp.lineColor, 
		m_pMeasureToolAppearance->connectorLineNormProp.lineWidth, 
		(INT)m_textBegPointf.X,(INT)m_textBegPointf.Y, (INT)m_txtAttachedPointf.X, (INT)m_txtAttachedPointf.Y);

}

bool ImageFreeHandTool::Draw(GraphicsImager *pImager) const
{
	const POINTFS_V *pPoints = NULL;

	pPoints = m_pFreeHandPathImpl->GetWalkPathPoints();
	showPathPoints(pPoints, pImager);

	if (m_state == FH_TS_EDITING)
	{
		pPoints = m_pFreeHandPathImpl->GetEditPathPoints();
		showPathPoints(pPoints, pImager);
	}

	if (IsFreeHandShapeDrawing())
	{
		if (m_pRegionPixelsStDrawing)
		{
			m_pRegionPixelsStDrawing->Draw(pImager, &(m_pMeasureToolAppearance->textFontProp), &(m_pMeasureToolAppearance->textFontHlProp));
		}
	}
	return false;
}

bool ImageFreeHandTool::IsFreeHandShapeDrawing() const
{
	return	(m_state != FH_TS_INIT ) && (m_state != FH_TS_ABORT) &&
			(m_state != FH_TS_START_SEG_DRAW);
}

int ImageFreeHandTool::OnToolActiveStateEvent(MeasureToolDrawing *pSender, bool bActive)
{
	bool bRet = false;

	if (bActive && pSender != this)
	{
		if (IsShapeCompleted() && m_pFreeHandPathImpl->IsWalkPathValid())
		{
			if (IsShapeActive())
			{
				m_state = FH_TS_END_DRAW;
				bRet = true;
			}
		}
		else
		{
			delete this;
			bRet = true;
		}
	}
	return bRet;
}

bool ImageFreeHandTool::IsShapeCompleted() const
{
	return m_state >= FH_TS_END_DRAW;
}

bool ImageFreeHandTool::IsShapeActive() const
{
	return (m_state == FH_TS_DRAWING_SEG_MOVING) || (m_state == FH_TS_DRAWING_SEG_LINING) ||
		   (m_state == FH_TS_START_SEG_DRAW) || (m_state > FH_TS_END_DRAW) ;
}


bool ImageFreeHandTool::onTransformation(const TransformationArgs *curTransformationArgs)
{
	bool bResult = false;

	bResult = m_pFreeHandPathImpl->onTransformation(curTransformationArgs);
	if (bResult)
	{
		bool b1 = TransformPointsByCellCoord(curTransformationArgs, &m_textBegPointf, 1);
		bool b2 = TransformPointsByCellCoord(curTransformationArgs, &m_txtAttachedPointf, 1);
		if (b1 && b2)
		{
			TransformableDrawing::onTransformation(curTransformationArgs);
			m_pRegionPixelsStDrawing->UpdateTextStartPointf(m_textBegPointf);
			bResult = true;
		}
	}
	return bResult;
}

MCSF_DJ2DENGINE_END_NAMESPACE