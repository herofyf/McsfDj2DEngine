#include "stdafx.h"
#include "localizer_lines_ref_sides.h"
#include "study_series.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

LocalizerLinesRefSides::LocalizerLinesRefSides(void)
{
	m_pReferredSide = m_pReferringSide = NULL;
}


LocalizerLinesRefSides::~LocalizerLinesRefSides(void)
{
}


bool LocalizerLinesRefSides::IsRefSidesValid() const
{
	return ((m_pReferringSide != NULL) && (m_pReferredSide != NULL));
}

void LocalizerLinesRefSides::UpdateRefSide(const DicomSeriesDescription *pChangedSeries)
{
	if (pChangedSeries == NULL)
		return;

	StudySeries *pStudySeries = const_cast<StudySeries *>(pChangedSeries->MyStudySeries());
	if (pStudySeries == NULL)
		return;

	const LocalizerLinesResourceId *pLlResId = pStudySeries->GetLocalizerLinesResId();

	if (pLlResId->IsReferringSide())
		m_pReferringSide = pChangedSeries;
	else
		m_pReferredSide = pChangedSeries;
}

bool LlRefSidesResources::GetRefSidesResIdId(const DicomSeriesDescription *pChangedSeries, int id)
{
	if (pChangedSeries == NULL || pChangedSeries->MyStudySeries() == NULL)
		return false;

	const LocalizerLinesResourceId *pLlResId = const_cast<StudySeries *>(pChangedSeries->MyStudySeries())->GetLocalizerLinesResId();
	id = pLlResId->ResourceId();

	return true;
}

void LlRefSidesResources::onLlReferSidesChanged(const DicomSeriesDescription *pChangedSeries)
{
	if (pChangedSeries == NULL) 
		return;

	int illResId = 0;
	if (GetRefSidesResIdId(pChangedSeries, illResId) == false)
		return;
	
	LlRefSidesResMapIt it = m_Resources.find(illResId);
	if (it != m_Resources.end())
	{
		it->second.UpdateRefSide(pChangedSeries);
	}
	else
	{
		LocalizerLinesRefSides LiRefSides;
		LiRefSides.UpdateRefSide(pChangedSeries);
		m_Resources[illResId] = LiRefSides;
	}

}

bool LlRefSidesResources::IsValidReferSides(const LocalizerLinesResourceId *pLlResId) const
{
	if (pLlResId == NULL)
		return false;

	LlRefSidesResMapCIt it = m_Resources.find(pLlResId->ResourceId());
	if (it != m_Resources.end())
	{
		return it->second.IsRefSidesValid();
	}
	return false;
}

void LlRefSidesResources::NotifyRefSeriesRedrawLocalizerLine(const DicomSeriesDescription *pChangedSeries)
{
	int illResId = 0;
	if (GetRefSidesResIdId(pChangedSeries, illResId) == false)
		return;

	LlRefSidesResMapIt it = m_Resources.find(illResId);
	if (it != m_Resources.end())
	{
		NotifyRefSideRedrawLocalizerLine(it->second);
	}
}

void LlRefSidesResources::onLocalizerLineMove(const DicomSeriesDescription *pRefSeriesDesc,
	const DicomImageDescription *pImageDesc,
	int nRefedSeriesCellNum, PointF refImagePixel)
{
	int illResId = 0;
	if (GetRefSidesResIdId(pRefSeriesDesc, illResId) == false)
		return;

	LlRefSidesResMapIt it = m_Resources.find(illResId);
	if (it != m_Resources.end())
	{
		NotifyReferredSideLocalizerLineMove(it->second, pImageDesc, nRefedSeriesCellNum, refImagePixel);
	}
}

void LlRefSidesResources::NotifyRefSideRedrawLocalizerLine(const LocalizerLinesRefSides &sides)
{
	if (sides.IsRefSidesValid())
	{
		StudySeries *pStudySeries = const_cast<StudySeries *>(sides.GetReferringSide()->MyStudySeries());
		if (pStudySeries)
		{
			pStudySeries->OnLocalizerLinesRefSidesChanged(sides.GetReferredSide());
		}
	}
}

void LlRefSidesResources::ClearLlRefResource(const DicomSeriesDescription *pSeriesDesc)
{
	int illResId = 0;
	if (GetRefSidesResIdId(pSeriesDesc, illResId) == false)
		return;

	LlRefSidesResMapIt it = m_Resources.find(illResId);
	if (it != m_Resources.end())
	{
		LocalizerLinesRefSides sides = it->second;

		m_Resources.erase(it->first);

		NotifyRefSideRedrawLocalizerLine(sides);
	}
}

void LlRefSidesResources::NotifyReferredSideLocalizerLineMove(const LocalizerLinesRefSides &sides,
	const DicomImageDescription *pImageDesc,
	int nRefedSeriesCellNum, PointF refImagePixel)
{
	if (sides.IsRefSidesValid())
	{
		StudySeries *pStudySeries = const_cast<StudySeries *>(sides.GetReferredSide()->MyStudySeries());
		if (pStudySeries)
		{
			pStudySeries->OnReferSeriesLocalizerLineMove(sides.GetReferredSide(), pImageDesc, nRefedSeriesCellNum, refImagePixel);
		}
	}
}


MCSF_DJ2DENGINE_END_NAMESPACE