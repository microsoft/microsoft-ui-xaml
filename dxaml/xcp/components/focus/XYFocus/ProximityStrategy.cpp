// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ProximityStrategy.h>
#include <AlgorithmHelper.h>

using namespace Focus::XYFocusPrivate;

double ProximityStrategy::GetScore(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& candidateBounds,
    _In_ const double maxDistance,
    _In_ bool considerSecondaryAxis)
{
    double score = 0;

    double primaryAxisDistance = CalculatePrimaryAxisDistance(direction, bounds, candidateBounds);
    double secondaryAxisDistance = CalculateSecondaryAxisDistance(direction, bounds, candidateBounds);

    if (primaryAxisDistance >= 0)
    {
        // We do not want to use the secondary axis if the candidate is within the shadow of the element
        std::pair<double, double> potential;
        std::pair<double, double> reference;

        if (IsLeft(direction) || IsRight(direction))
        {
            reference.first = bounds.top;
            reference.second = bounds.bottom;

            potential.first = candidateBounds.top;
            potential.second = candidateBounds.bottom;
        }
        else
        {
            reference.first = bounds.left;
            reference.second = bounds.right;

            potential.first = candidateBounds.left;
            potential.second = candidateBounds.right;
        }

        if (considerSecondaryAxis == false || CalculatePercentInShadow(reference, potential) != 0)
        {
            secondaryAxisDistance = 0;
        }

        score = maxDistance - (primaryAxisDistance + secondaryAxisDistance);
    }

    return score;
}