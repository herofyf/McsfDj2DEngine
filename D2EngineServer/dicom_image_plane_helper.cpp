#include "stdafx.h"
#include "dicom_image_plane_helper.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

DicomImagePlaneHelper::DicomImagePlaneHelper()
{

}

DicomImagePlaneHelper::DicomImagePlaneHelper(const glm::mat2x3 &imageOriMatrix,
	const glm::vec3 &imagePosVec3, float pixelSpX, float pixelSpY,
	int nDicomWidth, int nDicomHeight)
{
	m_imageOriMatrix = imageOriMatrix;
	m_imagePosVec3   = imagePosVec3;
	m_fPixelSpX      = pixelSpX;
	m_fPixelSpY		 = pixelSpY;
	m_nDicomWidth	 = nDicomWidth;
	m_nDicomHeight	 = nDicomHeight;

	CalParams();
}

void DicomImagePlaneHelper::CalParams()
{
	CalImageNormalPatient();
	CalImageToPatientTransform();

	CalRotationMatrix();

	m_imageTopLeftPatient		= m_imagePosVec3;
	m_imageTopRightPatient		= ConvertToPatient(PointF(m_nDicomWidth, 0));
	m_imageBottomLeftPatient	= ConvertToPatient(PointF(0, m_nDicomHeight));
	m_imageBottomRightPatient	= ConvertToPatient(PointF(m_nDicomWidth, m_nDicomHeight));
	m_imageCenterPatient		= ConvertToPatient(PointF(m_nDicomWidth / 2, m_nDicomHeight / 2));
}

void DicomImagePlaneHelper::CalImageNormalPatient()
{
	if (m_imageOriMatrix == EMPTY_MAT2X3())
	{
		m_imageNormalPatient = EMPTY_VEC3();
		return;
	}

	vec3 normal_res =glm::cross(GetImageRowOrientationPatientVec3(), GetImageColumnOrientationPatientVec3());
	if (glm::length(normal_res) == 0)
	{
		m_imageNormalPatient =  EMPTY_VEC3();
	}

	m_imageNormalPatient =  glm::normalize(normal_res);
}

// to patient coordinate system
void DicomImagePlaneHelper::CalImageToPatientTransform()
{
	m_pixelToPatientTransform = EMPTY_MAT4();

	m_pixelToPatientTransform[0] = glm::vec4(
		m_imageOriMatrix[0].x * m_fPixelSpX, 
		m_imageOriMatrix[0].y * m_fPixelSpX, 
		m_imageOriMatrix[0].z * m_fPixelSpX,
		0);

	m_pixelToPatientTransform[1] = glm::vec4(
		m_imageOriMatrix[1].x * m_fPixelSpY, 
		m_imageOriMatrix[1].y * m_fPixelSpY, 
		m_imageOriMatrix[1].z * m_fPixelSpY,
		0);

	m_pixelToPatientTransform[3] = glm::vec4(m_imagePosVec3, 1);

	m_pixelToPatientTransform = glm::transpose(m_pixelToPatientTransform);
}

void DicomImagePlaneHelper::CalRotationMatrix()
{
	if (m_imageOriMatrix == EMPTY_MAT2X3())
	{
		m_rotationMat3 = EMPTY_MAT3();
		return;
	}

	vec3 normal = GetImageNormalPatient();
	if (normal == EMPTY_VEC3())
	{
		m_rotationMat3 = EMPTY_MAT3();
		return;
	}

	m_rotationMat3[0] = m_imageOriMatrix[0];
	m_rotationMat3[1] = m_imageOriMatrix[1];
	m_rotationMat3[2] = normal;
}

DicomImagePlaneHelper::~DicomImagePlaneHelper(void)
{
}

vec3 DicomImagePlaneHelper::ConvertToPatient(PointF positionPixel) const
{
	vec3 result = EMPTY_VEC3();

	if (m_imageOriMatrix == EMPTY_MAT2X3() || m_fPixelSpX == 0 || m_fPixelSpY == 0)
		return result;

	if (GdiPlusTypeOperator::IsPointFEmpty(positionPixel))
		return GetImagePositionPatientVec3();

	mat4x4 imageToPatientTransform = GetImageToPatientTransform();
	vec4 columnVec4 = glm::vec4(positionPixel.X, positionPixel.Y, 0, 1);
	//vec4 columnVec4 = glm::vec4(304.6916f, 118.5043, 0, 1);
	vec4 transResult = columnVec4 * imageToPatientTransform;

	result = glm::vec3(transResult.x, transResult.y, transResult.z);
	return result;
}


vec3 DicomImagePlaneHelper::ConvertToImagePlane(vec3 positionPatient) const
{
	vec3 translated = positionPatient;

	if (positionPatient != EMPTY_VEC3())
		translated = translated - m_imagePosVec3;

	mat3 rotationMatrix = GetRotationMatrix();
	if (rotationMatrix == EMPTY_MAT3())
		return EMPTY_VEC3();

	return rotationMatrix * translated;
}
MCSF_DJ2DENGINE_END_NAMESPACE