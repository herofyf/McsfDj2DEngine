#include "stdafx.h"
#include "graphics_imager.h"
#include <stdio.h>
#include "trace_performance_func.h"
#include "geometry_math.h"
#include "mcsf_charset_converter.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

DrawImage_HighQualityAndSpeed::DrawImage_HighQualityAndSpeed(Gdiplus::Graphics *pGraphics) :
	m_pGdiPlusGraphics(pGraphics)
{
	if (m_pGdiPlusGraphics)
	{
		m_compositingMode = m_pGdiPlusGraphics->GetCompositingMode();
		m_pGdiPlusGraphics->SetCompositingMode(CompositingModeSourceCopy);

		m_interpolationMode = m_pGdiPlusGraphics->GetInterpolationMode();
		m_pGdiPlusGraphics->SetInterpolationMode(InterpolationModeHighQuality);

		m_pixelOffsetMode = m_pGdiPlusGraphics->GetPixelOffsetMode();
		m_pGdiPlusGraphics->SetPixelOffsetMode(PixelOffsetModeHighSpeed);

		// on gamma correction, because there no need to adjust with black background
		m_compositingQuality = m_pGdiPlusGraphics->GetCompositingQuality();
		m_pGdiPlusGraphics->SetCompositingQuality(CompositingQualityHighSpeed);

		m_smoothingMode = m_pGdiPlusGraphics->GetSmoothingMode();
		m_pGdiPlusGraphics->SetSmoothingMode(SmoothingModeHighSpeed);

	}
}

DrawImage_HighQualityAndSpeed::~DrawImage_HighQualityAndSpeed()
{
	if (m_pGdiPlusGraphics)
	{
		m_pGdiPlusGraphics->SetCompositingMode(m_compositingMode);
		m_pGdiPlusGraphics->SetInterpolationMode(m_interpolationMode);
		m_pGdiPlusGraphics->SetPixelOffsetMode(m_pixelOffsetMode);
		m_pGdiPlusGraphics->SetCompositingQuality(m_compositingQuality);
		m_pGdiPlusGraphics->SetSmoothingMode(m_smoothingMode);
	}
}

DrawString_HQS::DrawString_HQS(Gdiplus::Graphics *pGraphics) :
	m_pGdiPlusGraphics(pGraphics)
{
	if (m_pGdiPlusGraphics)
	{
		m_pixelOffsetMode = m_pGdiPlusGraphics->GetPixelOffsetMode();
		m_pGdiPlusGraphics->SetPixelOffsetMode(PixelOffsetModeHighSpeed);

		m_smoothingMode = m_pGdiPlusGraphics->GetSmoothingMode();
		m_pGdiPlusGraphics->SetSmoothingMode(SmoothingModeHighSpeed);

		/*m_textRenderingHint = m_pGdiPlusGraphics->GetTextRenderingHint();
		m_pGdiPlusGraphics->SetTextRenderingHint(TextRenderingHintAntiAlias);*/
		/*m_compositingQuality = m_pGdiPlusGraphics->GetCompositingQuality();
		m_pGdiPlusGraphics->SetCompositingQuality(CompositingQualityHighSpeed);*/

		m_interpolationMode = m_pGdiPlusGraphics->GetInterpolationMode();
		m_pGdiPlusGraphics->SetInterpolationMode(InterpolationModeHighQuality);
	}
}

DrawString_HQS::~DrawString_HQS()
{
	if (m_pGdiPlusGraphics)
	{
		m_pGdiPlusGraphics->SetSmoothingMode(m_smoothingMode);
		m_pGdiPlusGraphics->SetPixelOffsetMode(m_pixelOffsetMode);
		/*m_pGdiPlusGraphics->SetTextRenderingHint(m_textRenderingHint);*/
		/*m_pGdiPlusGraphics->SetCompositingQuality(m_compositingQuality);*/
		m_pGdiPlusGraphics->SetInterpolationMode(m_interpolationMode);
	}
}

boost::shared_ptr<Gdiplus::ColorMatrix> ColorMatrixGenerator::Generate(DrawImageColorMaskType type)
{
	boost::shared_ptr<Gdiplus::ColorMatrix> resultColorMatrix;
	Gdiplus::ColorMatrix *pColorMarix = NULL;

	if (type == Negate)
	{
		resultColorMatrix = boost::make_shared<Gdiplus::ColorMatrix>();
			
		Gdiplus::ColorMatrix cm  = {
					-1,  0,  0,  0,  0,
					 0, -1,  0,  0,  0,
					 0,  0, -1,  0,  0,
					 0,  0,  0,  1,  0,
					 1,  1,  1,  0,  1
		};

		Gdiplus::ColorMatrix *pColorMarix = resultColorMatrix.get();
		*pColorMarix = cm;
	}

	return resultColorMatrix;
}

Graphics *GraphicsImager::NewGdiplusGraphics(Gdiplus::Image *pImage)
{
	Gdiplus::Graphics *pGraphics = new Gdiplus::Graphics(pImage);

	return pGraphics;
}

// construct 1
GraphicsImager::GraphicsImager(Gdiplus::Image * pImage) : m_pImage(NULL), m_pGraphics(NULL)
{
	if (pImage)
	{
		m_pImage = pImage;
		m_pGraphics = NewGdiplusGraphics(pImage);
	}
}

// construct 2
GraphicsImager::GraphicsImager(INT width, INT height, long flag) : m_pImage(NULL), m_pGraphics(NULL)
{
	m_pImage = new Gdiplus::Bitmap(width, height, (Gdiplus::PixelFormat) flag);

	assert(m_pImage != NULL);
	
	m_pGraphics = NewGdiplusGraphics(m_pImage);
}

GraphicsImager::GraphicsImager(const GraphicsImager *pGraphicsImager)
{
	if (pGraphicsImager)
	{
		this->m_pImage = pGraphicsImager->m_pImage;
		this->m_pGraphics = pGraphicsImager->m_pGraphics;
	}
}

int GraphicsImager::Clear(Color color)
{
	if (m_pImage && m_pGraphics)
	{
		m_pGraphics->Clear(color);
	}
	return 0;
}

GraphicsImager::~GraphicsImager()
{
	DEL_PTR(m_pImage);

	DEL_PTR(m_pGraphics);
}

// f
GraphicsImager  *GraphicsImager::FromFile(std::string fileName, bool bUsedPixelFormat, Gdiplus::PixelFormat usedPixelFormat)
{
	GraphicsImager *pGraphicsImager = NULL;

	std::wstring wstrFileName(fileName.begin(), fileName.end());
	
	Gdiplus::Bitmap * pBitmap = new Gdiplus::Bitmap(wstrFileName.c_str());
	Gdiplus::Image * pImage = dynamic_cast<Gdiplus::Image *>(pBitmap);
	if (pBitmap == NULL || ((pBitmap->GetPixelFormat() != usedPixelFormat) && bUsedPixelFormat))
	{
		Gdiplus::Image * pLoadFileImage = pBitmap ? 
										  (dynamic_cast<Gdiplus::Image *>(pBitmap)) : 
										  (Gdiplus::Image::FromFile(wstrFileName.c_str()));
										  
		if (pLoadFileImage)
		{
			// just use the load file directly.
			if (bUsedPixelFormat == false || (bUsedPixelFormat && (pLoadFileImage->GetPixelFormat() == usedPixelFormat)))
			{
				pGraphicsImager = new GraphicsImager(pLoadFileImage);
			}
			else
			{
				GraphicsImager *pLoadFileGraphicsImager = new GraphicsImager(pLoadFileImage);

				pGraphicsImager = new GraphicsImager(pLoadFileImage->GetWidth(), pLoadFileImage->GetHeight(), usedPixelFormat);
				if (pGraphicsImager)
				{
					pGraphicsImager->DrawImage_QS(pLoadFileGraphicsImager, 0, 0, pLoadFileImage->GetWidth(), pLoadFileImage->GetHeight());
				}

				DEL_PTR(pLoadFileGraphicsImager);
			}
		}
	}
	else if (pImage)
	{
		pGraphicsImager = new GraphicsImager(pImage);
	}

	//assert(pImage != NULL);
	return pGraphicsImager;
}

UINT GraphicsImager::GetHeight()
{
	return m_pImage ? m_pImage->GetHeight() : 0;
}

UINT GraphicsImager::GetWidth()
{
	return m_pImage ? m_pImage->GetWidth() : 0;
}

int GraphicsImager::DrawImage_QS(GraphicsImager *pImager, INT x, INT y, INT width, INT height)
{
	if (m_pImage && pImager)
	{
		Gdiplus::Image *pImage = pImager->m_pImage;
		
		DrawImage_HighQualityAndSpeed enable_drawimage_hqs(this->m_pGraphics);

		m_pGraphics->DrawImage(pImage, x, y, width, height);
		
		return 0;
	}

	return -1;
}


bool GraphicsImager::ImageToBitmap(boost::shared_ptr<GraphicsImager> &imagerPtr)
{
	imagerPtr.reset();

	if (m_pImage)
	{
		// IF bitmap just clone directly.
		Gdiplus::Bitmap *pBitmap = dynamic_cast<Gdiplus::Bitmap *>(m_pImage);
		if (pBitmap)
		{
			imagerPtr = boost::make_shared<GraphicsImager>(this->Clone());;
		}
		else
		{
			long pixelFormat = m_pImage->GetPixelFormat();
			UINT width = m_pImage->GetWidth();
			UINT height = m_pImage->GetHeight();

			imagerPtr = boost::make_shared<GraphicsImager>(width, height, pixelFormat);

			if (imagerPtr.get() != NULL)
			{
				imagerPtr->DrawImage_QS(this, 0, 0, width, height);
				return true;
			}
		}
		
	}

	return (imagerPtr.get() != NULL);
}


int GraphicsImager::DrawImage_QS(GraphicsImager *pImager, PointF *points, int size)
{
	if (m_pImage && pImager)
	{
		Gdiplus::Image *pImage = pImager->m_pImage;
		
		DrawImage_HighQualityAndSpeed enable_drawimage_hqs(this->m_pGraphics);
		m_pGraphics->DrawImage(pImage, points, size);
		
		return 0;
	}

	return -1;
}

int GraphicsImager::DrawImage_QS(GraphicsImager *pImager, PointF *points, int size, DrawImageColorMaskType type)
{
	if (m_pImage && pImager)
	{
		Gdiplus::Image *pImage = pImager->m_pImage;

		DrawImage_HighQualityAndSpeed enable_drawimage_hqs(this->m_pGraphics);

		Gdiplus::ImageAttributes imageAttr;

		if (type == Negate)
		{
			Gdiplus::ColorMatrix cm = {
				-1,  0,  0,  0,  0,
				0, -1,  0,  0,  0,
				0,  0, -1,  0,  0,
				0,  0,  0,  1,  0,
				1,  1,  1,  0,  1
			};
			imageAttr.SetColorMatrix(&cm);
		}

		m_pGraphics->DrawImage(pImage, points, size, 0, 0, pImage->GetWidth(), pImage->GetHeight(), Gdiplus::UnitPixel, &imageAttr);

		return true;
	}

	return false;
}

GraphicsImager *GraphicsImager::Clone() const
{
	if (m_pImage == NULL)
		return NULL;

	int nWidth = m_pImage->GetWidth();
	int nHeight = m_pImage->GetHeight();

	Gdiplus::Bitmap *pBitmap = dynamic_cast<Gdiplus::Bitmap *>(m_pImage);
	Gdiplus::Image *pCloneImage = NULL;
	if (pBitmap)
	{
		Rect bmRect(0, 0, nWidth, nHeight);
		pCloneImage = pBitmap->Clone(bmRect, pBitmap->GetPixelFormat());
	}
	else if (m_pImage)
	{
		pCloneImage = new Gdiplus::Bitmap(nWidth, nHeight, SystemGlobalResource::MediateImagePixelFormat());
		if (pCloneImage)
		{
			Graphics g(pCloneImage);
			g.DrawImage(m_pImage, 0, 0, nWidth, nHeight);
		}
	}
	
	if (pCloneImage)
	{
		return new GraphicsImager(pCloneImage);
	}

	return NULL;
}

int GraphicsImager::FillRectangle(Color brushColor, INT x, INT y, INT width, INT height)
{
	if (m_pImage)
	{
		SolidBrush blackBrush(brushColor);
		Status status = m_pGraphics->FillRectangle(&blackBrush, 0, 0, width, height);

		return status == Status::Ok;
	}

	return -1;
}

int GraphicsImager::FillEllipse(Color brushColor, INT x, INT y, INT width, INT height)
{
	if (m_pImage)
	{
		SolidBrush blackBrush(brushColor);
		Status status = m_pGraphics->FillEllipse(&blackBrush, x, y, width, height);

		return status == Status::Ok;
	}

	return -1;
}

int GraphicsImager::SaveToFile(std::string fileName, EncoderClsidType type)
{
	int result = 0;

	CLSID typeClsid;
	result = ImageEnDecoder::GetEncoderClsid(type, &typeClsid);
	if (result == -1 || m_pImage == NULL)
	{
		return -1;
	}

	std::wstring name = std::wstring(fileName.begin(), fileName.end());

	Gdiplus::Status status = m_pImage->Save(name.c_str(), &typeClsid, NULL);
	return status == Gdiplus::Ok;
}

int GraphicsImager::SaveToBuffer(EncoderClsidType type, boost::shared_array<char> &bufPtr, const std::string &strTag)
{
	CLSID typeClsid;

	int result = ImageEnDecoder::GetEncoderClsid(type, &typeClsid);
	if (result == -1)
		return result;

	HGLOBAL  hImage = ::GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hImage == NULL)
	{
		return -1;
	}

	IStream* pStream = NULL;
	if (::CreateStreamOnHGlobal(hImage, FALSE, &pStream) != S_OK)
	{
		::GlobalFree(hImage);
		return -1;
	}

	m_pImage->Save(pStream, &typeClsid, NULL);
	
	int size = ::GlobalSize(hImage);

	int tagLen = strTag.length();

	if (tagLen > 0)
	{
		size += tagLen;
	}

	boost::shared_array<char> imageBufPtr(new char[size]);
	// add tag
	memset(imageBufPtr.get(), 0, size);
	// add tag ahead
	if (tagLen > 0)
	{
		memcpy(imageBufPtr.get(), strTag.c_str(), tagLen);
	}
	
	char *srcBuffer= (char *)::GlobalLock(hImage);

	memcpy((imageBufPtr.get() + tagLen), srcBuffer, size - tagLen);

	::GlobalUnlock(hImage);

	::GlobalFree(hImage);

	pStream->Release();
	bufPtr = imageBufPtr;

	return size;
}

int GraphicsImager::DrawLine(Color color, UINT width, INT x1, INT y1, INT x2, INT y2, bool bSmooth)
{
	if (m_pImage == NULL) return -1;

	Pen  pen(color, width);
	DrawSmoothingLine(&pen, x1, y1, x2, y2, bSmooth);

	return 0;
}
/*
			.p1
		p0
		    . p2
*/
int GraphicsImager::DrawStartCapLine(Color color, UINT width, PointF point1, PointF point2, int lineCapType, bool bEnableSmoothing)
{
	if (m_pImage == NULL) return -1;

	Pen  pen(color, width);
	//pen.SetEndCap(static_cast<Gdiplus::LineCap>(lineCapType));
	pen.SetStartCap(static_cast<Gdiplus::LineCap>(lineCapType));
	DrawSmoothingLine(&pen, point1.X, point1.Y,  point2.X, point2.Y, bEnableSmoothing);
	
}

int GraphicsImager::DrawEllipse(Color color, UINT width, RectF rectF, bool bEnableAntiAlias /* = false */)
{
	int iRet = 0;
	if (bEnableAntiAlias)
	{
		m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
	}
	Pen  pen(color, width);
	iRet = m_pGraphics->DrawEllipse(&pen, rectF);
	m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighSpeed);
	return iRet;
}

int GraphicsImager::DrawArrowLine(Color color, UINT width, INT x1, INT y1, INT x2, INT y2, bool begArrow, bool endArrow, bool bEnableSmoothing)
{
	if (m_pImage == NULL) return -1;

	Pen  pen(color, width);
	AdjustableArrowCap cap(10,5, true);
	//cap.SetWidthScale(2);

	if (begArrow)
		pen.SetCustomStartCap(&cap);
	if (endArrow)
		pen.SetCustomStartCap(&cap);

	DrawSmoothingLine(&pen, x1, y1,  x2, y2, bEnableSmoothing);
}

int GraphicsImager::DrawCapLine(Color color, UINT width, INT x1, INT y1, INT x2, INT y2, int lineCapType, bool bEnableSmoothing)
{
	if (m_pImage == NULL) return -1;

	Pen  pen(color, width);
	pen.SetEndCap(static_cast<Gdiplus::LineCap>(lineCapType));
	pen.SetStartCap(static_cast<Gdiplus::LineCap>(lineCapType));
	DrawSmoothingLine(&pen, x1, y1, x2, y2, bEnableSmoothing);
}

int GraphicsImager::DrawRectangle(Color lineColor, UINT lineWidth, INT x, INT y, INT width, INT height, bool bDash /* = false */, bool bEnableSmoothing /* = false */)
{
	if (m_pImage == NULL) return -1;

	Pen blackPen(lineColor, lineWidth);
	if (bDash)
	{
		REAL dashValues[4] = {2, 2, 2, 2};

		blackPen.SetDashPattern(dashValues, 4);
		blackPen.SetDashStyle(Gdiplus::DashStyleDot);
	}

	DrawSmoothingRect(&blackPen, x, y , width, height, bEnableSmoothing);
}

int GraphicsImager::DrawDashLine(Color color, UINT width, INT x1, INT y1, INT x2, INT y2)
{
	if (m_pImage == NULL) return -1;
	
	REAL dashValues[4] = {2, 2, 2, 2};
	Pen blackPen(color, width);
	blackPen.SetDashPattern(dashValues, 4);
	blackPen.SetDashStyle(Gdiplus::DashStyleDot);
	DrawSmoothingLine(&blackPen, x1, y1, (INT)x2, (INT)y2);
}

int GraphicsImager::DrawString(std::string str, INT x1, INT y1, const std::string &fontName, INT fontSize,
	INT fontType, Color fontColor, bool usedHQS)
{
	if (m_pGraphics == NULL || str.length() <= 0) return -1;

	std::wstring wstrFontName;
	bool b = MBToUnicode(fontName, wstrFontName);
	if (b == false)
		return -1;

	Gdiplus::FontFamily  fontFamily(wstrFontName.c_str());
	Gdiplus::Font        font(&fontFamily, fontSize, fontType, UnitPixel);

	SolidBrush blackBrush(fontColor);

	Gdiplus::PointF txtPointF(x1, y1);
	std::wstring wstr;
	b = MBToUnicode(str,wstr);
	if (b == false)
		return -1;

	if (usedHQS)
	{
		// seems need some cpu resource
		DrawString_HQS enable_anti_alias(m_pGraphics);
		m_pGraphics->DrawString(wstr.c_str(), wstr.length(), &font, txtPointF,&blackBrush);
	}
	else
	{
		m_pGraphics->DrawString(wstr.c_str(), wstr.length(), &font, txtPointF,&blackBrush);
	}
	
	return 0;
}

int GraphicsImager::MeasureString(std::string str, std::string fontName, INT fontSize, INT fontType, Color fontColor, RectF *pRectF)
{
	if (m_pGraphics == NULL || pRectF == NULL || str.length() <= 0) return -1;

	std::wstring wstrFontName;

	bool b = MBToUnicode(fontName, wstrFontName);
	if (b == false)
		return -1;

	Gdiplus::FontFamily  fontFamily(wstrFontName.c_str());
	Gdiplus::Font        font(&fontFamily, fontSize, fontType, UnitPixel);

	std::wstring wstr;
	b = MBToUnicode(str,wstr);
	if (b == false)
		return -1;

	Gdiplus::PointF orgP(0, 0);
	
	m_pGraphics->MeasureString(wstr.c_str(), wstr.length(), &font, orgP, pRectF);
}

int GraphicsImager::DrawSmoothingLine(const Pen *pPen, float x1, float y1, float x2, float y2, bool bEnable)
{
	int iRet = 0;
	if (m_pGraphics)
	{
		if (bEnable)
		{
			m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		}
		
		iRet = m_pGraphics->DrawLine(pPen, x1, y1, x2, y2);
		m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighSpeed);
	}
	return iRet;
}

int GraphicsImager::DrawSmoothingRect(const Pen *pPen, float x1, float y1, float width, float height, bool bEnable)
{
	int iRet = 0;
	if (m_pGraphics)
	{
		if (bEnable)
		{
			m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighQuality);
		}

		iRet = m_pGraphics->DrawRectangle(pPen, x1, y1, width, height);
		m_pGraphics->SetSmoothingMode(SmoothingMode::SmoothingModeHighSpeed);
	}
	return iRet;
}

int GraphicsImager::DrawCircleArc(Color color, UINT lineWidth, PointF center, PointF p1, PointF p2, bool isClockWise)
{
	if (m_pGraphics == NULL) return -1;

	int r = (int)sqrt((p1.X - center.X)*(p1.X - center.X) + (p1.Y - center.Y)*(p1.Y - center.Y));
	int x = center.X - r;
	int y = center.Y - r;

	int width = 2 * r;
	int height = 2 * r;

	float startAngle, endAngle, diffAngle;

	startAngle = (180 / MATH_PI * atan2(p1.Y - center.Y, p1.X - center.X));
	endAngle =  (180 / MATH_PI * atan2(p2.Y - center.Y, p2.X - center.X));

	if (isClockWise)
	{
		diffAngle = endAngle - startAngle;
	}
	else
	{
		diffAngle = 360 -(startAngle - endAngle);
		diffAngle = (int)diffAngle % 360;
	}
	
	Pen  pen(color, lineWidth);
	m_pGraphics->DrawArc(&pen, x, y, width, height, startAngle, diffAngle);
}


int GraphicsImager::DrawMagnifyGlass(const GraphicsImager *pSrcGraphicImager, PointF centerPoint, float radius, float magnifyingRatio)
{
	if (radius <= 0 || magnifyingRatio <= 0 || pSrcGraphicImager == NULL || pSrcGraphicImager->m_pImage == NULL) return 0;

	if ( (2 * radius) > m_pImage->GetWidth() || (2 * radius) > m_pImage->GetHeight())
		return 0;

	
	// clip image area
	RectF clipImageArea(centerPoint.X - radius, centerPoint.Y - radius, 2 * radius, 2 * radius);
	
	RectF magnifyingArea(centerPoint.X - radius * magnifyingRatio, centerPoint.Y - radius * magnifyingRatio,
						2 * radius * magnifyingRatio, 2 * radius * magnifyingRatio);

	TextureBrush TB(pSrcGraphicImager->m_pImage, clipImageArea);

	TB.ScaleTransform(magnifyingRatio, magnifyingRatio, MatrixOrderAppend);
	
	/*translate all of pixel coordinate value in texture */
	TB.TranslateTransform(magnifyingArea.X, magnifyingArea.Y, MatrixOrderAppend);

	TB.SetWrapMode(WrapModeClamp);

	InterpolationMode oldInMode = m_pGraphics->GetInterpolationMode();
	m_pGraphics->SetInterpolationMode(InterpolationModeHighQuality);

	m_pGraphics->FillEllipse(&TB, magnifyingArea);
	
	m_pGraphics->SetInterpolationMode(oldInMode);
	return 1;
}


MCSF_DJ2DENGINE_END_NAMESPACE
