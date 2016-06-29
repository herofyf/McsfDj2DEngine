#include "StdAfx.h"
#include "image_rec_operator.h"
#include "image_rec.h"
#include "ado2.h"
#include "d2engine_dbwrapper.h"

ImageRecOperator::ImageRecOperator(void)
{
}


ImageRecOperator::~ImageRecOperator(void)
{
}


 ImageRecPtrList ImageRecOperator::GetImagesBySeriesId(int seriesId)
{
	ImageRecPtrList imageRecList;

	std::stringstream ssQuery;
	ImageRecPtr imageRecPtr;

	ssQuery << "SELECT * FROM image1 WHERE series_uid_id= " << seriesId << "order by image_number";
		
	boost::shared_ptr<CADORecordset> adoRecordPtr;
	bool b = SqlServerWrapper::Instance()->QueryRecord(ssQuery.str(), adoRecordPtr);
	if (b)
	{
		adoRecordPtr->MoveFirst();
		while(adoRecordPtr->IsEOF() == false)
		{
			imageRecPtr = boost::make_shared<ImageRec>();
			imageRecPtr->UpdateObjData(adoRecordPtr);

			adoRecordPtr->MoveNext();
			imageRecList.push_back(imageRecPtr);
		}
	}
	
	return imageRecList;
}