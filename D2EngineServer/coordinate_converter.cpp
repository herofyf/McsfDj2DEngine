#include "stdafx.h"
#include "coordinate_converter.h"
#include "study_image.h"
#include "image_property_state.h"
#include <assert.h>
#include <math.h>

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

CoordinateTranslator::CoordinateTranslator(StudyImage *pStudyImage) :
	m_pStudyImage(pStudyImage)
{
	m_fWidth = m_fHeight = m_fCenterX = m_fCenterY = 0;
}

void CoordinateTranslator::BuildScaleMatrix(float scaleXF, float scaleYF, Matrix &m)
{
	m.Translate(-m_fCenterX, -m_fCenterY, MatrixOrderAppend);
	m.Scale(1 + scaleXF, 1 + scaleYF, MatrixOrderAppend);
	m.Translate(m_fCenterX, m_fCenterY, MatrixOrderAppend);
}

// points[0] : left-top
// points[1] : right-top
// points[2] : left-bottom
bool CoordinateTranslator::ScaleShape(float scaleXF, float scaleYF, PointF points[], int size)
{
	if (scaleXF <= -1 || scaleYF <= -1) return false;

	Matrix m;
	BuildScaleMatrix(scaleXF, scaleYF, m);
	m.TransformPoints(points, size);
	return true;
}

void CoordinateTranslator::BuildTranslateMatrix(float translateX, float translateY, Matrix &m)
{
	m.Translate(translateX, translateY);
}
void CoordinateTranslator::TranslateShape(float translateX, float translateY, PointF points[], int size)
{
	Matrix m;
	BuildTranslateMatrix(translateX, translateY, m);
	m.TransformPoints(points, size);
}

void CoordinateTranslator::BuildRotateMatrix(float angle, Matrix &m)
{
	PointF center(GetCenterX(), GetCenterY());
	m.RotateAt(angle, center);
}

void CoordinateTranslator::RotateShape(float angle, PointF points[], int size)
{
	Matrix m;

	BuildRotateMatrix(angle, m);

	m.TransformPoints(points, size);		
}

void CoordinateTranslator::BuildFlipXMatrix(Matrix &m)
{
	Matrix translateMatrix1(1, 0, 0, 1, GetCenterX(), GetCenterX());
	Matrix translateMatrix2(1, 0, 0, 1, -GetCenterX(), -GetCenterX());
	Matrix horizontalMatrix(1, 0, 0, -1, 0, 0);

	m.Multiply(&translateMatrix1);
	m.Multiply(&horizontalMatrix);
	m.Multiply(&translateMatrix2);
}

void CoordinateTranslator::FlipX(PointF points[], int size)
{
	Matrix m;
	BuildFlipXMatrix(m);
	m.TransformPoints(points, size);
}

void CoordinateTranslator::BuildFlipYMatrix(Matrix &m)
{
	Matrix translateMatrix1(1, 0, 0, 1, GetCenterX(), GetCenterX());
	Matrix translateMatrix2(1, 0, 0, 1, -GetCenterX(), -GetCenterX());
	Matrix verticalMatrix(-1, 0, 0, 1, 0, 0);

	m.Multiply(&translateMatrix1);
	m.Multiply(&verticalMatrix);
	m.Multiply(&translateMatrix2);
}


void CoordinateTranslator::FlipY(PointF points[], int size)
{
	Matrix m;
	BuildFlipYMatrix(m);
	m.TransformPoints(points, size);
}

MCSF_DJ2DENGINE_END_NAMESPACE
