#include "stdafx.h"
#include "image_region_pixels_drawing.h"
#include "graphics_imager.h"
#include "dicom_image_helper.h"
#include "image_tool_interface.h"
#include "image_drawing_theme_template.h"
#include "dicom_region_pixel_helper.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageDicomRegPixelsStDrawing::ImageDicomRegPixelsStDrawing(const MeasureToolDrawing *pOwner) :
	m_pOwnerMeasureTool(pOwner)
{
	for (int i = 0; i < REGPIXST_TEXTS_NUM; i ++)
	{
		m_textComments[i].Reset();
		m_hlTextComments[i].Reset();
	}

	m_fHLTextHeight = m_fHLTextWidth = m_fNormTextWidth = m_fNormTextHeight = 0;
	m_textStartPointF.X = m_textStartPointF.Y = 0;
}


ImageDicomRegPixelsStDrawing::~ImageDicomRegPixelsStDrawing(void)
{

}

void ImageDicomRegPixelsStDrawing::UpdateTextStartPointf(const PointF &textStartPointf)
{
	m_textStartPointF = textStartPointf;
}

void ImageDicomRegPixelsStDrawing::UpdateStValues(const PointF &textStartPointf, const DicomRegionPixelsStatistics *pSt)
{
	m_textStartPointF = textStartPointf;
	
	if (pSt == NULL) return;

	float fMaxWidth = 0, fSumHeight = 0, fHLMaxWidth = 0, fHLSumHeight = 0;
	RectF textRectf;
	std::string strValue;
	for (int i = 0; i < REGPIXST_TEXTS_NUM; i ++)
	{
		if (i == REGPIXST_TEXT_PIXELS)
		{
			strValue = pSt->StrCount();
		}
		else if (i == REGPIXST_TEXT_AREA)
		{
			strValue = pSt->StrArea();
		}
		else if (i == REGPIXST_TEXT_MEAN)
		{
			strValue = pSt->StrMeans();
		}
		else if (i == REGPIXST_TEXT_MAX)
		{
			strValue = pSt->StrMax();
		}
		else if (i ==  REGPIXST_TEXT_MIN)
		{
			strValue = pSt->StrMin();
		}
		else if (i == REGPIXST_TEXT_SD)
		{
			strValue = pSt->StrSD();
		}

		if (m_textComments[i].Text().length() != strValue.length())
		{
			textRectf = m_pOwnerMeasureTool->CalNormTextSize(strValue);
			m_textComments[i].Width(textRectf.Width, strValue);
			m_textComments[i].Height(textRectf.Height);

			textRectf = m_pOwnerMeasureTool->CalHlTextSize(strValue);
			m_hlTextComments[i].Width(textRectf.Width, strValue);
			m_hlTextComments[i].Height(textRectf.Height);
		}

		m_textComments[i].Text(strValue);
		m_hlTextComments[i].Text(strValue);

		if (m_textComments[i].Width() > fMaxWidth)
		{
			fMaxWidth = m_textComments[i].Width();
		}
		if (m_hlTextComments[i].Width() > fHLMaxWidth)
		{
			fHLMaxWidth = m_hlTextComments[i].Width();
		}

		fSumHeight += m_textComments[i].Height();
		fHLSumHeight += m_hlTextComments[i].Height();

	}

	m_fNormTextWidth = fMaxWidth;
	m_fNormTextHeight = fSumHeight;

	m_fHLTextWidth = fHLMaxWidth;
	m_fHLTextHeight = fHLSumHeight;
}

bool ImageDicomRegPixelsStDrawing::IsWithinTextRegion(float x, float y)
{
	float fWidth = 0, fHeight = 0;
	if (m_pOwnerMeasureTool == NULL) return false;

	if (m_pOwnerMeasureTool->IsShapeActive())
	{
		fWidth  = m_fHLTextWidth;
		fHeight = m_fHLTextHeight;
	}
	else
	{
		fWidth = m_fNormTextWidth;
		fHeight = m_fNormTextHeight;
	}

	RectF textRect(m_textStartPointF.X, m_textStartPointF.Y, fWidth, fHeight);
	PointF pointf(x, y);
	return textRect.Contains(pointf);
}

void ImageDicomRegPixelsStDrawing::Draw(GraphicsImager *pImager, const FontPropMetaInfo *pNormFontInf, const FontPropMetaInfo *pHlFontInf) const
{
	if ((pImager == NULL) || (pNormFontInf == NULL) || (pHlFontInf == NULL) || (m_pOwnerMeasureTool == NULL)) return;

	const ImageMesauredSizeLineText *pTextLines = m_textComments;
	const FontPropMetaInfo *pCurFontInfo = pNormFontInf;
	if (m_pOwnerMeasureTool->IsShapeActive())
	{
		pTextLines = m_hlTextComments;
		pCurFontInfo = pHlFontInf;
	}

	float sumHeight = 0, fX = 0, fY = 0;
	fX = m_textStartPointF.X;
	fY = m_textStartPointF.Y;

	std::string strVal;
	for (int i = 0; i < REGPIXST_TEXTS_NUM; i ++)
	{
		strVal = pTextLines[i].Text();
		if (strVal.length() <= 0) break;

		pImager->DrawString(strVal, fX, fY, 
			pCurFontInfo->sFontName, pCurFontInfo->fontSize, pCurFontInfo->fontStyle, pCurFontInfo->fontColor, true);

		if (i == REGPIXST_TEXT_AREA)
		{
			// to draw cm2's 2
			int areaSymFontSize = pCurFontInfo->fontSize / 2;
			areaSymFontSize = areaSymFontSize > 0 ? areaSymFontSize : 2;
			pImager->DrawString("2", fX + pTextLines[i].Width() + areaSymFontSize, fY,
				pCurFontInfo->sFontName, areaSymFontSize, pCurFontInfo->fontStyle, pCurFontInfo->fontColor, false);
		}
		fY += pTextLines[i].Height() + 1;
	}
}
MCSF_DJ2DENGINE_END_NAMESPACE