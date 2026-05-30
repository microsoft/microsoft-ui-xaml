// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AlgorithmHelper.h>

using namespace Focus;

double XYFocusPrivate::CalculatePrimaryAxisDistance(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& candidateBounds)
{
    double primaryAxisDistance = -1;
    bool isOverlapping = (DoRectsIntersect(bounds, candidateBounds) == TRUE);

    if (bounds == candidateBounds) { return -1; } //We shouldn't be calculating the distance from ourselves

    if (IsLeft(direction) && (candidateBounds.right <= bounds.left || (isOverlapping && candidateBounds.left <= bounds.left)))
    {
        primaryAxisDistance = std::abs(bounds.left - candidateBounds.right);
    }
    else if (IsRight(direction) && (candidateBounds.left >= bounds.right || (isOverlapping && candidateBounds.right >= bounds.right)))
    {
        primaryAxisDistance = std::abs(candidateBounds.left - bounds.right);
    }
    else if (IsUp(direction) && (candidateBounds.bottom <= bounds.top || (isOverlapping && candidateBounds.top <= bounds.top)))
    {
        primaryAxisDistance = std::abs(bounds.top - candidateBounds.bottom);
    }
    else if (IsDown(direction) && (candidateBounds.top >= bounds.bottom || (isOverlapping && candidateBounds.bottom >= bounds.bottom)))
    {
        primaryAxisDistance = std::abs(candidateBounds.top - bounds.bottom);
    }

    return primaryAxisDistance;
}

double XYFocusPrivate::CalculateSecondaryAxisDistance(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& candidateBounds)
{
    double secondaryAxisDistance;

    if (IsLeft(direction) || IsRight(direction))
    {
        // calculate secondary axis distance for the case where the element is not in the shadow
        secondaryAxisDistance = candidateBounds.top < bounds.top ? abs(bounds.top - candidateBounds.bottom) : abs(candidateBounds.top - bounds.bottom);
    }
    else
    {
        // calculate secondary axis distance for the case where the element is not in the shadow
        secondaryAxisDistance = candidateBounds.left < bounds.left ? abs(bounds.left - candidateBounds.right) : abs(candidateBounds.left - bounds.right);
    }

    return secondaryAxisDistance;
}

// Calculates the percentage of the potential element that is in the shadow of the reference element.
double XYFocusPrivate::CalculatePercentInShadow(
    _In_ const std::pair<double, double>& referenceManifold,
    _In_ const std::pair<double, double>& potentialManifold)
{
    if ((referenceManifold.first > potentialManifold.second) || (referenceManifold.second <= potentialManifold.first))
    {
        // Potential is not in the reference's shadow.
        return 0;
    }

    double shadow = std::min(referenceManifold.second, potentialManifold.second) - std::max(referenceManifold.first, potentialManifold.first);
    shadow = std::abs(shadow);

    double potentialEdgeLength = std::abs(potentialManifold.second - potentialManifold.first);
    double referenceEdgeLength = std::abs(referenceManifold.second - referenceManifold.first);

    double comparisonEdgeLength = referenceEdgeLength;

    if (comparisonEdgeLength >= potentialEdgeLength)
    {
        comparisonEdgeLength = potentialEdgeLength;
    }

    double percentInShadow = 1;

    if (comparisonEdgeLength != 0)
    {
        percentInShadow = std::min(shadow / comparisonEdgeLength, 1.0);
    }

    return percentInShadow;
}