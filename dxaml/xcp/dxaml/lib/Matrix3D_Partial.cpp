// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Matrix3D.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Matrix3D from the specified elements.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::FromElements(
    _In_ DOUBLE m11,
    _In_ DOUBLE m12,
    _In_ DOUBLE m13,
    _In_ DOUBLE m14,
    _In_ DOUBLE m21,
    _In_ DOUBLE m22,
    _In_ DOUBLE m23,
    _In_ DOUBLE m24,
    _In_ DOUBLE m31,
    _In_ DOUBLE m32,
    _In_ DOUBLE m33,
    _In_ DOUBLE m34,
    _In_ DOUBLE offsetX,
    _In_ DOUBLE offsetY,
    _In_ DOUBLE offsetZ,
    _In_ DOUBLE m44,
    _Out_ xaml_media::Media3D::Matrix3D* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->M11 = m11;
    pReturnValue->M12 = m12;
    pReturnValue->M13 = m13;
    pReturnValue->M14 = m14;
    pReturnValue->M21 = m21;
    pReturnValue->M22 = m22;
    pReturnValue->M23 = m23;
    pReturnValue->M24 = m24;
    pReturnValue->M31 = m31;
    pReturnValue->M32 = m32;
    pReturnValue->M33 = m33;
    pReturnValue->M34 = m34;
    pReturnValue->OffsetX = offsetX;
    pReturnValue->OffsetY = offsetY;
    pReturnValue->OffsetZ = offsetZ;
    pReturnValue->M44 = m44;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the current matrix
//      can be inverted, by checking for a non-zero determinant.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::GetHasInverse(
    _In_ xaml_media::Media3D::Matrix3D target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    *pValue = !DoubleUtil::IsZero(Matrix3DHelper::GetDeterminant(target));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a new 4x4 Identity matrix.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::get_Identity(_Out_ xaml_media::Media3D::Matrix3D* pValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(pValue);
    
    pValue->M11 = 1.0;
    pValue->M12 = 0.0;
    pValue->M13 = 0.0;
    pValue->M14 = 0.0;
    pValue->M21 = 0.0;
    pValue->M22 = 1.0;
    pValue->M23 = 0.0;
    pValue->M24 = 0.0;
    pValue->M31 = 0.0;
    pValue->M32 = 0.0;
    pValue->M33 = 1.0;
    pValue->M34 = 0.0;
    pValue->OffsetX = 0.0;
    pValue->OffsetY = 0.0;
    pValue->OffsetZ = 0.0;
    pValue->M44 = 1.0;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the current matrix is
//      equal to the 4x4 Identity matrix.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::GetIsIdentity(
    _In_ xaml_media::Media3D::Matrix3D target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);

    *pValue = (target.M11     == 1.0 && target.M12     == 0.0 && target.M13     == 0.0 && target.M14 == 0.0 &&
               target.M21     == 0.0 && target.M22     == 1.0 && target.M23     == 0.0 && target.M24 == 0.0 &&
               target.M31     == 0.0 && target.M32     == 0.0 && target.M33     == 1.0 && target.M34 == 0.0 &&
               target.OffsetX == 0.0 && target.OffsetY == 0.0 && target.OffsetZ == 0.0 && target.M44 == 1.0);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inverts the current matrix if possible, otherwise returns E_FAIL.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::Invert(
    _In_ xaml_media::Media3D::Matrix3D target,
    _Out_ xaml_media::Media3D::Matrix3D* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (!Matrix3DHelper::InvertCore(target, pReturnValue))
    {
        // TODO: throw new InvalidOperationException();
        RRETURN(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Multiplies two Matrix3Ds together.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
Matrix3DFactory::Multiply(
    _In_ xaml_media::Media3D::Matrix3D matrix1,
    _In_ xaml_media::Media3D::Matrix3D matrix2,
    _Out_ xaml_media::Media3D::Matrix3D* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->M11 = matrix1.M11 * matrix2.M11 +
                        matrix1.M12 * matrix2.M21 +
                        matrix1.M13 * matrix2.M31 +
                        matrix1.M14 * matrix2.OffsetX;
    pReturnValue->M12 = matrix1.M11 * matrix2.M12 +
                        matrix1.M12 * matrix2.M22 +
                        matrix1.M13 * matrix2.M32 +
                        matrix1.M14 * matrix2.OffsetY;
    pReturnValue->M13 = matrix1.M11 * matrix2.M13 +
                        matrix1.M12 * matrix2.M23 +
                        matrix1.M13 * matrix2.M33 +
                        matrix1.M14 * matrix2.OffsetZ;
    pReturnValue->M14 = matrix1.M11 * matrix2.M14 +
                        matrix1.M12 * matrix2.M24 +
                        matrix1.M13 * matrix2.M34 +
                        matrix1.M14 * matrix2.M44;
    pReturnValue->M21 = matrix1.M21 * matrix2.M11 +
                        matrix1.M22 * matrix2.M21 +
                        matrix1.M23 * matrix2.M31 +
                        matrix1.M24 * matrix2.OffsetX;
    pReturnValue->M22 = matrix1.M21 * matrix2.M12 +
                        matrix1.M22 * matrix2.M22 +
                        matrix1.M23 * matrix2.M32 +
                        matrix1.M24 * matrix2.OffsetY;
    pReturnValue->M23 = matrix1.M21 * matrix2.M13 +
                        matrix1.M22 * matrix2.M23 +
                        matrix1.M23 * matrix2.M33 +
                        matrix1.M24 * matrix2.OffsetZ;
    pReturnValue->M24 = matrix1.M21 * matrix2.M14 +
                        matrix1.M22 * matrix2.M24 +
                        matrix1.M23 * matrix2.M34 +
                        matrix1.M24 * matrix2.M44;
    pReturnValue->M31 = matrix1.M31 * matrix2.M11 +
                        matrix1.M32 * matrix2.M21 +
                        matrix1.M33 * matrix2.M31 +
                        matrix1.M34 * matrix2.OffsetX;
    pReturnValue->M32 = matrix1.M31 * matrix2.M12 +
                        matrix1.M32 * matrix2.M22 +
                        matrix1.M33 * matrix2.M32 +
                        matrix1.M34 * matrix2.OffsetY;
    pReturnValue->M33 = matrix1.M31 * matrix2.M13 +
                        matrix1.M32 * matrix2.M23 +
                        matrix1.M33 * matrix2.M33 +
                        matrix1.M34 * matrix2.OffsetZ;
    pReturnValue->M34 = matrix1.M31 * matrix2.M14 +
                        matrix1.M32 * matrix2.M24 +
                        matrix1.M33 * matrix2.M34 +
                        matrix1.M34 * matrix2.M44;
    pReturnValue->OffsetX = matrix1.OffsetX * matrix2.M11 +
                        matrix1.OffsetY * matrix2.M21 +
                        matrix1.OffsetZ * matrix2.M31 +
                        matrix1.M44 * matrix2.OffsetX;
    pReturnValue->OffsetY = matrix1.OffsetX * matrix2.M12 +
                        matrix1.OffsetY * matrix2.M22 +
                        matrix1.OffsetZ * matrix2.M32 +
                        matrix1.M44 * matrix2.OffsetY;
    pReturnValue->OffsetZ = matrix1.OffsetX * matrix2.M13 +
                        matrix1.OffsetY * matrix2.M23 +
                        matrix1.OffsetZ * matrix2.M33 +
                        matrix1.M44 * matrix2.OffsetZ;
    pReturnValue->M44 = matrix1.OffsetX * matrix2.M14 +
                        matrix1.OffsetY * matrix2.M24 +
                        matrix1.OffsetZ * matrix2.M34 +
                        matrix1.M44 * matrix2.M44;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the determinant for the current matrix.
//
//------------------------------------------------------------------------

XDOUBLE
Matrix3DHelper::GetDeterminant(
    _In_ xaml_media::Media3D::Matrix3D target)
{

    if (Matrix3DHelper::IsAffine(target))
    {
        return Matrix3DHelper::GetNormalizedAffineDeterminant(target);
    }

    // NOTE: The beginning of this code is duplicated between
    //       the Invert and GetDeterminant methods.

    // compute all six 2x2 determinants of 2nd two columns
    XDOUBLE y01 = target.M13 * target.M24 - target.M23 * target.M14;
    XDOUBLE y02 = target.M13 * target.M34 - target.M33 * target.M14;
    XDOUBLE y03 = target.M13 * target.M44 - target.OffsetZ * target.M14;
    XDOUBLE y12 = target.M23 * target.M34 - target.M33 * target.M24;
    XDOUBLE y13 = target.M23 * target.M44 - target.OffsetZ * target.M24;
    XDOUBLE y23 = target.M33 * target.M44 - target.OffsetZ * target.M34;

    // Compute 3x3 cofactors for 1st the column
    XDOUBLE z30 = target.M22 * y02 - target.M32 * y01 - target.M12 * y12;
    XDOUBLE z20 = target.M12 * y13 - target.M22 * y03 + target.OffsetY * y01;
    XDOUBLE z10 = target.M32 * y03 - target.OffsetY * y02 - target.M12 * y23;
    XDOUBLE z00 = target.M22 * y23 - target.M32 * y13 + target.OffsetY * y12;

    return target.OffsetX * z30 + target.M31 * z20 + target.M21 * z10 + target.M11 * z00;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the determinant for the current affine matrix.
//
//------------------------------------------------------------------------

XDOUBLE
Matrix3DHelper::GetNormalizedAffineDeterminant(
    _In_ xaml_media::Media3D::Matrix3D target)
{
    ASSERT(Matrix3DHelper::IsAffine(target));

    // NOTE: The beginning of this code is duplicated between
    //       GetNormalizedAffineDeterminant() and NormalizedAffineInvert()

    XDOUBLE z20 = target.M12 * target.M23 - target.M22 * target.M13;
    XDOUBLE z10 = target.M32 * target.M13 - target.M12 * target.M33;
    XDOUBLE z00 = target.M22 * target.M33 - target.M32 * target.M23;

    return target.M31 * z20 + target.M21 * z10 + target.M11 * z00;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inverts the current matrix and returns TRUE if successful.
//
//------------------------------------------------------------------------

bool
Matrix3DHelper::InvertCore(
    _In_ xaml_media::Media3D::Matrix3D target,
    _Out_ xaml_media::Media3D::Matrix3D* pReturnValue)
{
    if (Matrix3DHelper::IsAffine(target))
    {
        return Matrix3DHelper::NormalizedAffineInvert(target, pReturnValue);
    }

    // NOTE: The beginning of this code is duplicated between
    //       the Invert and GetDeterminant methods.

    // compute all six 2x2 determinants of 2nd two columns
    XDOUBLE y01 = target.M13 * target.M24 - target.M23 * target.M14;
    XDOUBLE y02 = target.M13 * target.M34 - target.M33 * target.M14;
    XDOUBLE y03 = target.M13 * target.M44 - target.OffsetZ * target.M14;
    XDOUBLE y12 = target.M23 * target.M34 - target.M33 * target.M24;
    XDOUBLE y13 = target.M23 * target.M44 - target.OffsetZ * target.M24;
    XDOUBLE y23 = target.M33 * target.M44 - target.OffsetZ * target.M34;

    // Compute 3x3 cofactors for 1st the column
    XDOUBLE z30 = target.M22 * y02 - target.M32 * y01 - target.M12 * y12;
    XDOUBLE z20 = target.M12 * y13 - target.M22 * y03 + target.OffsetY * y01;
    XDOUBLE z10 = target.M32 * y03 - target.OffsetY * y02 - target.M12 * y23;
    XDOUBLE z00 = target.M22 * y23 - target.M32 * y13 + target.OffsetY * y12;

    // Compute 4x4 determinant
    XDOUBLE det = target.OffsetX * z30 + target.M31 * z20 + target.M21 * z10 + target.M11 * z00;

    // If Determinant is computed using a different method then Inverse can throw
    // NotInvertable when HasInverse is true.  (Windows OS #901174)
    //
    // The strange logic below is equivalent to "det == Determinant", but NaN safe.
    ASSERT(!(det < Matrix3DHelper::GetDeterminant(target) || det > Matrix3DHelper::GetDeterminant(target)),
                 "Matrix3D.Invert: GetDeterminant method does not match value computed in Invert.");

    if (DoubleUtil::IsZero(det))
    {
        return false;
    }

    // Compute 3x3 cofactors for the 2nd column
    XDOUBLE z31 = target.M11 * y12 - target.M21 * y02 + target.M31 * y01;
    XDOUBLE z21 = target.M21 * y03 - target.OffsetX * y01 - target.M11 * y13;
    XDOUBLE z11 = target.M11 * y23 - target.M31 * y03 + target.OffsetX * y02;
    XDOUBLE z01 = target.M31 * y13 - target.OffsetX * y12 - target.M21 * y23;

    // Compute all six 2x2 determinants of 1st two columns
    y01 = target.M11 * target.M22 - target.M21 * target.M12;
    y02 = target.M11 * target.M32 - target.M31 * target.M12;
    y03 = target.M11 * target.OffsetY - target.OffsetX * target.M12;
    y12 = target.M21 * target.M32 - target.M31 * target.M22;
    y13 = target.M21 * target.OffsetY - target.OffsetX * target.M22;
    y23 = target.M31 * target.OffsetY - target.OffsetX * target.M32;

    // Compute all 3x3 cofactors for 2nd two columns
    XDOUBLE z33 = target.M13 * y12 - target.M23 * y02 + target.M33 * y01;
    XDOUBLE z23 = target.M23 * y03 - target.OffsetZ * y01 - target.M13 * y13;
    XDOUBLE z13 = target.M13 * y23 - target.M33 * y03 + target.OffsetZ * y02;
    XDOUBLE z03 = target.M33 * y13 - target.OffsetZ * y12 - target.M23 * y23;
    XDOUBLE z32 = target.M24 * y02 - target.M34 * y01 - target.M14 * y12;
    XDOUBLE z22 = target.M14 * y13 - target.M24 * y03 + target.M44 * y01;
    XDOUBLE z12 = target.M34 * y03 - target.M44 * y02 - target.M14 * y23;
    XDOUBLE z02 = target.M24 * y23 - target.M34 * y13 + target.M44 * y12;

    XDOUBLE rcp = 1.0 / det;

    // Multiply all 3x3 cofactors by reciprocal & transpose
    pReturnValue->M11 = z00 * rcp;
    pReturnValue->M12 = z10 * rcp;
    pReturnValue->M13 = z20 * rcp;
    pReturnValue->M14 = z30 * rcp;

    pReturnValue->M21 = z01 * rcp;
    pReturnValue->M22 = z11 * rcp;
    pReturnValue->M23 = z21 * rcp;
    pReturnValue->M24 = z31 * rcp;

    pReturnValue->M31 = z02 * rcp;
    pReturnValue->M32 = z12 * rcp;
    pReturnValue->M33 = z22 * rcp;
    pReturnValue->M34 = z32 * rcp;

    pReturnValue->OffsetX = z03 * rcp;
    pReturnValue->OffsetY = z13 * rcp;
    pReturnValue->OffsetZ = z23 * rcp;

    pReturnValue->M44 = z33 * rcp;

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the current matrix is
//      an affine transformation matrix.
//
//------------------------------------------------------------------------

bool
Matrix3DHelper::IsAffine(
    _In_ xaml_media::Media3D::Matrix3D target)
{
    return (target.M14 == 0.0 && target.M24 == 0.0 && target.M34 == 0.0 && target.M44 == 1.0);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inverts the current affine matrix and returns TRUE if successful.
//
//------------------------------------------------------------------------

bool
Matrix3DHelper::NormalizedAffineInvert(
    _In_ xaml_media::Media3D::Matrix3D target,
    _Out_ xaml_media::Media3D::Matrix3D* pReturnValue)
{
    ASSERT(Matrix3DHelper::IsAffine(target));

    // NOTE: The beginning of this code is duplicated between
    //       GetNormalizedAffineDeterminant() and NormalizedAffineInvert()

    XDOUBLE z20 = target.M12 * target.M23 - target.M22 * target.M13;
    XDOUBLE z10 = target.M32 * target.M13 - target.M12 * target.M33;
    XDOUBLE z00 = target.M22 * target.M33 - target.M32 * target.M23;
    XDOUBLE det = target.M31 * z20 + target.M21 * z10 + target.M11 * z00;

    // Fancy logic here avoids using equality with possible nan values.
    ASSERT(!(det < Matrix3DHelper::GetDeterminant(target) || det > Matrix3DHelper::GetDeterminant(target)),
                 "Matrix3D.Invert: GetDeterminant method does not match value computed in Invert.");

    if (DoubleUtil::IsZero(det))
    {
        return false;
    }

    // Compute 3x3 non-zero cofactors for the 2nd column
    XDOUBLE z21 = target.M21 * target.M13 - target.M11 * target.M23;
    XDOUBLE z11 = target.M11 * target.M33 - target.M31 * target.M13;
    XDOUBLE z01 = target.M31 * target.M23 - target.M21 * target.M33;

    // Compute all six 2x2 determinants of 1st two columns
    XDOUBLE y01 = target.M11 * target.M22 - target.M21 * target.M12;
    XDOUBLE y02 = target.M11 * target.M32 - target.M31 * target.M12;
    XDOUBLE y03 = target.M11 * target.OffsetY - target.OffsetX * target.M12;
    XDOUBLE y12 = target.M21 * target.M32 - target.M31 * target.M22;
    XDOUBLE y13 = target.M21 * target.OffsetY - target.OffsetX * target.M22;
    XDOUBLE y23 = target.M31 * target.OffsetY - target.OffsetX * target.M32;

    // Compute all non-zero and non-one 3x3 cofactors for 2nd
    // two columns
    XDOUBLE z23 = target.M23 * y03 - target.OffsetZ * y01 - target.M13 * y13;
    XDOUBLE z13 = target.M13 * y23 - target.M33 * y03 + target.OffsetZ * y02;
    XDOUBLE z03 = target.M33 * y13 - target.OffsetZ * y12 - target.M23 * y23;
    XDOUBLE z22 = y01;
    XDOUBLE z12 = -y02;
    XDOUBLE z02 = y12;

    XDOUBLE rcp = 1.0 / det;

    // Multiply all 3x3 cofactors by reciprocal & transpose
    pReturnValue->M11 = z00 * rcp;
    pReturnValue->M12 = z10 * rcp;
    pReturnValue->M13 = z20 * rcp;

    pReturnValue->M21 = z01 * rcp;
    pReturnValue->M22 = z11 * rcp;
    pReturnValue->M23 = z21 * rcp;

    pReturnValue->M31 = z02 * rcp;
    pReturnValue->M32 = z12 * rcp;
    pReturnValue->M33 = z22 * rcp;

    pReturnValue->OffsetX = z03 * rcp;
    pReturnValue->OffsetY = z13 * rcp;
    pReturnValue->OffsetZ = z23 * rcp;

    pReturnValue->M14 = 0.0;
    pReturnValue->M24 = 0.0;
    pReturnValue->M34 = 0.0;

    pReturnValue->M44 = 1.0;

    return true;
}
