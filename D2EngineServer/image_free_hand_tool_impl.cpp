#include "stdafx.h"
#include "image_free_hand_tool_impl.h"
#include <math.h>
#include <limits>
#include <algorithm>
#include <xutility>
#include "geometry_math.h"
#include "image_free_hand_tool.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageFreeHandPathImpl::ImageFreeHandPathImpl(ImageFreeHandTool *pParent) :
	m_pParent(pParent)
{
	fTolerance = 15;
}

ImageFreeHandPathImpl::~ImageFreeHandPathImpl()
{
}

const POINTFS_V *ImageFreeHandPathImpl::GetWalkPathPoints()
{
	return &m_walkPathPoints;
}

const POINTFS_V *ImageFreeHandPathImpl::GetEditPathPoints()
{
	return &m_editPathPoints;
}

void ImageFreeHandPathImpl::BeginWalkPath()
{
	Reset();
}


bool ImageFreeHandPathImpl::IsEqualLastPoint(const POINTFS_V &ps, const PointF &p)
{
	int size = ps.size();

	if (size > 0)
	{
		PointF last = ps[size - 1];
		if (last.Equals(p))
			return true;
	}
	return false;
}

void ImageFreeHandPathImpl::AddWalkPathPoint(const PointF &p)
{
	bool b = IsEqualLastPoint(m_walkPathPoints, p);
	if (b) return;

	m_walkPathPoints.push_back(p);
}

void ImageFreeHandPathImpl::ChangeWalkPathLastPoint(const PointF &p)
{
	int count = m_walkPathPoints.size();
	if (count > 0)
	{
		// before change last one, need check previous of last
		if (count > 1)
		{
			PointF prev_last = m_walkPathPoints[count - 2];
			if (prev_last.Equals(p))
				return;
		}

		m_walkPathPoints[count -1] = p;
	}
	
}

void ImageFreeHandPathImpl::EndWalkPath()
{
	int size = m_walkPathPoints.size();
	if (size > 1)
	{
		PointF first = m_walkPathPoints[0];
		PointF last = m_walkPathPoints[size - 1];
		if (first.Equals(first))
		{
			m_walkPathPoints.push_back(first);
		}

		OnWalkPathChanged();
	}
	else
	{
		m_walkPathPoints.clear();
	}
}

bool ImageFreeHandPathImpl::IsWalkPathValid()
{
	return (m_walkPathPoints.size() > 3);
}

void ImageFreeHandPathImpl::BeginEditPath()
{
	m_editPathPoints.clear();
}

void ImageFreeHandPathImpl::AddEditPath(const PointF &p)
{
	bool b = IsEqualLastPoint(m_editPathPoints, p);
	if (b) return;

	m_editPathPoints.push_back(p);
}

void ImageFreeHandPathImpl::EndEditPath()
{
	POINTFS_V result_points;
	
	bool b = Join(result_points);
	if (b)
	{
		m_walkPathPoints.clear();
		m_walkPathPoints.insert(m_walkPathPoints.end(), result_points.begin(), result_points.end());

		OnWalkPathChanged();

		m_editPathPoints.clear();
	}
}

bool ImageFreeHandPathImpl::Join(POINTFS_V &result)
{
	int nEditPathSize = m_editPathPoints.size();
	if (nEditPathSize <= 0) 
		return false;

	POINTFS_V *pEditPath = &m_editPathPoints;
	POINTFS_V *pWalkPath = &m_walkPathPoints;
	POINTFS_V reverseEditPath(m_editPathPoints);

	const PointF &firstEditPathP = pEditPath->at(0);
	const PointF &lastEditPathP = pEditPath->at(nEditPathSize - 1);
	int firstNearPointIndex = FindNearestPoint(m_walkPathPoints, firstEditPathP);
	if (firstNearPointIndex < 0) 
		return false;

	int lastNearPointIndex = FindNearestPoint(m_walkPathPoints, lastEditPathP);
	if (lastNearPointIndex < 0) 
		return false;

	int nWalkPathSize = pWalkPath->size();

	result.clear();

	if (firstNearPointIndex == lastNearPointIndex)
	{
		float len1 = CalLineLen(firstEditPathP, pWalkPath->at(firstNearPointIndex));
		float len2 = CalLineLen(lastEditPathP, pWalkPath->at(firstNearPointIndex));
		if (len1 < 5 && len2 < 5)
			return false;

		if (len1 > len2)
		{
			std::reverse(reverseEditPath.begin(), reverseEditPath.end());
			pEditPath = &reverseEditPath;
		}

		for (int i = 0; i <= firstNearPointIndex; ++i)
			result.push_back(m_walkPathPoints[i]);
		
		result.insert(result.end(), pEditPath->begin(), pEditPath->end());

		for (int i = lastNearPointIndex + 1; i < nWalkPathSize; ++i)
			result.push_back(m_walkPathPoints[i]);

		return true;
	}

	if (firstNearPointIndex > lastNearPointIndex)
	{
		std::swap(firstNearPointIndex, lastNearPointIndex);

		std::reverse(reverseEditPath.begin(), reverseEditPath.end());
		pEditPath = &reverseEditPath;
	}

	float startSegmentLen = 0, endSegmentLen = 0;
	if (firstNearPointIndex == 0 && lastNearPointIndex == (nWalkPathSize -1))
	{
		startSegmentLen += CalLineLen(pEditPath->at(0), pWalkPath->at(1));
		for (int i = 2; i < nWalkPathSize; i++)
		{
			startSegmentLen += CalLineLen(pEditPath->at(i), pWalkPath->at(i -1));
		}

		startSegmentLen += CalLineLen(pWalkPath->at(nWalkPathSize -1), pEditPath->at(nEditPathSize -1));

		endSegmentLen += CalLineLen(pWalkPath->at(0), pEditPath->at(0));
		endSegmentLen += CalLineLen(pWalkPath->at(0), pEditPath->at(nEditPathSize -1));
	}
	else
	{
		for (int i = 1; i <= firstNearPointIndex; i ++)
		{
			startSegmentLen += CalLineLen(pWalkPath->at(i), pWalkPath->at(i-1));
		}

		startSegmentLen += CalLineLen(pEditPath->at(0), pWalkPath->at(firstNearPointIndex));
		int index1 = (lastNearPointIndex + 1) % nWalkPathSize;
		startSegmentLen += CalLineLen(pWalkPath->at(index1), pEditPath->at(nEditPathSize -1));

		for (int i = lastNearPointIndex + 2; i <= nWalkPathSize; i ++)
		{
			if (i != nWalkPathSize)
			{
				startSegmentLen += CalLineLen(pWalkPath->at(i), pWalkPath->at(i - 1));
			}
			else
			{
				startSegmentLen += CalLineLen(pWalkPath->at(i-1), pWalkPath->at(0));
			}
		}

		endSegmentLen += CalLineLen(pWalkPath->at(firstNearPointIndex + 1), pEditPath->at(0));
		for (int i = firstNearPointIndex + 2; i <= lastNearPointIndex; i ++)
		{
			endSegmentLen += CalLineLen(pWalkPath->at(i), pWalkPath->at(i -1));
		}
		endSegmentLen += CalLineLen(pWalkPath->at(lastNearPointIndex), pEditPath->at(nEditPathSize -1));
	}

	if (startSegmentLen >= endSegmentLen)
	{
		if (firstNearPointIndex == 0 && lastNearPointIndex == (nWalkPathSize - 1))
		{
			std::reverse(reverseEditPath.begin(), reverseEditPath.end());
			pEditPath = &reverseEditPath;

			for (int i = 1; i <= lastNearPointIndex; i ++)
			{
				result.push_back(pWalkPath->at(i));
			}

			result.insert(result.end(), pEditPath->begin(), pEditPath->end());
		}
		else
		{
			for (int i = 0; i <= firstNearPointIndex; ++ i)
			{
				result.push_back(pWalkPath->at(i));
			}

			result.insert(result.end(), pEditPath->begin(), pEditPath->end());

			for (int i = lastNearPointIndex + 1; i < nWalkPathSize; ++ i)
			{
				result.push_back(pWalkPath->at(i));
			}
		}
	}
	else
	{
		for (int i = firstNearPointIndex; i < lastNearPointIndex; ++ i)
			result.push_back(pWalkPath->at(i));

		for (int i = nEditPathSize - 1; i >= 0; -- i)
			result.push_back(pEditPath->at(i));
	}

	return true;
}

void ImageFreeHandPathImpl::Reset()
{
	m_walkPathPoints.clear();
	m_editPathPoints.clear();
}

bool ImageFreeHandPathImpl::IsHit(const PointF &p)
{
	int i = FindNearestPoint(m_walkPathPoints, p);
	return (i >= 0);
}


int ImageFreeHandPathImpl::FindNearestPoint(const POINTFS_V &ps, const PointF &point)
{
	int nCount = ps.size();

	if (nCount == 0) return -1;

	int nearestPointIndex = -1;
	float minLen = INT_MAX, dis = 0, length = 0, minX, maxX, minY, maxY, x, y;
	int nextI = 0;
	for (int i = 0; i < nCount; ++i)
	{
		nextI = (i + 1) % nCount;
		const PointF &pi = ps[i];
		const PointF &pnxt = ps[nextI];
		
		minX = fmin(pi.X, pnxt.X);
		maxX = fmax(pi.X, pnxt.X);
		minY = fmin(pi.Y, pnxt.Y);
		maxY = fmax(pi.Y, pnxt.Y);
		
		// Step1: point is between in the line
		if ((point.X <= maxX && point.X >= minX && point.Y <= maxY && point.Y >= minY))
		{
			dis = GetMinDistance(pi, pnxt, point);
			if (minLen > dis)
			{
				nearestPointIndex = i;
				minLen = dis;
			}
		}
		else
		{
			length = CalLineLen(pi, point); 

			if (length < fTolerance)
			{
				if (minLen > length)
				{
					nearestPointIndex = i;
					minLen = length;
				}
			}
		}

	}
	return nearestPointIndex;
}

float ImageFreeHandPathImpl::GetMinDistance(const PointF &pt1, const PointF &pt2, const PointF &pt3)
{
	float dis = 0;
	if (pt1.X == pt2.X)
	{
		dis = fabsf(pt3.X - pt1.X);
		return dis;
	}

	float lineK = (pt2.Y - pt1.Y) / (pt2.X - pt1.X);
	float lineC = (pt2.X * pt1.Y - pt1.X * pt2.Y) / (pt2.X - pt1.X);
	dis = fabsf(lineK * pt3.X - pt3.Y + lineC) / sqrtf(lineK * lineK + 1);
	return dis;
}

const PointF *ImageFreeHandPathImpl::GetWalkPathIndexPointF(int index)
{
	if (index < 0 || index >= m_walkPathPoints.size())
		return NULL;

	return &(m_walkPathPoints[index]);
}

PointF *ImageFreeHandPathImpl::CloneWalkPathPoints(int &count)
{
	int nWalkPathSize = m_walkPathPoints.size();
	if (nWalkPathSize <= 0) 
		return NULL;

	PointF *pPointFs = new PointF[nWalkPathSize];
	int nIndex = 0;
	for (POINTFS_V_CIT cit = m_walkPathPoints.begin(); cit != m_walkPathPoints.end(); cit ++)
	{
		pPointFs[nIndex].X = cit->X;
		pPointFs[nIndex].Y = cit->Y;
		nIndex ++;
	}
	count = nWalkPathSize;

	return pPointFs;
}

bool ImageFreeHandPathImpl::onTransformation(const TransformationArgs *curTransformationArgs)
{
	int count = 0, nIndex = 0;

	PointF *pPointFs = CloneWalkPathPoints(count);
	if (pPointFs == NULL) 
		return false;

	bool b1 = m_pParent->TransformPointsByCellCoord(curTransformationArgs, pPointFs, count);
	if (b1)
	{
		nIndex = 0;
		for (POINTFS_V_IT it = m_walkPathPoints.begin(); it != m_walkPathPoints.end(); it ++)
		{
			it->X = pPointFs[nIndex].X;
			it->Y = pPointFs[nIndex].Y;
			nIndex ++;
		}
	}

	DEL_PTR_ARRAY(pPointFs);

	return true;
}

const PointF *ImageFreeHandPathImpl::GetNearestDistPoint(const PointF &p)
{
	int nCount = m_walkPathPoints.size();
	float fMin = 1e9, fVal = 0;
	PointF *pTempPointF = NULL, *pMinPointF = NULL;
	
	for(int i = 0; i < nCount; i ++)
	{
		pTempPointF = &(m_walkPathPoints[i]);

		if (pTempPointF == NULL) continue;;

		fVal = CalPointPairRelative(p.X, p.Y, pTempPointF->X, pTempPointF->Y);
		if (fVal < fMin)
		{
			pMinPointF = pTempPointF;
			fMin = fVal;
		}
	}

	return pMinPointF;
}


void ImageFreeHandPathImpl::OnWalkPathChanged()
{
	
}


bool ImageFreeHandPathImpl::IsWithin(const PointF &pointf)
{
	BGL::ringf regionRingf;
	BGL::pointf_xy bglPoint;
	for (POINTFS_V_CIT cit = m_walkPathPoints.begin(); cit != m_walkPathPoints.end(); cit ++)
	{
		bglPoint.x(cit->X);
		bglPoint.y(cit->Y);
		regionRingf.push_back(bglPoint);
	}
	
	bglPoint.x(pointf.X);
	bglPoint.y(pointf.Y);
	return boost::geometry::within(bglPoint, regionRingf);
}

MCSF_DJ2DENGINE_END_NAMESPACE