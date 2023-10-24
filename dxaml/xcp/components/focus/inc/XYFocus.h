// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"
#include "minxcptypes.h"

#include <vector>

#include <CommonUtilities.h>

class CDependencyObject;
class VisualTree;
class RootVisual;

namespace Focus {

    struct XYFocusOptions
    {
        XYFocusOptions() :
            searchRoot(nullptr),
            exclusionRect(nullptr),
            focusedElementBounds(),
            focusHintRectangle(nullptr),
            ignoreClipping(true),
            ignoreCone(false),
            shouldConsiderXYFocusKeyboardNavigation(false),
            considerEngagement(true),
            updateManifold(true),
            navigationStrategyOverride(DirectUI::XYFocusNavigationStrategyOverride::None),
            updateManifoldsFromFocusHintRect(false),
            ignoreOcclusivity(false) { }

        size_t hash() const
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, this->searchRoot);
            CommonUtilities::hash_combine(hash, this->navigationStrategyOverride);
            CommonUtilities::hash_combine(hash, this->ignoreClipping);
            CommonUtilities::hash_combine(hash, this->ignoreCone);
            CommonUtilities::hash_combine(hash, this->exclusionRect);
            CommonUtilities::hash_combine(hash, this->focusHintRectangle);
            CommonUtilities::hash_combine(hash, this->ignoreOcclusivity);
            CommonUtilities::hash_combine(hash, this->updateManifoldsFromFocusHintRect);

            return hash;
        }

        CDependencyObject* searchRoot;

        XRECTF_RB* exclusionRect;
        XRECTF_RB* focusHintRectangle;

        XRECTF_RB focusedElementBounds;
        DirectUI::XYFocusNavigationStrategyOverride navigationStrategyOverride;

        bool ignoreClipping;
        bool ignoreCone;
        bool shouldConsiderXYFocusKeyboardNavigation;
        bool updateManifold;
        bool considerEngagement;
        bool ignoreOcclusivity;
        bool updateManifoldsFromFocusHintRect;
    private:
        size_t XRectFRBHash(_In_ XRECTF_RB* rect) const
        {
            std::size_t hash = 0;

            if (rect == nullptr) { return hash; }

            CommonUtilities::hash_combine(hash, rect->left);
            CommonUtilities::hash_combine(hash, rect->right);
            CommonUtilities::hash_combine(hash, rect->top);
            CommonUtilities::hash_combine(hash, rect->bottom);

            return hash;
        }
    };

    class XYFocus
    {
    public:

        struct XYFocusParams
        {
            XYFocusParams() : element(nullptr), bounds(), score(0) {}

            CDependencyObject* element;
            XRECTF_RB bounds;
            double score;
        };

        struct Manifolds
        {
            Manifolds()
            {
                Reset();
            }

            void Reset()
            {
                vManifold = std::make_pair(-1.0, -1.0);
                hManifold = std::make_pair(-1.0, -1.0);
            }

            std::pair<double, double> vManifold;
            std::pair<double, double> hManifold;
        };

        Manifolds ResetManifolds();
        void SetManifolds(_In_ Manifolds& manifolds);

        CDependencyObject* GetNextFocusableElement(
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_opt_ CDependencyObject* element,
            _In_opt_ CDependencyObject* engagedControl,
            _In_ VisualTree* visualTree,
            _In_ bool updateManifolds,
            _In_ XYFocusOptions& xyFocusOptions);

        void UpdateManifolds(
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ const XRECTF_RB& elementBounds,
            _In_ CDependencyObject* const candidate,
            _In_ bool ignoreClipping);

        void SetPrimaryAxisDistanceWeight(int primaryAxisDistanceWeight);
        void SetSecondaryAxisDistanceWeight(int secondaryAxisDistanceWeight);
        void SetPercentInManifoldShadowWeight(int percentInManifoldShadowWeight);
        void SetPercentInShadowWeight(int percentInShadowWeight);
        void ClearCache();

    private:
        CDependencyObject* ChooseBestFocusableElementFromList(
            _In_ std::vector<XYFocusParams>& scoreList,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ VisualTree* visualTree,
            _In_ const XRECTF_RB* bounds,
            _In_ bool ignoreClipping,
            _In_ bool ignoreOcclusivity,
            _In_ bool isRightToLeft,
            _In_ bool updateManifolds);

        static std::vector<Focus::XYFocus::XYFocusParams> GetAllValidFocusableChildren(
            _In_ CDependencyObject* startRoot,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ const CDependencyObject* currentElement,
            _In_ CDependencyObject* engagedControl,
            _In_ CDependencyObject* searchScope,
            _In_ VisualTree* visualTree,
            _In_ const CDependencyObject* activeScroller,
            _In_ bool ignoreClipping,
            _In_ bool shouldConsiderXYFocusKeyboardNavigation);

        void RankElements(
            _Inout_ std::vector<XYFocusParams>& candidateList,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ const XRECTF_RB* bounds,
            _In_ const double maxRootBoundsDistance,
            _In_ DirectUI::XYFocusNavigationStrategy mode,
            _In_ XRECTF_RB* exclusionRect,
            _In_opt_ bool ignoreClipping,
            _In_opt_ bool ignoreCone);

        static double GetMaxRootBoundsDistance(
            _In_ const std::vector<XYFocusParams>& list,
            _In_ const XRECTF_RB& bounds,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_opt_ bool ignoreClipping);

        static const CDependencyObject* const GetActiveScrollerForScrollInput(
            _In_ const DirectUI::FocusNavigationDirection direction,
            _In_opt_ CDependencyObject* const focusedElement);

        static bool IsHorizontalNavigationDirection(
            _In_ const DirectUI::FocusNavigationDirection direction);

        static bool IsVerticalNavigationDirection(
            _In_ const DirectUI::FocusNavigationDirection direction);

        static std::size_t ExploredListHash(
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ CDependencyObject* element,
            _In_opt_ CDependencyObject* engagedControl,
            _In_ const XYFocusOptions& xyFocusOptions);

        static void FocusWalkTraceBegin(_In_ DirectUI::FocusNavigationDirection direction);
        static void CacheHitTrace(_In_ DirectUI::FocusNavigationDirection direction);

        Manifolds m_manifolds;

        std::vector<std::size_t> m_exploredList;
    };
}
