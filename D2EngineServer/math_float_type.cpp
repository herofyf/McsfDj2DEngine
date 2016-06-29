#include "math_float_type.h"



/* check if double a is less than double b. */
bool isLessThan_d (const double a, const double b) 
{
	return (b - a) > ((fabs (a) < fabs (b) ? fabs (b) : fabs (a)) *
		std::numeric_limits<double>::epsilon ());
}

/* check if double a is greater than double b. */
bool isGreaterThan_d (const double a, const double b) 
{
	return (a - b) > ((fabs (a) < fabs (b) ? fabs (b) : fabs (a)) *
			std::numeric_limits<double>::epsilon ());
}

/* check if double a is approximately equal to double b. */
bool isApproximatelyEqual_d (const double a, const double b) {
	
	return fabs (a - b) <= ((fabs (a) < fabs (b) ? fabs (b) : fabs (a)) *
			std::numeric_limits<double>::epsilon ());
}

/* check if double a is essentially equal to double b. */
bool isEssentiallyEqual_d(const double a, const double b) {
	
	return fabs (a - b) <= ((fabs (a) > fabs (b) ? fabs (b) : fabs (a)) *
			std::numeric_limits<double>::epsilon ());
}