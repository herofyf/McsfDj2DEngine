#include "stdafx.h"
#include "backg_image_compositor.h"
#include <boost/make_shared.hpp>
#include <sstream>
#include "coordinate_converter.h"
#include "image_property_state.h"
#include "trace_performance_func.h"
#include "math_float_type.h"
#include "image_vertical_gauge_ruler.h"
#include "study_image.h"
#include "dicom_component_information.h"
#include "trace_performance_func.h"
#include "image_logic_unit_helper.h"
#include "study_series_command_request_args.h"
#include "geometry_math.h"
#include "graphics_imager.h"
#include "study_series_image_cell.h"
#include "site_work_task.h"
#include "d2engine_netfile_operator.h"
#include "series_cache_files.h"
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

BackgImageCompositor::BackgImageCompositor(StudyImage *pImage) :  TransformableDrawing(pImage),
	m_nWidth(0), m_nHeight(0), m_imageCommentsDrawer(pImage), 
	m_imageLengthUnitGauge(pImage), m_nDicomWinWidth(0), m_nDicomWinCenter(0), m_fOrgDicomImageWidth(0),
	m_fOrgDicomImageHeight(0), m_bDicomImageColorInvert(false), m_dicomImageHelper(pImage)
{
	m_pDicomImager =  m_pFinalDrawing = NULL;

	m_points[BACKI_P_LEFTTOP].X = m_points[BACKI_P_LEFTTOP].Y = 0;

	m_points[BACKI_P_RIGHTBOTTOM] = m_points[BACKI_P_LEFTBOTTOM] = m_points[BACKI_P_RIGHTTOP] = m_points[BACKI_P_LEFTTOP];
	
}


BackgImageCompositor::~BackgImageCompositor(void)
{
	DEL_PTR(m_pDicomImager);
	DEL_PTR(m_pFinalDrawing);
}

bool BackgImageCompositor::onApplyTransformationState(const TransformationArgs *curTransformationArgs)
{
	PointF points[BACKI_P_COUNT];
	memcpy(&points, &m_points, sizeof(m_points));

	bool b = TransformPointsByCellCoord(curTransformationArgs, points, BACKI_P_COUNT);
	if (b)
	{
		if (curTransformationArgs->transformationType == TRANS_SCALE)
		{
			float diffX1 = fabs(points[1].X - points[0].X);
			float diffX2 = fabs(points[2].X - points[1].X);
			float diffX = (diffX1 > diffX2) ? diffX1 : diffX2;

			float diffY1 = fabs(points[1].Y - points[0].Y);
			float diffY2 = fabs(points[2].Y - points[1].Y);
			float diffY = (diffY1 > diffY2) ? diffY1 : diffY2;

			if ((diffX < (m_nWidth / 10))  || (diffX > m_nWidth * 9) || 
				(diffY < (m_nHeight / 10)) || (diffY > (m_nHeight * 9)) )
			{
				return false;
			}

			LOGIC_U_OBJ(OwnerImage())->OnDicomImageScale(
				curTransformationArgs->args.scaleArgs.scaleX, 
				curTransformationArgs->args.scaleArgs.scaleY);
		}

		memcpy(&m_points, points, sizeof(m_points));

		m_imageLengthUnitGauge.onTransformation(curTransformationArgs);

	}

	return b;
}

bool BackgImageCompositor::onTransformation(const TransformationArgs *pTransformationArgs)
{
	bool b1 = onApplyTransformationState(pTransformationArgs);
	if (b1)
	{
		CompositeDicomBgImage();

		if (pTransformationArgs->bManualTransform)
		{
			// to notify cell to update
			IStudySeriesImageCell *pCellObj = CELL_OBJ(OwnerImage());
			if (pCellObj)
			{
				pCellObj->onTransformation(pTransformationArgs);
			}
		}
		
		TransformableDrawing::onTransformation(pTransformationArgs);

	}
	return b1;
}


void BackgImageCompositor::TransformPts_OrgDicomImage2Cell(PointF *pPointF, int n)
{
	ApplyTransformStatesOnPoints(pPointF, n);
}

void BackgImageCompositor::TransformPts_Cell2OrgDicomImage(PointF *pPointF, int n)
{
	ReverseTransformStatesOnPoints(pPointF, n);
}

void BackgImageCompositor::StatisticsRectPixels(const Rect &rect, DicomRegionPixelsStatistics *p)
{
	
#define RECT_POINTS 4

	PointF points[RECT_POINTS] = {
		PointF(rect.X, rect.Y),
		PointF(rect.X + rect.Width, rect.Y),
		PointF(rect.X, rect.Y + rect.Height),
		PointF(rect.X + rect.Width, rect.Y + rect.Height)
	};

	TransformPts_Cell2OrgDicomImage(points, RECT_POINTS);
	
	Rect newRect(points[0].X, points[0].Y, points[1].X - points[0].X, points[2].Y - points[0].Y);

	m_dicomImageHelper.StatisticsRectPixels(newRect, p);
}

void BackgImageCompositor::StatisticsCirclePixels(PointF centerPf, float fRadis, DicomRegionPixelsStatistics *p)
{
	
#define CIRCLE_REF_POINTS 2

	PointF points[CIRCLE_REF_POINTS] = {
		centerPf,
		PointF(centerPf.X + fRadis, centerPf.Y)
	};

	TransformPts_Cell2OrgDicomImage(points, CIRCLE_REF_POINTS);
	
	float fNewRadius = CalLineLen(points[0].X, points[0].Y, points[1].X, points[1].Y);
	m_dicomImageHelper.StatisticsCirclePixels(points[0], fNewRadius, p);
}

void BackgImageCompositor::StatisticsFreeHandPixels(PointF *pPointFs, int count, DicomRegionPixelsStatistics *p)
{
	if (pPointFs == NULL || count == 0 || p == NULL) return;

	TransformPts_Cell2OrgDicomImage(pPointFs, count);

	m_dicomImageHelper.StatisticsFreeHandPixels(pPointFs, count, p);
}

int BackgImageCompositor::OnMouseEvent(const MouseEvtRequestArgs *mouseEventInfo)
{
	return 0;
}

int BackgImageCompositor::OnKeyboard(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

/*
	if pImage == NULL, Means compositor dicom with background
	otherwise, means to compositor image with background
*/
bool BackgImageCompositor::Draw(GraphicsImager *pImager) const
{
	//CPU_PERF_CAL cal("BackgImageCompositor::Draw");
	bool bChanged = false;

	bool b1 = m_imageCommentsDrawer.Draw(pImager);
	bChanged = b1 ? true : bChanged;

	b1 = m_imageLengthUnitGauge.Draw(pImager);
	bChanged = b1 ? true : bChanged;

	return bChanged;
}

void BackgImageCompositor::CompositeDicomBgImage()
{
	CPU_PERF_CAL cal("BackgImageCompositor::CompositeFinalImage");
	
	DrawImageColorMaskType type = None;
	// no fore image, just return background
	if (m_pDicomImager)
	{
		DEL_PTR(m_pFinalDrawing);

		m_pFinalDrawing = NewBackImager();

		type = m_bDicomImageColorInvert ? Negate : None;
		m_pFinalDrawing->DrawImage_QS(m_pDicomImager, m_points, 3, type);
	}
}

// to drawing the effect image following the points after transformation in background picture
GraphicsImager *BackgImageCompositor::CloneComposite() const
{
	//CPU_PERF_CAL cal("BackgImageCompositor::CloneComposite");
	if (m_pFinalDrawing)
	{
		return m_pFinalDrawing->Clone();
	}
	
	return NULL;
}

const GraphicsImager *BackgImageCompositor::RawComposite() const
{
	return m_pFinalDrawing;
}

GraphicsImager *BackgImageCompositor::NewBackImager()
{
	GraphicsImager *pBkImager = new GraphicsImager(m_nWidth, m_nHeight, SystemGlobalResource::MediateImagePixelFormat());
	Color color(0, 0, 0);
	//m_pBkImager->FillRectangle(color, 0, 0, m_nWidth, m_nHeight);
	pBkImager->Clear(color);
	return pBkImager;
}

int BackgImageCompositor::OnImageSizeChanged(float width, float height)
{
	return 0;
}

bool BackgImageCompositor::InvertDicomImageColor()
{
	if (m_pDicomImager)
	{
		m_bDicomImageColorInvert = !m_bDicomImageColorInvert;

		CompositeDicomBgImage();

		IStudySeriesImageCell *pCellObj = CELL_OBJ(OwnerImage());
		if (pCellObj)
		{
			pCellObj->onImageColorInverted(m_bDicomImageColorInvert);
		}
		return true;
	}

	return false;
}

bool BackgImageCompositor::OnShowActive()
{
	if (m_pDicomImager == NULL || m_pFinalDrawing == NULL)
	{
		return RebuildFinalDrawing();
	}

	return false;
}

bool BackgImageCompositor::OnShowDeactive()
{
	DEL_PTR(m_pDicomImager);

	DEL_PTR(m_pFinalDrawing);

	m_dicomImageHelper.ReleaseDicomResource();
	return true;
}

void BackgImageCompositor::onCommentTagsChanged()
{
	SetupImageComments();
}

bool BackgImageCompositor::ReadDicomImageFile(const std::string &strFileName, int frameIndex, DicomImageDescription *pDicomImageDescription)
{
	std::stringstream ssLog;

	if (strFileName.size() <= 0) 
		return false;

	std::string strCacheNewFileName;

	bool b = OwnerImage()->OwnerSeries()->CacheNetDicomFile(strFileName, strCacheNewFileName);
	if (b == false)
	{
		ssLog <<  strFileName << " <-> " << strCacheNewFileName << "cached file failed";
		LOG_INFO(ssLog.str());
		return false;
	}
	if (m_dicomImageHelper.LoadDicomFile(strCacheNewFileName, frameIndex, pDicomImageDescription) == false )
	{
		ssLog <<  strFileName << " <-> " << strCacheNewFileName << " load cached dicom failed.";
		LOG_INFO(ssLog.str());
		return false;
	}
	return true;
}

bool BackgImageCompositor::RebuildFinalDrawing()
{
	//m_dicomImageHelper.DoScaleS(m_dicomImageRectF.Width, m_dicomImageRectF.Height);
	if (false == ReadDicomImageFile(m_dicomImageFrameId.DicomFileName(), m_dicomImageFrameId.DicomFrameIndex(), NULL))
		return false;

	GraphicsImager *pGraphicsImager = NewDicomImager(m_nDicomWinCenter,  m_nDicomWinWidth);
	if (pGraphicsImager == NULL)
	{
		LOG_INFO("new graphic image object failed.");
		return false;
	}

	DEL_PTR(m_pDicomImager);
	m_pDicomImager = pGraphicsImager;

	CompositeDicomBgImage();

	return true;
}

bool BackgImageCompositor::SetDicomWindowWidthCenter(float widthFactor, float centerFactor, bool isAbsValue)
{
	if (isAbsValue)
	{
		if (centerFactor <= 0 || widthFactor <= 0 ||
			(centerFactor == m_nDicomWinCenter && widthFactor == m_nDicomWinWidth)) 
		{
			return false;
		}
	}
	else
	{
		if (centerFactor < -1 || centerFactor > 1 || centerFactor == 0 ||
			widthFactor  < -1 || widthFactor > 1  || widthFactor == 0)
		{
			return false;
		}

	}
	
	float oldWinWidth  = m_nDicomWinWidth;
	float oldWinCenter = m_nDicomWinCenter;

	if (isAbsValue)
	{
		m_nDicomWinWidth = widthFactor;
		m_nDicomWinCenter= centerFactor;
	}
	else
	{
		float widthChanged = m_nDicomWinWidth * widthFactor;
		float centerChanged = m_nDicomWinCenter * centerFactor;

		m_nDicomWinWidth  += widthChanged;
		m_nDicomWinCenter += centerChanged;
	}
	
	bool b = RebuildFinalDrawing();
	if (b)
	{
		IStudySeriesImageCell *pCellObj = CELL_OBJ(OwnerImage());
		if (pCellObj)
		{
			pCellObj->onImageWinWidthCenterChanged(m_nDicomWinWidth, m_nDicomWinCenter);

			DcmTagKey updKey = DCM_WindowCenter;
			SetupImageComments(&updKey);

			updKey = DCM_WindowWidth;
			SetupImageComments(&updKey);
		}
		return true;
	}

	m_nDicomWinWidth  = oldWinWidth;
	m_nDicomWinCenter = oldWinCenter;
	return false;
}

GraphicsImager *BackgImageCompositor::NewDicomImager(int newWinCenter, int newWinWidth)
{
	GraphicsImager *pResultGraphicsImager = NULL;

	m_dicomImageHelper.ChangeWinWidthCenter(newWinCenter, newWinWidth);

	pResultGraphicsImager = m_dicomImageHelper.SaveDicomFrameToImager(SystemGlobalResource::MediateImagePixelFormat());

	return pResultGraphicsImager;
}

bool BackgImageCompositor::CalDicomAutoFitInformation(DicomImageDescription *pDicomImageAttrib, int nImageWidth, int nImageHeight, RectF &dicomRectF, float &fOrgDicomImageWidth, float &fOrgDicomImageHeight)
{
	const int orgDicomWidth = m_dicomImageHelper.GetDicomWidth();
	const int orgDicomHeight = m_dicomImageHelper.GetDicomHeight();
	if (orgDicomHeight == 0 || orgDicomWidth == 0) return NULL;

	fOrgDicomImageWidth  = orgDicomWidth;
	fOrgDicomImageHeight = orgDicomHeight;

	// each side has 0.1 space margin
	float actWidth =  nImageWidth * 0.8;

	//  extent width
	float scaleWidthFactor = (float) actWidth / (float) orgDicomWidth ;
	//  extent height
	float scaleHeightFactor =(float) nImageHeight/ (float) orgDicomHeight;
	
	// scaleWidthFactor < scaleHeightFactor
	// height is more extent than width, height has margin, width with 10% with left and right
	if (isLessThan_d(scaleWidthFactor, scaleHeightFactor))
	{
		dicomRectF.Width = actWidth;
		// left 
		dicomRectF.X = (nImageWidth - dicomRectF.Width) / 2;
		// recalculate width let left, right has equal length
		dicomRectF.Width = nImageWidth - dicomRectF.X * 2;

		// top and bottom has equal margin
		dicomRectF.Height = orgDicomHeight * scaleWidthFactor;
		int margin = nImageHeight - dicomRectF.Height;
		int topMargin = margin / 2;
		dicomRectF.Height = nImageHeight - topMargin  * 2;
		dicomRectF.Y = abs(topMargin);
	}
	//  scaleWidthFactor > scaleHeightFactor
	//  width is more bigger than height, height fill with no margin
	else if (isGreaterThan_d(scaleWidthFactor , scaleHeightFactor))
	{
		dicomRectF.Height = nImageHeight;
		dicomRectF.Y = 0;

		// left and right
		dicomRectF.Width = orgDicomWidth * scaleHeightFactor;
		int margin = nImageWidth - dicomRectF.Width;
		int leftMargin = margin / 2;
		dicomRectF.Width = nImageWidth - leftMargin * 2;
		dicomRectF.X = abs(leftMargin);
	}
	else
	{
		dicomRectF.Y = 0;
		dicomRectF.Height = nImageHeight;
		
		dicomRectF.Width = actWidth;
		dicomRectF.X = (nImageWidth - actWidth) / 2;
		dicomRectF.Width = nImageWidth - dicomRectF.X * 2;
	}

	return true;
}

bool BackgImageCompositor::IsPtInDicomImageRegion(float x, float y)
{
	// the points should be continuous clockwise
	PointF points[] = {
		m_points[BACKI_P_LEFTTOP],
		m_points[BACKI_P_RIGHTTOP],
		m_points[BACKI_P_RIGHTBOTTOM],
		m_points[BACKI_P_LEFTBOTTOM]
	};

	return IsPointInPolygon(points, (sizeof(points) / sizeof(points[0])), x, y);
}

IStudySeriesImageCell *BackgImageCompositor::GetImageCellObj()
{
	IStudySeriesImageCell *pCellObj = CELL_OBJ(OwnerImage());

	StudySeriesImageCell *pStateCellObj = dynamic_cast<StudySeriesImageCell *>(pCellObj);
	
	return pStateCellObj;
}

void BackgImageCompositor::ApplyCellColorInvertOnImage()
{
	IStudySeriesImageCell *pStateCellObj = GetImageCellObj();

	if (pStateCellObj && pStateCellObj->IsImageTransfSyncEnabled())
	{
		m_bDicomImageColorInvert = pStateCellObj->IsColorInverted();
	}
}

bool BackgImageCompositor::GetCellSyncWinWidthCenter(int &nResultWinWidth, int &nResultWinCenter)
{
	bool bResult = false;

	IStudySeriesImageCell *pStateCellObj = GetImageCellObj();

	if (pStateCellObj && pStateCellObj->IsImageTransfSyncEnabled())
	{
		int nWinCenter = pStateCellObj->GetWindowCenter();
		if (nWinCenter > 0) 
		{
			nResultWinCenter = nWinCenter;
			bResult = true;
		}

		int nWinWidth = pStateCellObj->GetWindowWidth();
		if (nWinWidth > 0) 
		{
			nResultWinWidth = nWinWidth;
			bResult = true;
		}
		return bResult;
	}

	return bResult;
}

void BackgImageCompositor::ApplyCellTransformStatesOnImage()
{
	IStudySeriesImageCell *pStateCellObj = GetImageCellObj();

	if (pStateCellObj && pStateCellObj->IsImageTransfSyncEnabled())
	{
		const TransformReqsQ *pTreqs = pStateCellObj->GetTransformReqs()->GetTransformReqsQ();
		if (pTreqs)
		{
			for (TransformReqsQCIt cit = pTreqs->begin(); cit != pTreqs->end(); cit ++)
			{
				bool b1 = onApplyTransformationState(&(*cit));
				if (b1)
				{
					TransformableDrawing::onTransformation(&(*cit));
				}
			}
			
		}
	}
}

void BackgImageCompositor::ArrangeAutoFitDicomImage(RectF &dicomImageRectF, float fOrgDicomImageWidth, float fOrgDicomImageHeight)
{
	// update based on scale action
	float scaleX = ( ((float) (dicomImageRectF.Width)  / fOrgDicomImageWidth )  -1);
	float scaleY = ( ((float) (dicomImageRectF.Height) / fOrgDicomImageHeight) - 1);
	LOGIC_U_OBJ(OwnerImage())->OnDicomImageScale(scaleX, scaleY);

	Matrix m;
	m.Translate(-fOrgDicomImageWidth / 2, -fOrgDicomImageHeight / 2, MatrixOrderAppend);
	m.Scale(1+ scaleX, 1+scaleY, MatrixOrderAppend);
	m.Translate(dicomImageRectF.X + dicomImageRectF.Width / 2, 
		dicomImageRectF.Y + dicomImageRectF.Height / 2, MatrixOrderAppend);

	AppendTransforms(&m);
	m_points[BACKI_P_LEFTTOP].X = 0;
	m_points[BACKI_P_LEFTTOP].Y = 0;

	m_points[BACKI_P_RIGHTTOP].X = fOrgDicomImageWidth;
	m_points[BACKI_P_RIGHTTOP].Y = 0;

	m_points[BACKI_P_LEFTBOTTOM].X = 0;
	m_points[BACKI_P_LEFTBOTTOM].Y = fOrgDicomImageHeight;

	m_points[BACKI_P_RIGHTBOTTOM].X = fOrgDicomImageWidth;
	m_points[BACKI_P_RIGHTBOTTOM].Y = fOrgDicomImageHeight;

	m.TransformPoints(m_points, 4);

}

bool BackgImageCompositor::LoadDicomImage(DicomImageDescription *pDicomImageDescription, int width, int height)
{
	int iRet = 0, offsetX = 0, offsetY = 0;
	
	DicomImageFrameId newDicomImageFrameId;
	newDicomImageFrameId.set(pDicomImageDescription->FilePath(), pDicomImageDescription->DicomFrameIndex());
	bool isFileChanged = (m_dicomImageFrameId != newDicomImageFrameId);
	bool isSizeChanged = (m_nWidth != width) || (m_nHeight != height);

	RectF dicomImageRectF;
	
	if (false == ReadDicomImageFile(pDicomImageDescription->FilePath(), pDicomImageDescription->DicomFrameIndex(), pDicomImageDescription))
		return false;


	// init dicom
	bool bRet = CalDicomAutoFitInformation(pDicomImageDescription, width, height, dicomImageRectF, m_fOrgDicomImageWidth,  m_fOrgDicomImageHeight);
	if (bRet == false) 
		return false;

	int newWinWidth = m_nDicomWinWidth, newWinCenter = m_nDicomWinCenter;
	if (isFileChanged)
	{
		newWinWidth = pDicomImageDescription->WindowWidth();
		newWinCenter = pDicomImageDescription->WindowCenter();

		GetCellSyncWinWidthCenter(newWinWidth, newWinCenter);
	}

	GraphicsImager *pGraphicsManager = NewDicomImager(newWinCenter, newWinWidth);
	if (pGraphicsManager == NULL) 
		return false;
	
	DEL_PTR(m_pDicomImager);
	DEL_PTR(m_pFinalDrawing);

	m_pDicomImager = pGraphicsManager;

	if (isFileChanged || NeedResetDrawing())
	{
		ResetDrawing(false);

		m_nDicomWinWidth = newWinWidth;
		m_nDicomWinCenter = newWinCenter;

		SetupImageComments();

		ResetTransformStates();

		m_nWidth = width;
		m_nHeight = height;

		// because use new dicom image, which scale causes the logic unit changes.
		// first to set initialized value
		LOGIC_U_OBJ(OwnerImage())->Reset(pDicomImageDescription->PhysicalUnitType(), pDicomImageDescription->PixelSpaceX(),
			pDicomImageDescription->PixelSpaceY());

		ArrangeAutoFitDicomImage(dicomImageRectF, m_fOrgDicomImageWidth, m_fOrgDicomImageHeight);

		ApplyCellTransformStatesOnImage();
		ApplyCellColorInvertOnImage();

		m_dicomImageFrameId = newDicomImageFrameId;
	}
	else 
	{
		m_imageCommentsDrawer.OnImageSizeChanged(width, height);
		
		offsetX = (width - m_nWidth) / 2;
		offsetY = (height -m_nHeight) / 2;
		m_nWidth = width;
		m_nHeight = height;

		// the ratio between old dicom rect and the rect after changed dicom size
		float scaleX = ( dicomImageRectF.Width /  m_dicomImageRectF.Width  -1);
		float scaleY = ( dicomImageRectF.Height / m_dicomImageRectF.Height -1);
		// ##### attention, we not only let my image know the transformation and also 
		// let all of components in this image know, there have transformations.
		OwnerImage()->NotifyImageScale(scaleX, scaleY, false);
		
		OwnerImage()->NotifyImageTranslation(offsetX, offsetY, false);
	}
	
	if (m_pFinalDrawing == NULL)
	{
		CompositeDicomBgImage();
	}
	// scale cause logic unit changed, so need to select ruler again
	m_imageLengthUnitGauge.ReselectGaugeRuler();

	m_dicomImageRectF = dicomImageRectF;
	
	return true;
}

void BackgImageCompositor::SetupImageComments(const DcmTagKey *pUpdatedKey)
{
	CUSTOMIZED_COMMENTS_DEQ *pCommTags = OwnerImage()->GetCustComments();
	if (pCommTags)
	{
		// init comment
		std::string strVal;
		std::stringstream ssVal;
		
		bool bRet;
		DcmTagKey dcmTagKey;
		for (CUSTOMIZED_COMMENTS_DEQ_IT cit = pCommTags->begin(); cit != pCommTags->end(); cit ++)
		{
			const CUSTOMIZED_COMMENT_LINE_DEQ *pCommLineDeq = cit->GetLineComments();
			for(CUSTOMIZED_COMMENT_LINE_DEQ_CIT line_cit = pCommLineDeq->begin(); line_cit != pCommLineDeq->end(); line_cit ++)
			{
				CustomizedPosDcmTagComment &dcmTagComm =const_cast<CustomizedPosDcmTagComment &> (*line_cit);

				dcmTagKey = dcmTagComm.DcmCommentTagKey();

				if (pUpdatedKey && *pUpdatedKey != dcmTagKey)
					continue;

				if (dcmTagKey == DCM_WindowCenter || dcmTagKey == DCM_WindowWidth)
				{
					ssVal.str("");
					if (dcmTagKey == DCM_WindowWidth)
						ssVal << m_nDicomWinWidth;

					if (dcmTagKey == DCM_WindowCenter)
						ssVal << m_nDicomWinCenter;

					strVal = ssVal.str();
					bRet = true;
				}
				else
				{
					bRet = m_dicomImageHelper.RetriveTagStrValue(dcmTagKey.getGroup(), dcmTagKey.getElement(), strVal);
				}
				
				if (bRet == false)
				{
					continue;
				}

				dcmTagComm.CalDcmTagFormatValue(strVal);
			}
		}

		m_imageCommentsDrawer.Reset();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 the empty blank black study image
*/
BlankBackgImageCompositor::BlankBackgImageCompositor(IStudyImage *pImage) :
	m_pParentStudyImage(pImage), m_pFinalDrawing(NULL), m_nHeight(0), m_nWidth(0)
{

}

BlankBackgImageCompositor::~BlankBackgImageCompositor()
{
	DEL_PTR(m_pFinalDrawing);
}
bool BlankBackgImageCompositor::Reset(int width, int height)
{
	if (m_nWidth != width || m_nHeight != height)
	{
		m_nWidth = width;
		m_nHeight = height;
		DEL_PTR(m_pFinalDrawing);

		m_pFinalDrawing = new GraphicsImager(width, height, SystemGlobalResource::MediateImagePixelFormat());
		Color color(0, 0, 0);
		//m_pBkImager->FillRectangle(color, 0, 0, m_nWidth, m_nHeight);
		m_pFinalDrawing->Clear(color);
		return true;
	}

	return false;
}

GraphicsImager *BlankBackgImageCompositor::CloneComposite() const
{
	if (m_pFinalDrawing)
	{
		return m_pFinalDrawing->Clone();
	}

	return NULL;
}



MCSF_DJ2DENGINE_END_NAMESPACE
