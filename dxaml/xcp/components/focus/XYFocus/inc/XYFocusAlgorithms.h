// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"
#include "minxcptypes.h"

class XYFocusAlgorithms
{
public:

    XYFocusAlgorithms();

    double GetScore(
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& candidateBounds,
        _Inout_ std::pair<double, double>& hManifold,
        _Inout_ std::pair<double, double>& vManifold,
        _In_ const double maxDistance);

    static bool ShouldCandidateBeConsideredForRanking(
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& candidateBounds,
        _In_ double maxDistance,
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& exclusionRect,
        _In_ bool ignoreCone=false);

    static void UpdateManifolds(
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& newFocusBounds,
        _Inout_ std::pair<double, double>& hManifold,
        _Inout_ std::pair<double, double>& vManifold);

    void SetPrimaryAxisDistanceWeight(_In_ int primaryAxisDistanceWeight) { m_primaryAxisDistanceWeight = primaryAxisDistanceWeight; }
    void SetSecondaryAxisDistanceWeight(_In_ int secondaryAxisDistanceWeight) { m_secondaryAxisDistanceWeight = secondaryAxisDistanceWeight; }
    void SetPercentInManifoldShadowWeight(_In_ int percentInManifoldShadowWeight) { m_percentInManifoldShadowWeight = percentInManifoldShadowWeight; }
    void SetPercentInShadowWeight(_In_ int percentInShadowWeight) { m_percentInShadowWeight = percentInShadowWeight; }

private:
    static const double INSHADOWTHRESHOLD;
    static const double INSHADOWTHRESHOLD_FORSECONDARYAXIS;
    static const double CONE_ANGLE;

    double CalculateScore(
        _In_ const double percentInShadow,
        _In_ const double primaryAxisDistance,
        _In_ const double secondaryAxisDistance,
        _In_ const double percentInManifoldShadow);

    int m_primaryAxisDistanceWeight;
    int m_secondaryAxisDistanceWeight;
    int m_percentInManifoldShadowWeight;
    int m_percentInShadowWeight;
};
