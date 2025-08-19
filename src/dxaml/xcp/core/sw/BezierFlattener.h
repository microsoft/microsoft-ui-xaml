// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//     Definition of CBezierFlattener.
//     Carved out of FigureTask.h to make it portable
//  Synopsis: Callback interface for the results of curve flattening
//  Notes:    Methods are implemented rather than pure, for callers who do not
//            use all of them.

class CFlatteningSink
{
public:
    CFlatteningSink() {}

    virtual ~CFlatteningSink() {}

    virtual _Check_return_ HRESULT Begin(
        _In_ const XPOINTF &)
            // First point (transformed)
    {
        // Do nothing stub, should not be called
        ASSERT(FALSE, L"Base class Begin called");
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT AcceptPoint(
        _In_ const XPOINTF &pt,
            // The point
        _In_ XFLOAT t,
            // Parameter we're at
        _Out_ XINT32 &fAborted)
            // Set to TRUE to signal aborting
    {
        // Do nothing stub, should not be called
        ASSERT(FALSE, L"Base class AcceptPoint called");
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT AcceptPointAndTangent(
        _In_ const XPOINTF &,
            //The point
        _In_ const XPOINTF &,
            //The tangent there
        _In_ XINT32 fLast)         // Is this the last point on the curve?
    {
        // Do nothing stub, should not be called
        ASSERT(FALSE, L"Base class AcceptPointAndTangent called");
        return E_NOTIMPL;
    }
};
//+-----------------------------------------------------------------------------
//
//  Class:    CBezierFlattener
//
//  Synopsis: Generates a polygonal apprximation to a given Bezier curve
//
//------------------------------------------------------------------------------
class CBezierFlattener  :   public CBezier
{
public:
    CBezierFlattener(
        _In_reads_opt_(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        _In_ XFLOAT          rTolerance)
            // Flattening tolerance
    {
        Initialize(pSink, rTolerance);
    }

    void SetTarget(_In_reads_opt_(1) CFlatteningSink *pSink)
    {
        m_pSink = pSink;
    }

    void Initialize(
        _In_opt_ CFlatteningSink *pSink,
            // The reciptient of the flattened data
        _In_ XFLOAT rTolerance);
        // Flattening tolerance

    void SetPoint(
        _In_ XUINT32 i,
            // index of the point (must be between 0 and 3)
        _In_ const XPOINTF &pt)
            // point value
    {
        if (i < 4)
        {
            m_ptB[i] = pt;
        }
    }

    _Check_return_ HRESULT GetFirstTangent(
        _Out_ XPOINTF &vecTangent) const;
            // Tangent vector there

    XPOINTF GetLastTangent() const;

    _Check_return_ HRESULT Flatten(
        _In_ XINT32 fWithTangents);   // Return tangents with the points if TRUE

private:
    // Disallow copy constructor
    CBezierFlattener(_In_ const CBezierFlattener &)
    {
        ASSERT(FALSE, L"CBezierFlattener copy constructor reached.");
    }

protected:
    _Check_return_ HRESULT Step(
        _Out_ XINT32 &fAbort);   // Set to TRUE if flattening should be aborted

    void HalveTheStep();

    XINT32 TryDoubleTheStep();

    // Flattening defining data
    CFlatteningSink *m_pSink;           // The recipient of the flattening data
    double          m_rTolerance;       // Prescribed tolerance
    XINT32          m_fWithTangents{};  // Generate tangent vectors if TRUE
    double          m_rQuarterTolerance;// Prescribed tolerance/4 (for doubling the step)
    double          m_rFuzz;            // Computational zero

    // Flattening working data
    XPOINTF         m_ptE[4]{};         // The moving basis of the curve definition
    int             m_cSteps;           // The number of steps left to the end of the curve
    double          m_rParameter;       // Parameter value
    double          m_rStepSize;        // Steps size in parameter domain
};
