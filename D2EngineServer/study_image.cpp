#include "stdafx.h"
#include "study_image.h"
#include "mcsf_dj2dengine_containee.h"
#include "study_series.h"
#include "image_tool_interface.h"
#include <boost/shared_array.hpp>
#include "image_line_tool.h"
#include "trace_performance_func.h"
#include "dicom_component_information.h"
#include "image_angle_tool.h"
#include <algorithm>
#include "mcsf_dj2dengine_log.h"
#include "system_global_configuration.h"
#include <boost/filesystem.hpp>
#include "image_circle_tool.h"
#include <sstream>
#include "image_tool_interface.h"
#include "graphics_imager.h"
#include "image_free_hand_tool.h"
#include "study_series_image_cell.h"
#include "site_work_task.h"
#include "site_page.h"
#include "image_arrow_note_tool.h"
#include "mcsf_note_meta_operator.h"
#include "study_series_command_request_args_note.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

StudyImage::StudyImage(StudySeries *pParent, const SeriesImageId &seriesImageId) :
	IStudyImage(pParent, seriesImageId), m_imageMagnifyingGlassTool(this),
	m_backgImageCompositor(this),  m_pCurActiveMeasureTool(NULL),
	m_CoordinateTranslator(this), m_imageTranslateToolHelper(this, TranslateToolType),
	m_imageScaleToolHelper(this, ScaleToolType), m_imageRotateToolHelper(this, RotateToolType),
	m_imageSetWinCenterHelper(this, SetWinCenterWidthType), 
	m_itemsDrawingAppearance(this), m_pDicomImageDescription(NULL), m_bFlipX(false), m_bFlipY(false),
	m_bDicomImageColorInvert(false), m_pAccessoryDrawing(NULL),
	m_localizerLinesDrawing(this), m_bResetDrawing(false)
{
	m_imageTools.clear();
	m_lOperationFlag = IMAGE_OPERATION_FLAG_NONE;
	std::stringstream tempCompositedImageName;
	tempCompositedImageName << TEMP_IMAGE_DIR_NAME << "\\";
	tempCompositedImageName << this << ".bmp";
	m_strLogCompositedImageName = tempCompositedImageName.str();

	m_custComments = *(OwnerSite()->GetCustomizedComments());
}


void StudyImage::ChangeImageCellPos(int cellPos, IStudySeriesImageCell *pImageHostCell)
{
	IStudyImage::ChangeImageCellPos(cellPos, pImageHostCell);

	m_pCurActiveMeasureTool = NULL;
}


void StudyImage::ReleaseTools()
{
	MeasureToolListItr it;
	MeasureToolDrawing *pMeasureTool = NULL;
	for (it = m_imageTools.begin(); it != m_imageTools.end(); )
	{
		pMeasureTool = (*it);
		it = m_imageTools.erase(it);
		delete pMeasureTool;
	}
}

void StudyImage::NotifyToolsImageSizeChanged(float width, float height)
{
	MeasureToolListItr it;
	MeasureToolDrawing *pMeasureTool = NULL;
	for (it = m_imageTools.begin(); it != m_imageTools.end(); ++it)
	{
		(*it)->OnImageSizeChanged(width, height);
	}
}

StudyImage::~StudyImage(void)
{
	ReleaseTools();

	RemoveCompositedLogImage();
}



void StudyImage::RemoveCompositedLogImage()
{
// removed the temp composited image file
	boost::filesystem::path logCompositedImageName(m_strLogCompositedImageName);
	if (boost::filesystem::exists(logCompositedImageName))
	{
		boost::filesystem::remove(logCompositedImageName);
	}
}


int StudyImage::onToolTypeChangedCommandRequest(const ToolChgRequestArgs *pToolInformation)
{
	int iRet = 0;
	
	if (pToolInformation->toolType == FlipXType				||
		pToolInformation->toolType == FlipYType				||
		pToolInformation->toolType == ColorInvertType		||
		pToolInformation->toolType == SetWinCenterWidthType || 
		pToolInformation->toolType == RotateToolType)
	{
		if (pToolInformation->toolType == FlipXType)
		{
			m_bFlipX = !m_bFlipX;
			iRet = NotifySeriesFlip(true, true);
		}
		if (pToolInformation->toolType == FlipYType)
		{
			m_bFlipY = !m_bFlipY;
			iRet = NotifySeriesFlip(false, true);
		}

		if (pToolInformation->toolType == ColorInvertType)
		{
			m_bDicomImageColorInvert = !m_bDicomImageColorInvert;
			iRet = NotifySeriesInvertColor();
		}
		
		// only set value just once, otherwise, to let mouse change those values
		if (pToolInformation->toolType == SetWinCenterWidthType)
		{
			const WinWidthCenterRequestArgs *pWinWidthCenterRequestArgs = dynamic_cast<const WinWidthCenterRequestArgs*>(pToolInformation);
			if (pWinWidthCenterRequestArgs == NULL) 
				return false;
			
			if (pWinWidthCenterRequestArgs->HasValueSet())
			{
				// at here, just try to use another way :). let win width/center changed, to send image
				NotifySeriesSetWinWidthCenter(pWinWidthCenterRequestArgs->WinWidth(), 
											  pWinWidthCenterRequestArgs->WinCenter(), 
											  true);
			}
		}

		if (pToolInformation->toolType == RotateToolType)
		{
			const RotateToolChgRequestArgs *pRotateToolChgRequestArgs = dynamic_cast<const RotateToolChgRequestArgs*>(pToolInformation);
			if (pRotateToolChgRequestArgs == NULL) 
				return 0;

			if (pRotateToolChgRequestArgs->angle != 0)
			{
				iRet = NotifySeriesRotate(pRotateToolChgRequestArgs->angle);
			}
		}
	}
	else if (pToolInformation->toolType == ImageToolType::SetNotePropType)
	{
		NoteMetaInfoOperator *pNoteUpdater = dynamic_cast<NoteMetaInfoOperator *>(m_pCurActiveMeasureTool);
		const NoteToolRequestArgs *pNoteToolReqArgs = dynamic_cast<const NoteToolRequestArgs *>(pToolInformation);
		if (pNoteUpdater && pNoteToolReqArgs)
		{
			iRet = pNoteUpdater->UpdateMetaInforamtion(pNoteToolReqArgs->noteMetaInformation);
		}
	}

	if (iRet > 0)
	{
		RefreshRenderImage();
	}
	
	return iRet;
}

void StudyImage::StatisticsRectPixels(const Rect &rect, DicomRegionPixelsStatistics *p)
{
	m_backgImageCompositor.StatisticsRectPixels(rect, p);
}

void StudyImage::StatisticsCirclePixels(PointF centerPf, float fRadis, DicomRegionPixelsStatistics *p)
{
	m_backgImageCompositor.StatisticsCirclePixels(centerPf, fRadis, p);
}

void StudyImage::StatisticsFreeHandPixels(PointF *pPointFs, int count, DicomRegionPixelsStatistics *p)
{
	m_backgImageCompositor.StatisticsFreeHandPixels(pPointFs, count, p);
}

void StudyImage::TransformPts_Cell2OrgDicomImage(PointF *pPointF, int n)
{
	m_backgImageCompositor.TransformPts_Cell2OrgDicomImage(pPointF, n);
}

void StudyImage::TransformPts_OrgDicomImage2Cell(PointF *pPointF, int n)
{
	m_backgImageCompositor.TransformPts_OrgDicomImage2Cell(pPointF, n);
}

bool StudyImage::DispatchTransformationReq(const TransformationArgs *arg)
{
	bool b = m_backgImageCompositor.onTransformation(arg);
	if (b == false)
		return false;

	MeasureToolListItr it, temp_it;
	for (it = m_imageTools.begin(); it != m_imageTools.end(); )
	{
		temp_it = it;
		it ++;
		(*temp_it)->onTransformation(arg);
	}

	return true;
}

void StudyImage::ResetDrawing()
{
	if (m_bResetDrawing == false)
	{
		m_bResetDrawing = true;

		m_backgImageCompositor.ResetDrawing(true);
	}
}

bool StudyImage::ReloadImage()
{
	int nImageWidth = ImageWidth();
	int nImageHeight = ImageHeight();

	if (m_pDicomImageDescription == NULL || nImageWidth <= 0 || nImageHeight <= 0)
		return false;

	return LoadImageFile(m_pDicomImageDescription, nImageWidth, nImageHeight);
}

bool StudyImage::LoadImageFile(DicomImageDescription *pDicomImageDescription, int width, int height)
{
	std::string perfTag = GetSeriesImageId()->toLogString() + " LoadImageFile ";
	CPU_PERF_CAL cpuCal(perfTag.c_str());

	stringstream log;
	log << std::hex << this << ":";

	bool bRet = false, isFileChanged = false, isSizeChanged = false;
	
	if (width <= 10 || height <= 10 || pDicomImageDescription == NULL) return false;

	DicomImageFrameId newDicomImageFrameId;
	newDicomImageFrameId.set(pDicomImageDescription->FilePath(), pDicomImageDescription->DicomFrameIndex());

	isFileChanged = (m_dicomImageFrameId != newDicomImageFrameId);
	isSizeChanged = (ImageWidth() != width) || (ImageHeight() != height);
	if (isFileChanged || isSizeChanged || m_bResetDrawing)
	{
		m_pDicomImageDescription = pDicomImageDescription;

		// to put ahead, because the following sub components would get the new value
		SetImageSizeValue(width, height);

		m_itemsDrawingAppearance.OnImageSizeChanged();

		if (isFileChanged || m_bResetDrawing)
		{
			m_bResetDrawing = false;

			ReleaseTools();
			m_pCurActiveMeasureTool = NULL;

			// update dicom file name.
			m_dicomImageFrameId = newDicomImageFrameId;

			// set init value
			m_CoordinateTranslator.OnLoadNewImage( width, height);

		}
		else if (isSizeChanged)
		{
			NotifyToolsImageSizeChanged(width, height);
		}

		m_backgImageCompositor.LoadDicomImage(m_pDicomImageDescription, width, height);

		// based on old coordinate to made transformation,
		// after done to update the newer view port 
		m_CoordinateTranslator.OnResizeImage( width, height);

		log << m_strImageUidTag << " to load image " << m_dicomImageFrameId.DicomFileName();

		bRet = true;
	}
	else
	{
		log << m_strImageUidTag << " not to reload image " << m_dicomImageFrameId.DicomFileName() << ", because the same arguments.";
			
	}

	LOG_DEBUG(log.str());
	return bRet;
}

void StudyImage::DisableAccessoryDrawing()
{
	if (m_pAccessoryDrawing)
	{
		m_pAccessoryDrawing = NULL;
	}
}

void StudyImage::DoImageTransformation(const TransformationArgs *pTransformationArgs)
{
	// while doing transformation, disable drawing localizer line
	DisableAccessoryDrawing();

	DispatchTransformationReq(pTransformationArgs);
}

/*
	study image got message from series to sync transformation
*/
void StudyImage::onSyncTransformation(const TransformationArgs *pTransformationArgs)
{
	DoImageTransformation(pTransformationArgs);

	// because if this is trigger by myself, we should send based on request
	// but the other images of this series didn't got any request, we need to force to send
	if (pTransformationArgs->pSender != this)
	{
		RefreshRenderImage();
	}
}

void StudyImage::NotifySeriesTransformation(const TransformationArgs *transformationArgs)
{
	if (transformationArgs)
	{
		// to call onSyncTransformation by series
		m_pStudySeries->HandleSyncImageTransformation(transformationArgs);
	}
}

bool StudyImage::NotifySeriesFlip(bool isX /* = true */, bool bManualTransform)
{
	TransformationArgs transformationArgs;

	transformationArgs.SetupFlip(isX, bManualTransform, this);
	
	NotifySeriesTransformation(&transformationArgs);
	return true;
}


bool StudyImage::NotifySeriesRotate(float angle, bool bManualTransform)
{
	TransformationArgs transformationArgs;
	
	transformationArgs.SetupRotate(angle, bManualTransform, this);
	
	NotifySeriesTransformation(&transformationArgs);
	return true;
}

bool StudyImage::NotifySeriesScale(float scaleX, float scaleY, bool bManualTransform)
{
	TransformationArgs transformationArgs;
	
	transformationArgs.SetupScale(scaleX, scaleY, bManualTransform, this);

	NotifySeriesTransformation(&transformationArgs);
	return true;
}

bool StudyImage::NotifySeriesTranslation(float offsetX, float offsetY, bool bManualTransform)
{
	TransformationArgs transformationArgs;
	
	transformationArgs.SetupTranslation(offsetX, offsetY, bManualTransform, this);

	NotifySeriesTransformation(&transformationArgs);
	return true;
}

bool StudyImage::NotifyImageScale(float scaleX, float scaleY, bool bManualTransform /* = true */)
{
	TransformationArgs transformationArgs;

	transformationArgs.SetupScale(scaleX, scaleY, bManualTransform, this);

	DoImageTransformation(&transformationArgs);
	return true;
}

bool StudyImage::NotifyImageTranslation(float offsetX, float offsetY, bool bManualTransform /* = true */)
{
	TransformationArgs transformationArgs;
	
	transformationArgs.SetupTranslation(offsetX, offsetY, bManualTransform, this);

	DoImageTransformation(&transformationArgs);
	return true;
}

void StudyImage::onSyncInvertImageColor(void *pOwner)
{
	m_backgImageCompositor.InvertDicomImageColor();

	if (pOwner != this)
	{
		RefreshRenderImage();
	}
}

void StudyImage::onSyncSetImageWinWidthCenter(float fWinWidth, float fWinCenter, bool isAbsoluteVal, void *pOwner)
{
	bool b = m_backgImageCompositor.SetDicomWindowWidthCenter(fWinWidth, fWinCenter, isAbsoluteVal);

	// comment owner check not like other, just because this is set by client, not by mouse
	if (/*pOwner != this*/ b)
	{
		RefreshRenderImage();
	}
}

bool StudyImage::NotifySeriesSetWinWidthCenter(float fWinWidth, float fWinCenter, bool isAbsoluteVal)
{
	m_pStudySeries->HandleSyncImageSetWinWidthCenter(fWinWidth, fWinCenter, isAbsoluteVal, this);
	return true;
}

bool StudyImage::NotifySeriesInvertColor()
{
	m_pStudySeries->HandleSyncImageInvertColor(this);
	return true;
}

int StudyImage::DispatchActiveStateEvt(MeasureToolDrawing *pSender, bool bActive)
{
	bool bChanged = false;
	int iRet = 0;
	// sync to all of image tools active state
	// when line tool and triangle tool
	MeasureToolListItr it;
	MeasureToolList copyTools;
	copyTools = m_imageTools;
	for (it = copyTools.begin(); it != copyTools.end(); it ++)
	{
		iRet = (*it)->OnToolActiveStateEvent(pSender, bActive);
		bChanged = iRet > 0 ? true : bChanged;
	}

	return bChanged;
}

// the study tool processed the message and notify study image its status
int StudyImage::HandleToolActiveStateChanged(MeasureToolDrawing *pImageTool, bool bActive, bool bDispatchToolsEvt)
{
	int iRet = 0;

	if (bActive)
	{
		if (m_pCurActiveMeasureTool != pImageTool)
		{
			m_pCurActiveMeasureTool = pImageTool;
		}
	}
	else
	{
		if (m_pCurActiveMeasureTool == pImageTool)
		{
			m_pCurActiveMeasureTool = NULL;
		}
	}

	// CHANGE CURRENT AND NEED TO SYNC TO EVERY MEASURE TOOLS OBJECT
	if (bDispatchToolsEvt)
	{
		iRet = DispatchActiveStateEvt(pImageTool, bActive);
	}
	
	return iRet;
}

int StudyImage::CreateStudyToolOnMouseEvt(const MouseEvtRequestArgs *pMouseEventInfo)
{
	int iRet = 0;
	bool leftDown = pMouseEventInfo->leftDown;

	const ImageToolTypeContext *pImageToolTypeContext = GetToolTypeContext();

	if (m_pCurActiveMeasureTool == NULL && pImageToolTypeContext)
	{
		if (leftDown)	
		{
			switch(pImageToolTypeContext->ImageToolTypeVal())
			{
			case LineToolType:
				{
					m_pCurActiveMeasureTool = new ImageLineTool(this, LineToolType);
				}
				break;
			case AngleToolType:
				{
					m_pCurActiveMeasureTool = new ImageAngleTool(this, AngleToolType);
				}
				break;
			case CircleToolType:
				{
					m_pCurActiveMeasureTool = new ImageCircleTool(this, CircleToolType);
				}
				break;
			case FreeHandType:
				{
					m_pCurActiveMeasureTool = new ImageFreeHandTool(this, FreeHandType);
				}
				break;
			case ImageToolType::NoteToolType:
				{
					const ImageToolTypeContext *pImageToolTypeContext = GetToolTypeContext();
					if (pImageToolTypeContext)
					{
						if (pImageToolTypeContext->UseNoteType() == NoteToolArrowNote)
						{
							m_pCurActiveMeasureTool = new ImageArrowNoteTool(this, ImageToolType::NoteToolType);
						}
					}
				}
				break;
			}

			// current active object should handle this mouse event immediately.
			if (m_pCurActiveMeasureTool)
			{
				m_pCurActiveMeasureTool->OnMouseEvent(pMouseEventInfo);
			}
		}
	}

	return iRet;
}

bool StudyImage::DispatchToolsKeyboardReq(const KeyboardEvtRequestArgs *pKeyboard)
{
	bool bChanged = false;
	int iRet = 0;

	MeasureToolListItr it, temp_it;
	for (it = m_imageTools.begin(); it != m_imageTools.end(); )
	{
		temp_it = it;
		it ++;
		iRet = (*temp_it)->OnKeyboard(pKeyboard);
		bChanged = (iRet > 0) ? true : bChanged;
	}

	return bChanged;
}

int StudyImage::onKeyboardEvtCommandRequest(const KeyboardEvtRequestArgs *pKeyboard)
{
	bool bChanged = false;
	int iRet = 0;
	std::string strKey = pKeyboard->keyVal;
	MCSF_DJ2DENGINE_NAMESPACE::eKeyboardFeatureKeyType featureKey = pKeyboard->featureKey;
	if (featureKey == MCSF_DJ2DENGINE_NAMESPACE::FK_DEL)
	{
		if (m_pCurActiveMeasureTool != NULL)
		{
			delete m_pCurActiveMeasureTool;
			m_pCurActiveMeasureTool = NULL;
			iRet = 1;
		}
	}

	bChanged = (iRet > 0) ? true : bChanged;

	iRet = DispatchToolsKeyboardReq(pKeyboard);
	bChanged = (iRet > 0) ? true : bChanged;

	if (bChanged)
	{
		RefreshRenderImage();
	}
	return iRet;
}

bool StudyImage::RefreshRenderImage()
{
	return IStudyImage::RefreshRenderImage();
}

boost::shared_ptr<GraphicsImager> StudyImage::SnapshotBackground() const
{
	GraphicsImager *pCloneBackgroundImage = m_backgImageCompositor.CloneComposite();

	boost::shared_ptr<GraphicsImager> resultPtr(pCloneBackgroundImage);
	return resultPtr;
}

const GraphicsImager *StudyImage::PeekBackground() const 
{
	return m_backgImageCompositor.RawComposite();
}

bool StudyImage::IsPtInDicomImageRegion(float x, float y)
{
	bool b = m_backgImageCompositor.IsPtInDicomImageRegion(x, y);
	return b;
}

boost::shared_array<char> StudyImage::CompositeImage(int &size) const
{
	std::string perfTag = GetSeriesImageId()->toLogString() + " CompositeImage ";
	CPU_PERF_CAL cpuCal(perfTag.c_str());
	
	size = 0;
	boost::shared_array<char> resultImageBufPtr;

	MeasureToolListCItr it, temp_it;
	// compositor a picture to draw each action
	boost::shared_ptr<GraphicsImager> resultPtr = SnapshotBackground();

	GraphicsImager *pResultImager = resultPtr.get();
	if (pResultImager == NULL)
		return resultImageBufPtr;

	// process to draw image tool object
	// generate compositor
	for (it = m_imageTools.begin(); it != m_imageTools.end(); )
	{
		temp_it = it;
		it ++;
		(*temp_it)->Draw(pResultImager);
	}

	// to draw dynamic line
	if (m_pAccessoryDrawing)
	{
		m_pAccessoryDrawing->Draw(pResultImager);
	}

	m_imageMagnifyingGlassTool.Draw(pResultImager);

	m_backgImageCompositor.Draw(pResultImager);

	// get image result and convert to jpg to send client

	if (SystemGlobalResource::IsDumpImageOn())
	{
		std::stringstream log;
		
		BackgImageCompositor::SaveToFile(pResultImager, m_strLogCompositedImageName, ENCODER_CLSID_BMP);
		
		log << m_strImageUidTag << "-" << m_dicomImageFrameId.DicomFileName()  << " ==" << m_strLogCompositedImageName;
		LOG_INFO(log.str());
	}
	
	size = BackgImageCompositor::SaveToBuffer(pResultImager, ENCODER_CLSID_JPG, resultImageBufPtr, m_strImageUidTag);
	
	return resultImageBufPtr;
}


bool StudyImage::IsLocalizerLineMoving(const MouseEvtRequestArgs *requestArgs)
{
	if (m_pCurActiveMeasureTool == NULL)
	{
		if (m_pAccessoryDrawing)
		{
			if (IS_IN_TRANSFORMING(m_lOperationFlag) == FALSE)
			{
				m_pAccessoryDrawing->OnMouseEvent(requestArgs);

				if (IS_IN_REFLINE_MOVE(m_lOperationFlag))
					return true;
			}
			else
			{
				m_pAccessoryDrawing = NULL;
			}
		}
	}

	return false;
}

int StudyImage::onMouseEvtCommandRequest(const MouseEvtRequestArgs *requestArgs)
{
	if (requestArgs->leftDown)
	{
		CPU_PERF_CAL cal("StudyImage::onMouseEvtCommandRequest");
	}
	else
	{
		CLEAR_OPFLAG_ALL(m_lOperationFlag)
	}
	
	int iRet = 0;
	bool bChanged = false;

	if (IsLocalizerLineMoving(requestArgs))
		return false;
	
	if (OwnerPage()->IsTransfomationToolUsed())
	{
		iRet = HandleTransformationToolMouseEvt(requestArgs);

		bChanged = iRet > 0 ? true : bChanged;
	}
	else if (OwnerPage()->IsMeasureToolUsed() ||
			 OwnerPage()->IsHandModeToolUsed())
	{
		iRet = HandleMeasureToolMouseEvt(requestArgs);
		bChanged = iRet > 0 ? true : bChanged;
	}

	if (bChanged > 0)
	{
		RefreshRenderImage();
	}
	return iRet;
}

bool StudyImage::DispatchToolsMouseReq(const MouseEvtRequestArgs *requestArgs)
{
	bool bChanged = false;
	int iRet = 0;

	MeasureToolListItr lit;
	MeasureToolV copyTools;
	copyTools.insert(copyTools.end(), m_imageTools.begin(), m_imageTools.end());
	MeasureToolVItr vit;
	for (vit = copyTools.begin(); vit != copyTools.end(); vit ++)
	{
		lit = std::find(m_imageTools.begin(), m_imageTools.end(), *vit);
		if (lit != m_imageTools.end())
		{
			iRet = (*vit)->OnMouseEvent(requestArgs);
			bChanged = (iRet > 0) ? true : bChanged;
		}
	}

	return bChanged;
}


/*
	while using transformation tool, and got mouse changed events.
*/
int StudyImage::HandleTransformationToolMouseEvt(const MouseEvtRequestArgs *requestArgs)
{
	bool bChanged = false;
	int iRet = 0;

	if (requestArgs->leftDown == false)
	{
		CLEAR_TRANSFORMING(m_lOperationFlag);
	}

	if (IS_IN_TRANSFORMING(m_lOperationFlag) == false)
	{
		// first to check active object
		if (m_pCurActiveMeasureTool != NULL)
		{
			iRet = m_pCurActiveMeasureTool->OnMouseEvent(requestArgs);
		}
		else
		{
			iRet = DispatchToolsMouseReq(requestArgs);
		}
		bChanged = iRet > 0 ? true : bChanged;
	}
	// second if the click inactive object and we need to do transformation
	if (m_pCurActiveMeasureTool != NULL)
	{
		return bChanged;
	}
	
	if (requestArgs->leftDown)
	{
		SET_OPFLAG_TRANSFORMING(m_lOperationFlag);
	}

	const ImageToolTypeContext *pImageToolTypeContext = GetToolTypeContext();
	if (pImageToolTypeContext == NULL)
		return bChanged;

	ImageToolType imageToolType = pImageToolTypeContext->ImageToolTypeVal();
	if (imageToolType == TranslateToolType)
	{
		iRet = m_imageTranslateToolHelper.OnMouseEvent(requestArgs);
	}
	else if (imageToolType == ScaleToolType)
	{
		float factorX = pImageToolTypeContext->ScaleFactorX();
		float factorY = pImageToolTypeContext->ScaleFactorY();

		m_imageScaleToolHelper.SetScaleFactor(factorX, factorY);

		iRet = m_imageScaleToolHelper.OnMouseEvent(requestArgs);
	}
	else if (imageToolType == RotateToolType)
	{
		iRet = m_imageRotateToolHelper.OnMouseEvent(requestArgs);
	}
	else if (imageToolType == SetWinCenterWidthType)
	{
		iRet = m_imageSetWinCenterHelper.OnMouseEvent(requestArgs);
	}
	else if (imageToolType == MagnifyGlassType)
	{
		iRet = m_imageMagnifyingGlassTool.OnMouseEvent(requestArgs);
	}
	bChanged = iRet > 0 ? true : bChanged;

	return bChanged;
}


void StudyImage::OnShowActive()
{
	m_backgImageCompositor.OnShowActive();
}

void StudyImage::onSitePageToolTypeChanged(const ImageToolType &newToolType)
{
	// need to change active measure object.
	MeasureToolDrawing *pActiveTool = m_pCurActiveMeasureTool;
	
	HandleToolActiveStateChanged(NULL, true, true);

	bool bChanged = (pActiveTool != m_pCurActiveMeasureTool);
	if (bChanged == true)
	{
		RefreshRenderImage();
	}
}

const ImageToolTypeContext *StudyImage::GetToolTypeContext()
{
	SitePage *pSitePage = OwnerPage();
	if (pSitePage != NULL)
	{
		return pSitePage->GetToolTypeContext();
	}

	return NULL;
}

void StudyImage::DiscardUncompletedMeasureTool()
{
	MeasureToolDrawing *pActiveTool = m_pCurActiveMeasureTool;
	if (pActiveTool)
	{
		bool bCompleted = pActiveTool->IsShapeCompleted(); 
		if (bCompleted == false)
		{
			HandleToolActiveStateChanged(NULL, true, true);
		}
	}
}

void StudyImage::OnShowDeactive()
{
	m_backgImageCompositor.OnShowDeactive();
	
	DiscardUncompletedMeasureTool();
}

/*
	while using measure tool, and got mouse events;
*/
int StudyImage::HandleMeasureToolMouseEvt(const MouseEvtRequestArgs *requestArgs)
{
	int iRet = 0;
	bool bChanged = false;
	MeasureToolListItr it, temp_it;
	//CPU_PERF_CAL cal("StudyImage::HandleMeasureToolMouseEvt");
	// when there has active tool and the left mouse is down, only for current object
	if (m_pCurActiveMeasureTool && requestArgs->leftDown)
	{
		iRet = m_pCurActiveMeasureTool->OnMouseEvent(requestArgs);
		bChanged = (iRet > 0) ? true : bChanged;

		// if active changed, we need to inspect whether active someone
		if (m_pCurActiveMeasureTool == NULL)
		{
			iRet = DispatchToolsMouseReq(requestArgs);
		}
	}
	else
	{
		// 
		iRet = DispatchToolsMouseReq(requestArgs);
		bChanged = (iRet > 0) ? true : bChanged;

		iRet = CreateStudyToolOnMouseEvt(requestArgs);
		bChanged = (iRet > 0) ? true : bChanged;
	}

	return bChanged;
}

void StudyImage::onCommentTagsChanged()
{
	m_custComments = *(OwnerSite()->GetCustomizedComments());

	m_backgImageCompositor.onCommentTagsChanged();
}

void StudyImage::OnReferLineMoveToShowRefedSeriesImage(int nCellNum, PointF imagePixelLoc)
{
	// at here we can't let other objects get focus
	SET_OPFLAG_REFLINE_MOVE(m_lOperationFlag);
	
	OwnerSeries()->OnDrawingLocalizerLineMove(GetDicomImageDesc(),
		nCellNum, imagePixelLoc);
}

void StudyImage::ShowLocalizerLinesForReferredSeries(const DicomSeriesDescription *pReferredSeriesDesc)
{
	bool update = false;

	if (OwnerPage()->IsLocalizerLinesEnabled(OwnerSeries()->GetLocalizerLinesResId()))
	{
		bool b  = m_localizerLinesDrawing.CalculateReferenceLines(pReferredSeriesDesc);
		if (b)
		{
			if (m_pAccessoryDrawing == NULL)
			{
				m_pAccessoryDrawing = &m_localizerLinesDrawing;
			}
			update = true;
		}
	}
	else
	{
		if (m_pAccessoryDrawing != NULL)
		{
			m_pAccessoryDrawing = NULL;
			update = true;
		}
	}

	if (update)
	{
		RefreshRenderImage();
	}

}


void StudyImage::Release()
{
	delete this;
}

void StudyImage::AddTool(const MeasureToolDrawing *pImageTool)
{
	RemoveTool(pImageTool);

	m_imageTools.push_back(const_cast<MeasureToolDrawing*>(pImageTool));
}

void StudyImage::RemoveTool(const MeasureToolDrawing *pImageTool)
{
	MeasureToolListItr it, temp_it;
	for (it = m_imageTools.begin(); it != m_imageTools.end(); )
	{
		temp_it = it;
		it ++;
		if ((*temp_it) == pImageTool)
		{
			m_imageTools.erase(temp_it);
			break;
		}
	}
}

MCSF_DJ2DENGINE_END_NAMESPACE
