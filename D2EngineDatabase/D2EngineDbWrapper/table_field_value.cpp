#include "StdAfx.h"
#include "table_field_value.h"
#include "ado2.h"

// boost::lexical_cast<int>(std::string)

void FieldValueSetter::UpdateObjData(boost::shared_ptr<CADORecordset> &retRecordPtr)
{
	CString cstrValue, cstrFileName;

	if (retRecordPtr->IsEOF() == false)
	{
		FieldNameInfMap &FieldsInfo = GetFieldsInfo();

		for (FieldNameInfMapIt it = FieldsInfo.begin(); it != FieldsInfo.end(); it ++)
		{
			const std::string &fieldName = (it->first);
			TableFieldInf &tfi = (it->second);

			cstrFileName = fieldName.c_str();
			if (tfi.FieldValueType == FIELD_VALUE_STR)
			{
				retRecordPtr->GetFieldValue(cstrFileName,  cstrValue);
				*static_cast<std::string *>(tfi.FieldValueAddr) = CStringA(cstrValue);
			}
			else if (tfi.FieldValueType == FIELD_VALUE_INT)
			{
				retRecordPtr->GetFieldValue(cstrFileName,  *static_cast<long *>(tfi.FieldValueAddr));
			}
			else if (tfi.FieldValueType == FIELD_VALUE_FLOAT)
			{
				double d;
				retRecordPtr->GetFieldValue(cstrFileName,  d);
				*static_cast<double *>(tfi.FieldValueAddr) =d;
			}

		}
	}
}
