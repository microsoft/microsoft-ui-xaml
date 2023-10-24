// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Implementation of methods used to create intermediate brush
// representations from user-defined state.

#include "precomp.h"
#include <BrushTypeUtils.h>

//------------------------------------------------------------------------
//
//  Method:   CBrushTypeUtils::GetBrushTransform
//
//  Synopsis:
//      Obtains immediate (realized) value of the brush transform.
//      We derive the brush transform from converting the relative
//      transform to absolute space using the bounding box and combining
//      it with the absolute transform.  Per spec, the relative transform
//      is applied before the absolute transform.  This allows users to do
//      things like rotating about the center of a shape using the relative
//      transform and then offsetting it by a constant amount amongst all shapes
//      being filled using the absolute transform.
//
//------------------------------------------------------------------------
void
CBrushTypeUtils::GetBrushTransform(
    _In_reads_opt_(1) const CMILMatrix *pmatRelative,  // Current value of user-specified Brush.RelativeTransform property
    _In_reads_opt_(1) const CMILMatrix *pmatTransform, // Current value of user-specified Brush.Transform property
    _In_reads_(1) const XRECTF *pBoundingBox,          // Bounding box the relative transform is relative to
    _Out_writes_(1) CMILMatrix *pResultTransform        // Output combined transform
    )
{

// Assert required in-parameters
    ASSERT(pBoundingBox && pResultTransform);

// fResultSet specifies whether or not the result matrix has been set.
// We use this knowledge to avoid unnecessary matrix operations when
// the relative and/or absolute transforms aren't set.
    bool fResultSet = false;

// Apply the relative transform

    if (pmatRelative)
    {
    // Handle relative transforms applied to degenerate shapes.  This equality
    // check has been added to maintain parity with the previous InferAffineMatrix
    // implementation.  But we need to handle dimensions close to zero, in addition
    // to zero.
        if ( (pBoundingBox->Width != 0.0) &&
             (pBoundingBox->Height != 0.0))
        {
        // Calculate matrix that transforms absolute coordinates by the relative transform
            ConvertRelativeTransformToAbsolute(
                pBoundingBox,
                pmatRelative,
                pResultTransform
                );

            fResultSet = true;
        }
    }

// Apply the absolute transform, if one was specified

    if (pmatTransform)
    {
        if (fResultSet)
        {
        // Append the absolute transform to the relative transform if
        // a relative transform was set
            pResultTransform->Append(*pmatTransform);
        }
        else
        {
        // Copy the absolute transform directly to the out-param if no
        // relative transform was specified
            *pResultTransform = *pmatTransform;
        }
        fResultSet = true;
    }

// Set the matrix to identity if the matrix hasn't been initialized because
// no transforms were specified
    if (!fResultSet)
    {
        pResultTransform->SetToIdentity();
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CBrushTypeUtils::ConvertRelativeTransformToAbsolute
//
//  Synopsis:
//      Given the relative transform & bounding box it's relative to,
//      this function calculates an absolute derivation of the relative
//      transform.
//
//      The relative transform is modified to take absolute coordinates as input,
//      transform those coordinates by the user-specified relative transform,
//      and then output absolute coordinates.
//
//  Notes:
//
//      This function is an optimized equivalent of the following operations:
//
//      XRECTF relativeBounds = {0.0, 0.0, 1.0, 1.0};
//      pResultTransform->InferAffineMatrix(relativeBounds, pBoundingBox);
//      pResultTransform->Multiply(*pmatRelative);
//      relativeToAbsolute.InferAffineMatrix(pBoundingBox, relativeBounds);
//      pResultTransform->Multiply(relativeToAbsolute);
//
//      To avoid inferring 2 rectangle mappings, & performing 2 full matrix
//      multiplications, the resultant math performed by these 4 operations
//      was expanded out, and terms which cancel or always evaluate to 0 were
//      removed.  As a final optimization, this function assumes (and asserts)
//      that the input relative transform only has 6 elements set to
//      non-identity values.
//
//-------------------------------------------------------------------------
void
CBrushTypeUtils::ConvertRelativeTransformToAbsolute(
    _In_reads_(1) const XRECTF *pBoundingBox,
        // Bounds that the relative transform is relative to.  The unit square in the relative
        // coordinate space is mapped to these bounds in the absolute coordinate space.
    _In_reads_(1) const CMILMatrix *pRelativeTransform,
        // User-specified relative transform
    _Out_writes_(1) CMILMatrix* pConvertedTransform
        // Relative transform that has been modified to take absolute coordinates as input,
        // transform those coordinates by the relative transform, and then output absolute
        // coordinates.
    )
{
// Copy commonly used variables to the stack for quicker access (and to make the implementation
// more readable)
    XFLOAT X = pBoundingBox->X;
    XFLOAT Y = pBoundingBox->Y;
    XFLOAT W = pBoundingBox->Width;
    XFLOAT H = pBoundingBox->Height;

// Precompute divides that are needed more than once.
    XFLOAT rHeightDividedByWidth = H / W;
    XFLOAT rWidthDividedByHeight = W / H;

// Calculate the first vector

    pConvertedTransform->_11 = pRelativeTransform->_11;
    pConvertedTransform->_12 = pRelativeTransform->_12 * rHeightDividedByWidth;

// Calculate the second vector

    pConvertedTransform->_21 = pRelativeTransform->_21 * rWidthDividedByHeight;
    pConvertedTransform->_22 = pRelativeTransform->_22;

// Calculate third vector

    pConvertedTransform->_31 =
        pRelativeTransform->_31 * W -
        pRelativeTransform->_11 * X -
        pRelativeTransform->_21 * Y * rWidthDividedByHeight +
        X;

    pConvertedTransform->_32 =
        pRelativeTransform->_32 * H -
        pRelativeTransform->_12 * X * rHeightDividedByWidth -
        pRelativeTransform->_22 * Y +
        Y;
}

//------------------------------------------------------------------------
//
//  Method:   AdjustRelativePoint
//
//  Synopsis:
//     Calculates an absolute point from a relative point and
//     bounding box.
//
//------------------------------------------------------------------------
void
AdjustRelativePoint(
    _In_ const XRECTF *pBoundingBox,  // Bounding box pt is relative to
    _Inout_ XPOINTF *pt               // IN:  Relative point OUT: Absolute Point
    )
{
    ASSERT(pt != NULL);

// Must have bounding box

    ASSERT(pBoundingBox != NULL);

// Relative points are defined as a decimal percentage of a bounding box
// dimension.  Any given coordinate, "A", will reside within the bounding
// box over the range 0.0 <= A <= 1.0.  E.g., if pt->X is 0.5, then the
// absolute X coordinate is half the width of the bounding box,
// or: pBoundingBox->X + 0.5 * pBoundingBox->Width.
// Likewise, if pt->X is defined as 3.1, then the absolute coordinate is
// 3.1 times the width of the bounding box + the bounding box's X coordinate

// Calculate absolute point

    pt->x = pBoundingBox->X + pt->x * pBoundingBox->Width;
    pt->y = pBoundingBox->Y + pt->y * pBoundingBox->Height;
}

//------------------------------------------------------------------------
//
//  Method:   AdjustRelativeRectangle
//
//  Synopsis:
//      Calculates an absolute rectangle from a relative rectangle and
//      bounding box.
//
//------------------------------------------------------------------------
void
AdjustRelativeRectangle(
    _In_ const XRECTF *prcBoundingBox,
    _Inout_ XRECTF *prcAdjustRectangle
    )
{
    ASSERT(prcBoundingBox && prcAdjustRectangle);

    prcAdjustRectangle->X = prcBoundingBox->X + (prcAdjustRectangle->X * prcBoundingBox->Width);
    prcAdjustRectangle->Y = prcBoundingBox->Y + (prcAdjustRectangle->Y * prcBoundingBox->Height);

    prcAdjustRectangle->Width = prcAdjustRectangle->Width * prcBoundingBox->Width;
    prcAdjustRectangle->Height = prcAdjustRectangle->Height * prcBoundingBox->Height;
}

