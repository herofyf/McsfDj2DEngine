#include "stdafx.h"
#include "image_drawing_theme_template.h"
#include "study_image.h"
#include "system_global_configuration.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

FontsSizeInformationCache *FontsSizeInformationCache::m_pInstance = NULL;

FontsSizeInformationCache::FontsSizeInformationCache()
{

}

FontsSizeInformationCache * FontsSizeInformationCache::Instance()
{
	if (m_pInstance == NULL)
	{
		m_pInstance = new FontsSizeInformationCache();
	}

	return m_pInstance;
}

bool FontsSizeInformationCache::FindCachedTextRectF(const std::string &strText, const std::string &fontName, int fontSize, int fontType, Gdiplus::RectF &rectF)
{
	TextFontPropMetaInfo tfmi;
	tfmi.fontSize = fontSize;
	tfmi.sText    = strText;
	tfmi.sFontName= fontName;
	tfmi.fontStyle = fontType;

	ACE_Guard<ACE_Thread_Mutex> lock(m_mutex);

	CachedTextFontSizeMapIt it = m_fontsCachedSize.find(tfmi);
	if (it != m_fontsCachedSize.end())
	{
		rectF = it->second;
		return true;
	}
	else
	{
		SystemGlobalResource::DefBmpGraphicsImager()->MeasureString(tfmi.sText, tfmi.sFontName, tfmi.fontSize, tfmi.fontStyle, Color(255, 255, 255), &rectF);

		m_fontsCachedSize[tfmi] = rectF;

		return true;
	}
}

void FontsSizeInformationCache::Release()
{
	delete this;
}

ObjectDrawingAppearance::ObjectDrawingAppearance(StudyImage *pStudyImage) :
	m_pStudyImage(pStudyImage)
{

}

RectF ObjectDrawingAppearance::CalTextSize(const std::string &str, const FontPropMetaInfo &fontProp) const
{
	RectF resultRectF;

	FontsSizeInformationCache::Instance()->FindCachedTextRectF(str, fontProp.sFontName, fontProp.fontSize, fontProp.fontStyle, resultRectF);
	
	return resultRectF;
}

int ObjectDrawingAppearance::CalNormalFontSize(float magr)
{
	if (m_pStudyImage)
	{
		int imageWidth = m_pStudyImage->ImageWidth();
		if (imageWidth < 500) imageWidth = 500;

		return (imageWidth / 50) * magr;
	}
	return 10;
}

MeasureToolDrawingAppearance::MeasureToolDrawingAppearance(StudyImage *pStudyImage) :
	ObjectDrawingAppearance(pStudyImage)
{
	lineNormProp.lineColor = Color(0, 255, 0);
	lineNormProp.lineWidth = 1.5;
	lineHlProp.lineColor = Color(255, 255, 0);
	lineHlProp.lineWidth = 1.8;

	textFontProp.sFontName = "Tahoma";
	textFontProp.fontStyle = FontStyleBold;
	textFontProp.fontSize = CalNormalFontSize(1.2);
	textFontProp.fontColor = Color(0, 255, 255);

	textFontHlProp = textFontProp;
	textFontHlProp.fontColor = lineHlProp.lineColor;

	connectorLineNormProp.lineColor = Color(0, 255, 255);
	connectorLineNormProp.lineWidth = 1;

	textXOffset = textFontProp.fontSize * 4;
}

void MeasureToolDrawingAppearance::OnImageSizeChanged()
{
	textFontHlProp.fontSize = textFontProp.fontSize = CalNormalFontSize(1.2);
	textXOffset = textFontProp.fontSize * 4;
}

RectF MeasureToolDrawingAppearance::CalNormTextSize(const std::string &str)
{
	return CalTextSize(str, textFontProp);
}

RectF MeasureToolDrawingAppearance::CalHlTextSize(const std::string &str)
{
	return CalTextSize(str, textFontHlProp);
}

ImageGaugeDrawingAppearance::ImageGaugeDrawingAppearance(StudyImage *pStudyImage) :
	ObjectDrawingAppearance(pStudyImage)
{
	lineColor = Color(255, 255, 255);
	boundaryLineWidth = 2;
	boundaryLineLength = 15;
	tickLineWidth = 1;
	rightMargin = 15;
	textFontProp.fontColor = Color(255, 255, 255);
	textFontProp.fontSize = CalNormalFontSize(1);
	textFontProp.fontStyle = FontStyleRegular;
	textFontProp.sFontName = "Tahoma";
	
	textYOffset = 3;
}

void ImageGaugeDrawingAppearance::OnImageSizeChanged()
{
	textFontProp.fontSize = CalNormalFontSize(1);
}

RectF ImageGaugeDrawingAppearance::CalTextSize(const std::string &str)
{
	return ObjectDrawingAppearance::CalTextSize(str, textFontProp);
}

ImageCommentsDrawingAppearance::ImageCommentsDrawingAppearance(StudyImage *pStudyImage) :
	ObjectDrawingAppearance(pStudyImage)
{
	textFontProp.fontColor = Color(255, 255, 255);
	textFontProp.fontSize = CalNormalFontSize(1);
	textFontProp.fontStyle = FontStyleRegular;
	textFontProp.sFontName = "Tahoma";
}

void ImageCommentsDrawingAppearance::OnImageSizeChanged()
{
	textFontProp.fontSize = CalNormalFontSize(1);
}

RectF ImageCommentsDrawingAppearance::CalTextSize(const std::string &str)
{
	return ObjectDrawingAppearance::CalTextSize(str, textFontProp);
}

ImageItemsDrawingAppearance::ImageItemsDrawingAppearance(StudyImage *pStudyImage) :
	lineToolAppearance(pStudyImage), gaugeAppearance(pStudyImage), commentsAppearance(pStudyImage),
	angleToolAppearance(pStudyImage), circleToolAppearance(pStudyImage), freeHandAppearance(pStudyImage),
	noteToolAppearance(pStudyImage)
{
	// make special setting
	angleToolAppearance.textXOffset = 20;
}

void ImageItemsDrawingAppearance::OnImageSizeChanged()
{
	lineToolAppearance.OnImageSizeChanged();
	angleToolAppearance.OnImageSizeChanged();
	freeHandAppearance.OnImageSizeChanged();
	gaugeAppearance.OnImageSizeChanged();
	commentsAppearance.OnImageSizeChanged();
}


MCSF_DJ2DENGINE_END_NAMESPACE