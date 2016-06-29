#include "stdafx.h"
#include "image_localizer_lines_drawing.h"
#include "dicom_component_information.h"
#include "study_image.h"
#include <sstream>
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageLocalizerLinesDrawing::ImageLocalizerLinesDrawing(StudyImage *pImage) :
	ImageDrawingItem(pImage), m_localizerLineAlgo(this), m_pReferredSeriesDesc(NULL),
	m_pFirstImgRefLine(NULL), m_pLastImgRefLine(NULL)
{
}


ImageLocalizerLinesDrawing::~ImageLocalizerLinesDrawing(void)
{
	ClearStaticLines();
}

bool ImageLocalizerLinesDrawing::OnReferenceLineMove(int nCellNum, PointF mousePos)
{
	// to calculate the image pixel location
	PointF imagePixelPos = mousePos;

	OwnerImage()->TransformPts_Cell2OrgDicomImage(&imagePixelPos, 1);

	OwnerImage()->OnReferLineMoveToShowRefedSeriesImage(nCellNum, imagePixelPos);
	
	return true;
}

int  ImageLocalizerLinesDrawing::OnMouseEvent(const MouseEvtRequestArgs *pMouseEvt)
{
	if (IsInterestMouseEvt(pMouseEvt) == false)
		return false;

	if (pMouseEvt->leftDown == false)
	{
		m_nTargetCellNum = -1;
	}
	else 
	{
		PointF mousePos(pMouseEvt->pointX, pMouseEvt->pointY);

		if (m_nTargetCellNum == -1)
		{
			for (CellImgRefLineMapIt it = m_AnchorRefLines.begin(); it != m_AnchorRefLines.end(); it ++)
			{
				if (it->second->HitTest(mousePos))
				{
					m_nTargetCellNum = it->first;
					break;
				}
			}
		}
		
		if (m_nTargetCellNum >= 0)
		{
			OnReferenceLineMove(m_nTargetCellNum, mousePos);
		}
	}
	
	return false;
}

int  ImageLocalizerLinesDrawing::OnKeyboard(const KeyboardEvtRequestArgs *pKeyboard)
{
	bool bRet = false;

	return bRet;
}

int  ImageLocalizerLinesDrawing::OnImageSizeChanged(float width, float height)
{
	bool bRet = false;

	return bRet;
}

bool ImageLocalizerLinesDrawing::Draw(GraphicsImager *pImager) const
{
	bool bRet = false;

	if (m_pFirstImgRefLine)
	{
		m_pFirstImgRefLine->Draw(pImager);
	}

	for (CellImgRefLineMapCIt it = m_AnchorRefLines.begin(); it != m_AnchorRefLines.end(); it ++)
	{
		(it->second)->Draw(pImager);
	}

	if (m_pLastImgRefLine)
	{
		m_pLastImgRefLine->Draw(pImager);
	}

	return bRet;
}

void ImageLocalizerLinesDrawing::ClearStaticLines()
{
	DEL_PTR(m_pFirstImgRefLine);
	DEL_PTR(m_pLastImgRefLine);

	for (CellImgRefLineMapIt it = m_AnchorRefLines.begin(); it != m_AnchorRefLines.end(); it ++)
	{
		DEL_PTR(it->second);
	}

	m_AnchorRefLines.clear();
}

ImageDrawingStaticLine *ImageLocalizerLinesDrawing::CreateStaticLine(const DicomImageDescription *pImageDesc)
{
	if (pImageDesc == NULL)
		return NULL;

	PointF linePts[3];

	bool bRet = m_localizerLineAlgo.CalculateReferenceLine(pImageDesc, &(linePts[0]), &(linePts[1]));

	linePts[2].X = linePts[1].X + 5;
	linePts[2].Y = linePts[1].Y;

	OwnerImage()->TransformPts_OrgDicomImage2Cell(linePts, 3);

	if (linePts[0].Equals(linePts[1]))
		return NULL;

	ImageDrawingStaticLine *pStaticLine = new ImageDrawingStaticLine(this, pImageDesc);
	if (pStaticLine == NULL)
		return NULL;

	std::stringstream ssText;
	ssText << pImageDesc->DicomFrameIndex() + pImageDesc->ImageIndex();
	pStaticLine->UpdateText(ssText.str(), linePts[2]);

	pStaticLine->UpdatePoints(linePts, 2);

	return pStaticLine;
}

bool ImageLocalizerLinesDrawing::CalculateReferenceLines(const DicomSeriesDescription *pReferredSeriesDesc)
{
	m_pReferredSeriesDesc = pReferredSeriesDesc;

	ClearStaticLines();

	m_pFirstImgRefLine = CreateStaticLine(pReferredSeriesDesc->GetFirstImageDesc());
	
	m_pLastImgRefLine = CreateStaticLine(pReferredSeriesDesc->GetLastImageDesc());
	
	CellImageDescMap cellImageDescs;

	pReferredSeriesDesc->MyStudySeries()->GetShownCellImgeDescs(cellImageDescs);

	const DicomImageDescription *pShownImageDesc = NULL;
	for (CellImageDescMapCIt cit = cellImageDescs.begin(); cit != cellImageDescs.end(); cit ++)
	{
		pShownImageDesc = cit->second;
		ImageDrawingStaticLine *pStaticLine = CreateStaticLine(pShownImageDesc);
		if (pStaticLine)
			m_AnchorRefLines[cit->first] = (pStaticLine);
	}

	return (m_AnchorRefLines.size() > 0);
}


MCSF_DJ2DENGINE_END_NAMESPACE
