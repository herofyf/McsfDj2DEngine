#include "stdafx.h"
#include "image_drawing_static_line.h"
#include "graphics_imager.h"
#include "study_image.h"
#include "image_localizer_lines_drawing.h"
#include "image_tool_interface.h"
#include "image_drawing_theme_template.h"
#include "geometry_math.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageDrawingStaticLine::ImageDrawingStaticLine(ImageLocalizerLinesDrawing *pOwner, const DicomImageDescription *pShownImageDesc) :
	m_pOwner(pOwner), m_pDicomImageDesc(pShownImageDesc), ImageDrawingItem(pOwner->OwnerImage())
{
	StudyImage *pStudyImage = pOwner->OwnerImage();

	m_pMeasureToolAppearance = &(pStudyImage->GetDrawingAppearance()->lineToolAppearance);
}


ImageDrawingStaticLine::~ImageDrawingStaticLine(void)
{
}

bool ImageDrawingStaticLine::Draw(GraphicsImager *pImager) const
{
	if (pImager == NULL)
		return false;

	Color lineDrawingColor = m_pMeasureToolAppearance->lineNormProp.lineColor;
	int   lineDrawingWidth = m_pMeasureToolAppearance->lineNormProp.lineWidth;
	pImager->DrawCapLine(lineDrawingColor, lineDrawingWidth, m_pointfs[0].X, m_pointfs[0].Y, m_pointfs[1].X, m_pointfs[1].Y, 
		LineCapNoAnchor, true);

	if (m_strText.length() > 0)
	{
		FontPropMetaInfo fontProp = m_pMeasureToolAppearance->textFontProp;
		pImager->DrawString(m_strText, m_txtPtf.X, m_txtPtf.Y, 
			fontProp.sFontName, fontProp.fontSize, fontProp.fontStyle, fontProp.fontColor, true);
	}
	
	return true;
}

int ImageDrawingStaticLine::OnMouseEvent(const MouseEvtRequestArgs *mouseEventInfo)
{
	
	
	return false;
}

int ImageDrawingStaticLine::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return false;
}

int ImageDrawingStaticLine::OnImageSizeChanged(float width, float height)
{
	return false;
}

void ImageDrawingStaticLine::UpdatePoints(PointF *ptfs, int n)
{
	if (n != 2)
		return;

	m_pointfs[0] = ptfs[0];
	m_pointfs[1] = ptfs[1];
}

void ImageDrawingStaticLine::UpdateText(const std::string &strText, const PointF &txtPtf)
{
	m_strText = strText;

	m_txtPtf = txtPtf;
}

bool ImageDrawingStaticLine::HitTest(const PointF &pt)
{
	Point pts[] = {
		Point((int)m_pointfs[0].X, (int)m_pointfs[0].Y),
		Point((int)m_pointfs[1].X, (int)m_pointfs[1].Y),
	};

	int dis = ComputePointToLineDistance((int)pt.X, (int)pt.Y, pts, 2);
	return (dis < 3);
}

MCSF_DJ2DENGINE_END_NAMESPACE