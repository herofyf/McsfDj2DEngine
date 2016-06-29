#include "stdafx.h"
#include "image_vertical_gauge_ruler.h"
#include "study_image.h"
#include "system_global_configuration.h"
#include "trace_performance_func.h"
#include "study_series_command_request_args.h"
#include "image_drawing_theme_template.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
ImageVerticalGaugeRuler::ImageVerticalGaugeRuler(StudyImage *pStudyImage) : 
	TransformableDrawing(pStudyImage), m_nWinWidthSize(0), m_nWinHeightSize(0), m_nRulerSelector(-1),
	m_nDescWidth(-1), m_nTickMarksNum(0), m_nSubTickMarksNum(0)
{
	m_pGaugeAppearance = &(APPEAR_OBJ(OwnerImage())->gaugeAppearance);

	m_pImageLogicUnitHelper = LOGIC_U_OBJ(OwnerImage());
}

void ImageVerticalGaugeRuler::ReselectGaugeRuler()
{
	m_nWinWidthSize = m_pStudyImage->ImageWidth();
	m_nWinHeightSize = m_pStudyImage->ImageHeight();

	// init ruler list
	if (m_pImageLogicUnitHelper->GetLogicUnitType() == UNIT_TYPE_PIXEL)
		SetupPixelRulers();
	else if (m_pImageLogicUnitHelper->GetLogicUnitType() == UNIT_TYPE_MM)
		SetupCmRulers();
}

void ImageVerticalGaugeRuler::DefineCmRuler(float logicLength, int marks, int subMarks)
{
	int quarterWinPixels = m_nWinHeightSize / 4;
	GaugeRuler ruler;
	ruler.logicValuef.SetUnitType(UNIT_TYPE_CM);
	ruler.logicValuef.SetLogicUnitValue(logicLength);
	ruler.scaleUnit = logicLength * 10 / quarterWinPixels;
	
	ruler.tickMarksNum = marks;
		 
	ruler.subtickMarksNum = subMarks;
	m_rulers.push_back(ruler);
}

void ImageVerticalGaugeRuler::ApplyRuler(int nRulerSelector)
{
	if (nRulerSelector < 0) return ;

	m_nRulerSelector = nRulerSelector;

	const GaugeRuler &ruler = m_rulers[m_nRulerSelector];

	LogicUnitValuef logicUnitValf = ruler.logicValuef;
	bool b = CalVerticalCmRulerPoints(logicUnitValf, m_points, 2);

	std::string strVal = logicUnitValf.toString();

	Gdiplus::RectF rectF = m_pGaugeAppearance->CalTextSize(strVal);

	m_nDescWidth = rectF.Width;

	m_strText = strVal;

	m_nTickMarksNum = ruler.tickMarksNum;
	m_nSubTickMarksNum = ruler.subtickMarksNum;
}

void ImageVerticalGaugeRuler::SetupCmRulers()
{
	// 0.1 cm
	DefineCmRuler(0.1, 5-1, 1);

	// 0.2 cm
	DefineCmRuler(0.2, 2-1, 0);

	// 0.3 cm
	DefineCmRuler(0.3, 3-1, 0);

	// 0.5 cm
	DefineCmRuler(0.5, 5-1, 0);

	// 1cm
	DefineCmRuler(1, 10-1, 0);

	// 3cm
	DefineCmRuler(3, 3-1, 3-1);

	// 5cm
	DefineCmRuler(5, 5-1, 2-1);

	// 10cm
	DefineCmRuler(10, 10-1, 0);

	// 20cm
	DefineCmRuler(20, 2-1, 5-1);

	// 30cm
	DefineCmRuler(30, 3-1, 5-1);

	// 50cm
	DefineCmRuler(50, 5-1, 5-1);


	int nRulerSelector = SelectRightRuler();
	
	ApplyRuler(nRulerSelector);

	CorrectTickMarksNumber();
}

bool ImageVerticalGaugeRuler::CalVerticalCmRulerPoints(const LogicUnitValuef &logicValf, PointF *points, int n)
{
	int ypixeles = m_pImageLogicUnitHelper->CalVerticalPixel(logicValf);

	int top = (m_nWinHeightSize - ypixeles) / 2;
	int bot = (m_nWinHeightSize + ypixeles) / 2;
	
	points[0].X = m_nWinWidthSize - m_pGaugeAppearance->rightMargin;
	points[0].Y = top;

	points[1].X = m_nWinWidthSize - m_pGaugeAppearance->rightMargin;
	points[1].Y = bot;

	return true;
}

void ImageVerticalGaugeRuler::SetupPixelRulers()
{

}

ImageVerticalGaugeRuler::~ImageVerticalGaugeRuler(void)
{

}

int ImageVerticalGaugeRuler::OnImageSizeChanged(float width, float height)
{
	
	return 0;
}

bool ImageVerticalGaugeRuler::Draw(GraphicsImager *pImager) const
{	
	

	//CPU_PERF_CAL cal("ImageVerticalGaugeRuler::Draw");
	// height line
	pImager->DrawLine(m_pGaugeAppearance->lineColor, 
		m_pGaugeAppearance->boundaryLineWidth,
		m_points[0].X, m_points[0].Y, m_points[1].X, m_points[1].Y);
	
	//
	// top line
	pImager->DrawLine(m_pGaugeAppearance->lineColor, 
		m_pGaugeAppearance->tickLineWidth,
		(m_points[0].X - m_pGaugeAppearance->boundaryLineLength),
		m_points[0].Y,
		m_points[0].X,
		m_points[0].Y);

	// bottom line
	pImager->DrawLine(m_pGaugeAppearance->lineColor, 
		m_pGaugeAppearance->tickLineWidth,
		(m_points[1].X - m_pGaugeAppearance->boundaryLineLength),
		m_points[1].Y,
		m_points[1].X,
		m_points[1].Y);

	float markStep = (m_points[1].Y - m_points[0].Y) / (m_nTickMarksNum + 1);
	float subMarkStep = markStep / (m_nSubTickMarksNum + 1);
	float yPos = m_points[0].Y;
	for (int i =1; i <= m_nTickMarksNum + 1; i ++)
	{
		for (int j = 1; j <= m_nSubTickMarksNum + 1; j ++)
		{
			float subyPos = yPos + subMarkStep * j;
			pImager->DrawLine(m_pGaugeAppearance->lineColor, 
				m_pGaugeAppearance->tickLineWidth,
				(m_points[1].X - m_pGaugeAppearance->boundaryLineLength * 0.5),
				subyPos,
				m_points[1].X,
				subyPos);
		}

		yPos = m_points[0].Y + markStep * i;
		pImager->DrawLine(m_pGaugeAppearance->lineColor, 
			m_pGaugeAppearance->tickLineWidth,
			(m_points[1].X - m_pGaugeAppearance->boundaryLineLength * 0.85),
			yPos,
			m_points[1].X,
			yPos);

	
	}

	// show physical length
	
	pImager->DrawString(m_strText, m_points[1].X - m_nDescWidth, m_points[1].Y -  m_pGaugeAppearance->textYOffset,
			m_pGaugeAppearance->textFontProp.sFontName, m_pGaugeAppearance->textFontProp.fontSize,
		 m_pGaugeAppearance->textFontProp.fontStyle, m_pGaugeAppearance->textFontProp.fontColor);

	return true;
}


bool ImageVerticalGaugeRuler::onTransformation(const TransformationArgs *pTransformationArgs)
{
	if (pTransformationArgs == NULL)
		return false;

	CoordinateTranslator *pCoordHelper = CO_TR_OBJ(OwnerImage());

	if (pTransformationArgs->transformationType == TRANS_SCALE)
	{
		float fScaleY = pTransformationArgs->args.scaleArgs.scaleY;

		int nSelector = SelectRightRuler();
		if (nSelector != m_nRulerSelector)
		{
			ApplyRuler(nSelector);
		}
		else
		{
			bool b = pCoordHelper->ScaleShape(0,fScaleY, m_points, 2);
			if (b)
			{
				TransformableDrawing::onTransformation(pTransformationArgs);
			}
			
		}
		
		CorrectTickMarksNumber();
	}

	return true;
}

int ImageVerticalGaugeRuler::SelectRightRuler()
{
	float verticalLogicUnit = m_pImageLogicUnitHelper->GetVerticalLogicUnit();
	int nRulerSelector = 0, nSize = m_rulers.size();
	if (nSize <= 0) return -1;

	float cur = -1, next = 0;
	
	// lessest
	if (verticalLogicUnit < m_rulers[0].scaleUnit)
	{
		nRulerSelector = 0;
		return nRulerSelector;
	}

	// smallest
	if (verticalLogicUnit > m_rulers[nSize -1].scaleUnit)
	{
		nRulerSelector = nSize -1;
		return nRulerSelector;
	}

	for (int i =0; i < nSize - 1; i ++)
	{
		cur =  m_rulers[i].scaleUnit;
		next = m_rulers[i + 1].scaleUnit;

		if (cur  < verticalLogicUnit &&
			next > verticalLogicUnit)
		{
			nRulerSelector = i + 1;
			break;
		}
	}

	return nRulerSelector;
}

void ImageVerticalGaugeRuler::CorrectTickMarksNumber()
{
	if (m_nRulerSelector < 0 || m_nRulerSelector > m_rulers.size())
		return;

	float fHeight = fabsf(m_points[1].Y - m_points[0].Y);

	int tickMarksNum = m_nTickMarksNum, subTickMarksNum = m_nSubTickMarksNum;

	subTickMarksNum = subTickMarksNum <= 0 ? 1: subTickMarksNum;
	int totalMarks = (tickMarksNum + 1) * subTickMarksNum;
	if (totalMarks == 0) totalMarks = 1;

	float subTickMarkSpace = fHeight / totalMarks;
	float tickMarkSpace = fHeight / (m_nTickMarksNum);

#define SUBTICK_MIN  3
#define TICK_MIN  6
	
	if (subTickMarkSpace < SUBTICK_MIN)
	{
		// if tick and subtick both small
		if (tickMarkSpace < TICK_MIN)
		{
			tickMarksNum = fHeight / TICK_MIN;
			if (m_nTickMarksNum > tickMarksNum)
				m_nTickMarksNum = tickMarksNum;

			subTickMarksNum = TICK_MIN / m_nSubTickMarksNum;
			if (m_nSubTickMarksNum > subTickMarksNum)
				m_nSubTickMarksNum = subTickMarksNum;
		}
		else
		{
			subTickMarksNum = tickMarkSpace / SUBTICK_MIN;
			if (m_nSubTickMarksNum > subTickMarksNum)
				m_nSubTickMarksNum = subTickMarksNum;
		}
		
	}
}

int ImageVerticalGaugeRuler::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

int ImageVerticalGaugeRuler::OnMouseEvent(const MouseEvtRequestArgs *mouseEventInfo)
{
	return 0;
}
MCSF_DJ2DENGINE_END_NAMESPACE
