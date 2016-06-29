#include "stdafx.h"
#include "image_property_state.h"
#include "coordinate_converter.h"
#include "study_image.h"
#include "image_tool_interface.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE

TransformReqs::TransformReqs()
{

}

TransformReqs::~TransformReqs()
{
	m_transformReqs.clear();
}

void TransformReqs::RecordTransformReq(const TransformationArgs *pParam)
{
	if (pParam)
	{
		m_transformReqs.push_back(*pParam);
	}
}

void TransformReqs::ClearReqs()
{
	m_transformReqs.clear();
}

TransformationStates::TransformationStates(TransformableDrawing *pOwner) :
	m_pOwner(pOwner)
{
	Clear();
}

void TransformationStates::Clear()
{
	m_transMatrix.Reset();
}

bool TransformationStates::AppendTransformOnReq(const TransformationArgs *pParam)
{
	if (pParam == NULL) 
		return false;

	Matrix m;
	m.Reset();

	TRANSFORMATION_TYPE transType = pParam->transformationType;
	if (transType == TRANS_TRANLATE)
	{
		CO_TR_OBJ(m_pOwner->OwnerImage())->BuildTranslateMatrix(
			pParam->args.translateArgs.offsetX,
			pParam->args.translateArgs.offsetY,
			m);
	}
	else if (transType == TRANS_SCALE)
	{
		CO_TR_OBJ(m_pOwner->OwnerImage())->BuildScaleMatrix(
			pParam->args.scaleArgs.scaleX,
			pParam->args.scaleArgs.scaleY,
			m);
	}
	else if (transType == TRANS_ROTATE)
	{
		CO_TR_OBJ(m_pOwner->OwnerImage())->BuildRotateMatrix(
			pParam->args.rotateArg.angle,
			m);
	}
	else if (transType == TRANS_FLIPX)
	{
		CO_TR_OBJ(m_pOwner->OwnerImage())->BuildFlipXMatrix(m);	
	}
	else if (transType == TRANS_FLIPY)
	{
		CO_TR_OBJ(m_pOwner->OwnerImage())->BuildFlipYMatrix(m);
	}
	else
	{
		assert(0);
	}
	
	m_transMatrix.Multiply(&m, MatrixOrderAppend);


	return true;
}

bool TransformationStates::AppendTransformWithMatrix(const Matrix *pMatrix)
{
	if (pMatrix)
	{
		m_transMatrix.Multiply(pMatrix, MatrixOrderAppend);
		return true;
	}
	return false;
}

/*
	init the shape with those transformation states
*/
bool TransformationStates::MakeAllTransformationsOnPoints(PointF *pPoints, int n) const
{
	if (m_pOwner == NULL) 
		return false;

	m_transMatrix.TransformPoints(pPoints, n);
	
	return true;
}

bool TransformationStates::ReverseAllTransformationsOnPoints(PointF *pPoints, int n) const
{
	if (m_pOwner == NULL) 
		return false;

	Matrix *pCloneMatrix = m_transMatrix.Clone();
	pCloneMatrix->Invert();
	pCloneMatrix->TransformPoints(pPoints, n);
	delete pCloneMatrix;

	return true;
}

MCSF_DJ2DENGINE_END_NAMESPACE
