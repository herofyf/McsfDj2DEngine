#include "stdafx.h"
#include "dcmtk/dcmdata/dctk.h" 
#include "dicom_component_information.h"
#include "image_comments_drawer.h"
#include "dicom_image_helper.h"
#include "mcsf_dj2dengine_utility.h"
#include "study_series.h"
#include "dicom_image_plane_helper.h"
#include <sstream>

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

const DcmTagKey *DicomImageAttrib::AttribDcmTagKeys(const std::string &strModality, int &count)
{
	if (strModality.length() > 0 && stricmp(strModality.c_str(), "XA") == 0)
	{
		static DcmTagKey forXA[] = {
			DCM_NumberOfFrames,
			DCM_Rows,
			DCM_Columns,
			DCM_PixelRepresentation,
			DCM_RescaleSlope,
			DCM_RescaleIntercept,
			DCM_WindowCenter,
			DCM_WindowWidth,
			DCM_ImagerPixelSpacing,
			DCM_ImagePositionPatient,
			DCM_ImageOrientationPatient,
			DCM_StudyInstanceUID,
			DCM_PatientPosition,
			DCM_FrameOfReferenceUID,
			DCM_SeriesInstanceUID,
			DCM_SliceThickness,
			DCM_SpacingBetweenSlices
		};

		count = sizeof(forXA) / sizeof(forXA[0]);
		return forXA;
	}

	static DcmTagKey forCT[] = {
		DCM_NumberOfFrames,
		DCM_Rows,
		DCM_Columns,
		DCM_PixelRepresentation,
		DCM_RescaleSlope,
		DCM_RescaleIntercept,
		DCM_WindowCenter,
		DCM_WindowWidth,
		DCM_PixelSpacing,
		DCM_ImagePositionPatient,
		DCM_ImageOrientationPatient,
		DCM_StudyInstanceUID,
		DCM_PatientPosition,
		DCM_FrameOfReferenceUID,
		DCM_SeriesInstanceUID,
		DCM_SliceThickness,
		DCM_SpacingBetweenSlices
	};
	count = sizeof(forCT) / sizeof(forCT[0]);
	return forCT;
}

DicomImageAttrib::~DicomImageAttrib()
{
}

void DicomImageAttrib::OnLoadAttribsDone()
{
	m_bAttribsLoaded = true; 
	
	// to translate attributes from the string to matrix or vec3
	std::deque<std::string> strQ;
	strQ = StringDelimiter::DelimiteString(m_strImagePosPatient, "\\");
	if (strQ.size() == 3)
	{
		m_imagePositionVec3.x = atof(strQ.at(0).c_str());
		m_imagePositionVec3.y = atof(strQ.at(1).c_str());
		m_imagePositionVec3.z = atof(strQ.at(2).c_str());
	}

	float fTemp;

	strQ.clear();
	strQ = StringDelimiter::DelimiteString(m_strImageOritPatient, "\\");
	if (strQ.size() == 6)
	{
		fTemp = atof(strQ.at(0).c_str());
		m_imageOrientationMatrixmm[0].x = fTemp  * m_fPixelSpaceX;
		m_imageOrientationMatrix[0].x = fTemp;

		fTemp = atof(strQ.at(1).c_str());
		m_imageOrientationMatrixmm[0].y = fTemp  * m_fPixelSpaceX;
		m_imageOrientationMatrix[0].y = fTemp;

		fTemp = atof(strQ.at(2).c_str());
		m_imageOrientationMatrixmm[0].z = fTemp  * m_fPixelSpaceX;
		m_imageOrientationMatrix[0].z = fTemp;


		fTemp = atof(strQ.at(3).c_str());
		m_imageOrientationMatrixmm[1].x = fTemp * m_fPixelSpaceY;
		m_imageOrientationMatrix[1].x = fTemp;

		fTemp = atof(strQ.at(4).c_str());
		m_imageOrientationMatrixmm[1].y = fTemp * m_fPixelSpaceY;
		m_imageOrientationMatrix[1].y = fTemp;

		fTemp = atof(strQ.at(5).c_str());
		m_imageOrientationMatrixmm[1].z = fTemp * m_fPixelSpaceY;
		m_imageOrientationMatrix[1].z = fTemp;

	}

	if (m_imagePlaneHelperPtr.get() == NULL)
	{
		m_imagePlaneHelperPtr = boost::make_shared<DicomImagePlaneHelper>(m_imageOrientationMatrix, 
			m_imagePositionVec3, 
			m_fPixelSpaceX,
			m_fPixelSpaceY, 
			m_nDicomWidth, 
			m_nDicomHeight);
	}
}

bool DicomImageAttrib::IsInSameCoordSystem(const DicomImageAttrib *pDicomImageAttrib) const
{
	bool bRet = false;
	if (pDicomImageAttrib == NULL)
		return bRet;

	bRet = (GetStudyInstUID() == pDicomImageAttrib->GetStudyInstUID());
	if (bRet == false)
		return false;

	bRet = (GetPatientPosition() == pDicomImageAttrib->GetPatientPosition());
	if (bRet == false)
		return false;

	bRet = (GetFrameOfRefUID() == pDicomImageAttrib->GetFrameOfRefUID());
	if (bRet == false)
		return false;

	//bRet = (GetSeriesInstUID() == pDicomImageAttrib->GetSeriesInstUID());
	return bRet;
}

bool DicomImageAttrib::IsContinuousScanned(const DicomImageAttrib *pDicomImageAttrib) const
{
	const glm::mat4 & myOrit = m_imageOrientationMatrixmm;
	const glm::mat4 &othOrit = pDicomImageAttrib->GetImageOrientationMatrixmm();

	bool bRet = true;

	bRet &= fabs(myOrit[0].x - othOrit[0].x) < 1e-6;
	bRet &= fabs(myOrit[0].y - othOrit[0].y) < 1e-6;
	bRet &= fabs(myOrit[0].z - othOrit[0].z) < 1e-6;
	bRet &= fabs(myOrit[1].x - othOrit[1].x) < 1e-6;
	bRet &= fabs(myOrit[1].y - othOrit[1].y) < 1e-6;
	bRet &= fabs(myOrit[1].z - othOrit[1].z) < 1e-6;

	return bRet;
}


bool DicomImageAttrib::IsMyPatientPos(vec3 patientPos) const
{
	vec3  imagePixelPos = m_imagePlaneHelperPtr->ConvertToImagePlane(patientPos);

	float halfThickness = fabs(m_fSliceThickness / 2);
	float halfSpacing = fabs(m_fSpacingBetweenSlices / 2);
	float toleranceDistanceToImagePlane = fmax(halfSpacing, halfThickness);

	return (fabs(imagePixelPos.z) < toleranceDistanceToImagePlane);
}

vec3 DicomImageAttrib::ConvertImagePixelToPatientCoordSys(const PointF &ptf) const
{
	return m_imagePlaneHelperPtr->ConvertToPatient(ptf);
}

DicomSeriesDescription::DicomSeriesDescription(StudySeries *pStudySeries) :
	m_pMyStudySeries(pStudySeries)
{
	
}

DicomSeriesDescription::~DicomSeriesDescription()
{
	Reset();
}

const DicomImageDescription *DicomSeriesDescription::GetFirstImageDesc() const
{
	return QueryDicomImageDescription(0);
}

const DicomImageDescription *DicomSeriesDescription::GetLastImageDesc() const
{
	int count = m_imagesInfo.size();

	return QueryDicomImageDescription(count -1);
}

DicomImageDescription *DicomSeriesDescription::QueryDicomImageDescription(int index) const
{
	if (index >= m_imagesInfo.size() || index <0)
		return NULL;

	DicomImageDescription *pImageDesc =  m_imagesInfo[index];

	return pImageDesc;
}


const DicomImageDescription *DicomSeriesDescription::QueryDicomImageDescription(vec3 patientPos, int &index) const
{
	index  = 0;
	for (DicomImgInfoVecCIt it = m_imagesInfo.begin(); it != m_imagesInfo.end(); ++it, ++index)
	{
		if ((*it)->IsMyPatientPos(patientPos))
		{
			/*std::stringstream log;
			log << patientPos.x << "," << patientPos.y << "," << patientPos.z;
			LOG_INFO(log.str());*/

			return (*it);
		}
	}

	return NULL;
}

int DicomSeriesDescription::GetImagesCount() const
{
	return m_imagesInfo.size();
}

void DicomSeriesDescription::AddDicomImageDescription(DicomImageDescription *dicomDbInfo)
{
	m_imagesInfo.push_back(dicomDbInfo);
}

void DicomSeriesDescription::AppendImageDesObjs(const DicomImageDescription *dicomImageFrame, const DicomImgInfoVec &objs)
{
	if (dicomImageFrame)
	{
		for (DicomImgInfoVecIt it = m_imagesInfo.begin(); it != m_imagesInfo.end(); it ++)
		{
			if ((*it) == dicomImageFrame)
			{
				++it;
				m_imagesInfo.insert(it, objs.begin(), objs.end());
				break;
			}
		}
	}
	else
	{
		m_imagesInfo.insert(m_imagesInfo.begin(), objs.begin(), objs.end());
	}
}

void DicomSeriesDescription::SetPatientName(const std::string &pn)
{
	m_strPatientName = pn;
}

const std::string &DicomSeriesDescription::GetPatientName()
{
	return m_strPatientName;
}

void DicomSeriesDescription::ResetImagesList()
{
	for (DicomImgInfoVecIt it = m_imagesInfo.begin(); it != m_imagesInfo.end();
		it ++)
	{
		delete *it;
	}
	m_imagesInfo.clear();
}

void DicomSeriesDescription::Reset()
{
	m_strPatientName = "";

	ResetImagesList();	
}

	
MCSF_DJ2DENGINE_END_NAMESPACE
