// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XYFocusAlgorithms.h>
#include "AlgorithmHelper.h"

using namespace Focus::XYFocusPrivate;

const double XYFocusAlgorithms::INSHADOWTHRESHOLD = 0.25;
const double XYFocusAlgorithms::INSHADOWTHRESHOLD_FORSECONDARYAXIS = 0.02;
const double XYFocusAlgorithms::CONE_ANGLE = M_PI_4;

XYFocusAlgorithms::XYFocusAlgorithms()
{
    m_primaryAxisDistanceWeight = 15;
    m_secondaryAxisDistanceWeight = 1;
    m_percentInManifoldShadowWeight = 10000;
    m_percentInShadowWeight = 50;
}

double XYFocusAlgorithms::GetScore(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& candidateBounds,
    _Inout_ std::pair<double, double>& hManifold,
    _Inout_ std::pair<double, double>& vManifold,
    _In_ const double maxDistance)
{
    double score = 0;
    double primaryAxisDistance = maxDistance;
    double secondaryAxisDistance = maxDistance;
    double percentInManifoldShadow = 0;
    double percentInShadow = 0;

    std::pair<double, double> potential;
    std::pair<double, double> reference;
    std::pair<double, double> currentManifold;

    if (IsLeft(direction) || IsRight(direction))
    {
        reference.first = bounds.top;
        reference.second = bounds.bottom;

        currentManifold = hManifold;

        potential.first = candidateBounds.top;
        potential.second = candidateBounds.bottom;
    }
    else
    {
        reference.first = bounds.left;
        reference.second = bounds.right;

        currentManifold = vManifold;

        potential.first = candidateBounds.left;
        potential.second = candidateBounds.right;
    }

    primaryAxisDistance = CalculatePrimaryAxisDistance(direction, bounds, candidateBounds);
    secondaryAxisDistance = CalculateSecondaryAxisDistance(direction, bounds, candidateBounds);

    if (primaryAxisDistance >= 0)
    {
        percentInShadow = CalculatePercentInShadow(reference, potential);

        if (percentInShadow >= INSHADOWTHRESHOLD_FORSECONDARYAXIS)
        {
            percentInManifoldShadow = CalculatePercentInShadow(currentManifold, potential);
            secondaryAxisDistance = maxDistance;
        }

        // The score needs to be a positive number so we make these distances positive numbers
        primaryAxisDistance = maxDistance - primaryAxisDistance;
        secondaryAxisDistance = maxDistance - secondaryAxisDistance;

        if (percentInShadow >= INSHADOWTHRESHOLD)
        {
            percentInShadow = 1;
            primaryAxisDistance = primaryAxisDistance * 2;
        }

        // Potential elements in the shadow get a multiplier to their final score
        score = CalculateScore(percentInShadow, primaryAxisDistance, secondaryAxisDistance, percentInManifoldShadow);
    }

    return score;
}

void XYFocusAlgorithms::UpdateManifolds(
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& newFocusBounds,
    _Inout_ std::pair<double, double>& hManifold,
    _Inout_ std::pair<double, double>& vManifold)
{
    if (vManifold.second < 0)
    {
        vManifold = std::make_pair(bounds.left, bounds.right);
    }

    if (hManifold.second < 0)
    {
        hManifold = std::make_pair(bounds.top, bounds.bottom);
    }

    if (IsLeft(direction) || IsRight(direction))
    {
        hManifold.first = std::max(std::max((float)newFocusBounds.top, (float)bounds.top), (float)hManifold.first);
        hManifold.second = std::min(std::min((float)newFocusBounds.bottom, (float)bounds.bottom), (float)hManifold.second);

        // It's possible to get into a situation where the newFocusedElement to the right / left has no overlap with the current edge.
        if (hManifold.second <= hManifold.first)
        {
            hManifold.first = newFocusBounds.top;
            hManifold.second = newFocusBounds.bottom;
        }

        vManifold.first = newFocusBounds.left;
        vManifold.second = newFocusBounds.right;
    }
    else if (IsUp(direction) || IsDown(direction))
    {
        vManifold.first = std::max(std::max((float)newFocusBounds.left, (float)bounds.left), (float)vManifold.first);
        vManifold.second = std::min(std::min((float)newFocusBounds.right, (float)bounds.right), (float)vManifold.second);

        // It's possible to get into a situation where the newFocusedElement to the right / left has no overlap with the current edge.
        if (vManifold.second <= vManifold.first)
        {
            vManifold.first = newFocusBounds.left;
            vManifold.second = newFocusBounds.right;
        }

        hManifold.first = newFocusBounds.top;
        hManifold.second = newFocusBounds.bottom;
    }
}

double XYFocusAlgorithms::CalculateScore(
    _In_ const double percentInShadow,
    _In_ const double primaryAxisDistance,
    _In_ const double secondaryAxisDistance,
    _In_ const double percentInManifoldShadow)
{
    double score = (percentInShadow * m_percentInShadowWeight) +
        (primaryAxisDistance * m_primaryAxisDistanceWeight) +
        (secondaryAxisDistance * m_secondaryAxisDistanceWeight) +
        (percentInManifoldShadow * m_percentInManifoldShadowWeight);

    return score;
}

bool XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
    _In_ const XRECTF_RB& bounds,
    _In_ const XRECTF_RB& candidateBounds,
    _In_ double maxDistance,
    _In_ const DirectUI::FocusNavigationDirection direction,
    _In_ const XRECTF_RB& exclusionRect,
    _In_ bool ignoreCone)
{
    //Consider a candidate only if:
    // 1. It doesn't have an empty rect as it's bounds
    // 2. It doesn't contain currently focused element
    // 3. It's bounds don't intersect with the rect we were asked to avoid looking into (Exclusion Rect)
    // 4. It's bounds aren't contained in the rect we were asked to avoid looking into (Exclusion Rect)
    if (IsEmptyRectF(candidateBounds) ||
        DoesRectContainRect(&candidateBounds, &bounds) ||
        DoRectsIntersect(exclusionRect, candidateBounds) ||
        DoesRectContainRect(/*container*/&exclusionRect, /*contained element*/&candidateBounds))
    {
        return false;
    }

    //We've decided to disable the use of the cone for vertical navigation.
    if (ignoreCone || IsDown(direction) || IsUp(direction)) { return true; }

    XPOINTF originTop;
    XPOINTF originBottom;
    originTop.y = bounds.top;
    originBottom.y = bounds.bottom;

    XPOINTF candidateAsPoints[4];
    RectToPoint(candidateBounds, candidateAsPoints);

    //We make the maxDistance twice the normal distance to ensure that all the elements are encapsulated inside the cone. This
    //also aids in scenarios where the original max distance is still less than one of the points (due to the angles)
    maxDistance = maxDistance * 2;

    XPOINTF cone[4];
    //Note: our y axis is inverted
    if (IsLeft(direction))
    {
        // We want to start the origin one pixel to the left to cover overlapping scenarios where the end of a candidate element 
        // could be overlapping with the origin (before the shift)
        originTop.x = bounds.left - 1;
        originBottom.x = bounds.left - 1;

        XPOINTF sides[2];

        //We have two angles. Find a point (for each angle) on the line and rotate based on the direction
        const double rotation = M_PI; //180 degrees
        sides[0].x = (float)(originTop.x + maxDistance * std::cos(rotation + CONE_ANGLE));
        sides[0].y = (float)(originTop.y + maxDistance * std::sin(rotation + CONE_ANGLE));
        sides[1].x = (float)(originBottom.x + maxDistance * std::cos(rotation - CONE_ANGLE));
        sides[1].y = (float)(originBottom.y + maxDistance * std::sin(rotation - CONE_ANGLE));

        // order points in counter clockwise direction
        cone[0] = originTop;
        cone[1] = sides[0];
        cone[2] = sides[1];
        cone[3] = originBottom;
    }
    else if (IsRight(direction))
    {
        // We want to start the origin one pixel to the right to cover overlapping scenarios where the end of a candidate element 
        // could be overlapping with the origin (before the shift)
        originTop.x = bounds.right + 1;
        originBottom.x = bounds.right + 1;

        XPOINTF sides[2];

        //We have two angles. Find a point (for each angle) on the line and rotate based on the direction
        const double rotation = 0;
        sides[0].x = (float)(originTop.x + maxDistance * std::cos(rotation + CONE_ANGLE));
        sides[0].y = (float)(originTop.y + maxDistance * std::sin(rotation + CONE_ANGLE));
        sides[1].x = (float)(originBottom.x + maxDistance * std::cos(rotation - CONE_ANGLE));
        sides[1].y = (float)(originBottom.y + maxDistance * std::sin(rotation - CONE_ANGLE));

        // order points in counter clockwise direction
        cone[0] = originBottom;
        cone[1] = sides[0];
        cone[2] = sides[1];
        cone[3] = originTop;
    }

    //There are three scenarios we should check that will allow us to know whether we should consider the candidate element.
    //1) The candidate element and the vision cone intersect
    //2) The candidate element is completely inside the vision cone
    //3) The vision cone is completely inside the bounds of the candidate element (unlikely)

    return DoPolygonsIntersect(4, cone, 4, candidateAsPoints) || IsEntirelyContained(4, candidateAsPoints, 4, cone)
        || IsEntirelyContained(4, cone, 4, candidateAsPoints);
}
