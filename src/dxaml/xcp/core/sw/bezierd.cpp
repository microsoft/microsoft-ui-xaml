// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:  Double precision Bezier curve with basic services

#include "precomp.h"

/////////////////////////////////////////////////////////////////////////////////
//
//              Implementation of CBezier
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezier::GetPoint
//
//  Synopsis: Get the point at a given parameter
//
//--------------------------------------------------------------------------------------------------
void
CBezier::GetPoint(
    _In_ double       t,
        // Parameter value
    _Out_ XPOINTF &pt) const
        // Point there
{
    double s = 1 - t;
    double s2 = s * s;
    double t2 = t * t;

    pt =  m_ptB[0] * (s * s2) + m_ptB[1] * (3 * s2 * t) +
          m_ptB[2] * (3 * s * t2) + m_ptB[3] * (t * t2);
}
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezier::TrimToStartAt
//
//  Synopsis: Set this curve as a portion of itself with a piece trimmed away from its start.
//
//  Notes:  The original curve is defined on [0,1].  Here we compute the coefficients of the
//          restriction of that curves to the interval [t,1] as a new Bezier curve.
//
//--------------------------------------------------------------------------------------------------
void
CBezier::TrimToStartAt(
    _In_ double t)              // Parameter value
{
    ASSERT(t > 0  &&  t < 1);
    double s = 1 - t;

    // The conventional De Casteljau algorithm (described in any book on Bezier curves) splits a
    // curve at t and computes coefficients for both pieces as independed Bezier curves.
    // Here we are only computing coefficients for the piece that corresponds to [t,1].

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
    m_ptB[1] = m_ptB[1] * s + m_ptB[2] * t;
    m_ptB[2] = m_ptB[2] * s + m_ptB[3] * t;

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
    m_ptB[1] = m_ptB[1] * s + m_ptB[2] * t;

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
}
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezier::TrimToEndAt
//
//  Synopsis: Set this curve as a portion of itself curve with a piece trimmed away from its end.
//
//  Notes:  The original curve is defined on [0,1].  Here we compute the coefficients of the
//          restriction of that curves to the interval [0,t] as a new Bezier curve.
//
//--------------------------------------------------------------------------------------------------
void
CBezier::TrimToEndAt(
    _In_ double t)              // Parameter value
{
    ASSERT(t > 0  &&  t < 1);
    double s = 1 - t;

    // The conventional De Casteljau algorithm (described in any book on Bezier curves) splits a
    // curve at t and computes coefficients for both pieces as independed Bezier curves.
    // Here we are only computing coefficients for the piece that corresponds to [t,1].

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
    m_ptB[2] = m_ptB[1] * s + m_ptB[2] * t;
    m_ptB[1] = m_ptB[0] * s + m_ptB[1] * t;

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
    m_ptB[2] = m_ptB[1] * s + m_ptB[2] * t;

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
}
