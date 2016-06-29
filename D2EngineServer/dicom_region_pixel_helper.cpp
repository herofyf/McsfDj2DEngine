#include "stdafx.h"
#include "dicom_region_pixel_helper.h"
#include "system_global_configuration.h"
#include "mcsf_dj2dengine_utility.h"
#include "geometry_math.h"
#include "dicom_component_information.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "study_image.h"
#include "dicom_polygon_pixels_statistics.h"


MCSF_DJ2DENGINE_BEGIN_NAMESPACE

std::string DicomRegionPixelsStatistics::StrMax() const
{
	std::stringstream ss;
	ss << "Max: " << ToUnitString(Max);
	return ss.str();
}

std::string DicomRegionPixelsStatistics::StrMin() const
{
	std::stringstream ss;
	ss << "Min: " << ToUnitString(Min);
	return ss.str();
}

std::string DicomRegionPixelsStatistics::StrMeans() const
{
	std::stringstream ss;
	ss << "Means: " << ToUnitString(Means);
	return ss.str();
}

std::string DicomRegionPixelsStatistics::StrSD() const
{
	std::stringstream ss;
	ss << "SD: " << ToUnitString(StandandDeviation);
	return ss.str();
}

std::string DicomRegionPixelsStatistics::StrCount() const
{
	std::stringstream ss;
	ss << "Pixels: " << ToString(Count);
	return ss.str();
}

std::string DicomRegionPixelsStatistics::StrArea() const
{
	std::stringstream ss;
	ss << "Area: " << Area.toString();
	return ss.str();
}

RegionPickedPixels::RegionPickedPixels(DicomImageHelper *pDicomImageHelper) :
	m_pDicomImageHelper(pDicomImageHelper)
{
	if (m_pDicomImageHelper)
	{
		m_pStudyImage = m_pDicomImageHelper->m_pStudyImage;
	}

	m_pRegionPixelsStatistics = new DicomRegionPixelsStatistics();
}


RegionPickedPixels::~RegionPickedPixels(void)
{
	DEL_PTR(m_pRegionPixelsStatistics);
}

const DicomRegionPixelsStatistics *RegionPickedPixels::GetRegionPixelsStatistics()
{
	bool b = false;

	try
	{
		b = CalRegionPixelsStatistics();
	}
	catch(...) {}

	if (b == false)
		m_pRegionPixelsStatistics->Dummy();

	return m_pRegionPixelsStatistics;
}

bool RegionPickedPixels::IsPointWithin(const Point &p)
{
	return false;
}

bool RegionPickedPixels::GetRegionOutterRect(Rect &regionRect)
{
	return false;
}

void RegionPickedPixels::Clear()
{
	if (m_pRegionPixelsStatistics)
		m_pRegionPixelsStatistics->Clear();
}

long RegionPickedPixels::GetPixelData(const EP_Representation &rep, int planes, const void *pixelData, int rowI, int colI, int clipWidth)
{
	if (planes == 1)
	{
		return GetPlaneData(rep, pixelData, rowI, colI, clipWidth);
	}
	else if (planes == 3)
	{
		int r = 0, g = 0, b = 0, ret = 0;
		for (int i = 0; i < 3; i ++)
		{
			void *pPlane = ((void **)pixelData)[i];
			ret = GetPlaneData(rep, pPlane, rowI, colI, clipWidth);
			if (i == 0)
				r = ret;
			else if (i == 1)
				g = ret;
			else if (i == 2)
				b = ret;
		}
		
		return RGB_B(r, g, b);
	}

	return 0;
}


long RegionPickedPixels::GetPlaneData(const EP_Representation &rep, const void *pixelData, int rowI, int colI, int clipImageWidth)
{
	long lVal = 0;

	if (pixelData == NULL) return lVal;

	switch(rep)
	{
	case EPR_Sint32:
		{
			lVal = *((Sint32*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	case EPR_Uint32:
		{
			lVal = *((Uint32*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	case EPR_Sint16:
		{
			lVal = *((Sint16*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	case EPR_Uint16:
		{
			lVal = *((Uint16*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	case EPR_Uint8:
		{
			lVal = *((Uint8*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	default:
		{
			lVal = *((Sint8*)pixelData + rowI * clipImageWidth + colI);
		}
		break;
	}

	return lVal;

}
bool RegionPickedPixels::CalRegionPixelsStatistics()
{
	if (m_pDicomImageHelper == NULL) return false;

	Rect outterRect;

	bool b1 = GetRegionOutterRect(outterRect);
	if (b1 == false) return false;

	Rect imageRect(0, 0, m_pDicomImageHelper->GetDicomWidth(), m_pDicomImageHelper->GetDicomHeight());
	const DicomImage *pCurFrameImage = m_pDicomImageHelper->GetCurFrameImage();
	if (imageRect.Contains(outterRect) == false || pCurFrameImage == NULL) return false;
	
	DicomImage *pOutterRegionDicomImage = pCurFrameImage->createClippedImage(outterRect.X, outterRect.Y, 
							outterRect.Width,
							outterRect.Height);

	boost::shared_ptr<DicomImage> clipDicomImagePtr(pOutterRegionDicomImage);
	int clipImageWidth = clipDicomImagePtr->getWidth();
	int clipImageHeight = clipDicomImagePtr->getHeight();
	
	const DiPixel *dmp = NULL;
	dmp = clipDicomImagePtr->getInterData();
	void *pixelData = NULL;
	if (dmp == NULL) return false;

	pixelData = (void *)dmp->getData();
	if (pixelData == NULL) return false;

	EP_Representation rep = dmp->getRepresentation();
	int lTotal = dmp->getCount();
	int lPlanes = dmp->getPlanes();
	Point point;
	
	if (lTotal != clipImageWidth * clipImageHeight || lTotal == 0 || ((lPlanes != 1) && (lPlanes != 3)) ) 
		return false;

	long lVal = 0; 
	double dMin = 1e9, dMax = -1e9, dSum = 0, dPow2Sum =0;
	
	for (int row = 0; row < clipImageHeight; row ++)
	{
		for (int col = 0; col < clipImageWidth; col ++)
		{
			point.X = outterRect.X + col;
			point.Y = outterRect.Y + row;
			if (IsPointWithin(point))
			{
				lVal = GetPixelData(rep, lPlanes, pixelData, row, col, clipImageWidth);
				
				if (lVal > dMax)
				{
					dMax = lVal;
				}

				if (lVal < dMin)
				{
					dMin = lVal;
				}

				dSum += lVal;
				dPow2Sum += lVal * lVal;
			}
		}
	}
	
	int nMeans = static_cast<int>(dSum / lTotal);
	int SD = sqrt(dPow2Sum / lTotal - nMeans * nMeans);
		
	UpdateStatisticsResult(lTotal, (int)dMax, (int)dMin, nMeans, SD);
	
	return true;
}

void RegionPickedPixels::UpdateStatisticsResult(int nTotal, int nMax, int nMin, int nMeans, double SD)
{
	if (m_pRegionPixelsStatistics)
	{
		m_pRegionPixelsStatistics->Count = nTotal;
		m_pRegionPixelsStatistics->Max = nMax;
		m_pRegionPixelsStatistics->Min = nMin;
		m_pRegionPixelsStatistics->Means = nMeans;
		m_pRegionPixelsStatistics->StandandDeviation = SD;

		m_pRegionPixelsStatistics->Area = LOGIC_U_OBJ(m_pStudyImage)->CalRawDicomPixelsLogicArea(nTotal);
	}
}
DicomPickedRectPixels::DicomPickedRectPixels(DicomImageHelper *pDicomImageHelper) :
	RegionPickedPixels(pDicomImageHelper)
{

}

DicomPickedRectPixels::~DicomPickedRectPixels()
{

}


bool DicomPickedRectPixels::GetRegionOutterRect(Rect &regionRect)
{
	regionRect = m_rect;

	return true;
}

bool DicomPickedRectPixels::IsPointWithin(const Point &p)
{
	return m_rect.Contains(p);
}


void DicomPickedRectPixels::SelectRegion(Rect rect)
{
	m_rect = rect;
}

bool DicomPickedCirclePixels::GetRegionOutterRect(Rect &regionRect)
{
	regionRect.X = (int)(m_centerPf.X - m_fRadius);
	regionRect.Y = (int)(m_centerPf.Y - m_fRadius);

	regionRect.Width  = (int) (2 * m_fRadius);
	regionRect.Height = (int) (2 * m_fRadius);

	return true;
}

DicomPickedCirclePixels::DicomPickedCirclePixels(DicomImageHelper *pDicomImageHelper):
	RegionPickedPixels(pDicomImageHelper)
{

}

// to check it is in this circle
bool DicomPickedCirclePixels::IsPointWithin(const Point &p)
{
	bool b = IsPointWithinCirlcle((int)m_centerPf.X, (int)m_centerPf.Y, m_fRadius, p.X, p.Y);
	return b;
}

void DicomPickedCirclePixels::SelectRegion(PointF centerPf, float fRadius)
{
	m_centerPf = centerPf;
	m_fRadius = fRadius;
	
}

DicomPickedCirclePixels::~DicomPickedCirclePixels()
{

}

ScanlinePolygonStatistics::ScanlinePolygonStatistics(DicomImageHelper *pDicomImageHelper) :
	RegionPickedPixels(pDicomImageHelper)
{
	m_bExceedBound = false;
}

ScanlinePolygonStatistics::~ScanlinePolygonStatistics()
{

}

void ScanlinePolygonStatistics::Reset()
{
	m_pDicomPixelData = NULL;
	m_iStride = 0;
	m_lTotal = 0;
	m_dMin = m_dMax = m_dSum = m_dPow2Sum = 0;
	m_pMask = NULL;
	m_pointsArray.pt.clear();
	m_iTopY = m_iHeight = m_iScanHeight = 0;
}
void ScanlinePolygonStatistics::SelectFreeHandRegion(PointF *pPointFs, int nCount)
{
	if (pPointFs == NULL || nCount == 0) return;

	Reset();
	int minx = 1e9, maxx =-1e9, miny = 1e9, maxy = -1e9;
	IntPoint intPoint;
	for(int i = 0; i < nCount; i ++)
	{
		intPoint.x = (int)pPointFs[i].X;
		intPoint.y = (int)pPointFs[i].Y;
		m_pointsArray.pt.push_back(intPoint);

		if (intPoint.x < minx)
		{
			minx = intPoint.x;
		}

		if (intPoint.y < miny)
		{
			miny = intPoint.y;
		}

		if (intPoint.x > maxx)
		{
			maxx = intPoint.x;
		}

		if (intPoint.y > maxy)
		{
			maxy = intPoint.y;
		}
	}

	m_iTopY = miny;
	m_iScanHeight = maxy - miny;
}

void ScanlinePolygonStatistics::CollectPixelStatistics(int x1, int x2, int y)
{
	long lVal = 0;
	for (int x = x1; x < x2; x++)
	{
		lVal = GetPixelData(m_dicomPixelRep, m_lPanes, m_pDicomPixelData, y, x, m_iStride);

		if (lVal < m_dMin)
		{
			m_dMin = lVal;
		}

		if (lVal > m_dMax)
		{
			m_dMax = lVal;
		}

		m_lTotal ++;

		m_dSum += lVal;
		m_dPow2Sum += lVal * lVal;
	}
	
}

bool ScanlinePolygonStatistics::CalRegionPixelsStatistics()
{
	if (m_pDicomImageHelper == NULL) 
	{
		return false;
	}

	int nDicomHeight = m_pDicomImageHelper->GetDicomHeight();
	// might out of dicom region
	if (m_iTopY < 0 || m_iScanHeight <= 0 || (m_iTopY + m_iScanHeight) > nDicomHeight)
	{
		return false;
	}

	try
	{
		Rect imageRect(0, 0, m_pDicomImageHelper->GetDicomWidth(),  nDicomHeight);
		const DicomImage *pCurFrameImage = m_pDicomImageHelper->GetCurFrameImage();
		if (pCurFrameImage == NULL) return false;

		const DiPixel *dmp = NULL;
		dmp = pCurFrameImage->getInterData();
		void *pixelData = NULL;
		if (dmp == NULL) return false;

		pixelData = (void *)dmp->getData();
		if (pixelData == NULL) return false;

		EP_Representation rep = dmp->getRepresentation();
		int lTotal = dmp->getCount();

		m_pDicomPixelData = pixelData;
		m_dicomPixelRep = rep;
		m_lPanes = dmp->getPlanes();
		Scan(m_pointsArray, NULL, imageRect.Width, imageRect.Height, m_iTopY, m_iScanHeight);

		int nMeans = static_cast<int>(m_dSum / m_lTotal);
		int SD = sqrt(m_dPow2Sum / m_lTotal - nMeans * nMeans);

		UpdateStatisticsResult(m_lTotal, (int)m_dMax, (int)m_dMin, nMeans, SD);
	}
	catch(...){}
}
MCSF_DJ2DENGINE_END_NAMESPACE