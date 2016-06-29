#include "StdAfx.h"
#include "series_rec_operator.h"
#include "d2engine_dbwrapper.h"
#include <sstream>
#include "ado2.h"

SeriesRecOperator::SeriesRecOperator(void)
{
}


SeriesRecOperator::~SeriesRecOperator(void)
{
}


SeriesRecPtr SeriesRecOperator::GetSeriesByName(const std::string &strSeriesName)
{
	std::stringstream ssQuery;
	SeriesRecPtr seriesRecPtr;
	
	ssQuery << "SELECT * FROM series1 WHERE series_uid='" << strSeriesName << "'";
	
	boost::shared_ptr<CADORecordset> adoRecordPtr;
	bool b = SqlServerWrapper::Instance()->QueryRecord(ssQuery.str(), adoRecordPtr);
	if (b)
	{
		adoRecordPtr->MoveFirst();
		seriesRecPtr = boost::make_shared<SeriesRec>();
		seriesRecPtr->UpdateObjData(adoRecordPtr);
	}
	return seriesRecPtr;
}