#include "stdafx.h"
#include "image_localizer_line_algo.h"
#include "dicom_component_information.h"
#include "image_localizer_lines_drawing.h"
#include "geometry_math.h"
#include "study_image.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

ImageLocalizerLineAlgo::ImageLocalizerLineAlgo(ImageLocalizerLinesDrawing *pOwner) :
	m_pImageLocalizerLinesDrawing(pOwner)
{
}


ImageLocalizerLineAlgo::~ImageLocalizerLineAlgo(void)
{
}

const DicomImageDescription *ImageLocalizerLineAlgo::GetOwnerImageDesc() const
{
	const DicomImageDescription *pShowRefLineImage = m_pImageLocalizerLinesDrawing->OwnerImage()->GetDicomImageDesc();
	return pShowRefLineImage;
}

bool ImageLocalizerLineAlgo::CalculateReferenceLine(const DicomImageDescription *pReferredImage, 
	PointF *pLinePa, PointF *pLinePb)
{
	vec3 referredVertex[4];
	vec3 referredDirecVect[2];
	vec3 refImageVertex[4];
	vec3 refImageDirecVect[2];

	const DicomImageDescription *pShowRefLineImage = GetOwnerImageDesc();
	if (pShowRefLineImage == NULL || pReferredImage == NULL)
		return false;


	GetRectangleInfo(pReferredImage, referredVertex, referredDirecVect);
	GetRectangleInfo(pShowRefLineImage, refImageVertex, refImageDirecVect);

	double referredPlaneEquParams[4];
	double refPlaneEquParams[4];

	GetPlaneEquation(pReferredImage, referredPlaneEquParams);
	GetPlaneEquation(pShowRefLineImage, refPlaneEquParams);

	vec3 referredPlaneNormal = vec3(referredPlaneEquParams[0], referredPlaneEquParams[1], referredPlaneEquParams[2]);
	vec3 refPlaneNormal = vec3(refPlaneEquParams[0], refPlaneEquParams[1], refPlaneEquParams[2]);

	if (CalAngleBetweenVect(referredPlaneNormal, refPlaneNormal) < 20)
		return false;

	Point3DsV intersectPt3Ds;
	GetIntersectionPoints(referredPlaneEquParams, refImageVertex, refImageDirecVect, intersectPt3Ds);
	GetIntersectionPoints(refPlaneEquParams, referredVertex, referredDirecVect, intersectPt3Ds);

	Point3DsV segmentPt3Ds;
	for (Point3DsVCIt cit = intersectPt3Ds.begin(); cit != intersectPt3Ds.end(); cit ++)
	{
		if (IsInRectangle(referredVertex, *cit) &&
			IsInRectangle(refImageVertex, *cit))
		{
			segmentPt3Ds.push_back(*cit);
		}
	}

	if (segmentPt3Ds.size() != 2)
		return false;

	TransformToLocalPixelPoint(refImageVertex[0], segmentPt3Ds[0], refImageDirecVect, *pLinePa);
	TransformToLocalPixelPoint(refImageVertex[0], segmentPt3Ds[1], refImageDirecVect, *pLinePb);
	return true;
}

bool ImageLocalizerLineAlgo::GetRectangleInfo(const DicomImageDescription *pImageDesc, vec3 *pVertex, vec3 *pDirecVect)
{
	float width = pImageDesc->GetDicomWidth();
	float height= pImageDesc->GetDicomHeight();

	pDirecVect[0] = vec3(pImageDesc->GetImageOrientationMatrixmm()[0].x,
		pImageDesc->GetImageOrientationMatrixmm()[0].y,
		pImageDesc->GetImageOrientationMatrixmm()[0].z);

	pDirecVect[1] = vec3(pImageDesc->GetImageOrientationMatrixmm()[1].x,
		pImageDesc->GetImageOrientationMatrixmm()[1].y,
		pImageDesc->GetImageOrientationMatrixmm()[1].z);

	
	pVertex[0] = pImageDesc->GetImagePositionVec3();
	pVertex[1] = pVertex[0] + (width - 1) * pDirecVect[0];
	pVertex[2] = pVertex[0] + (width - 1) * pDirecVect[0] + (height - 1) * pDirecVect[1];
	pVertex[3] = pVertex[0] + (height - 1) *pDirecVect[1];

	return true;
}

bool ImageLocalizerLineAlgo::GetPlaneEquation(const DicomImageDescription *pImageDesc, double *pPlaneEquParams)
{
	mat4 imageOrg = pImageDesc->GetImageOrientationMatrixmm();
	vec3 imagePos = pImageDesc->GetImagePositionVec3();

	pPlaneEquParams[0] = imageOrg[0].y * imageOrg[1].z - imageOrg[1].y * imageOrg[0].z;
	pPlaneEquParams[1] = imageOrg[0].z * imageOrg[1].x - imageOrg[1].z * imageOrg[0].x;
	pPlaneEquParams[2] = imageOrg[0].x * imageOrg[1].y - imageOrg[1].x * imageOrg[0].y;
	pPlaneEquParams[3] = -1 *(imagePos.x * pPlaneEquParams[0] + imagePos.y * pPlaneEquParams[1] + imagePos.z * pPlaneEquParams[2]);

	return true;
}

bool ImageLocalizerLineAlgo::GetIntersectionPoints(double *pPlane1EquParams, vec3 *pPlan2Vertex, vec3 *pPlan2DirecVect, Point3DsV &point3Ds)
{
	// the plane2 not perpendicular plan1 
	vec3 plane2xDirecVect = pPlan2DirecVect[0];
	vec3 plane2yDirecVect = pPlan2DirecVect[1];
	if (fabs(plane2xDirecVect.x * pPlane1EquParams[0] + plane2xDirecVect.y * pPlane1EquParams[1] +
		     plane2xDirecVect.z * pPlane1EquParams[2]) > 1e-6)
	{
		GetIntersectionPoints(pPlane1EquParams, pPlan2Vertex[0], plane2xDirecVect, point3Ds);
		GetIntersectionPoints(pPlane1EquParams, pPlan2Vertex[2], plane2xDirecVect, point3Ds);
	}

	if (fabs(plane2yDirecVect.x * pPlane1EquParams[0] + plane2yDirecVect.y * pPlane1EquParams[1] +
		plane2yDirecVect.z * pPlane1EquParams[2]) > 1e-6)
	{
		GetIntersectionPoints(pPlane1EquParams, pPlan2Vertex[0], plane2yDirecVect, point3Ds);
		GetIntersectionPoints(pPlane1EquParams, pPlan2Vertex[2], plane2yDirecVect, point3Ds);
	}

	return true;
}

bool ImageLocalizerLineAlgo::GetIntersectionPoints(double *pPlane1EquParams, vec3 plan2Vertex, vec3 plan2DirecVect, Point3DsV &point3Ds)
{
	double temp = (-1 * pPlane1EquParams[3] - pPlane1EquParams[0] * plan2Vertex.x - pPlane1EquParams[1] * plan2Vertex.y - pPlane1EquParams[2] * plan2Vertex.z) /
				(pPlane1EquParams[0] * plan2DirecVect.x + pPlane1EquParams[1] * plan2DirecVect.y + pPlane1EquParams[2] * plan2DirecVect.z);

	vec3 pt3D = vec3(plan2Vertex.x + plan2DirecVect.x * temp,
					plan2Vertex.y + plan2DirecVect.y * temp,
					plan2Vertex.z + plan2DirecVect.z * temp);

	for (Point3DsVCIt cit = point3Ds.begin(); cit != point3Ds.end(); cit ++)
	{
		const vec3 &interPt3D = *cit;

		if ((fabs(interPt3D.x - pt3D.x) < 1e-6) &&
			(fabs(interPt3D.y - pt3D.y) < 1e-6) &&
			(fabs(interPt3D.z - pt3D.z) < 1e-6))
		{
			return true;
		}
	}

	point3Ds.push_back(pt3D);

	return true;
}

bool ImageLocalizerLineAlgo::IsInRectangle(vec3 *pRectVerts, vec3 point)
{
	if (pRectVerts[0] == point || pRectVerts[1] == point || pRectVerts[2] == point ||
		pRectVerts[3] == point)
		return true;

	vec3 vect_leftTop = pRectVerts[0] - point;
	vec3 vect_rightTop = pRectVerts[1] - point;
	vec3 vect_rightBottom = pRectVerts[2] - point;
	vec3 vect_leftBottom = pRectVerts[3] - point;

	double angle = CalAngleBetweenVect(vect_leftTop, vect_rightTop);
	angle += CalAngleBetweenVect(vect_rightTop, vect_rightBottom);
	angle += CalAngleBetweenVect(vect_rightBottom, vect_leftBottom);
	angle += CalAngleBetweenVect(vect_leftTop, vect_leftBottom);

	return (fabs(angle - 360) < 1e-3);
}

void ImageLocalizerLineAlgo::TransformToLocalPixelPoint(vec3 dcmPt3D, vec3 inPt3D, vec3 *pDcmDirecVect, PointF &pixelPoint)
{
	vec3 vec = inPt3D - dcmPt3D;
	vec3 vecXAxis = pDcmDirecVect[0];
	vec3 vecYAxis = pDcmDirecVect[1];
	
	vec3 normVecXAxis = glm::normalize(vecXAxis);
	vec3 normVecYAxis = glm::normalize(vecYAxis);

	float fx = glm::dot(normVecXAxis, vec);
	float fy = glm::dot(normVecYAxis, vec);

	fx /= GetOwnerImageDesc()->PixelSpaceX();
	fy /= GetOwnerImageDesc()->PixelSpaceY();
	pixelPoint.X = fx;
	pixelPoint.Y = fy;
}

const DicomImageDescription * ImageLocalizerLineAlgo::FindReferredImageByPosition(const DicomImageDescription *pReferredImage, 
	const PointF &mousePos)
{
	return NULL;
}

MCSF_DJ2DENGINE_END_NAMESPACE