#include "stdafx.h"
#include "blank_study_image.h"
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE
BlankStudyImage::BlankStudyImage(StudySeries *pParent, const SeriesImageId &seriesImageId) :
	IStudyImage(pParent, seriesImageId), m_blankBackgImageCompositor(this)
{
}


BlankStudyImage::~BlankStudyImage(void)
{
}

void BlankStudyImage::Release()
{
	delete this;
}

int BlankStudyImage::onMouseEvtCommandRequest(const MouseEvtRequestArgs *pMouseEvt)
{
	return 0;
}


int BlankStudyImage::onToolTypeChangedCommandRequest(const ToolChgRequestArgs *toolInformation)
{
	return 0;
}

int BlankStudyImage::onKeyboardEvtCommandRequest(const KeyboardEvtRequestArgs *keyboard)
{
	return 0;
}

bool BlankStudyImage::LoadImageFile(DicomImageDescription *pDicomImageAttrib, int width, int height)
{
	SetImageSizeValue(width, height);
	return m_blankBackgImageCompositor.Reset(width, height);
}

void BlankStudyImage::ChangeImageCellPos(int cellPos)
{
	IStudyImage::ChangeImageCellPos(cellPos);
}

void BlankStudyImage::onSyncTransformation(const TransformationArgs *p)
{
	return ;
}

void BlankStudyImage::onSyncSetImageWinWidthCenter(float fWinWidth, float fWinCenter, bool isAbsoluteVal, void *p)
{

}
void BlankStudyImage::onSyncInvertImageColor(void *p)
{
}

boost::shared_array<char> BlankStudyImage::CompositeImage(int &size) const
{
	size = 0;
	boost::shared_array<char> resultImageBufPtr;
	GraphicsImager *pResultImager = m_blankBackgImageCompositor.CloneComposite();
	if (pResultImager == NULL)
	{
		return resultImageBufPtr;
	}

	size = BlankBackgImageCompositor::SaveToBuffer(pResultImager, ENCODER_CLSID_JPG, resultImageBufPtr, m_strImageUidTag);
	
	delete pResultImager;

	return resultImageBufPtr;
}

MCSF_DJ2DENGINE_END_NAMESPACE
