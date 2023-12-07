// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Matrix.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Multiplies the specified point by the current matrix.
//
//  Implementation details:
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 1) = (px, py, 1) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//------------------------------------------------------------------------

void
DirectUI::MatrixHelper::MultiplyPoint(_In_ xaml_media::Matrix target, _Inout_ XFLOAT* pX, _Inout_ XFLOAT* pY)
{
    XFLOAT x = *pX;
    XFLOAT y = *pY;

    *pX = static_cast<FLOAT>((target.M11 * x) + (target.M21 * y) + target.OffsetX);
    *pY = static_cast<FLOAT>((target.M12 * x) + (target.M22 * y) + target.OffsetY);
}

// Replaces target matrix with the product of itself and the incoming CMILMatrix.
void
DirectUI::MatrixHelper::Append(
    _Inout_ xaml_media::Matrix& target,
    _In_ CMILMatrix that)
{
    CMILMatrix targetAsCMILMatrix = ToCMILMatrix(target);
    targetAsCMILMatrix.Append(that);
    target = FromCMILMatrix(targetAsCMILMatrix);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the X and Y scale in the provided matrix.
//
//------------------------------------------------------------------------
/* static */ void
DirectUI::MatrixHelper::GetScaleDimensions(
    _In_ const xaml_media::Matrix &matrix,
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY
    )
{
    CMILMatrix coreMatrix = DirectUI::MatrixHelper::ToCMILMatrix(matrix);
    coreMatrix.GetScaleDimensions(pScaleX, pScaleY);
}

// Converts a Matrix to a CMILMatrix.
CMILMatrix
DirectUI::MatrixHelper::ToCMILMatrix(
    _In_ const xaml_media::Matrix &source)
{
    CMILMatrix returnVal;

    returnVal.SetM11(static_cast<float>(source.M11));
    returnVal.SetM12(static_cast<float>(source.M12));
    returnVal.SetM21(static_cast<float>(source.M21));
    returnVal.SetM22(static_cast<float>(source.M22));
    returnVal.SetDx(static_cast<float>(source.OffsetX));
    returnVal.SetDy(static_cast<float>(source.OffsetY));

    return returnVal;
}

// Converts a CMILMatrix to a Matrix.
xaml_media::Matrix
DirectUI::MatrixHelper::FromCMILMatrix(
    _In_ CMILMatrix source)
{
    xaml_media::Matrix returnVal;

    returnVal.M11 = source.GetM11();
    returnVal.M12 = source.GetM12();
    returnVal.M21 = source.GetM21();
    returnVal.M22 = source.GetM22();
    returnVal.OffsetX = source.GetDx();
    returnVal.OffsetY = source.GetDy();

    return returnVal;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Matrix from the specified elements.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
MatrixFactory::FromElements(
    _In_ DOUBLE m11,
    _In_ DOUBLE m12,
    _In_ DOUBLE m21,
    _In_ DOUBLE m22,
    _In_ DOUBLE offsetX,
    _In_ DOUBLE offsetY,
    _Out_ xaml_media::Matrix* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->M11 = m11;
    pReturnValue->M12 = m12;
    pReturnValue->M21 = m21;
    pReturnValue->M22 = m22;
    pReturnValue->OffsetX = offsetX;
    pReturnValue->OffsetY = offsetY;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the current matrix is
//      equal to the Identity matrix.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
MatrixFactory::GetIsIdentity(
    _In_ xaml_media::Matrix target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    *pValue = (target.M11     == 1.0 && target.M12     == 0.0 &&
               target.M21     == 0.0 && target.M22     == 1.0 &&
               target.OffsetX == 0.0 && target.OffsetY == 0.0);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transforms the specified point using the current matrix.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
MatrixFactory::Transform(
    _In_ xaml_media::Matrix target,
    _In_ wf::Point point,
    _Out_ wf::Point* pReturnValue)
{
    HRESULT hr = S_OK;
    wf::Point point2 = {point.X, point.Y};

    IFCPTR(pReturnValue);

    MatrixHelper::MultiplyPoint(target, &point2.X, &point2.Y);

    pReturnValue->X = point2.X;
    pReturnValue->Y = point2.Y;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a new Identity matrix.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
MatrixFactory::get_Identity(_Out_ xaml_media::Matrix* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    pValue->M11 = 1.0;
    pValue->M12 = 0.0;
    pValue->M21 = 0.0;
    pValue->M22 = 1.0;
    pValue->OffsetX = 0.0;
    pValue->OffsetY = 0.0;

Cleanup:
    RRETURN(hr);
}

