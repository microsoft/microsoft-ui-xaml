// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeySpline.h"
#include "real.h"

wfn::Vector2 CKeySpline::GetControlPoint1()
{
    wfn::Vector2 controlPoint;
    controlPoint.X = m_ControlPoint1.x;
    controlPoint.Y = m_ControlPoint1.y;
    return controlPoint;
}

wfn::Vector2 CKeySpline::GetControlPoint2()
{
    wfn::Vector2 controlPoint;
    controlPoint.X = m_ControlPoint2.x;
    controlPoint.Y = m_ControlPoint2.y;
    return controlPoint;
}

// Computes the spline contribution for a given progress value
float CKeySpline::GetSplineProgress(float linearProgress)
{
// NOTE: this numerical pseudo-Newton interpolation method was ported 'as-is'
// from WPF

// Compute the parametric value t
    if (linearProgress == 0 || linearProgress == 1)
    {
       return linearProgress;
    }
    else
    {
        // Dynamic search interval to clamp with
        float rBottom = 0.0f;
        float rTop = 1.0f;
        float rFuzz = 0.000001f; // computational zero
        float rAccuracy = 0.001f;   // 1/3 the desired accuracy in X

#if DBG
        unsigned int dbgLoopCounter = 0;
#endif /* DBG */

        // Loop while improving the guess
        float rT = m_rLastT;
        while (rTop - rBottom > rFuzz)
        {
#if DBG
            ++dbgLoopCounter;

            ASSERT(dbgLoopCounter < 50);
#endif /* DBG */

            float rX;
            float rDeltaX;
            float rAbsDeltaX;

            GetXAndDeltaX(rT, &rX, &rDeltaX);

            rAbsDeltaX = XcpAbsF(rDeltaX);

            // Clamp down the search interval, relying on the monotonicity of X(t)
            if (rX > linearProgress)
            {
                rTop = rT;     // because parameter > solution
            }
            else
            {
                rBottom = rT;  // because parameter < solution
            }

            // The desired accuracy is in ultimately in y, not in x, so the
            // accuracy needs to be multiplied by dx/dy = (dx/dt) / (dy/dt).
            // But dy/dt <=3, so we omit that
            if (XcpAbsF(rX - linearProgress) < rAccuracy * rAbsDeltaX)
            {
                break; // We're there
            }

            if (rAbsDeltaX > rFuzz)
            {
                // Nonzero derivative, use Newton-Raphson to obtain the next guess
                float rNext = rT - (rX - linearProgress) / rDeltaX;

                // If next guess is out of the search interval then clamp it in
                if (rNext >= rTop)
                {
                    rT = (rT + rTop) / 2;
                }
                else if (rNext <= rBottom)
                {
                    rT = (rT + rBottom) / 2;
                }
                else
                {
                    // Next guess is inside the search interval, accept it
                    rT = rNext;
                }
            }
            else    // Zero derivative, halve the search interval
            {
                rT = (rBottom + rTop) / 2.0f;
            }
        }

// Cache this value to reduce next iteration search
        m_rLastT = rT;

// Compute Y as a function of T
        return ComputeSplineValueY(rT);
    }
}

