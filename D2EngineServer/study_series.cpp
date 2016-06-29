#include "stdafx.h"
#include "study_series.h"
#include "site_page.h"
#include "image_tool_interface.h"
#include "study_image.h"
#include "study_series_command_request_args.h"
#include "mcsf_dj2dengine_containee.h"
#include "series_image_id.h"
#include "image_tool_interface.h"
#include "study_image_interface.h"
#include "blank_study_image.h"
#include "study_series_image_cell.h"
#include "d2engine_dbwrapper.h"
#include "series_rec.h"
#include "image_rec.h"
#include "dicom_series_attribs_loader_hp_.h"
#include "site_work_render_task.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE


StudySeries::StudySeries(SitePage *pParent, const PageSeriesId &pageSeriesId) :
	m_pSitePage(pParent), m_siteSeriesId(pageSeriesId), m_nFirstCellImageIndex(0),
	m_bEnableTransfSync(true), m_dicomSeriesDbInfo(this), m_nCellWidth(0),
	m_nCellHeight(0)
{
}


StudySeries::~StudySeries(void)
{
	ReleaseAllAllocatedResources();
}

bool StudySeries::CanMoveCellImage(int cellsCount)
{
	int imagesCount = m_dicomSeriesDbInfo.GetImagesCount();

	return (imagesCount > cellsCount);
}

int StudySeries::NormalizeImageIndex(int index, int cellsCount)
{
	int normalizedIndex = index;

	int imagesCount = m_dicomSeriesDbInfo.GetImagesCount();

	// only images count > cells coutn can cycle
	if (imagesCount > cellsCount)
	{
		if (index >= imagesCount)
		{
			normalizedIndex = index % imagesCount;
		}
		else if (index < 0)
		{
			normalizedIndex = index + imagesCount;
		}

	}
	
	return normalizedIndex;
}

void StudySeries::onOpenImage(int offset, bool isAbsOffset, int cellsCount, int imageWidth, int imageHeight)
{
	std::stringstream ssLog;

	OnOpenImageReqBegin();

	bool bCellsChanged = false, bShownImagesChanged = false, bSizeChg = false;

	if (cellsCount < 0)
	{
		cellsCount = 1;
	}

	if (m_nCellHeight != imageHeight || m_nCellWidth != imageWidth)
	{
		bSizeChg = true;
	}

	m_nCellWidth = imageWidth;
	m_nCellHeight = imageHeight;

	int nOpenedImageCellsNum = m_AllocatedSCellPosObjMap.size();

	// need to add more
	if (cellsCount > nOpenedImageCellsNum)
	{
		for (int i = nOpenedImageCellsNum ; i < cellsCount; i ++)
		{
			m_AllocatedSCellPosObjMap[i] = new StudySeriesImageCell(this, i);
		}

		bCellsChanged |= true;
	}
	else if (cellsCount < nOpenedImageCellsNum)
	{
		// remove the last images
		for(int i = cellsCount; i < nOpenedImageCellsNum; i ++)
		{
			delete m_AllocatedSCellPosObjMap[i];
			m_AllocatedSCellPosObjMap.erase(i);
		}

		bCellsChanged |= true;
	}

	cellsCount = m_AllocatedSCellPosObjMap.size();
	if (cellsCount <= 0)
	{
		return;
	}

	if (CanMoveCellImage(cellsCount))
	{
		if (isAbsOffset)
		{
			int newOfffset = NormalizeImageIndex(offset, cellsCount);
			if (newOfffset != m_nFirstCellImageIndex)
			{
				m_nFirstCellImageIndex = newOfffset;
				bShownImagesChanged |= true;
			}
		}
		else if (offset != 0)
		{
			m_nFirstCellImageIndex =  NormalizeImageIndex(m_nFirstCellImageIndex + offset, cellsCount);

			bShownImagesChanged |= true;
		}
	}
	
	RearrangeCellImagesOnPersist(m_nFirstCellImageIndex, cellsCount, m_nCellWidth, m_nCellHeight, 
								(bCellsChanged || bShownImagesChanged));

	// for each movement of image position
	OnOpenImagesReqDone();
}

void StudySeries::NotifyCellsRefreshImage()
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		it->second->RefeshImage();
	}
}

void StudySeries::GetShownCellImgeDescs(CellImageDescMap &cellImageDescs) const
{
	cellImageDescs.clear();

	for (StudyImagePosObjMapCIt it = m_CellAssignedSIMap.begin(); it != m_CellAssignedSIMap.end(); it ++)
	{
		StudyImage *pStudyImage = dynamic_cast<StudyImage *>(it->second);
		if (pStudyImage)
		{
			cellImageDescs[it->first] = pStudyImage->GetDicomImageDesc();
		}
	}
}

void StudySeries::NotifyRefSeriesRedrawLocalizerLine()
{
	if (OwnerPage()->IsLocalizerLinesEnabled(&m_LlResourceId) == false)
		return;

	CellImageDescMap cellImageDescs;
	GetShownCellImgeDescs(cellImageDescs);

	if (m_dicomSeriesDbInfo.GetFirstImageDesc() && m_dicomSeriesDbInfo.GetLastImageDesc() && 
		(cellImageDescs.size() > 0))
	{
		OwnerPage()->NotifyRefSeriesRedrawLocalizerLine(&m_dicomSeriesDbInfo);
	}
}

void StudySeries::OnOpenImageReqBegin()
{
	
}

void StudySeries::OnOpenImagesReqDone()
{
	NotifyRefSeriesRedrawLocalizerLine();
}


void StudySeries::OnDrawingLocalizerLineMove(const DicomImageDescription *pImageDesc, int nRefedSeriesCellNum, PointF refImagePixel)
{
	OwnerPage()->onLocalizerLineMove(&m_dicomSeriesDbInfo, pImageDesc, nRefedSeriesCellNum, refImagePixel);
}

void StudySeries::OnRearrangeCellImagesDone(bool bCellImageIndxChg)
{
	if (bCellImageIndxChg)
	{
		NotifyClient_SeriesStatusChanged();
	}

	NotifyCellsRefreshImage();
}

void StudySeries::OnReferSeriesLocalizerLineMove(const DicomSeriesDescription *pReferrSeriesDesc,
	const DicomImageDescription *pRefImageDesc,
	int nRefedSeriesCellNum, PointF refImagePixel)
{
	int outIndex = 0;
	int cellsCount = m_AllocatedSCellPosObjMap.size();

	if (nRefedSeriesCellNum < 0 || nRefedSeriesCellNum >= cellsCount)
		return;

	vec3 patientPos = pRefImageDesc->ConvertImagePixelToPatientCoordSys(refImagePixel);

	const DicomImageDescription *pImageDesc = m_dicomSeriesDbInfo.QueryDicomImageDescription(patientPos, outIndex);
	if (pImageDesc && pRefImageDesc->IsInSameCoordSystem(pImageDesc))
	{
		int nFirstImageIndex = outIndex - nRefedSeriesCellNum;

		if (m_nFirstCellImageIndex == nFirstImageIndex)
			return;

		m_nFirstCellImageIndex = NormalizeImageIndex(nFirstImageIndex, cellsCount);

		RearrangeCellImagesOnPersist(m_nFirstCellImageIndex, cellsCount, m_nCellWidth, m_nCellHeight, true);

		OnOpenImagesReqDone();
	}
}

void StudySeries::OnLocalizerLinesRefSidesChanged(const DicomSeriesDescription *pReferredSeriesDesc)
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		IStudySeriesImageCell *pIImageCell = dynamic_cast<IStudySeriesImageCell *>(it->second);
		StudySeriesImageCell *pImageCell = dynamic_cast<StudySeriesImageCell *>(pIImageCell);
		if (pImageCell)
		{
			pImageCell->RefreshLocalizerLines(pReferredSeriesDesc);
		}
	}
}

IStudySeriesImageCell *StudySeries::FindImageCellObj(int nCellNum)
{
	if (m_AllocatedSCellPosObjMap.find(nCellNum) != m_AllocatedSCellPosObjMap.end())
	{
		return m_AllocatedSCellPosObjMap[nCellNum];
	}
	return NULL;
}

void StudySeries::onSiteCommentTagsChanged()
{
	// to update all of study image and let cell to resent the image stream
	for (StudyImageNameObjMapIt it = m_AllocatedSINameObjsMap.begin(); it != m_AllocatedSINameObjsMap.end();
		it ++)
	{
		it->second->onCommentTagsChanged();
	}

	NotifyCellsRefreshImage();
}

void StudySeries::onLocalizerLinesToolRequest(const LocalizerLinesRequestArgs *pLlRequestArgs)
{
	if (pLlRequestArgs->Operation() == 0)
	{
		int resId = pLlRequestArgs->ResourceId();
		bool isRererringSide = pLlRequestArgs->IsReferringSide();

		m_LlResourceId.ResourceId(resId);
		m_LlResourceId.IsReferringSide(isRererringSide);

		OwnerPage()->onLlReferSidesChanged(&m_dicomSeriesDbInfo);

		NotifyRefSeriesRedrawLocalizerLine();
	}
	else if (pLlRequestArgs->Operation() == 1)
	{
		OwnerPage()->ClearLlRefResource(&m_dicomSeriesDbInfo);
	}
}

void StudySeries::ResetDrawingImage(int nCellIndex)
{
	// reset and refresh image

	int nCellCount = m_CellAssignedSIMap.size();

	if (nCellIndex < nCellCount && nCellIndex >= 0)
	{
		StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.find(nCellIndex);
		if (it != m_AllocatedSCellPosObjMap.end())
		{
			StudySeriesImageCell *pStudySeriesCell = dynamic_cast<StudySeriesImageCell *>(it->second);
			if (pStudySeriesCell)
			{
				pStudySeriesCell->ResetCellDrawing();
			}
		}
	}
	else if (nCellIndex == -1)
	{
		for (StudyImageNameObjMapIt it = m_AllocatedSINameObjsMap.begin(); it != m_AllocatedSINameObjsMap.end();
			it ++)
		{
			it->second->ResetDrawing();
		}

		for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
			it ++)
		{
			StudySeriesImageCell *pStudySeriesCell = dynamic_cast<StudySeriesImageCell *>(it->second);
			if (pStudySeriesCell)
			{
				pStudySeriesCell->ResetCellDrawing();
			}
		}
	}
}

void StudySeries::onCommunicationCommandRequest(const ImageCommRequestArgs *pImageCommRequestArgs)
{
	IStudySeriesImageCell * pStudyImageCell = NULL;

	if (pImageCommRequestArgs == NULL)
		return;
	else
	{
		if (m_strCommReceiver.length() <= 0)
		{
			m_strCommReceiver = pImageCommRequestArgs->sSender;
		}
	}
	switch (pImageCommRequestArgs->iCommandId)
	{
	case OpenImage:
		{
			const OpenImageRequestArgs *pOpenImageRequestArgs = dynamic_cast<const OpenImageRequestArgs *>(pImageCommRequestArgs);
			if (pOpenImageRequestArgs == NULL)
				return;

			onOpenImage(pOpenImageRequestArgs->offset, 
						pOpenImageRequestArgs->isAbsOffset, 
						pOpenImageRequestArgs->cellsNum,
						pOpenImageRequestArgs->imageWidth, 
						pOpenImageRequestArgs->imageHeight);
		}
		break;
	case MessageCommandType::ResetImage:
		{
			ResetDrawingImage(pImageCommRequestArgs->imagePosId.imageCellPos);
		}
		break;
	case ChangeToolType:
		{
			const ToolChgRequestArgs *pToolChgRequestArgs = dynamic_cast<const ToolChgRequestArgs *>(pImageCommRequestArgs);
			if (pToolChgRequestArgs == NULL)
				return;
			pStudyImageCell = FindImageCellObj(pToolChgRequestArgs->imagePosId.imageCellPos);
			if (pStudyImageCell)
			{
				if (pToolChgRequestArgs->toolType == LocalizerLines)
				{
					const LocalizerLinesRequestArgs *pLlRequestArgs = dynamic_cast<const LocalizerLinesRequestArgs *>(pToolChgRequestArgs);
					if (pLlRequestArgs)
					{
						onLocalizerLinesToolRequest(pLlRequestArgs);
					}
				}
				else
				{
					pStudyImageCell->onToolTypeChangedCommandRequest(pToolChgRequestArgs);
				}
				
			}
		}
		break;
	case Mouse:
		{
			const MouseEvtRequestArgs *pMouseEvtRequestArgs = dynamic_cast<const MouseEvtRequestArgs *>(pImageCommRequestArgs);
			if (pMouseEvtRequestArgs)
			{
				pStudyImageCell = FindImageCellObj(pMouseEvtRequestArgs->imagePosId.imageCellPos);
				if (pStudyImageCell)
				{
					pStudyImageCell->onMouseEvtCommandRequest(pMouseEvtRequestArgs);
				}
			}
		}
		break;
	case Keyboard:
		{
			const KeyboardEvtRequestArgs *pKeyboardEvtRequestArgs = dynamic_cast<const KeyboardEvtRequestArgs *>(pImageCommRequestArgs);
			if (pKeyboardEvtRequestArgs) 
			{
				pStudyImageCell = FindImageCellObj(pKeyboardEvtRequestArgs->imagePosId.imageCellPos);
				if (pStudyImageCell)
				{
					pStudyImageCell->onKeyboardEvtCommandRequest(pKeyboardEvtRequestArgs);
				}
			}
		}
		break;
	case SeriesSettings:
		{
			const SeriesSettingsRequestArgs *pSeriesSettingsRequestArgs = dynamic_cast<const SeriesSettingsRequestArgs *>(pImageCommRequestArgs);
			if (pSeriesSettingsRequestArgs)
			{
				if (pSeriesSettingsRequestArgs->enableSyncTransfChanged)
				{
					m_bEnableTransfSync = pSeriesSettingsRequestArgs->enableSyncTransf;
				}
			}
		}
		break;
	default:
		{
			assert(0);
		}
		break;
	}
}

void StudySeries::HandleSyncImageInvertColor(void *pOwner)
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		it->second->HandleSyncImageInvertColor(pOwner);
	}
}

void StudySeries::HandleSyncImageSetWinWidthCenter(float fWinWidth, float fWinCenter, bool isAbsoluteVal, void *pOwner)
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		it->second->HandleSyncImageSetWinWidthCenter(fWinWidth, fWinCenter, isAbsoluteVal, pOwner);
	}
}

void StudySeries::HandleSyncImageTransformation(const TransformationArgs *p)
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		it->second->HandleSyncImageTransformation(p);
	}
}

void StudySeries::ReleaseSeriesCells()
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		 it ++)
	{
		DEL_PTR (it->second);
	}

	m_AllocatedSCellPosObjMap.clear();
}


void StudySeries::ReleaseSeriesImageObjects()
{
	m_CellAssignedSIMap.clear();
	
	for (StudyImageNameObjMapIt it = m_AllocatedSINameObjsMap.begin(); it != m_AllocatedSINameObjsMap.end();
		it ++)
	{
		DEL_PTR (it->second);
	}

	m_AllocatedSINameObjsMap.clear();
}

void StudySeries::ReleaseSeriesBlankImageObjs()
{
	for (BlankStudyImagesVIt it = m_AllocatedBSIList.begin(); it != m_AllocatedBSIList.end(); it ++)
	{
		DEL_PTR(*it);
	}
	m_AllocatedBSIList.clear();
}

void StudySeries::ReleaseAllAllocatedResources()
{
	ReleaseSeriesCells();
	ReleaseSeriesImageObjects();
	ReleaseSeriesBlankImageObjs();
}

#ifdef USE_MCSF_MYSQL_TEST

void StudySeries::LoadSeriesImgFnamesFromDb(const std::string &seriesName, std::vector<std::string> &image_files)
{
	
	std::string studyInstanceUidFk, patientUidFk, patientName;
	int iRet = 0;
	// get patient
	Mcsf::ISeriesBasePtr seriesPtr;
	//sConditon = "SeriesInstanceUID='" + seriesName +"'";
	iRet = DBWRAPPER->GetSeriesObjectByUID(seriesName, seriesPtr);
	if (iRet == ERROR_DB_NULL)
	{
		studyInstanceUidFk = seriesPtr->GetStudyInstanceUIDFk();

		Mcsf::IStudyPtr studyptr;
		iRet = DBWRAPPER->GetStudyObjectByUID(studyInstanceUidFk, studyptr);
		if (iRet == ERROR_DB_NULL)
		{
			patientUidFk = studyptr->GetPatientUIDFk();

			Mcsf::IPatientPtr patientPtr;
			iRet = DBWRAPPER->GetPatientObjectByUID(patientUidFk, patientPtr);
			if (iRet == ERROR_DB_NULL)
			{
				patientName = patientPtr->GetPatientName();
			}
		}
	}

	// get image information
	Mcsf::ISeriesBasePtr querySeriesObj;
	std::vector<Mcsf::IImagePtr> ImagePtrArray;

	std::string sConditon = "SeriesInstanceUIDFk='" + seriesName +"'";
	std::string sOrderBy = "InstanceNumber";
	iRet = DBWRAPPER->GetImageListByConditionWithOrder(sConditon, sOrderBy, ImagePtrArray);
	if (iRet == ERROR_DB_NULL)
	{
		DicomImageDescription *pDicomImageInf = 0;
		int imagesSize = ImagePtrArray.size();
		for (int i  =0; i < imagesSize; i ++)
		{
			image_files.push_back(ImagePtrArray[i]->GetFilePath());
		}
	}
}
#else

void ListAllFiles(std::string strDir, std::vector<std::string> &files)
{
	WIN32_FIND_DATAA fda;

	std::string strAllFiles;
	strAllFiles = strDir + "*.*";
	HANDLE handle = FindFirstFileA(strAllFiles.c_str(), &fda);
	if (handle == INVALID_HANDLE_VALUE)
		return;

	while (FindNextFileA(handle, &fda))
	{
		if (fda.cFileName[0] == '.')
		{
			continue;
		}

		if ((fda.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == false)
		{
			files.push_back(strDir + fda.cFileName);
		}
	}
}

void StudySeries::LoadSeriesImgFnamesFromDb(const std::string &seriesName, std::vector<std::string> &image_files)
{
//#define TEST_FILES
#ifdef TEST_FILES

	std::vector<std::string> files;
	if (seriesName == "1")
	{
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.132711499846128072556482677404546433123.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.166138144319394745197714093299508664076.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.184192307774319368122648577782952761667.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.192783140394988960939659018177170290911.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.194511120589774203609672941709150396678.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.2003253420986172195400308231923959679.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.227595525314611324438048963768386159269.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.243505411019033920545140639359456909557.dcm");
		files.push_back("z:\\temp\\dicom\\CT\\1.2.826.0.1.3680043.26388364640230214214366565033796682954.dcm");
	}
	else if (seriesName == "2")
	{
		files.push_back("Z:\\temp\\dicom\\bigdata\\0deda800.dcm");
		files.push_back("z:\\temp\\dicom\\XA\\1.3.46.670589.28.114114317508.201508200124310225982212221.dcm");
	}
	else if (seriesName == "3")
	{
		files.push_back("Z:\\temp\\dicom\\STUDY\\series1\\1.2.840.113704.7.1.1.6632.1127829031.2.dcm");
	}
	else if (seriesName == "4")
	{
		//ListAllFiles("Z:\\temp\\dicom\\STUDY\\series2\\", files);
		ListAllFiles("g:\\dicomimg\\img2016\\0419", files);
	}
	else if (seriesName == "5")
	{
		ListAllFiles("Z:\\temp\\dicom\\STUDY\\series2_more\\", files);
	}
	DicomImageDescription *pDicomImageInf = 0;
	for (int i = 0; i < files.size(); i ++)
	{
		image_files.push_back(files[i]);
	}

#else

	SeriesRecPtr seriesRecPtr = DBWRAPPER->GetSeriesByName(seriesName);
	if (seriesRecPtr.get() == NULL)
		return;
	
	ImageRecPtrList images = DBWRAPPER->GetImagesBySeriesId(seriesRecPtr->get_series_uid_id());
	for (ImageRecPtrListCIt cit = images.begin(); cit != images.end(); cit ++)
	{
		const ImageRecPtr &imageRecPtr = *cit;
		if (imageRecPtr.get() != NULL)
		{
			const std::string &strImageFileName = imageRecPtr->get_image_file();
			if (strImageFileName.length() > 0)
			{
				image_files.push_back(strImageFileName);
			}
		}
	}
#endif	
}
#endif

bool StudySeries::CacheNetDicomFile(const std::string &strFileName, std::string &strCacheNewFile)
{
	return m_SeriesFilesCache.CacheFile(strFileName, strCacheNewFile);
}

void StudySeries::LoadNewSeriesData(const std::string &seriesName)
{
	std::stringstream log;

	m_dicomSeriesDbInfo.Reset();
	
	std::vector<std::string> image_files;

	LoadSeriesImgFnamesFromDb(seriesName, image_files);

	DicomSeriesAttribsLoaderHP::LoadImageDicomAttribs(image_files, &m_dicomSeriesDbInfo);

	int imagesSize = m_dicomSeriesDbInfo.GetImagesCount();

	log << m_siteSeriesId.toString() << " Load image ok. [S]=" << imagesSize << ".";
	LOG_DEBUG(log.str());
}

void StudySeries::LoadSeries(const std::string &seriesName)
{
	if (this->m_siteSeriesId.sSeriesId != seriesName)
	{
		m_SeriesFilesCache.onNewSeriesLoad();

		ReleaseAllAllocatedResources();

		m_nFirstCellImageIndex = 0;
				
		this->m_siteSeriesId.sSeriesId = seriesName;

		LoadNewSeriesData(seriesName);
	}
	else
	{
		std::stringstream log;

		int imagesSize = m_dicomSeriesDbInfo.GetImagesCount();
		log << m_siteSeriesId.toString()  << " has loaded, and it's size = " << imagesSize << ".";
		LOG_INFO(log.str());
	}
}


void StudySeries::Release()
{
	delete this;
}


void StudySeries::EnableTransfSync(bool bEnable)
{
	m_bEnableTransfSync = bEnable;
}


void StudySeries::RearrangeCellImagesOnPersist(int firstCellImageIndex, int cellsCount, int width, int height, bool bCellImageChg)
{
	std::stringstream ssLog;
	// we need to 
	IStudySeriesImageCell *pImageCell = NULL;
	std::string strDicomFileName;
	DicomImageDescription *pDicomImageInfo = NULL;
	StudyImage *pStudyImage = NULL;
	BlankStudyImage *pBlankStudyImage = NULL;
	IStudyImage *pIStudyImage = NULL;
	DicomImageFrameId dicomImageFrameId;
	BlankStudyImagesV freeBlankImages = m_AllocatedBSIList;

	StudyImagePosObjMap  oldActiveCellsImages = m_CellAssignedSIMap;

	m_CellAssignedSIMap.clear();
	int curImageIndex = 0;
	for (int i = 0; i < cellsCount; i ++)
	{
		pImageCell = FindImageCellObj(i);

		if (pImageCell == NULL) return;

		curImageIndex = NormalizeImageIndex(firstCellImageIndex + i, cellsCount);
		pDicomImageInfo = m_dicomSeriesDbInfo.QueryDicomImageDescription(curImageIndex);
		if (pDicomImageInfo == NULL)
		{
			if (freeBlankImages.size() > 0)
			{
				pIStudyImage = *(freeBlankImages.begin());
				if (pIStudyImage == NULL) 
				{
					continue;
				}

				pImageCell->AssignNewStudyImage(pIStudyImage, pDicomImageInfo, width, height);
				freeBlankImages.erase(freeBlankImages.begin());
				continue;
			}
			else
			{
				pIStudyImage = NewStudyImageObj(i, true);
				pBlankStudyImage = dynamic_cast<BlankStudyImage *>(pIStudyImage);
				if (pBlankStudyImage == NULL)
				{
					DEL_PTR(pIStudyImage);
					continue;
				}
				pImageCell->AssignNewStudyImage(pIStudyImage, pDicomImageInfo, width, height);
				m_AllocatedBSIList.push_back(pBlankStudyImage);
			}
		}
		else
		{
			strDicomFileName = pDicomImageInfo->FilePath();
			dicomImageFrameId.set(strDicomFileName, pDicomImageInfo->DicomFrameIndex());
			StudyImageNameObjMapIt findStudyImageIt = m_AllocatedSINameObjsMap.find(dicomImageFrameId);
			if (findStudyImageIt != m_AllocatedSINameObjsMap.end())
			{
				pIStudyImage = findStudyImageIt->second;
				if (pIStudyImage == NULL) continue;

				pImageCell->AssignNewStudyImage(pIStudyImage, pDicomImageInfo, width, height);

				bool bFound = false;

				for (StudyImagePosObjMapIt it = oldActiveCellsImages.begin(); it != oldActiveCellsImages.end();
					it ++)
				{
					if (it->second == pIStudyImage)
					{
						bFound = true;
						oldActiveCellsImages.erase(it);
						break;
					}
				}

				if (bFound == false)
				{
					pIStudyImage->OnShowActive();
				}
			}
			else
			{	
				pIStudyImage = NewStudyImageObj(i, false);
				pStudyImage = dynamic_cast<StudyImage *>(pIStudyImage);
				if (pStudyImage == NULL) 
				{
					DEL_PTR(pIStudyImage);
					continue;
				}
				m_AllocatedSINameObjsMap[dicomImageFrameId] = pStudyImage;
				pImageCell->AssignNewStudyImage(pIStudyImage, pDicomImageInfo, width, height);
			}

			ssLog.str("");
			ssLog << std::hex << pImageCell << "(" << i << ") " << " to host " << pStudyImage;
			LOG_DEBUG(ssLog.str());
		}
	
		m_CellAssignedSIMap[i] = pIStudyImage;
	}
	
	for (StudyImagePosObjMapIt it = oldActiveCellsImages.begin(); it != oldActiveCellsImages.end();
		it ++)
	{
		it->second->OnShowDeactive();
	}

	OnRearrangeCellImagesDone(bCellImageChg);
}


IStudyImage * StudySeries::NewStudyImageObj(int imageCellPos, bool isBlank)
{
	IStudyImage *pIStudyImage = NULL;
	BlankStudyImage *pBlankStudyImage = NULL;
	StudyImage *pStudyImage = NULL;
	std::stringstream log;

	SeriesImageId seriesImageId;
	seriesImageId.siteSeriesId = m_siteSeriesId;
	seriesImageId.imageCellPos = imageCellPos;

	log << seriesImageId.toString();

	if (isBlank)
	{
		pIStudyImage = new BlankStudyImage(this, seriesImageId);
		
		log << " create blank image, 0x";
	}
	else
	{
		pIStudyImage = new StudyImage(this, seriesImageId);

		log << " create study image, 0x";
	}

	log << std::hex << pIStudyImage;
	LOG_DEBUG(log.str());

	return pIStudyImage;
}

void StudySeries::onSitePageToolTypeChanged(const ImageToolType &newToolType)
{
	for (StudySeriesCellPosObjMapMapIt it = m_AllocatedSCellPosObjMap.begin(); it != m_AllocatedSCellPosObjMap.end();
		it ++)
	{
		IStudySeriesImageCell *pIImageCell = dynamic_cast<IStudySeriesImageCell *>(it->second);
		StudySeriesImageCell *pImageCell = dynamic_cast<StudySeriesImageCell *>(pIImageCell);
		if (pImageCell)
		{
			pImageCell->onSitePageToolTypeChanged(newToolType);
		}
	}
}

void StudySeries::NotifyClient_SeriesStatusChanged()
{
	Mcsf2DEngineContainee::GetInstance()->NotifyClient_ReportSeriesStatus(
		&m_siteSeriesId, this);
}

MCSF_DJ2DENGINE_END_NAMESPACE
