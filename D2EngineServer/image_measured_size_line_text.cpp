#include "stdafx.h"
#include "image_measured_size_line_text.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

void ImageMesauredSizeLineText::Reset(const char *szValue) 
{	
	if (szValue)
	{
		m_strText= szValue;
	}
	m_nWidth = m_nHeight = m_nUnitWidth = 0;
}

void ImageMesauredSizeLineText::copy(const ImageMesauredSizeLineText &a)
{
	this->m_strText = a.m_strText;
	this->m_nWidth = a.m_nWidth;
	this->m_nHeight = a.m_nHeight;
	this->m_nUnitWidth = a.m_nUnitWidth;
}

ImageMesauredSizeLineText &ImageMesauredSizeLineText::operator=(const ImageMesauredSizeLineText &a)
{
	copy(a);
	return *this;
}




MCSF_DJ2DENGINE_END_NAMESPACE