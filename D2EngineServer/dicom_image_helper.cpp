#include "stdafx.h"
#include "dicom_image_helper.h"
#include <fstream>
#include <sstream>
#include <boost/make_shared.hpp>
#include <Shlwapi.h>
#include "dicom_component_information.h"
#include "dicom_region_pixel_helper.h"
#include "graphics_imager.h"
#include "study_image.h"
#include "dicom_polygon_pixels_statistics.h"
#include "dicom_dcmtk_inc.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

DicomRegionPixelsStatistics::DicomRegionPixelsStatistics() : m_bIsDummy(false)
{

}

DicomRegionPixelsStatistics::~DicomRegionPixelsStatistics()
{

}

void DicomImageHelper::InitDecoder()
{
	// register RLE decompression codec
	DcmRLEDecoderRegistration::registerCodecs();
#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
	E_DecompressionColorSpaceConversion opt_decompCSconversion = EDC_photometricInterpretation;
	// register JPEG decompression codecs
	DJDecoderRegistration::registerCodecs(opt_decompCSconversion);
#endif

	DCM_dcmdataGetLogger();
}

void DicomImageHelper::FiniDecoder()
{
	// deregister RLE decompression codec
	DcmRLEDecoderRegistration::cleanup();
#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
	// deregister JPEG decompression codecs
	DJDecoderRegistration::cleanup();
#endif
}

DicomImageHelper::DicomImageHelper(StudyImage *pStudyImage) : m_pDicomImage(NULL), 
	m_bDictLoaded(false), m_pStudyImage(pStudyImage)
{
	if (!dcmDataDict.isDictionaryLoaded())
		return;
	
	m_pDcmFileFormat = NULL;
	m_bDictLoaded = true;
}

void DicomImageHelper::ReleaseDicomResource()
{
	DEL_PTR(m_pDicomImage);
}

DicomImageHelper::~DicomImageHelper(void)
{
	ReleaseDicomResource();

	DEL_PTR(m_pDcmFileFormat);
}

/*
1: ok
0: failed
*/
int DicomImageHelper::SaveResultToFile(std::string destFileName, ConvertFileType destFileType, int frameIndex)
{
	if (m_pDicomImage == NULL || m_pDcmFileFormat == NULL)
		return false;
	
	OFCmdUnsignedInt    opt_frameCount = 1;               /* default: one frame */
#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
	// JPEG parameters
	OFCmdUnsignedInt    opt_quality = 90;                 /* default: 90% JPEG quality */
	E_SubSampling       opt_sampling = ESS_422;           /* default: 4:2:2 sub-sampling */
#endif
	/* finally create output image file */
	int result = false;
	FILE *ofile = NULL;
	const char *opt_ofname = destFileName.c_str();
	
	if (frameIndex < 0 || frameIndex >= SelFramesCount())
		return false;

	if (opt_ofname)
	{
		ofile = fopen(opt_ofname, "wb");
		if (ofile == NULL)
		{
			return false;
		}
	} 
	else
	{
		//OFLOG_INFO(dcm2pnmLogger, "writing frame " << (opt_frame + frame) << " to stdout");	
		return false;
	}

	switch (destFileType)
	{

	case EFT_BMP:
		result = m_pDicomImage->writeBMP(ofile, 0, frameIndex);
		break;
	case EFT_8bitBMP:
		result = m_pDicomImage->writeBMP(ofile, 8, frameIndex);
		break;
	case EFT_24bitBMP:
		result = m_pDicomImage->writeBMP(ofile, 24, frameIndex);
		break;
	case EFT_32bitBMP:
		result = m_pDicomImage->writeBMP(ofile, 32, frameIndex);
		break;
#ifdef BUILD_DCM2PNM_AS_DCMJ2PNM
	case EFT_JPEG:
		{
			/* initialize JPEG plugin */
			DiJPEGPlugin plugin;
			plugin.setQuality(OFstatic_cast(unsigned int, opt_quality));
			plugin.setSampling(opt_sampling);
			result = m_pDicomImage->writePluginFormat(&plugin, ofile, frameIndex);
		}
		break;
#endif

	default:
		if (opt_ofname)
			result = m_pDicomImage->writeRawPPM(ofile, 8, frameIndex);
		else /* stdout */
			result = m_pDicomImage->writePPM(ofile, 8, frameIndex);
		break;
	}

	fclose(ofile);

	return result;
}

GraphicsImager *DicomImageHelper::SaveDicomFrameToImager(Gdiplus::PixelFormat pixelFormat)
{
	return SaveImageFrameToGhImager(pixelFormat, 0);
}

GraphicsImager *DicomImageHelper::SaveImageFrameToGhImager(Gdiplus::PixelFormat pixelFormat, int relFrameIndex /* = 0 */)
{
	std::stringstream ssDestBmpFileName;
/*
ACE_Utils::UUID uuid;
ACE_Utils::UUID_Generator generater;
generater.generate_UUID(uuid);
m_sName = uuid.to_string()->c_str();
*/
	time_t cur;
	time(&cur);
	ssDestBmpFileName << TEMP_DIR_NAME << "\\" << &ssDestBmpFileName << this << cur;
	std::string strDestBmpFileName = ssDestBmpFileName.str();
	
	ConvertFileType type = EFT_24bitBMP;
	if (pixelFormat == PixelFormat32bppRGB)
		type = EFT_32bitBMP;
	
	int result = SaveResultToFile(strDestBmpFileName, type, relFrameIndex);
	if (false == result)
	{
		return NULL;
	}

	// wchar_t *pFile = ConvertToWCharString(szImage);
	GraphicsImager *pResultImage = GraphicsImager::FromFile(strDestBmpFileName.c_str(), true, pixelFormat);

	// delete the temporary file
	DeleteFileA(strDestBmpFileName.c_str());

	return pResultImage;
}

DicomTagValueType DicomImageHelper::RetriveTagValue(DcmFileFormat *pDcmFileFormat, int group,
	int tag, long &lVal, double &fVal, std::string &strVal)
{
	DcmDataset *pDcmDataset = NULL;
	if (pDcmFileFormat == NULL || (pDcmDataset = pDcmFileFormat->getDataset()) == NULL) 
		return TAG_VAL_ERR;

	const char *szVal = NULL;

	const DcmDataDictionary &globalDataDict = dcmDataDict.rdlock();
	DcmTagKey dcmTagKey;
	dcmTagKey.set(group, tag);
	const DcmDictEntry *dicent = globalDataDict.findEntry(dcmTagKey, NULL);
	if (dicent == NULL) 
		return TAG_VAL_ERR;

	DcmEVR valueRepresentation = dicent->getEVR();

	if (valueRepresentation == EVR_AS ||
		valueRepresentation == EVR_CS ||
		valueRepresentation == EVR_DS ||
		valueRepresentation == EVR_DS ||
		valueRepresentation == EVR_DT ||
		valueRepresentation == EVR_IS ||
		valueRepresentation == EVR_LO ||
		valueRepresentation == EVR_PN ||
		valueRepresentation == EVR_SH ||
		valueRepresentation == EVR_TM ||
		valueRepresentation == EVR_UI ||
		valueRepresentation == EVR_DA)
	{
		OFCondition status = pDcmDataset->findAndGetString(dcmTagKey, szVal, OFTrue);
		if (status.good() && szVal)
		{
			strVal = szVal;
			return TAG_VAL_STR;
		}
	}
	else if (valueRepresentation == EVR_SL)
	{
		OFCondition status = pDcmDataset->findAndGetSint32(dcmTagKey, lVal, 0, OFTrue);
		if (status.good())
		{
			return TAG_VAL_LONG;
		}
	}
	else if (valueRepresentation == EVR_UL)
	{
		Uint32 ui32 = 0;
		OFCondition status = pDcmDataset->findAndGetUint32(dcmTagKey, ui32, 0, OFTrue);
		if (status.good())
		{
			lVal = static_cast<long>(ui32);
			return TAG_VAL_LONG;
		}
	}
	else if (valueRepresentation == EVR_US || valueRepresentation == EVR_OW)
	{
		Uint16 ui16 = 0;
		OFCondition status = pDcmDataset->findAndGetUint16(dcmTagKey, ui16, 0, OFTrue);
		if (status.good())
		{
			lVal = static_cast<long>(ui16);
			return TAG_VAL_LONG;
		}
	}
	else if (valueRepresentation == EVR_FL || valueRepresentation == EVR_OF)
	{
		Float32 f32 = 0;
		OFCondition status = pDcmDataset->findAndGetFloat32(dcmTagKey, f32, 0, OFTrue);
		if (status.good())
		{
			fVal = static_cast<double>(f32);
			return TAG_VAL_FLOAT;
		}
	}
	else if (valueRepresentation == EVR_FD)
	{
		OFCondition status = pDcmDataset->findAndGetFloat64(dcmTagKey, fVal, 0, OFTrue);
		if (status.good())
		{
			return TAG_VAL_FLOAT;
		}
	}
	dcmDataDict.unlock();

	
	return TAG_VAL_ERR;
}

inline void ClearDiffTypeValues(long &lVal, double &dVal, std::string &strVal)
{
	lVal = 0;
	dVal = 0;
	strVal = "";
}

bool DicomImageHelper::RetriveTagStrValue(DcmFileFormat *pDcmFileFormat, int group, int tag, std::string &strRetVal)
{
	long lVal = 0;
	double dVal = 0;
	std::string strVal;
	DicomTagValueType retValType;
	strRetVal = "";

	DcmDataset *pDcmDataset = NULL;
	if (pDcmFileFormat == NULL || (pDcmDataset = pDcmFileFormat->getDataset()) == NULL) 
		return false;

	retValType = RetriveTagValue(pDcmFileFormat, group, tag, lVal, dVal, strVal);
	if (retValType == TAG_VAL_STR)
	{
		strRetVal = strVal;
		return true;
	}

	if (retValType == TAG_VAL_LONG)
	{
		std::stringstream ss;
		ss << lVal;
		strRetVal = ss.str();
		return true;
	}

	if (retValType == TAG_VAL_FLOAT)
	{
		std::stringstream ss;
		ss << setiosflags(ios::fixed) << dVal;
		strRetVal = ss.str();
		return true;
	}

	return false;
}

bool DicomImageHelper::RetriveTagStrValue(int group, int tag, std::string &strVal)
{
	return RetriveTagStrValue(m_pDcmFileFormat, group, tag, strVal);
}

bool DicomImageHelper::RetriveTagStrValue(const std::string &dicomFileName,int group, int tag, std::string &strVal)
{
	DcmFileFormat *pDcmFormat = new DcmFileFormat();
	OFCondition cond = pDcmFormat->loadFile(dicomFileName.c_str(), EXS_Unknown, EGL_withoutGL, DCM_MaxReadLength, ERM_autoDetect);
	if (cond.bad())
	{
		delete pDcmFormat;
		return false;
	}

	RetriveTagStrValue(pDcmFormat, group, tag, strVal);

	DEL_PTR(pDcmFormat);
	return true;

}

bool DicomImageHelper::RetriveImageAttribs(DcmFileFormat *pDcmFileFormat, DicomImageAttrib *pDcmImgAttribs)
{
	if (pDcmFileFormat == NULL || pDcmImgAttribs == NULL) 
		return false;

	if (pDcmImgAttribs->IsAttribsLoadDone())
		return true;

	DicomTagValueType retValType;
	long lVal = 0;
	double dVal = 0;
	std::string strRetVal;

	DcmTagKey item =  DCM_Modality;

	retValType = RetriveTagValue(pDcmFileFormat, item.getGroup(), item.getElement(), lVal, dVal, strRetVal);
	if (retValType == TAG_VAL_ERR)
	{
		return false;
	}

	pDcmImgAttribs->SetModalityName(strRetVal);

	int count = 0;
	const DcmTagKey * dcmKeys = DicomImageDescription::AttribDcmTagKeys(strRetVal, count);
	for (int i = 0; i < count; i ++)
	{
		item = dcmKeys[i];
	
		ClearDiffTypeValues(lVal, dVal, strRetVal);
		
		retValType = RetriveTagValue(pDcmFileFormat, item.getGroup(), item.getElement(), lVal, dVal, strRetVal);
		if (retValType == TAG_VAL_ERR)
		{
			continue;
		}

		if (item == DCM_NumberOfFrames)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetFrameCount(atoi(strRetVal.c_str()));
			}
			else
			{
				pDcmImgAttribs->SetFrameCount(lVal);
			}
		}
		else if (item == DCM_Rows)
		{
			pDcmImgAttribs->SetDicomHeight(lVal);
		}
		else if (item == DCM_Columns)
		{
			pDcmImgAttribs->SetDicomWidth(lVal);
		}
		else if (item == DCM_PixelRepresentation)
		{
			pDcmImgAttribs->SetPixelRepresentation(lVal);
		}
		else if (item == DCM_RescaleIntercept)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetRescaleIntercept(atoi(strRetVal.c_str()));
			}
		}
		else if (item == DCM_RescaleSlope)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetRescaleSlope(atoi(strRetVal.c_str()));
			}
		}
		else if (item == DCM_WindowWidth)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->WindowWidth(atoi(strRetVal.c_str()));
			}

		}
		else if (item == DCM_WindowCenter)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->WindowCenter(atoi(strRetVal.c_str()));
			}

		}
		else if (item == DCM_PixelSpacing || item == DCM_ImagerPixelSpacing)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->PhysicalUnitType(UNIT_TYPE_MM);
				char *szPixelSpaceX = strtok(const_cast<char*>(strRetVal.c_str()), "\\");
				char *szPixelSpaceY = strtok(NULL, "\\");
				if (szPixelSpaceX)
				{
					pDcmImgAttribs->PixelSpaceX(atof(szPixelSpaceX));
				}

				if (szPixelSpaceY)
				{
					pDcmImgAttribs->PixelSpaceY(atof(szPixelSpaceY));
				}
				else
				{
					pDcmImgAttribs->PixelSpaceY(pDcmImgAttribs->PixelSpaceX());
				}
			}
		}
		else if (item == DCM_ImagePositionPatient)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetImagePosPatient(strRetVal);
			}
		}
		else if (item == DCM_ImageOrientationPatient)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetImageOritPatient(strRetVal);
			}
		}
		else if (item == DCM_StudyInstanceUID)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetStudyInstUID(strRetVal);
			}
		}
		else if (item == DCM_PatientPosition)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetPatientPosition(strRetVal);
			}
		}
		else if (item == DCM_FrameOfReferenceUID)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetFrameOfRefUID(strRetVal);
			}
		}
		else if (item == DCM_SeriesInstanceUID)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetSeriesInstUID(strRetVal);
			}
		}
		else if (item == DCM_SliceThickness)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetSliceThickness(atof(strRetVal.c_str()));
			}
		}
		else if (item == DCM_SpacingBetweenSlices)
		{
			if (strRetVal.length() > 0)
			{
				pDcmImgAttribs->SetSpacingBetweenSlices(atof(strRetVal.c_str()));
			}
		}
	}

	pDcmImgAttribs->OnLoadAttribsDone();

	return true;
}
bool DicomImageHelper::RetriveImageAttribs(const std::string &dicomFileName, DicomImageAttrib *pDicomImageAttrib)
{
	if (pDicomImageAttrib == NULL)
		return false;

	if (pDicomImageAttrib->IsAttribsLoadDone())
		return true;

	DcmFileFormat *pDcmFormat = new DcmFileFormat();
	OFCondition cond = pDcmFormat->loadFile(dicomFileName.c_str(), EXS_Unknown, EGL_withoutGL, DCM_MaxReadLength, ERM_autoDetect);
	if (cond.bad())
	{
		delete pDcmFormat;
		return false;
	}

	RetriveImageAttribs(pDcmFormat, pDicomImageAttrib);

	DEL_PTR(pDcmFormat);
	return true;
}

bool DicomImageHelper::LoadDicomFile(std::string dicomFile, int nFrameIndex, DicomImageDescription *pDicomDescription) 
{
	if (m_bDictLoaded == false) 
		return false;

	DEL_PTR(m_pDicomImage);

	E_FileReadMode      opt_readMode = ERM_autoDetect;
	E_TransferSyntax    opt_transferSyntax = EXS_Unknown;
	unsigned long       opt_compatibilityMode = CIF_DecompressCompletePixelData | CIF_UsePartialAccessToPixelData;
	DcmDataset			*pDcmDataset = NULL;
	const char *        opt_ifname = dicomFile.c_str();

	DEL_PTR(m_pDcmFileFormat);

	m_pDcmFileFormat = new DcmFileFormat();
	OFCondition cond = m_pDcmFileFormat->loadFile(opt_ifname, opt_transferSyntax, EGL_withoutGL, DCM_MaxReadLength, opt_readMode);
	if (cond.bad())
	{
		DEL_PTR(m_pDcmFileFormat);
		return false;
	}
	
	DicomImageFrameDesc *pDicomImageFrameDesc = dynamic_cast<DicomImageFrameDesc *>(pDicomDescription);
	if (pDicomImageFrameDesc)
	{
		DicomImageAttrib *pDicomImageAttrib = dynamic_cast<DicomImageAttrib *>(pDicomImageFrameDesc);
		if (pDicomImageAttrib)
		{
			bool b = RetriveImageAttribs(m_pDcmFileFormat, pDicomImageAttrib);
			if (b)
			{
				dynamic_cast<DicomImageFrameDesc *>(this)->operator =(*pDicomImageFrameDesc);

				m_fNewWinWidth = WindowWidth();
				m_fNewWinWidth = WindowCenter();
			}
		}
	}

	// the last parameters =0 means all of frame
	pDcmDataset = m_pDcmFileFormat->getDataset();
	if (pDcmDataset == NULL)
	{
		DEL_PTR(m_pDcmFileFormat);
		return false;
	}

	E_TransferSyntax xfer = pDcmDataset->getOriginalXfer();

	m_pDicomImage = new DicomImage(m_pDcmFileFormat, xfer, opt_compatibilityMode, DicomFrameIndex(), SelFramesCount());
	if (m_pDicomImage == NULL || (m_pDicomImage->getStatus() != EIS_Normal))
	{
		DEL_PTR(m_pDcmFileFormat);
		DEL_PTR(m_pDicomImage);
		return false;
	}
	
	return true;
}

void DicomImageHelper::StatisticsRectPixels(const Rect &rect, DicomRegionPixelsStatistics *p)
{
	DicomPickedRectPixels rectRegionStatistics(this);
	rectRegionStatistics.SelectRegion(rect);
	*p =  *rectRegionStatistics.GetRegionPixelsStatistics();
}

void DicomImageHelper::StatisticsCirclePixels(PointF center, float fRadius, DicomRegionPixelsStatistics *p)
{
	DicomPickedCirclePixels circleRegionStatistics(this);
	circleRegionStatistics.SelectRegion(center, fRadius);
	*p = *circleRegionStatistics.GetRegionPixelsStatistics();
}

void DicomImageHelper::StatisticsFreeHandPixels(PointF *pPointFs, int count, DicomRegionPixelsStatistics *p)
{
	ScanlinePolygonStatistics freeHandStatistics(this);

	freeHandStatistics.SelectFreeHandRegion(pPointFs, count);
	
	*p = *freeHandStatistics.GetRegionPixelsStatistics();
}



void DicomImageHelper::ChangeWinWidthCenter( double center, double window)
{
	if (m_pDicomImage)
	{
		m_pDicomImage->setWindow(center, window);

		m_fNewWinCenter = center;
		m_fNewWinWidth  = window;
	}
}


MCSF_DJ2DENGINE_END_NAMESPACE
