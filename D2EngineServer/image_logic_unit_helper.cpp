#include "stdafx.h"
#include "image_logic_unit_helper.h"
#include <sstream>
#include "geometry_math.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

LogicUnitValuef ImageLogicUnitHelper::CalVerticalLineLogicLength(PointF *points, int n)
{
	LogicUnitValuef logicUnitValf;
	if (points == NULL || n != 2)
		return logicUnitValf;

	float diffx = fabsf(points[1].X - points[0].X);
	float diffy = fabsf(points[1].Y - points[0].Y);

	float logicY = diffy * m_fCurYLogicUnitVal;
	
	NormalizeLogicUnit(logicY, logicUnitValf);
	return logicUnitValf;
}

LogicUnitValuef ImageLogicUnitHelper::CalHorizonLineLogicLength(PointF *points, int n)
{
	LogicUnitValuef logicUnitValf;
	if (points == NULL || n != 2)
		return logicUnitValf;

	float diffx = fabsf(points[1].X - points[0].X);
	float diffy = fabsf(points[1].Y - points[0].Y);

	float logicX= diffx * m_fCurXLogicUnitVal;

	NormalizeLogicUnit(logicX, logicUnitValf);
	
	return logicUnitValf;
}

LogicUnitValuef ImageLogicUnitHelper::CalLineLogicLength(PointF *points, int n)
{
	LogicUnitValuef logicUnitValf;
	if (points == NULL || n != 2)
		return logicUnitValf;

	float diffx = fabsf(points[1].X - points[0].X);
	float diffy = fabsf(points[1].Y - points[0].Y);

	float logicX= diffx * m_fCurXLogicUnitVal;
	float logicY = diffy * m_fCurYLogicUnitVal;

	float logicLength = sqrt(pow(logicX,2) + pow(logicY,2));
	NormalizeLogicUnit(logicLength, logicUnitValf);

	return logicUnitValf;
}

LogicUnitValuef ImageLogicUnitHelper::CalRawDicomPixelsLogicArea(int pixelsCount)
{
	LogicUnitValuef logicUnitRadius;

	float fpixelesLogicArea = m_fOrigXLogicUnitVal * m_fOrigYLogicUnitVal * pixelsCount;

	// to normalize the radius to logic
	NormalizeLogicUnit(fpixelesLogicArea, logicUnitRadius, NORMALIZE_AREA);

	return logicUnitRadius;
}

int ImageLogicUnitHelper::CalVerticalPixel(const LogicUnitValuef &logicVal) 
{
	if (logicVal.GetUnitType() == UNIT_TYPE_MM && m_eLogicUnitType == UNIT_TYPE_MM)
	{
		int val = logicVal.GetLogicUnitValue()  / m_fCurYLogicUnitVal;
		return val;
	}
	else if (logicVal.GetUnitType() == UNIT_TYPE_PIXEL && m_eLogicUnitType == UNIT_TYPE_PIXEL)
	{
		return logicVal.GetLogicUnitValue();
	}
	else if (logicVal.GetUnitType() == UNIT_TYPE_CM && m_eLogicUnitType == UNIT_TYPE_MM)
	{
		int val = logicVal.GetLogicUnitValue() * 10 / m_fCurYLogicUnitVal;
		return val;
	}
	else
	{
		assert(0);
	}

	return 1;
}

int ImageLogicUnitHelper::CalHorizonPixel(const LogicUnitValuef &logicVal)
{
	return 1;
}

long ImageLogicUnitHelper::CorrectNormalizeUnit(int val, NORMALIZE_MODE mode)
{
	long result = val;
	if (mode == NORMALIZE_AREA)
	{
		result = val * val;
	}
	return result;
}

template<typename T>
bool ImageLogicUnitHelper::NormalizeLogicUnit(T length, LogicUnitValue<T> &result, NORMALIZE_MODE mode)
{
	std::stringstream ss;
	
	if (m_eLogicUnitType == UNIT_TYPE_PIXEL)
	{
		result.SetLogicUnitValue(length);
		result.SetUnitType(UNIT_TYPE_PIXEL);
		return true;
	}

	int unit = CorrectNormalizeUnit(1000000, mode);
	int km = length / unit;
	if (km > 0)
	{
		result.SetLogicUnitValue(length / unit);
		result.SetUnitType(UNIT_TYPE_KM);
		return true;
	}

	unit = CorrectNormalizeUnit(1000, mode);
	int m = length / unit;
	if (m > 0)
	{
		result.SetLogicUnitValue((length / unit));
		result.SetUnitType(UNIT_TYPE_M);
		return true;
	}

	unit = CorrectNormalizeUnit(10, mode);
	int cm = length / unit;
	if (cm > 0)
	{
		result.SetLogicUnitValue((length / unit));
		result.SetUnitType(UNIT_TYPE_CM);
		return true;
	}

	result.SetLogicUnitValue(length);
	result.SetUnitType(UNIT_TYPE_MM);

	return true;
}



MCSF_DJ2DENGINE_END_NAMESPACE
