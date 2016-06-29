#include "stdafx.h"
#include "mcsf_dj2dengine_utility.h"



MCSF_DJ2DENGINE_BEGIN_NAMESPACE


std::string StringFormatter::toString()
{
	if (_srcType == FORMATTER_SRC_STRING)
		return StrToString();
	else if (_srcType == FORMATTER_SRC_DOUBLE)
		return DbToString();
	else if (_srcType == FORMATTER_SRC_DATETIME)
		return DateToString();
	else
		return "";
}

void StringFormatter::DigFormatterCharact()
{
	int nBrackPartLeft = _formatterTxt.find_first_of('{');
	if (nBrackPartLeft < 0) 
	{
		_formCharact._strPre = _formatterTxt;
		return;
	}

	_formCharact._nBrackLeftPos = nBrackPartLeft;
	_formCharact._strPre = _formatterTxt.substr(0, nBrackPartLeft);

	int nBrackPartRight = _formatterTxt.find_first_of('}', nBrackPartLeft + 1);
	if (nBrackPartRight < 0) 
	{
		_formCharact._strPre = _formatterTxt;
		return;
	}

	_formCharact._nBrackRightPos = nBrackPartRight;
	_formCharact._strPost = _formatterTxt.substr(nBrackPartRight+1);

	_formCharact._strParam = _formatterTxt.substr(nBrackPartLeft + 1, nBrackPartRight - nBrackPartLeft -1);

	int nColonPos  =  _formatterTxt.find_first_of(':', nBrackPartLeft + 1);
	if (nColonPos < 0) 
	{
		_formCharact._strPosParam = _formatterTxt.substr(nBrackPartLeft + 1, nBrackPartRight - nBrackPartLeft -1);
		return;
	}

	_formCharact._strPosParam = _formatterTxt.substr(nBrackPartLeft + 1, nColonPos - nBrackPartLeft -1);

	_formCharact._nColonPos = nColonPos;

	int nDotPos = _formatterTxt.find_first_of('.', nColonPos + 1);
	if (nDotPos < 0)
	{
		_formCharact._strDbAttrIntParam = _formatterTxt.substr(nColonPos + 1, nBrackPartRight - nColonPos -1);
		return;
	}

	_formCharact._strDbAttrIntParam = _formatterTxt.substr(nColonPos + 1, nDotPos - nColonPos -1);
	_formCharact._strDbAttrPreParam = _formatterTxt.substr(nDotPos + 1, nBrackPartRight - nDotPos -1);
	_formCharact._nDotPos = nDotPos;
}
std::string StringFormatter::StrToString()
{
	std::stringstream ss;

	int len = -1;
	if (_formCharact._strPosParam.length() > 0 )
	{
		if (_formCharact._strDbAttrIntParam.length() > 0)
		{
			len = atoi(_formCharact._strDbAttrIntParam.c_str());
		}

		ss << _formCharact._strPre << _srcParamValue.substr(0, len) << _formCharact._strPost;
	}
	else
	{
		ss << _formCharact._strPre;
	}
	return ss.str();
}

std::string StringFormatter::DbToString()
{
	std::stringstream ss;

	std::string replacedStr;

	int precision = 0;
	
	if (_formCharact._strPosParam.length() >  0 )
	{
		double dParamValue = atof(_srcParamValue.c_str());

		if (_formCharact._strDbAttrPreParam.length() > 0)
		{
			precision = _formCharact._strDbAttrPreParam.length();
		}
		ss << setiosflags(ios::fixed /*| ios::showpoint*/) << setprecision(precision) 
			<< dParamValue;
		replacedStr = ss.str();
	}
	
	return  _formCharact._strPre + replacedStr + _formCharact._strPost;;
}

std::string StringFormatter::DateToString()
{
	if (_srcParamValue.length() != 8)
		return _srcParamValue;

	std::string strResult = _formCharact._strDbAttrIntParam;
	if (strResult.length() <= 0)
		return _srcParamValue;

	std::string strYear = _srcParamValue.substr(0, 4);
	std::string strMonth = _srcParamValue.substr(4, 2);
	std::string strDay = _srcParamValue.substr(6, 2);

	std::string strDateFormat = _formCharact._strDbAttrIntParam;
	int nFormtLen = strDateFormat.length();
	int nIndex = nFormtLen -1 , nYearIndex = 0, nMonthIndex = 0, nDayIndex = 0;

	while (nIndex >= 0)
	{
		char c = strResult.at(nIndex);

		if (c == 'd' || c == 'D')
		{
			if (nDayIndex >= 2) 
				continue;

			strResult[nIndex] = strDay.at(1 - nDayIndex);
			nDayIndex ++;
		}
		if (c == 'm' || c == 'M')
		{
			if (nMonthIndex >= 2)
				continue;

			strResult[nIndex] = strMonth.at(1 - nMonthIndex);
			nMonthIndex ++;
		}

		if (c == 'y' || c == 'Y')
		{
			if (nYearIndex >= 4)
				continue;

			strResult[nIndex] = strYear.at(3 - nYearIndex);
			nYearIndex ++;
		}
		
		nIndex --;
	}

	return _formCharact._strPre + strResult + _formCharact._strPost;
}

std::deque<std::string> StringDelimiter::DelimiteString(std::string str, std::string strDelim)
{
	std::deque<std::string> retQ;
	if (str.length() <= 0 || strDelim.length() <= 0)
		return retQ;

	char *szText = (char *)str.c_str();
	const char *szDelim = strDelim.c_str();

	char *sToken = strtok(szText, szDelim);
	while (sToken)
	{
		if (strlen(sToken) > 0)
		{
			retQ.push_back(sToken);
		}

		sToken = strtok(NULL, szDelim);
	}

	return retQ;
}
MCSF_DJ2DENGINE_END_NAMESPACE