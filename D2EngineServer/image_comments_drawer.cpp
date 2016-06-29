#include "stdafx.h"
#include "image_comments_drawer.h"
#include "study_image.h"
#include "graphics_imager.h"
#include "system_global_configuration.h"
#include "trace_performance_func.h"
#include "study_series_command_request_args.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE


ImageLineCommentsDrawer::ImageLineCommentsDrawer()
{
	_nLineWidth = _nLineHeight = 0;
}

ImageLineCommentsDrawer::~ImageLineCommentsDrawer()
{
	ClearComments();
}

void ImageLineCommentsDrawer::ClearComments()
{
	_lineComments.clear();

	_nLineWidth = _nLineHeight = 0;
}

void ImageLineCommentsDrawer::ArrangeLineComments(int line_begx, int line_begy, int line_endx, int line_endy)
{
	int line_offsetx = line_begx;
	int unit = 0;
	for (DrawingLineCommentsIt it = _lineComments.begin(); it != _lineComments.end(); it ++)
	{
		if (it->Text().length() <= 0) 
			continue;

		if (line_offsetx + it->Width() > line_endx)
		{
			unit = it->Width() / it->Text().length();

			if (unit == 0) 
				continue;

			int char_count = (line_endx - line_offsetx) / unit;
			char_count -= 3;

			std::string str;
			if (char_count > 0)
			{
				str = it->Text().substr(0, char_count);
				str += "...";
			}
			else
				continue; // only ... no meaningless
			
			it->DrawingText(str);

		}
		else
		{
			it->DrawingText(it->Text());
		}

		it->Pos(PointF(line_offsetx, line_begy));
		line_offsetx += it->Width() + it->UnitWidth();
	}
}

void ImageLineCommentsDrawer::AddDrawingComment(const ImageDrawingComment &comm)
{
	_lineComments.push_back(comm);
	
	_nLineWidth += comm.Width() + comm.UnitWidth();
	int lineHeight = comm.Height();
	if (lineHeight > _nLineHeight)
	{
		_nLineHeight = lineHeight;
	}
}

bool ImageLineCommentsDrawer::Draw(GraphicsImager *pImager) const
{
	for (DrawingLineCommentsCIt comm_it = _lineComments.begin(); comm_it != _lineComments.end(); comm_it ++)
	{
		const ImageDrawingComment &comm = *comm_it;
		pImager->DrawString(comm.DrawingText(), 
			comm.Pos().X, 
			comm.Pos().Y,
			comm.FontName(),
			comm.FontSize(), 
			comm.FontStyle(), 
			comm.FontColor(),
			false);
	}
	return true;
}


ImageCornerCommentsDrawer::ImageCornerCommentsDrawer(ImageCommentsDrawer *pParent) :
	_pParent(pParent)
{
	ClearComments();
}

void ImageCornerCommentsDrawer::DoAddItem(const ImageLineCommentsDrawer &lineComms)
{
	_cornerComments.push_back(lineComms);
}

void ImageCornerCommentsDrawer::AddDrawingLineComment(const ImageLineCommentsDrawer &lineComms)
{
	DoAddItem(lineComms);

	_nHeight += lineComms.LineHeight();

	if (lineComms.LineWidth() > _nWidth)
	{
		_nWidth = lineComms.LineWidth();
	}

	AfterAddItem();
}


ImageCornerCommentsDrawer::~ImageCornerCommentsDrawer()
{
	
}

void ImageCornerCommentsDrawer::ClearComments()
{
	_cornerComments.clear();

	_nWidth = _nHeight = 0;
	_boundsRect.X = _boundsRect.Y = _limitRect.X = _limitRect.Y = 0;
	_boundsRect.Width = _boundsRect.Height = _limitRect.Width = _limitRect.Height = 0;
}

bool ImageCornerCommentsDrawer::Draw(GraphicsImager *pImager) const
{
	for (DrawingCommentsCIt it = _cornerComments.begin(); it != _cornerComments.end(); it ++)
		it->Draw(pImager);

	return true;
}

void ImageCornerCommentsDrawer::ArrangeComments()
{
	// show in limited view.
	int line_begy = _limitRect.Y;

	for (DrawingCommentsIt it = _cornerComments.begin(); it != _cornerComments.end(); it ++)
	{
		if (line_begy >= (_limitRect.Y + _limitRect.Height))
			break;

		if (_limitRect.Width <= 0 || _limitRect.Height <= 0)
			break;

		it->ArrangeLineComments(_limitRect.X, line_begy, 
								_limitRect.X + _limitRect.Width, 
								line_begy);

		line_begy += it->LineHeight();
	}
}

int ImageCornerCommentsDrawer::GetAvgLineLength()
{
	int lLineTotal = 0, lLinesCount = _cornerComments.size();
	if (lLinesCount == 0) return 0;

	for (DrawingCommentsIt it = _cornerComments.begin(); it != _cornerComments.end(); it ++)
	{
		lLineTotal += it->LineWidth();
	}

	if (lLineTotal > 0 )
		return lLineTotal / lLinesCount;
	else
		return 0;
}

ImageLeftTopCommentsDrawer::ImageLeftTopCommentsDrawer(ImageCommentsDrawer *pParent) :
	ImageCornerCommentsDrawer(pParent)
{
	
}

void ImageLeftTopCommentsDrawer::AfterAddItem()
{
	_boundsRect.X = COMMENT_LEFT_MARGIN;
	_boundsRect.Y = COMMENT_TOP_MARGIN;
	_boundsRect.Width = _nWidth;
	_boundsRect.Height = _nHeight;
}

ImageLeftBotCommentsDrawer::ImageLeftBotCommentsDrawer(ImageCommentsDrawer *pParent) :
	ImageCornerCommentsDrawer(pParent)
{
	
}

void ImageLeftBotCommentsDrawer::AfterAddItem()
{
	_boundsRect.X = COMMENT_LEFT_MARGIN;
	_boundsRect.Y = _pParent->OwnerImage()->ImageHeight() - COMMENT_BOTTOM_MARGIN - _nHeight;
	_boundsRect.Width = _nWidth;
	_boundsRect.Height = _nHeight;
}


ImageRightTopCommentsDrawer::ImageRightTopCommentsDrawer(ImageCommentsDrawer *pParent) :
	ImageCornerCommentsDrawer(pParent)
{
	
}

void ImageRightTopCommentsDrawer::AfterAddItem()
{
	_boundsRect.X = _pParent->OwnerImage()->ImageWidth() - COMMENT_RIGHT_MARGIN - _nWidth;
	_boundsRect.Y = COMMENT_TOP_MARGIN;
	_boundsRect.Width = _nWidth;
	_boundsRect.Height = _nHeight;
}



ImageRightBotCommentsDrawer::ImageRightBotCommentsDrawer(ImageCommentsDrawer *pParent) :
	ImageCornerCommentsDrawer(pParent)
{

} 

void ImageRightBotCommentsDrawer::AfterAddItem()
{
	_boundsRect.X = _pParent->OwnerImage()->ImageWidth() - COMMENT_RIGHT_MARGIN - _nWidth;
	_boundsRect.Y = _pParent->OwnerImage()->ImageHeight() - COMMENT_BOTTOM_MARGIN - _nHeight;
	_boundsRect.Width = _nWidth;
	_boundsRect.Height = _nHeight;
}


ImageCommentsDrawer::ImageCommentsDrawer(StudyImage *pStudyImage) : 
	ImageDrawingItem(pStudyImage), _pConfigedComments(NULL)
{
	m_pCommentsAppearance = &(m_pStudyImage->GetDrawingAppearance()->commentsAppearance);
	
	_cornerCommDrawers[VP_LEFT_TOP] = new ImageLeftTopCommentsDrawer(this);
	_cornerCommDrawers[VP_LEFT_BOTTOM] = new ImageLeftBotCommentsDrawer(this);
	_cornerCommDrawers[VP_RIGHT_TOP] = new ImageRightTopCommentsDrawer(this);
	_cornerCommDrawers[VP_RIGHT_BOTTOM] = new ImageRightBotCommentsDrawer(this);
}

ImageCommentsDrawer::~ImageCommentsDrawer(void)
{
	for (CornerCommentsDrawersIt it = _cornerCommDrawers.begin(); it != _cornerCommDrawers.end(); it ++)
	{
		DEL_PTR(it->second);
	}
	_cornerCommDrawers.clear();
}

bool ImageCommentsDrawer::Draw(GraphicsImager *pImager) const
{
	//CPU_PERF_CAL cal("ImageCommentsDrawer::Draw");
	
	for (CornerCommentsDrawersCIt it = _cornerCommDrawers.begin(); it != _cornerCommDrawers.end(); it ++)
	{
		it->second->Draw(pImager);
	}

	return true;
}

int ImageCommentsDrawer::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

int ImageCommentsDrawer::OnMouseEvent(const MouseEvtRequestArgs *mouseEventInfo)
{
	return 0;
}

void ImageCommentsDrawer::Reset()
{
	const CUSTOMIZED_COMMENTS_DEQ *pCommTags = OwnerImage()->GetCustComments();

	for (CornerCommentsDrawersCIt it = _cornerCommDrawers.begin(); it != _cornerCommDrawers.end(); it ++)
	{
		it->second->ClearComments();
	}

	for (CUSTOMIZED_COMMENTS_DEQ_CIT cit = pCommTags->begin(); cit != pCommTags->end(); cit ++)
	{
		const CUSTOMIZED_COMMENT_LINE_DEQ *pCommLineDeq = cit->GetLineComments();

		COMMENT_WINDOW_POSITION curPos = VP_LEFT_TOP;
		ImageLineCommentsDrawer lineComments;
		for(CUSTOMIZED_COMMENT_LINE_DEQ_CIT line_cit = pCommLineDeq->begin(); line_cit != pCommLineDeq->end(); line_cit ++)
		{
			const CustomizedPosDcmTagComment &dcmTagComm = (*line_cit);

			if (dcmTagComm.DcmTagStrValue().length() <= 0) continue;

			ImageDrawingComment commDrawing;
			commDrawing.Text(dcmTagComm.DcmTagStrValue());
			commDrawing.FontName(dcmTagComm.FontName());
			commDrawing.FontSize(dcmTagComm.FontSize());
			commDrawing.FontColor(dcmTagComm.Color());
			commDrawing.FontStyle(dcmTagComm.FontStyle());
			curPos = dcmTagComm.Position();

			RectF rectf;
			FontsSizeInformationCache::Instance()->FindCachedTextRectF(commDrawing.Text(), commDrawing.FontName(), 
				commDrawing.FontSize(), commDrawing.FontStyle(), rectf);

			commDrawing.Width((int)rectf.Width, commDrawing.Text());
			commDrawing.Height((int)rectf.Height);

			lineComments.AddDrawingComment(commDrawing);
		}
		if (_cornerCommDrawers.find(curPos) != _cornerCommDrawers.end())
		{
			_cornerCommDrawers[curPos]->AddDrawingLineComment(lineComments);
		}
	}

	ArrangeComments();
}

void ImageCommentsDrawer::ArrangeComments()
{
	int imageWidth = OwnerImage()->ImageWidth() - COMMENT_LEFT_MARGIN - COMMENT_RIGHT_MARGIN;
	int imageHeight= OwnerImage()->ImageHeight() - COMMENT_TOP_MARGIN - COMMENT_BOTTOM_MARGIN;

	// to get arrange
	Rect leftTopRect = _cornerCommDrawers[VP_LEFT_TOP]->GetBoundsRect();
	int  leftTopAvgLineLen = _cornerCommDrawers[VP_LEFT_TOP]->GetAvgLineLength();
	Rect leftTopLimitRect = leftTopRect;

	Rect leftBotRect = _cornerCommDrawers[VP_LEFT_BOTTOM]->GetBoundsRect();
	int  leftBotAvgLineLen = _cornerCommDrawers[VP_LEFT_BOTTOM]->GetAvgLineLength();
	Rect leftBotLimitRect = leftBotRect;

	Rect rightTopRect = _cornerCommDrawers[VP_RIGHT_TOP]->GetBoundsRect();
	int  rightTopAvgLineLen = _cornerCommDrawers[VP_RIGHT_TOP]->GetAvgLineLength();
	Rect rightTopLimitRect = rightTopRect;

	Rect rightBotRect = _cornerCommDrawers[VP_RIGHT_BOTTOM]->GetBoundsRect();
	int  rightBotAvgLineLen = _cornerCommDrawers[VP_RIGHT_BOTTOM]->GetAvgLineLength();
	Rect rightBotLimitRect = rightBotRect;

	// width check
	if (leftTopRect.Width + rightTopRect.Width > imageWidth)
	{
		int topAvgLineLen = (leftTopAvgLineLen + rightTopAvgLineLen);
		if (topAvgLineLen > 0)
		{
			int newLeftTopWidth = imageWidth * leftTopAvgLineLen / topAvgLineLen;
			leftTopLimitRect.Width = newLeftTopWidth;

			int newRightTopWidth = imageWidth - newLeftTopWidth;
			rightTopLimitRect.Width = newRightTopWidth;
			rightTopLimitRect.X = OwnerImage()->ImageWidth() - COMMENT_RIGHT_MARGIN - newRightTopWidth;
		}
	}
	
	if (leftBotRect.Width + rightBotRect.Width > imageWidth)
	{
		int botAvgLineLen = (leftBotAvgLineLen + rightBotAvgLineLen);
		if (botAvgLineLen > 0)
		{
			int newLeftBotWidth = imageWidth * leftBotAvgLineLen / botAvgLineLen;
			leftBotLimitRect.Width = newLeftBotWidth;

			int newRightBotWidth = imageWidth - newLeftBotWidth;
			rightBotLimitRect.Width = newRightBotWidth;
			rightBotLimitRect.X = OwnerImage()->ImageWidth() - COMMENT_RIGHT_MARGIN - newRightBotWidth;
		}
	}
	
	// height check
	if (leftTopRect.Height + leftBotRect.Height > imageHeight)
	{
		int newTopHeight = imageHeight * leftTopRect.Height / (leftTopRect.Height + leftBotRect.Height);
		leftTopLimitRect.Height = newTopHeight;

		int newBotHeight = imageHeight - newTopHeight;
		leftBotLimitRect.Height = newBotHeight;
		leftBotLimitRect.Y = OwnerImage()->ImageHeight() - COMMENT_BOTTOM_MARGIN - newBotHeight;
	}

	if (rightTopRect.Height + rightTopRect.Height > imageHeight)
	{
		int newRightTopHeight = imageHeight * rightTopRect.Height / (rightTopRect.Height + rightBotRect.Height);
		rightTopLimitRect.Height = newRightTopHeight;

		int newRightBotHeight = imageHeight - newRightTopHeight;
		rightBotLimitRect.Height = newRightBotHeight;
		rightBotLimitRect.Y = OwnerImage()->ImageHeight() - COMMENT_BOTTOM_MARGIN - newRightBotHeight;
	}

	_cornerCommDrawers[VP_LEFT_TOP]->SetLimitRect(leftTopLimitRect);
	_cornerCommDrawers[VP_LEFT_BOTTOM]->SetLimitRect(leftBotLimitRect);
	_cornerCommDrawers[VP_RIGHT_TOP]->SetLimitRect(rightTopLimitRect);
	_cornerCommDrawers[VP_RIGHT_BOTTOM]->SetLimitRect(rightBotLimitRect);

	for (CornerCommentsDrawersIt it = _cornerCommDrawers.begin(); it != _cornerCommDrawers.end(); it ++)
	{
		it->second->ArrangeComments();
	}
}

int ImageCommentsDrawer::OnImageSizeChanged(float width, float height)
{
	Reset();

	return 0;
}
MCSF_DJ2DENGINE_END_NAMESPACE
