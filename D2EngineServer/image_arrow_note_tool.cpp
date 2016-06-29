#include "stdafx.h"
#include "image_arrow_note_tool.h"
#include "study_image.h"
#include "graphics_imager.h"
#include "geometry_math.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageArrowNoteTool::ImageArrowNoteTool(StudyImage *pStudyImage, ImageToolType toolType) :
	MeasureToolDrawing(pStudyImage, toolType), m_state(ANT_TS_INIT), 
	NoteMetaInfoOperator(NoteToolArrowNote, (m_pStudyImage->GetDrawingAppearance()->noteToolAppearance).textFontProp)
{
	SetInterestMouseBehavior(MouseBehaviorDoubleClick | MouseBehaviorMouseMove | MouseBehaviorMouseDown);

	m_textStartPoint.X = m_textStartPoint.Y = m_lastMousePointF.X = m_lastMousePointF.Y = 0;

	for (int i = 0; i < ArrowNotePointCount; i ++)
	{
		m_arrowLinePts[i].X = m_arrowLinePts[i].Y = 0;
	}
	m_pMeasureToolAppearance = &(m_pStudyImage->GetDrawingAppearance()->noteToolAppearance);
	m_nLineWidth = m_pMeasureToolAppearance->lineNormProp.lineWidth;
	m_lineColor = m_pMeasureToolAppearance->lineNormProp.lineColor;

	const FontPropMetaInfo *pFontProp = dynamic_cast<const FontPropMetaInfo *>(&((m_pStudyImage->GetDrawingAppearance()->noteToolAppearance).textFontProp));
	CalEmptyTextExtent(pFontProp, m_nEmptyTextWidth, m_nEmptyTextHeight);

	m_nTextWidth = m_nEmptyTextWidth;
	m_nTextHeight = m_nEmptyTextHeight;
}


ImageArrowNoteTool::~ImageArrowNoteTool(void)
{
}

void ImageArrowNoteTool::CalEmptyTextExtent(const FontPropMetaInfo *pFontProp, int &width, int &height)
{
	if (pFontProp == NULL)
	{
		width = 10;
		height = 15;
		return;
	}

	RectF rectF = m_pMeasureToolAppearance->CalTextSize("a", *pFontProp);
	width = rectF.Width > 0 ? rectF.Width : 10;
	//width = 4;
	height = rectF.Height > 0 ? rectF.Height : 15;
}

bool ImageArrowNoteTool::onMetaInformationChanged()
{
	bool bChanged = false;

	if (m_nLineWidth != GetMetaInformation().arrowNoteLineInf.lineWidth)
	{
		m_nLineWidth = GetMetaInformation().arrowNoteLineInf.lineWidth;
		bChanged |= true;
	}
	
	if (m_lineColor.ToCOLORREF() != GetMetaInformation().arrowNoteLineInf.lineColor.ToCOLORREF())
	{
		m_lineColor = GetMetaInformation().arrowNoteLineInf.lineColor;
		bChanged |= true;
	}
	
	m_texts.clear();
	
	const FontPropMetaInfo *pFontProp = dynamic_cast<const FontPropMetaInfo *>(&(GetMetaInformation().noteTextInf));
	CalEmptyTextExtent(pFontProp, m_nEmptyTextWidth, m_nEmptyTextHeight);

	std::string chgText = GetMetaInformation().noteTextInf.sText;
	
	char *szText =const_cast<char *>(chgText.c_str());
	if (szText)
	{
		char *szToken = strtok(szText, "\n");
		int x = m_textStartPoint.X, y = m_textStartPoint.Y, sumHeight = 0,
			maxWidth = 0;
		
		ImageDrawingComment noteComment;

		noteComment.FontColor(GetMetaInformation().noteTextInf.fontColor);
		noteComment.FontSize(GetMetaInformation().noteTextInf.fontSize);
		noteComment.FontStyle(GetMetaInformation().noteTextInf.fontStyle);
		noteComment.FontName( GetMetaInformation().noteTextInf.sFontName);
		//noteComment.FontName( x1);
		while(szToken)
		{
			noteComment.DrawingText(szToken);
			noteComment.PosX(x + sumHeight);
			noteComment.PosY(y);

			RectF rectF = m_pMeasureToolAppearance->CalTextSize(szToken, *pFontProp);
			if (rectF.Width == 0 || rectF.Height == 0)
			{
				return bChanged;
			}

			noteComment.Width(rectF.Width, noteComment.DrawingText());
			noteComment.Height(rectF.Height);

			sumHeight += (int) rectF.Height;
			if (rectF.Width > maxWidth)
			{
				maxWidth = rectF.Width;
			}

			szToken = strtok(NULL, "\n");

			m_texts.push_back(noteComment);
		}

		m_nTextHeight = sumHeight;
		m_nTextWidth = maxWidth;

		bChanged |= true;
	}
	
	if (m_texts.size() == 0)
	{
		m_nTextWidth = m_nEmptyTextWidth;
		m_nTextHeight = m_nEmptyTextHeight;
	}
	return bChanged;
}

void ImageArrowNoteTool::RecalNoteLinesCommentPos()
{
	int x = m_textStartPoint.X, y = m_textStartPoint.Y, sumHeight = 0,
		maxWidth = 0;

	std::deque<ImageDrawingComment>::iterator it;
	for(it = m_texts.begin(); it != m_texts.end(); it ++)
	{
		it->PosX(x + sumHeight);
		it->PosY(y);

		sumHeight += it->Height();
	}
}

bool ImageArrowNoteTool::ReportNoteStatusChangedMsg(NoteStatusType noteStatusType)
{
	NoteStatusInformation noteStatusInf;
	noteStatusInf.noteReportedState = noteStatusType;
	noteStatusInf.noteMetaInformation = GetMetaInformation();

	return OwnerImage()->ReportPostRenderStatusMsgs(ImageNoteStatusMsg, &noteStatusInf);
}

int ImageArrowNoteTool::OnToolActiveStateEvent(MeasureToolDrawing *pSender, bool bActive)
{
	bool bRet = false;

	if (bActive && pSender != this)
	{
		if (IsShapeCompleted())
		{
			if (IsShapeActive())
			{
				m_state = ANT_TS_COMP;
				bRet =  true;
			}
		}
		else
		{
			delete this;
		}
	}
	return bRet;
}

int ImageArrowNoteTool::OnMouseEvent(const MouseEvtRequestArgs *pMouseEventInfo)
{
	bool bRedraw = false, bDoubleClickSel = false;

	if (IsInterestMouseEvt(pMouseEventInfo) == false)
		return bRedraw;

	bool leftDown = pMouseEventInfo->leftDown;
	int x = pMouseEventInfo->pointX;
	int y = pMouseEventInfo->pointY;

	if (m_state == ANT_TS_INIT)
	{
		if (leftDown)
		{
			m_arrowLinePts[ArrowNotePointArrow].X = x;
			m_arrowLinePts[ArrowNotePointArrow].Y = y;
			m_state = ANT_TS_WAITING_LN_TAIL;
			m_lastMousePointF = m_arrowLinePts[ArrowNotePointArrow];
			m_arrowLinePts[ArrowNotePointTail] = m_arrowLinePts[ArrowNotePointArrow];
		}
	}
	else if (m_state == ANT_TS_WAITING_LN_TAIL)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;
		if (leftDown && false == m_arrowLinePts[ArrowNotePointTail].Equals(m_lastMousePointF))
		{
			m_arrowLinePts[ArrowNotePointTail] = m_lastMousePointF;
			m_state = ANT_TS_WAITING_LN_COMP;
			bRedraw = true;
		}
		else
		{
			m_state = ANT_TS_ABORT;
		}
	}
	else if (m_state == ANT_TS_WAITING_LN_COMP)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;

		if (leftDown)
		{
			m_arrowLinePts[ArrowNotePointTail] = m_lastMousePointF;
			
			bRedraw = true;
			
		}
		else if (leftDown == false)
		{
			m_arrowLinePts[ArrowNotePointTail] = m_lastMousePointF;
			
			m_textStartPoint = m_lastMousePointF;

			if (m_arrowLinePts[ArrowNotePointTail].X > m_arrowLinePts[ArrowNotePointArrow].X)
			{
				m_textStartPoint.X += 7;
			}
			else
			{
				m_textStartPoint.X -= 7;
			}
			
			m_textStartPoint.Y += 5;

			m_state = ANT_TS_ACT;
			ReportNoteStatusChangedMsg(NoteStatusArrowNoteComp);
			bRedraw = true;
		}
	}
	else if (m_state == ANT_TS_COMP)
	{
		if (leftDown)
		{
			onArrowLineClick(x, y);
			if (m_state != ANT_TS_COMP)
			{
				bRedraw = true;
			}
		}
	}
	else if (m_state == ANT_TS_ACT)
	{
		if (leftDown)
		{
			onArrowLineClick(x, y);

			if (m_state != ANT_TS_ACT)
			{
				bRedraw = true;
			}
		}
	}
	else if (m_state == ANT_TS_LN_MOVE)
	{
		if (leftDown)
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(x - m_lastMousePointF.X),
				(y - m_lastMousePointF.Y),
				m_arrowLinePts,
				sizeof(m_arrowLinePts)/ sizeof(m_arrowLinePts[0]));

			m_lastMousePointF.X = x;
			m_lastMousePointF.Y = y;
		}
		else
		{
			m_state = ANT_TS_ACT;
		}
		bRedraw = true;
	}
	else if (m_state == ANT_TS_MOV_PT_ARROW)
	{
		if (leftDown)
		{
			m_arrowLinePts[ArrowNotePointArrow].X = x;
			m_arrowLinePts[ArrowNotePointArrow].Y = y;
			m_lastMousePointF = m_arrowLinePts[ArrowNotePointArrow];
		}
		else
		{
			m_state = ANT_TS_ACT;
		}
		bRedraw = true;
	}
	else if (m_state == ANT_TS_MOV_PT_TAIL)
	{
		if (leftDown)
		{
			m_arrowLinePts[ArrowNotePointTail].X = x;
			m_arrowLinePts[ArrowNotePointTail].Y = y;
			m_lastMousePointF = m_arrowLinePts[ArrowNotePointTail];
		}
		else
		{
			m_state = ANT_TS_ACT;
		}
		bRedraw = true;
	}
	else if (m_state == ANT_TS_ACTTXTMOV)
	{
		if (leftDown)
		{
			CO_TR_OBJ(OwnerImage())->TranslateShape(
				(x - m_lastMousePointF.X),
				(y - m_lastMousePointF.Y),
				&m_textStartPoint, 1);

			m_lastMousePointF.X = x;
			m_lastMousePointF.Y = y;
			RecalNoteLinesCommentPos();
		}
		else
		{
			m_state = ANT_TS_ACT;
		}
		bRedraw = true;
	}


	if (m_state == ANT_TS_ABORT)
	{
		bRedraw = false;
		delete this;
	}
	else if (bRedraw)
	{
		bool bActive = IsShapeActive();
		NotifyMyActiveState(bActive);
	}

	if (pMouseEventInfo->IsMouseDoubleClick())
	{
		bool b1 = IsPointInLine(m_arrowLinePts[ArrowNotePointArrow].X, m_arrowLinePts[ArrowNotePointArrow].Y, 
			m_arrowLinePts[ArrowNotePointTail].X, m_arrowLinePts[ArrowNotePointTail].Y, x, y);
		bool b2 = IsPointInViewPort(m_textStartPoint.X, m_textStartPoint.Y, m_nTextWidth, m_nTextHeight, x, y);
		if (b1 || b2)
		{
			ReportNoteStatusChangedMsg(NoteStatusDoubleClick);
		}
	}

	return bRedraw;
}

int ImageArrowNoteTool::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{

	return false;
}

bool ImageArrowNoteTool::Draw(GraphicsImager *pImager) const
{
	if (pImager == NULL)
		return false;

	bool bActive = IsShapeActive();
	bool bCompleted = IsShapeCompleted();

	pImager->DrawArrowLine(m_lineColor, m_nLineWidth, 
		m_arrowLinePts[ArrowNotePointArrow].X,
		m_arrowLinePts[ArrowNotePointArrow].Y,
		m_arrowLinePts[ArrowNotePointTail].X,
		m_arrowLinePts[ArrowNotePointTail].Y,
		true, false, true);

	// to draw text

	if (bCompleted && m_state != ANT_TS_WAITING_LN_COMP)
	{
		std::deque<ImageDrawingComment>::const_iterator it;
		for (it = m_texts.begin(); it != m_texts.end(); it ++)
		{
			const ImageDrawingComment &drawItem = *it;
			pImager->DrawString(drawItem.DrawingText(), drawItem.Pos().X, drawItem.Pos().Y,
				drawItem.FontName(), drawItem.FontSize(), drawItem.FontStyle(), drawItem.FontColor(),
				true);
		}

		int textLines = m_texts.size();
		if (bActive)
		{
			Gdiplus::Color hlColor = m_pMeasureToolAppearance->lineHlProp.lineColor;
			int hlLineWidth = m_pMeasureToolAppearance->lineHlProp.lineWidth;

			// to Draw Text board

			if (textLines > 0)
			{
				pImager->DrawRectangle(hlColor, hlLineWidth, (int)m_textStartPoint.X, (int)m_textStartPoint.Y,
					m_nTextWidth, m_nTextHeight, true);
			}
			else
			{
				pImager->DrawRectangle(hlColor, hlLineWidth, (int)m_textStartPoint.X, (int)m_textStartPoint.Y,
					m_nEmptyTextWidth, m_nEmptyTextHeight, false);
			}
		}

		if (bActive || textLines > 0)
		{
			pImager->DrawDashLine(m_pMeasureToolAppearance->connectorLineNormProp.lineColor, 
				m_pMeasureToolAppearance->connectorLineNormProp.lineWidth, 
				(INT)m_arrowLinePts[ArrowNotePointTail].X, (INT)m_arrowLinePts[ArrowNotePointTail].Y, 
				(INT)m_textStartPoint.X, (INT)m_textStartPoint.Y);

		}
	}
	return true;
}

int ImageArrowNoteTool::OnImageSizeChanged(float width, float height)
{
	return false;
}

bool ImageArrowNoteTool::onTransformation(const TransformationArgs *curTransformationArgs)
{
	bool bResult = false;

	bool b1 = TransformPointsByCellCoord(curTransformationArgs, m_arrowLinePts, (sizeof(m_arrowLinePts)/ sizeof(m_arrowLinePts[0])));
	if (b1)
	{
		TransformableDrawing::onTransformation(curTransformationArgs);
		bResult = true;
	}

	return bResult;
}

bool ImageArrowNoteTool::IsShapeActive() const
{
	return (m_state != ANT_TS_INIT) && 
		   (m_state != ANT_TS_COMP) &&
		   (m_state != ANT_TS_ABORT);
}

bool ImageArrowNoteTool::IsShapeCompleted() const
{
	return (m_state >= ANT_TS_WAITING_LN_COMP);
}

bool ImageArrowNoteTool::onArrowLineClick(float x, float y)
{
	bool bResult = true;

	bool b = checkPtArrow(x, y);
	if (b == false)
	{
		b = checkPtTail(x, y);
		if (b == false)
		{
			b = checkArrowLineActive(x, y);
			if (b == false)
			{
				b = checkLineTextActive(x, y);
				if (b == false)
				{
					m_state = ANT_TS_COMP;
					bResult = false;
				}
			}
		}
	}

	return bResult;
}

bool ImageArrowNoteTool::checkPtArrow(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_arrowLinePts[ArrowNotePointArrow].X, m_arrowLinePts[ArrowNotePointArrow].Y);
	if (b)
	{
		m_arrowLinePts[ArrowNotePointArrow].X = x;
		m_arrowLinePts[ArrowNotePointArrow].Y = y;
		m_lastMousePointF = m_arrowLinePts[ArrowNotePointArrow];
		m_state = ANT_TS_MOV_PT_ARROW;
	}
	return b;
}

bool ImageArrowNoteTool::checkPtTail(float x, float y)
{
	bool b = IsPointNearPoint(x, y, m_arrowLinePts[ArrowNotePointTail].X, m_arrowLinePts[ArrowNotePointTail].Y);
	if (b)
	{
		m_arrowLinePts[ArrowNotePointTail].X = x;
		m_arrowLinePts[ArrowNotePointTail].Y = y;
		m_lastMousePointF = m_arrowLinePts[ArrowNotePointTail];
		m_state = ANT_TS_MOV_PT_TAIL;
	}
	return b;
}

bool ImageArrowNoteTool::checkArrowLineActive(float x, float y)
{
	bool b = IsPointInLine(m_arrowLinePts[ArrowNotePointArrow].X, m_arrowLinePts[ArrowNotePointArrow].Y, 
						   m_arrowLinePts[ArrowNotePointTail].X, m_arrowLinePts[ArrowNotePointTail].Y, x, y);
	if (b)
	{
		m_state = ANT_TS_LN_MOVE;
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;
	}
	return b;
}

bool ImageArrowNoteTool::checkLineTextActive(float x, float y)
{
	bool b = IsPointInViewPort(m_textStartPoint.X, m_textStartPoint.Y, m_nTextWidth, m_nTextHeight, x, y);
	if (b)
	{
		m_lastMousePointF.X = x;
		m_lastMousePointF.Y = y;
		m_state = ANT_TS_ACTTXTMOV;
	}

	return b;
}

MCSF_DJ2DENGINE_END_NAMESPACE