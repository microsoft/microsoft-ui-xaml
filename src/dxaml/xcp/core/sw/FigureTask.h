// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Class:    CMILBezierFlattener
//  Synopsis: Generates a polygonal apprximation to a given Bezier curve
//  Note:     This class adds some specific methods to construct CBezierFlattener

class CMILBezierFlattener  :   public CBezierFlattener
{
public:
    CMILBezierFlattener(
        _In_reads_opt_(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        XFLOAT rTolerance)
            // Flattening tolerance
        :
    CBezierFlattener(pSink, rTolerance)
    {
    }

    CMILBezierFlattener(
        _In_ const XPOINTF  &ptFirst,
            // First point (transformed)
        _In_reads_(3) const XPOINTF *pt,
            // The last 3 points (raw)
        _In_reads_opt_(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        XFLOAT rTolerance,
            // Flattening tolerance 
        _In_reads_opt_(1) const CMILMatrix  *pMatrix)
            // Transformation matrix (NULL OK)
        : CBezierFlattener(pSink, rTolerance)
    {
        SetPoints(0, 1, ptFirst, pt, pMatrix);
    }


    CMILBezierFlattener(
        _In_ const XPOINTF       &ptFirst,
            // First point (transformed)
        _In_ const XPOINTF       &ptControl1,
            // First control point
        _In_ const XPOINTF       &ptControl2,
            // Second control point
        _In_ const XPOINTF       &ptEnd,
            // Last point
        _In_reads_opt_(1) CFlatteningSink   *pSink,
            // Flattening sink
        _In_ const CMILMatrix     &matrix);
            // Transformation matrix
    
    void SetPoints(
        double rStart,
            // Start parameter
        double rEnd,
            // End parameter
        _In_ const XPOINTF  &ptFirst,
            // First point (transformed)
        _In_reads_(3) const XPOINTF *pt,
            // The last 3 points (raw)
        _In_reads_opt_(1) const CMILMatrix  *pMatrix);
            // Transformation matrix (NULL OK)
    
    // No data
};

