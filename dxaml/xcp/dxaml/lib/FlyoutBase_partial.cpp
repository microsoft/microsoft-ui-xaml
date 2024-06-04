// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      FlyoutBase - base class for Flyout provides the following functionality:
//        * Showing/hiding.
//        * Enforcing requirement that only one FlyoutBase is open at the time.
//        * Placement logic.
//        * Raising events.
//        * Entrance/exit transitions.
//        * Expose Attached property as storage from Flyouts in XAML.

#include "precomp.h"

#include "FlyoutBase_partial.h"

#include "FlyoutBaseClosingEventArgs.g.h"
#include "Popup.g.h"
#include "Control.g.h"
#include "PickerFlyoutThemeTransition.g.h"
#include "PopupThemeTransition.g.h"
#include "PopupRoot.g.h"
#include "TransitionCollection.g.h"
#include "AppBar.g.h"
#include "Window.g.h"
#include "Page.g.h"
#include "InputServices.h"
#include "FocusMgr.h"
#include "VisualTreeHelper.h"
#include <windows.graphics.display.h>
#include <LightDismissOverlayHelper.h>
#include <FocusSelection.h>
#include "Flyout.g.h"
#include "FlyoutPresenter.g.h"
#include "FlyoutShowOptions.g.h"
#include "RootScrollViewer.g.h"
#include "IApplicationBarService.h"
#include "AutomationProperties.h"
#include "CommandBar.g.h"
#include "XamlRoot_Partial.h"
#include "microsoft.ui.xaml.phone.h"
#include "RootScale.h"
#include "ElementSoundPlayerService_Partial.h"
#include "MenuFlyoutPresenter_Partial.h"
#include "XamlTelemetry.h"
#include <Microsoft.UI.Input.Partner.h>

using namespace std::placeholders;

// #define DBG_FLYOUT

#if defined(DBG) && defined(DBG_FLYOUT)
#define DBG_FLYOUT_TRACE(msg) TRACE(TraceAlways, msg)
#else
#define DBG_FLYOUT_TRACE(msg) (void) (msg)
#endif

using namespace DirectUI;
using namespace DirectUISynonyms;
using xaml_primitives::FlyoutPlacementMode;
using xaml::FlowDirection;

const xaml_primitives::FlyoutPlacementMode FlyoutBase::g_defaultPlacementMode = xaml_primitives::FlyoutPlacementMode_Top;

// Margin between FlyoutBase and window's edge, AppBar and placement target.
const FLOAT FlyoutBase::FlyoutMargin = 4;

// Offsets for PopupThemeTransition
const DOUBLE FlyoutBase::g_entranceThemeOffset = 50.0;

const WCHAR FlyoutBase::c_visualStateLandscape[] = L"Landscape";
const WCHAR FlyoutBase::c_visualStatePortrait[] = L"Portrait";

//------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------


FlyoutBase::PreferredJustification GetJustificationFromPlacementMode(xaml_primitives::FlyoutPlacementMode placement)
{
    auto result = FlyoutBase::PreferredJustification::Center;

    switch (placement)
    {
    case xaml_primitives::FlyoutPlacementMode_Full:
    case xaml_primitives::FlyoutPlacementMode_Top:
    case xaml_primitives::FlyoutPlacementMode_Bottom:
    case xaml_primitives::FlyoutPlacementMode_Left:
    case xaml_primitives::FlyoutPlacementMode_Right:
        result = FlyoutBase::PreferredJustification::Center;
        break;
    case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedLeft:
    case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedLeft:
        result = FlyoutBase::PreferredJustification::Left;
        break;
    case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedRight:
    case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedRight:
        result = FlyoutBase::PreferredJustification::Right;
        break;
    case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedTop:
    case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedTop:
        result = FlyoutBase::PreferredJustification::Top;
        break;
    case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedBottom:
    case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedBottom:
        result = FlyoutBase::PreferredJustification::Bottom;
        break;
    default:
        ASSERT(FALSE, L"Unsupported FlyoutPlacementMode");
        break;
    }

    return result;
}

FlyoutBase::MajorPlacementMode GetMajorPlacementFromPlacement(xaml_primitives::FlyoutPlacementMode placement)
{
    FlyoutBase::MajorPlacementMode result = FlyoutBase::MajorPlacementMode::Full;

    switch (placement)
    {
    case xaml_primitives::FlyoutPlacementMode_Full:
        result = FlyoutBase::MajorPlacementMode::Full;
        break;
    case xaml_primitives::FlyoutPlacementMode_Top:
    case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedLeft:
    case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedRight:
        result = FlyoutBase::MajorPlacementMode::Top;
        break;
    case xaml_primitives::FlyoutPlacementMode_Bottom:
    case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedLeft:
    case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedRight:
        result = FlyoutBase::MajorPlacementMode::Bottom;
        break;
    case xaml_primitives::FlyoutPlacementMode_Left:
    case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedTop:
    case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedBottom:
        result = FlyoutBase::MajorPlacementMode::Left;
        break;
    case xaml_primitives::FlyoutPlacementMode_Right:
    case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedTop:
    case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedBottom:
        result = FlyoutBase::MajorPlacementMode::Right;
        break;
    default:
        ASSERT(FALSE, L"Unsupported FlyoutPlacementMode");
        break;
    }

    return result;
}

xaml_primitives::FlyoutPlacementMode GetPlacementFromMajorPlacement(FlyoutBase::MajorPlacementMode majorPlacement)
{
    xaml_primitives::FlyoutPlacementMode result = xaml_primitives::FlyoutPlacementMode_Full;

    switch(majorPlacement)
    {
    case FlyoutBase::MajorPlacementMode::Full:
        result = xaml_primitives::FlyoutPlacementMode_Full;
        break;
    case FlyoutBase::MajorPlacementMode::Top:
        result = xaml_primitives::FlyoutPlacementMode_Top;
        break;
    case FlyoutBase::MajorPlacementMode::Bottom:
        result = xaml_primitives::FlyoutPlacementMode_Bottom;
        break;
    case FlyoutBase::MajorPlacementMode::Left:
        result = xaml_primitives::FlyoutPlacementMode_Left;
        break;
    case FlyoutBase::MajorPlacementMode::Right:
        result = xaml_primitives::FlyoutPlacementMode_Right;
        break;
    default:
        ASSERT(FALSE, L"Unsupported FlyoutPlacementMode");
        break;
    }

    return result;
}

// Return true if placement is along vertical axis, false otherwise.
static inline BOOLEAN IsPlacementModeVertical(
    _In_ FlyoutBase::MajorPlacementMode placementMode)
{
    // We are safe even if placementMode is Full. because the case for placementMode is Full has already been put in another if branch in function PerformPlacement.
    // if necessary, we can add another function : IsPlacementModeHorizontal
    return (placementMode == FlyoutBase::MajorPlacementMode::Top ||
            placementMode == FlyoutBase::MajorPlacementMode::Bottom);
}

// Helper for finding parent of element whose type is IAppBar.
static _Check_return_ HRESULT FindParentAppBar(
    _In_ IFrameworkElement* pElement,
    _Out_ IAppBar** ppParentAppBar)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spCurrent;

    *ppParentAppBar = nullptr;
    spCurrent = pElement;

    while (spCurrent)
    {
        ctl::ComPtr<xaml::IDependencyObject> spNextAsDO;
        ctl::ComPtr<IAppBar> spAppBar = spCurrent.AsOrNull<IAppBar>();

        if (spAppBar)
        {
            IFC(spAppBar.MoveTo(ppParentAppBar));
            break;
        }

        IFC(spCurrent->get_Parent(&spNextAsDO));
        spCurrent = spNextAsDO.AsOrNull<IFrameworkElement>();
    }

Cleanup:
    RRETURN(hr);
}

// Helper which depending on FlowDirection will either directly copy placementMode to adjustedPlacementMode (LTR)
// or will reverse Left/Right if order is RTL.
static void GetEffectivePlacementMode(
    _In_ FlyoutBase::MajorPlacementMode placementMode,
    _In_ xaml::FlowDirection flowDirection,
    _Out_ FlyoutBase::MajorPlacementMode* pAdjustedPlacementMode)
{
    *pAdjustedPlacementMode = placementMode;

    if (flowDirection == xaml::FlowDirection_RightToLeft)
    {
        if (placementMode == FlyoutBase::MajorPlacementMode::Left)
        {
            *pAdjustedPlacementMode = FlyoutBase::MajorPlacementMode::Right;
        }
        else if (placementMode == FlyoutBase::MajorPlacementMode::Right)
        {
            *pAdjustedPlacementMode = FlyoutBase::MajorPlacementMode::Left;
        }
    }
}

// Fails if placementMode is not a valid value.
static inline _Check_return_ HRESULT ValidateFlyoutPlacementMode(
    _In_ xaml_primitives::FlyoutPlacementMode placementMode)
{
    switch (placementMode)
    {
        case xaml_primitives::FlyoutPlacementMode_Top:
        case xaml_primitives::FlyoutPlacementMode_Bottom:
        case xaml_primitives::FlyoutPlacementMode_Left:
        case xaml_primitives::FlyoutPlacementMode_Right:
        case xaml_primitives::FlyoutPlacementMode_Full:
        case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedLeft:
        case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedRight:
        case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedLeft:
        case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedRight:
        case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedTop:
        case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedBottom:
        case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedTop:
        case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedBottom:
            return S_OK;
        default:
            return E_INVALIDARG;
    }
}

// Helper for getting minimum size of FrameworkElement.
static inline _Check_return_ HRESULT GetMinimumSize(
    _In_ IFrameworkElement* pFrameworkElement,
    _Out_ wf::Size* pMinSize)
{
    HRESULT hr = S_OK;
    DOUBLE minWidth = 0.0;
    DOUBLE minHeight = 0.0;

    IFC(pFrameworkElement->get_MinWidth(&minWidth));
    IFC(pFrameworkElement->get_MinHeight(&minHeight));
    pMinSize->Width = static_cast<FLOAT>(minWidth);
    pMinSize->Height = static_cast<FLOAT>(minHeight);

Cleanup:
    RRETURN(hr);
}

// Helper for getting maximum size of FrameworkElement.
static inline _Check_return_ HRESULT GetMaximumSize(
    _In_ IFrameworkElement* pFrameworkElement,
    _Out_ wf::Size* pMaxSize)
{
    HRESULT hr = S_OK;
    DOUBLE maxWidth = 0.0;
    DOUBLE maxHeight = 0.0;

    IFC(pFrameworkElement->get_MaxWidth(&maxWidth));
    IFC(pFrameworkElement->get_MaxHeight(&maxHeight));
    pMaxSize->Width = static_cast<FLOAT>(maxWidth);
    pMaxSize->Height = static_cast<FLOAT>(maxHeight);

Cleanup:
    RRETURN(hr);
}


// Placement helpers

// Align centers of anchor and control while keeping control coordinates within limits.
static BOOLEAN TestAndCenterAlignWithinLimits(
    _In_ DOUBLE anchorPos,
    _In_ DOUBLE anchorSize,
    _In_ DOUBLE controlSize,
    _In_ DOUBLE lowLimit,
    _In_ DOUBLE highLimit,
    _In_ FlyoutBase::PreferredJustification justification,
    _Out_ DOUBLE* pControlPos)
{
    BOOLEAN fits = TRUE;

    ASSERT(anchorSize >= 0.0);
    ASSERT(controlSize >= 0.0);

    if ((highLimit - lowLimit) > controlSize &&
        anchorSize >= 0.0 &&
        controlSize >= 0.0)
    {

        if (justification == FlyoutBase::PreferredJustification::Center)
        {
            *pControlPos = anchorPos + 0.5 * (anchorSize - controlSize);
        }
        else if (justification == FlyoutBase::PreferredJustification::Top || justification == FlyoutBase::PreferredJustification::Left)
        {
            *pControlPos = anchorPos;
        }
        else if (justification == FlyoutBase::PreferredJustification::Bottom || justification == FlyoutBase::PreferredJustification::Right)
        {
            *pControlPos = anchorPos + (anchorSize - controlSize);
        }
        else
        {
            ASSERT(FALSE, L"Unsupported FlyoutBase::PreferredJustification");
        }

        if (*pControlPos < lowLimit)
        {
            *pControlPos = lowLimit;
        }
        else if (*pControlPos + controlSize > highLimit)
        {
            *pControlPos = highLimit - controlSize;
        }
    }
    else
    {
        *pControlPos = lowLimit;
        fits = FALSE;
    }

    return fits;
}

// Test if control can fit in space between anchor and limit.  Regardless of outcome, calculate
// position and clamp it against limits and anchor.
static BOOLEAN TestAgainstLimitsAndPlace(
    _In_ DOUBLE anchorPos,
    _In_ DOUBLE anchorSize,
    _In_ BOOLEAN isIncreasing,
    _In_ DOUBLE controlSize,
    _In_ DOUBLE lowLimit,
    _In_ DOUBLE highLimit,
    _Out_ DOUBLE* pControlPos)
{
    BOOLEAN fits = TRUE;
    DOUBLE extraSpace = 0.0;
    DOUBLE anchorEnd = anchorPos + anchorSize;

    ASSERT(anchorSize >= 0.0);
    ASSERT(controlSize >= 0.0);

    *pControlPos = lowLimit;

    // Pick the correct limit for comparison.
    if (isIncreasing)
    {
        extraSpace = highLimit - anchorEnd - controlSize;
        *pControlPos = anchorPos + anchorSize;
    }
    else
    {
        extraSpace = anchorPos - lowLimit - controlSize;
        *pControlPos = anchorPos - controlSize;
    }

    if (extraSpace < 0.0)
    {
        fits = FALSE;
    }

    // Clamp against limits...

    if (*pControlPos < lowLimit)
    {
        *pControlPos = lowLimit;
    }
    else if (*pControlPos + controlSize > highLimit)
    {
        *pControlPos = highLimit - controlSize;
    }

    return fits;
}

// Test if control has enough space to fit on the primary axis and center on the secondary axis.
// Even if it does not have enough room calculate the position.
static BOOLEAN TryPlacement(
    _In_ const wf::Rect& placementTarget,
    _In_ const wf::Size& controlSize,
    _In_ const wf::Rect& containerRect,
    _In_ FlyoutBase::MajorPlacementMode placementMode,
    _In_ FlyoutBase::PreferredJustification justification,
    _Out_ wf::Point* pControlPos)
{
    DOUBLE controlXPos = 0.0;
    DOUBLE controlYPos = 0.0;
    BOOLEAN placementFits = FALSE;

    pControlPos->X = 0.0;
    pControlPos->Y = 0.0;

    ASSERT(placementMode != FlyoutBase::MajorPlacementMode::Full, L"PlacementMode should not be Full when we come here.");

    if (IsPlacementModeVertical(placementMode))
    {
        if (TestAgainstLimitsAndPlace(
            placementTarget.Y,
            placementTarget.Height,
            (placementMode == FlyoutBase::MajorPlacementMode::Bottom) ? TRUE : FALSE,
            controlSize.Height,
            containerRect.Y,
            containerRect.Y + containerRect.Height,
            &controlYPos))
        {
            placementFits = TRUE;
        }

        placementFits &= TestAndCenterAlignWithinLimits(
            placementTarget.X,
            placementTarget.Width,
            controlSize.Width,
            containerRect.X,
            containerRect.X + containerRect.Width,
            justification,
            &controlXPos);
    }
    else
    {
        if (TestAgainstLimitsAndPlace(
            placementTarget.X,
            placementTarget.Width,
            (placementMode == FlyoutBase::MajorPlacementMode::Right) ? TRUE : FALSE,
            controlSize.Width,
            containerRect.X,
            containerRect.X + containerRect.Width,
            &controlXPos))
        {
            placementFits = TRUE;
        }

        placementFits &= TestAndCenterAlignWithinLimits(
            placementTarget.Y,
            placementTarget.Height,
            controlSize.Height,
            containerRect.Y,
            containerRect.Y + containerRect.Height,
            justification,
            &controlYPos);
    }

    pControlPos->X = static_cast<FLOAT>(controlXPos);
    pControlPos->Y = static_cast<FLOAT>(controlYPos);

    return placementFits;
}

// Try to place a control according to fallback order.  If none of the fallbacks produces result which does not
// violate placement rules, use the first one specified.
// Return true if control can fit entirely without adjusting its size.
static BOOLEAN PerformPlacementWithFallback(
    _In_ const wf::Rect& placementTarget,
    _In_ const wf::Size& controlSize,
    _In_ const wf::Rect& containerRect,
    _In_reads_(placementCount) FlyoutBase::MajorPlacementMode orderedPlacements[],
    _In_ INT placementCount,
    _In_ FlyoutBase::PreferredJustification justification,
    _Out_ wf::Point* pControlPos,
    _Out_ FlyoutBase::MajorPlacementMode* pEffectiveMode)
{
    wf::Point firstChoicePos = {};

    ASSERT(placementCount > 0);

    pControlPos->X = 0.0;
    pControlPos->Y = 0.0;
    *pEffectiveMode = FlyoutBase::MajorPlacementMode::Top;

    // Try the initial placement and retain the coordinate regardless of the outcome.
    if (TryPlacement(placementTarget, controlSize, containerRect, orderedPlacements[0], justification, &firstChoicePos))
    {
        *pEffectiveMode = orderedPlacements[0];
        *pControlPos = firstChoicePos;
        return TRUE;
    }
    else
    {
        // If failed, work through the fallback list.
        BOOLEAN fallbackSuccessful = FALSE;

        for (INT placementIndex = 1; placementIndex < placementCount; ++placementIndex)
        {
            if (TryPlacement(placementTarget, controlSize, containerRect, orderedPlacements[placementIndex], justification, pControlPos))
            {
                *pEffectiveMode = orderedPlacements[placementIndex];
                fallbackSuccessful = TRUE;
                break;
            }
        }

        // If none of the fallbacks was successful, go back to the first one.
        if (!fallbackSuccessful)
        {
            *pEffectiveMode = orderedPlacements[0];
            *pControlPos = firstChoicePos;
        }

        return fallbackSuccessful;
    }
}

// Calculates space between container boundary and placement target for a given placement mode.
static DOUBLE CalculateAvailableSpace(
    _In_ FlyoutBase::MajorPlacementMode placement,
    _In_ const wf::Rect& placementTargetBounds,
    _In_ const wf::Rect& containerRect)
{
    DOUBLE availableSpace = 0.0;

    switch (placement)
    {
        case FlyoutBase::MajorPlacementMode::Top:
            availableSpace = placementTargetBounds.Y - containerRect.Y;
            availableSpace = DoubleUtil::Min(containerRect.Height, availableSpace);
            break;

        case FlyoutBase::MajorPlacementMode::Bottom:
            availableSpace = (containerRect.Y + containerRect.Height) - (placementTargetBounds.Y + placementTargetBounds.Height);
            availableSpace = DoubleUtil::Min(containerRect.Height, availableSpace);
            break;

        case FlyoutBase::MajorPlacementMode::Left:
            availableSpace = placementTargetBounds.X - containerRect.X;
            availableSpace = DoubleUtil::Min(containerRect.Width, availableSpace);
            break;

        case FlyoutBase::MajorPlacementMode::Right:
            availableSpace = (containerRect.X + containerRect.Width) - (placementTargetBounds.X + placementTargetBounds.Width);
            availableSpace = DoubleUtil::Min(containerRect.Width, availableSpace);
            break;

        default:
            ASSERT(FALSE, L"Unsupported FlyoutPlacementMode");
            break;
    }

    availableSpace = DoubleUtil::Max(0.0, availableSpace);

    return availableSpace;
}

// Update placement mode and calculate available space for the best placement mode
// along the same axis as specified by pPlacement.
static void SelectSideWithMoreSpace(
    _Inout_ FlyoutBase::MajorPlacementMode* pPlacement,
    _In_ const wf::Rect& placementTargetBounds,
    _In_ const wf::Rect& containerRect,
    _Out_ DOUBLE* pAvailableSpace)
{
    *pAvailableSpace = 0.0;

    ASSERT(*pPlacement != FlyoutBase::MajorPlacementMode::Full, L"PlacementMode should not be Full when we come here.");

    if (IsPlacementModeVertical(*pPlacement))
    {
        DOUBLE availableSpaceTop = CalculateAvailableSpace(
            FlyoutBase::MajorPlacementMode::Top,
            placementTargetBounds,
            containerRect);

        DOUBLE availableSpaceBottom = CalculateAvailableSpace(
            FlyoutBase::MajorPlacementMode::Bottom,
            placementTargetBounds,
            containerRect);

        if (availableSpaceTop > availableSpaceBottom)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Top;
            *pAvailableSpace = availableSpaceTop;
        }
        else if (availableSpaceTop < availableSpaceBottom)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Bottom;
            *pAvailableSpace = availableSpaceBottom;
        }
        else
        {
            *pAvailableSpace = availableSpaceTop;
        }
    }
    else
    {
        DOUBLE availableSpaceLeft = CalculateAvailableSpace(
            FlyoutBase::MajorPlacementMode::Left,
            placementTargetBounds,
            containerRect);

        DOUBLE availableSpaceRight = CalculateAvailableSpace(
            FlyoutBase::MajorPlacementMode::Right,
            placementTargetBounds,
            containerRect);

        if (availableSpaceLeft > availableSpaceRight)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Left;
            *pAvailableSpace = availableSpaceLeft;
        }
        else if (availableSpaceLeft < availableSpaceRight)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Right;
            *pAvailableSpace = availableSpaceRight;
        }
        else
        {
            *pAvailableSpace = availableSpaceLeft;
        }
    }
}

// Size and position control to fill available space constrained by container rectangle.
static void ResizeToFit(
    _Inout_ FlyoutBase::MajorPlacementMode* pPlacement,
    _In_ const wf::Rect& placementTargetBounds,
    _In_ const wf::Rect& containerRect,
    _In_ const wf::Size& minControlSize,
    _Inout_ wf::Point* pControlPos,
    _Inout_ wf::Size* pControlSize)
{
    DOUBLE availableSpace = CalculateAvailableSpace(
        *pPlacement,
        placementTargetBounds,
        containerRect);

    ASSERT(*pPlacement != FlyoutBase::MajorPlacementMode::Full, L"PlacementMode should not be Full when we come here.");

    // For primary axis, if exceeding available size on side specified by pPlacement
    // find side with more space and adjust position and size to be next to target.
    // For the secondary axis, if exceeding container size, set it to fill all available space.

    if (IsPlacementModeVertical(*pPlacement))
    {
        if (pControlSize->Height > availableSpace)
        {
            FlyoutBase::MajorPlacementMode moreSpacePlacement = *pPlacement;
            DOUBLE moreAvailableSpace = 0.0;

            SelectSideWithMoreSpace(
                &moreSpacePlacement,
                placementTargetBounds,
                containerRect,
                &moreAvailableSpace);

            // Switch sides only if there's enough space to fit minimum size Flyout.

            if (moreSpacePlacement != *pPlacement && moreAvailableSpace >= minControlSize.Height)
            {
                *pPlacement = moreSpacePlacement;
                availableSpace = moreAvailableSpace;
            }

            // Adjust size only if control has enough space to fit without going under minimum size.

            if (pControlSize->Height > availableSpace && availableSpace >= minControlSize.Height)
            {
                pControlSize->Height = static_cast<FLOAT>(availableSpace);
            }

            // Clamp against container size.

            if (pControlSize->Height > containerRect.Height)
            {
                pControlSize->Height = static_cast<FLOAT>(containerRect.Height);
            }

            // Adjust position to be next to placement target.
            // Note that the position will be corrected not to exceed boundaries at the end of this function.

            if (*pPlacement == FlyoutBase::MajorPlacementMode::Top)
            {
                pControlPos->Y = placementTargetBounds.Y - pControlSize->Height;
            }
            else
            {
                pControlPos->Y = placementTargetBounds.Y + placementTargetBounds.Height;
            }
        }

        if (pControlSize->Width > containerRect.Width)
        {
            pControlSize->Width = containerRect.Width;
            pControlPos->X = containerRect.X;
        }
    }
    else
    {
        if (pControlSize->Width > availableSpace)
        {
            FlyoutBase::MajorPlacementMode moreSpacePlacement = *pPlacement;
            DOUBLE moreAvailableSpace = 0.0;

            SelectSideWithMoreSpace(
                &moreSpacePlacement,
                placementTargetBounds,
                containerRect,
                &moreAvailableSpace);

            // Switch sides only if there's enough space to fit minimum size Flyout.

            if (moreSpacePlacement != *pPlacement && moreAvailableSpace >= minControlSize.Width)
            {
                *pPlacement = moreSpacePlacement;
                availableSpace = moreAvailableSpace;
            }

            // Adjust size only if control has enough space to fit without going under minimum size.

            if (pControlSize->Width > availableSpace && availableSpace >= minControlSize.Width)
            {
                pControlSize->Width = static_cast<FLOAT>(availableSpace);
            }

            // Clamp against container size.

            if (pControlSize->Width > containerRect.Width)
            {
                pControlSize->Width = static_cast<FLOAT>(containerRect.Width);
            }

            // Adjust position to be next to placement target.
            // Note that the position will be corrected not to exceed boundaries at the end of this function.

            if (*pPlacement == FlyoutBase::MajorPlacementMode::Left)
            {
                pControlPos->X = placementTargetBounds.X - pControlSize->Width;
            }
            else
            {
                pControlPos->X = placementTargetBounds.X + placementTargetBounds.Width;
            }
        }

        if (pControlSize->Height > containerRect.Height)
        {
            pControlSize->Height = containerRect.Height;
            pControlPos->Y = containerRect.Y;
        }
    }

    // Finally, make sure we do not exceed container boundaries.

    if (pControlPos->X < containerRect.X)
    {
        pControlPos->X = containerRect.X;
    }
    else if ((pControlPos->X + pControlSize->Width) > (containerRect.X + containerRect.Width))
    {
        pControlPos->X = (containerRect.X + containerRect.Width) - pControlSize->Width;
    }

    if (pControlPos->Y < containerRect.Y)
    {
        pControlPos->Y = containerRect.Y;
    }
    else if ((pControlPos->Y + pControlSize->Height) > (containerRect.Y + containerRect.Height))
    {
        pControlPos->Y = (containerRect.Y + containerRect.Height) - pControlSize->Height;
    }
}

//------------------------------------------------------------------------
// FlyoutBase
//------------------------------------------------------------------------

FlyoutBase::FlyoutBase()
    : m_allowPlacementFallbacks(FALSE)
    , m_cachedInputPaneHeight(0.0)
    , m_presenterResized(false)
    , m_isTargetPositionSet(FALSE)
    , m_isPositionedAtPoint(false)
    , m_usePickerFlyoutTheme(FALSE)
    , m_allowPresenterResizing(true)
    , m_isFlyoutPresenterRequestedThemeOverridden(false)
    , m_isLightDismissOverlayEnabled(TRUE)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::FlyoutBase()");

    m_majorPlacementMode = GetMajorPlacementFromPlacement(g_defaultPlacementMode);
}

FlyoutBase::~FlyoutBase()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::~FlyoutBase()");

    VERIFYHR(DetachHandler(m_epPresenterSizeChangedHandler, m_tpPresenter));
    VERIFYHR(DetachHandler(m_epPresenterLoadedHandler, m_tpPresenter));
    VERIFYHR(DetachHandler(m_epPresenterUnloadedHandler, m_tpPresenter));
    VERIFYHR(DetachHandler(m_epPopupLostFocusHandler, m_tpPopup));
    VERIFYHR(DetachHandler(m_epPlacementTargetUnloadedHandler, m_tpPlacementTarget));
    VERIFYHR(DetachHandler(m_epPlacementTargetActualThemeChangedHandler, m_tpPlacementTarget));

    if (DXamlCore* core = DXamlCore::GetCurrent())
    {
        core->SetTextControlFlyout(this, nullptr);
    }
}

_Check_return_ HRESULT
FlyoutBase::OnPropertyChanged2(const PropertyChangedParams& args)
{
    IFC_RETURN(__super::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::FlyoutBase_LightDismissOverlayMode:
        {
            // The overlay is configured whenever we open the popup,
            // so if this property changes while it's open, then we
            // need to re-configure it.
            if (m_tpPopup)
            {
                BOOLEAN isOpen = FALSE;
                IFC_RETURN(m_tpPopup->get_IsOpen(&isOpen));
                if (isOpen)
                {
                    IFC_RETURN(ConfigurePopupOverlay());
                }
            }
            break;
        }

        case KnownPropertyIndex::FlyoutBase_OverlayInputPassThroughElement:
        {
            m_ownsOverlayInputPassThroughElement = false;
            break;
        }

        case KnownPropertyIndex::FlyoutBase_ShowMode:
        {
            xaml_primitives::FlyoutShowMode showMode = xaml_primitives::FlyoutShowMode_Standard;
            IFC_RETURN(get_ShowMode(&showMode));
            IFC_RETURN(UpdateStateToShowMode(showMode));
            break;
        }

        case KnownPropertyIndex::FlyoutBase_ShouldConstrainToRootBounds:
        {
            if (m_tpPopup)
            {
                BOOLEAN shouldConstrainToRootBounds;
                IFC_RETURN(get_ShouldConstrainToRootBounds(&shouldConstrainToRootBounds));
                IFC_RETURN(m_tpPopup.Cast<Popup>()->put_ShouldConstrainToRootBounds(shouldConstrainToRootBounds));
            }
        }

        default:
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::CreatePresenterImpl(
    _Outptr_ IControl** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFC(CreatePresenterProtected(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

// Associates this flyout with the correct XamlRoot for its context, or returns an error
// if the context is ambiguous.
_Check_return_ HRESULT FlyoutBase::EnsureAssociatedXamlRoot(_In_opt_ xaml::IDependencyObject* placementTarget)
{
    ctl::ComPtr<DirectUI::DependencyObject> placementTargetDO;
    if (placementTarget)
    {
        IFC_RETURN(placementTarget->QueryInterface(IID_PPV_ARGS(&placementTargetDO)));
    }

    VisualTree* visualTree = nullptr;
    IFC_RETURN(VisualTree::GetUniqueVisualTreeNoRef(
        GetHandle(),
        placementTargetDO == nullptr ? nullptr : placementTargetDO->GetHandle(),
        nullptr,
        &visualTree));

    if (visualTree)
    {
        visualTree->AttachElement(GetHandle());
    }
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ShowAtImpl(
    _In_ IFrameworkElement* pPlacementTarget)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::ShowAtImpl()");

    ctl::ComPtr<IFrameworkElement> placementTarget(pPlacementTarget);
    ctl::ComPtr<xaml::IDependencyObject> placementTargetAsDO;

    IFC_RETURN(placementTarget.As(&placementTargetAsDO));

    IFC_RETURN(EnsureAssociatedXamlRoot(placementTargetAsDO.Get()));

    ctl::ComPtr<FlyoutShowOptions> showOptions;
    IFC_RETURN(ctl::make(&showOptions));

    xaml_primitives::FlyoutShowMode showMode = xaml_primitives::FlyoutShowMode_Standard;
    IFC_RETURN(get_ShowMode(&showMode));
    IFC_RETURN(showOptions->put_ShowMode(showMode));

    ctl::ComPtr<IFlyoutShowOptions> showOptionsAsI;
    IFC_RETURN(showOptions.As(&showOptionsAsI));
    IFC_RETURN(ShowAtWithOptions(placementTargetAsDO.Get(), showOptionsAsI.Get()));

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ShowAtWithOptionsImpl(
    _In_opt_ xaml::IDependencyObject* pPlacementTarget,
    _In_opt_ xaml_primitives::IFlyoutShowOptions* pShowOptions)
{
    PerfXamlEvent_RAII perfXamlEvent(reinterpret_cast<uint64_t>(this), "FlyoutBase::ShowAt[WithOptions]", true);

    DBG_FLYOUT_TRACE(L">>> FlyoutBase::ShowAtWithOptionsImpl()");

    BOOLEAN shouldConstrainToRootBounds;
    IFC_RETURN(get_ShouldConstrainToRootBounds(&shouldConstrainToRootBounds));

    auto scopeGuard = wil::scope_exit([&]
    {
        m_openingWindowedInProgress = false;
    });

    // MenuFlyouts are always windowed, whereas other flyouts are windowed if ShouldConstrainToRootBounds is false.
    m_openingWindowedInProgress = !shouldConstrainToRootBounds || ctl::is<xaml_controls::IMenuFlyout>(this);

    ctl::ComPtr<xaml::IDependencyObject> placementTargetAsDO(pPlacementTarget);
    ctl::ComPtr<IFlyoutShowOptions> showOptions(pShowOptions);
    ctl::ComPtr<wf::IReference<wf::Point>> position;
    ctl::ComPtr<wf::IReference<wf::Rect>> exclusionRect;
    xaml_primitives::FlyoutShowMode showMode = xaml_primitives::FlyoutShowMode_Auto;
    xaml_primitives::FlyoutPlacementMode placement = xaml_primitives::FlyoutPlacementMode_Auto;
    ctl::ComPtr<xaml_media::IGeneralTransform> transformToRoot;
    ctl::ComPtr<IFrameworkElement> visualRootAsFE;

    m_hasPlacementOverride = false;

    if (showOptions)
    {
        IFC_RETURN(showOptions->get_Position(&position));
        IFC_RETURN(showOptions->get_ExclusionRect(&exclusionRect));
        IFC_RETURN(showOptions->get_ShowMode(&showMode));
        IFC_RETURN(showOptions->get_Placement(&placement));
    }

    ctl::ComPtr<IUIElement> placementTarget = placementTargetAsDO.AsOrNull<IUIElement>();
    if (placementTarget == nullptr && position == nullptr)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_FLYOUT_EMPTYSHOWOPTIONS));
    }

    IFC_RETURN(EnsureAssociatedXamlRoot(placementTargetAsDO.Get()));
    ctl::ComPtr<xaml::IXamlRoot> xamlRoot;
    IFC_RETURN(this->get_XamlRoot(&xamlRoot));

    // The call to EnsureAssociatedXamlRoot guarantees that if it succeeds we'll have a valid xamlRoot pointer.
    FAIL_FAST_ASSERT(xamlRoot != nullptr);

    // If we're going to need to transform to root, let's create the transform now.
    if (placementTarget && (position || exclusionRect))
    {
        IFC_RETURN(placementTarget->TransformToVisual(nullptr, &transformToRoot));
    }

    if (position)
    {
        m_isPositionedAtPoint = true;

        wf::Point positionValue;
        IFC_RETURN(position->get_Value(&positionValue));

        if (transformToRoot)
        {
            IFC_RETURN(transformToRoot->TransformPoint(positionValue, &positionValue));
        }

        IFC_RETURN(EnsurePopupAndPresenter());

        wf::Rect contentRect = {};

        if (IsWindowedPopup())
        {
            IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(m_tpPopup.Cast<Popup>(), positionValue, &contentRect));
        }
        else
        {
            wf::Size contentSize = {};
            IFC_RETURN(xamlRoot->get_Size(&contentSize));

            contentRect.X = 0;
            contentRect.Y = 0;
            contentRect.Width = contentSize.Width;
            contentRect.Height = contentSize.Height;
        }

        if (DoubleUtil::IsNaN(positionValue.X) || DoubleUtil::IsNaN(positionValue.Y))
        {
            IFC_RETURN(E_INVALIDARG);
        }
        else
        {
            // Adjust the target position such that it is within the contentRect boundaries.
            positionValue.X = std::clamp(positionValue.X, contentRect.X, contentRect.X + contentRect.Width);
            positionValue.Y = std::clamp(positionValue.Y, contentRect.Y, contentRect.Y + contentRect.Height);

            // Set the target position to show the flyout
            SetTargetPosition(positionValue);
        }
    }
    else
    {
        m_isPositionedAtPoint = false;
    }

    if (exclusionRect)
    {
        wf::Rect exclusionRectValue;
        IFC_RETURN(exclusionRect->get_Value(&exclusionRectValue));

        if (transformToRoot)
        {
            IFC_RETURN(transformToRoot->TransformBounds(exclusionRectValue, &exclusionRectValue));
        }

        m_exclusionRect = exclusionRectValue;
    }
    else
    {
        m_exclusionRect = RectUtil::CreateEmptyRect();
    }

    IFC_RETURN(UpdateStateToShowMode(showMode));

    if (placement != xaml_primitives::FlyoutPlacementMode_Auto)
    {
        m_hasPlacementOverride = true;
        m_placementOverride = placement;
    }

    // Show at the target element, if target is null use the frame as target
    if (placementTarget)
    {
        IFC_RETURN(placementTarget.As(&visualRootAsFE));
    }
    else
    {
        ctl::ComPtr<IUIElement> rootElement;
        IFC_RETURN(xamlRoot->get_Content(&rootElement));
        IFC_RETURN(rootElement.As(&visualRootAsFE));
    }

    bool openDelayed = false;
    HRESULT hr = ShowAtCore(visualRootAsFE.Get(), openDelayed);
    BOOLEAN isOpen = FALSE;

    if (SUCCEEDED(hr))
    {
        // Set popup flow direction must be called after ShowAt
        // to ensure the placement target in the flyout is initialized.
        hr = ForwardPopupFlowDirection();

        // Check if the popup successfully opened unless the opening was intentionally
        // delayed for instance when a submenu closes and another one opens.
        if (!openDelayed && SUCCEEDED(hr))
        {
            hr = get_IsOpenImpl(&isOpen);
        }
    }

    if (FAILED(hr) || (!openDelayed && !isOpen))
    {
        // Since the popup actually did not open, reset the m_hasPlacementOverride flag so that
        // it has the correct value at the next opening attempt.
        m_hasPlacementOverride = false;
    }

    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ShowAtCore(
    _In_ xaml::IFrameworkElement* pPlacementTarget,
    _Out_ bool& openDelayed)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::ShowAtCore()");

    openDelayed = false;

    auto oldPlacementTarget = m_tpPlacementTarget.Get();

    bool shouldOpen = false;
    IFC_RETURN(ValidateAndSetParameters(pPlacementTarget, &shouldOpen));

    // If we shouldn't open, we'll early out here.
    if (!shouldOpen)
    {
        return S_OK;
    }

    ctl::ComPtr<xaml::IDependencyObject> placementTargetDO;
    IFC_RETURN(pPlacementTarget->QueryInterface(IID_PPV_ARGS(&placementTargetDO)))
    IFC_RETURN(EnsureAssociatedXamlRoot(placementTargetDO.Get()));

    // Determine whether this flyout is opening as a child flyout.
    ctl::ComPtr<FlyoutBase> parentFlyout;
    IFC_RETURN(FindParentFlyoutFromElement(m_tpPlacementTarget.Get(), &parentFlyout));
    if (parentFlyout)
    {
        // Allocate the metadata object for the parent's instance to track
        // the child flyouts.
        if (!parentFlyout->m_childFlyoutMetadata)
        {
            IFC_RETURN(ctl::make(&parentFlyout->m_childFlyoutMetadata));
        }

        // By setting the parent reference, we're marking ourselves as being a child flyout.
        IFC_RETURN(SetPtrValueWithQI(m_tpParentFlyout, parentFlyout.Get()));
    }

    // Determine whether there is already an open flyout tracked by the appropriate
    // flyout metadata, and stage this instance until it closes rather than opening
    // immediately.
    IFC_RETURN(CheckAndHandleOpenFlyout(&shouldOpen));

    if (shouldOpen)
    {
        IFC_RETURN(Open());

        // If our placement target has changed, then we'll be reopening the flyout and placement will happen on presenter load.
        // Otherwise, we should perform placement now, as that step may not happen.
        if (oldPlacementTarget == m_tpPlacementTarget.Get())
        {
            wf::Size presenterSize = {};
            IFC_RETURN(GetPresenterSize(&presenterSize));

            // RS3: Adjust the placement based on the screen size. Skip the PreparePopupTheme call
            // to avoid setting m_tpThemeTransition for compatibility with RS2.
            IFC_RETURN(PerformPlacement(presenterSize, 0.0, false /*preparePopupTheme*/));
        }
    }
    else
    {
        openDelayed = true;
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::HideImpl()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::HideImpl()");

    // Provide an opportunity for the app to cancel hiding the flyout.
    bool cancel = false;
    IFC_RETURN(OnClosing(&cancel));
    if (cancel)
    {
        return S_OK;
    }

    // If we're in the process of opening a flyout, a call to Hide() should cancel that.
    m_openingCanceled = true;

    if (m_childFlyoutMetadata)
    {
        // Hide any child flyouts above this instance.  We do this
        // here rather than letting the presenter unloaded handler
        // handle this to produce the effect of the entire chain of
        // child flyouts closing at the same time.  If we waited for
        // the presenter unloaded handler, you would see the chain
        // close one at a time from the bottom up, which would produce
        // an obvious delay.
        ctl::ComPtr<IFlyoutBase> childFlyout;
        IFC_RETURN(m_childFlyoutMetadata->GetOpenFlyout(&childFlyout, nullptr));
        if (childFlyout)
        {
            IFC_RETURN(childFlyout->Hide());
        }
    }

    IFC_RETURN(RemoveRootVisualPointerMovedHandler());

    m_wrRootVisual.Reset();

    if (m_tpPopup)
    {
        // Since popup is light-dismiss all closing logic is in unloaded event handler.  Here, we only hide the popup.
        IFC_RETURN(m_tpPopup->put_IsOpen(FALSE));
    }

    return S_OK;
}

// This will be called from either ShowAtImpl if there were no staged Flyouts or from OnPresenterUnloaded when
// previous FlyoutBase was closed.  Parameters should be validated and cached prior to calling this method.
_Check_return_ HRESULT FlyoutBase::Open()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::Open()");

    // Placement target should be validated and set.
    ASSERT(m_tpPlacementTarget.Get() != nullptr);

    if (auto publicRootVisualCore = m_tpPlacementTarget.Cast<FrameworkElement>()->GetHandle()->GetPublicRootVisual())
    {
        ctl::ComPtr<DependencyObject> publicRootVisual;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(publicRootVisualCore, &publicRootVisual));

        IFC_RETURN(publicRootVisual.AsWeak(&m_wrRootVisual));
    }

    ctl::ComPtr<FlyoutMetadata> flyoutMetadata;
    IFC_RETURN(GetFlyoutMetadata(&flyoutMetadata));

    m_isPositionedForDateTimePicker = false;

#ifdef DBG
    {
        ctl::ComPtr<IFlyoutBase> openFlyout;
        ctl::ComPtr<IFrameworkElement> openFlyoutPlacementTarget;

        IGNOREHR(flyoutMetadata->GetOpenFlyout(&openFlyout, &openFlyoutPlacementTarget));

        ASSERT(!openFlyout);
        ASSERT(!openFlyoutPlacementTarget);
    }
#endif

    IFC_RETURN(EnsurePopupAndPresenter());

    // Set how the popup will be dismissed. This needs to be done each time the flyout is
    // opened because the placement mode may change. In theory we should wait until the
    // placement logic has been executed to get the final placement, but the light dismiss
    // value must be set before the popup is opened to have an effect.
    IFC_RETURN(SetPopupLightDismissBehavior());

    // Configure the popup to have an overlay (depends on the value of LightDismissOverlayMode),
    // as well as set the correct brush to use.
    IFC_RETURN(ConfigurePopupOverlay());

    IFC_RETURN(ForwardTargetPropertiesToPresenter());

    ctl::ComPtr<IFrameworkElement> presenterAsFE;
    IFC_RETURN(m_tpPresenter.As(&presenterAsFE));
    if (m_majorPlacementMode == MajorPlacementMode::Full)
    {
        // In full mode we don't need to measure the presenter's size to determine placement.
        // By default Popup will give it infinite size, which we don't want for perf reasons.
        // The actual size will be set after the popup is loaded. For now we'll just set size to 1.
        IFC_RETURN(presenterAsFE->put_Width(1.0));
        IFC_RETURN(presenterAsFE->put_Height(1.0));
    }
    else
    {
        // Reset forced resizing of width/height to get content's auto dimensions.
        IFC_RETURN(presenterAsFE->put_Width(DoubleUtil::NaN));
        IFC_RETURN(presenterAsFE->put_Height(DoubleUtil::NaN));
    }

    m_presenterResized = false;

    IFC_RETURN(OnOpening());

    if (m_openingCanceled)
    {
        return S_OK;
    }

    IFC_RETURN(ApplyTargetPosition());

    if (m_shouldHideIfPointerMovesAway)
    {
        IFC_RETURN(AddRootVisualPointerMovedHandler());
    }

    IFC_RETURN(CoreImports::Popup_SetShouldTakeFocus(m_tpPopup.Cast<Popup>()->GetHandle(), !!m_shouldTakeFocus));
    IFC_RETURN(m_tpPopup.Cast<Popup>()->put_Opacity(1.0));

    xaml_primitives::FlyoutPlacementMode placementMode = g_defaultPlacementMode;
    IFC_RETURN(GetEffectivePlacement(&placementMode));
    m_majorPlacementMode = GetMajorPlacementFromPlacement(placementMode);
    m_preferredJustification = GetJustificationFromPlacementMode(placementMode);

    flyoutMetadata->SetOpenFlyout(this, m_tpPlacementTarget.Get());

    if (VisualTree* visualTree = GetHandle()->GetVisualTree())
    {
        // Currently we can't set Popup.XamlRoot here because that API doesn't allow the Popup to move between
        // two different XamlRoots/VisualTrees, and we want to support that here.  Instead, we use SetVisualTree
        // which doesn't enforce this.  This is safe as long as Popup isn't active.
        CPopup* corePopup = static_cast<CPopup*>(m_tpPopup.Cast<DirectUI::Popup>()->GetHandle());
        ASSERT(!corePopup->IsActive());
        corePopup->SetVisualTree(visualTree);
    }

    IFC_RETURN(m_tpPopup->put_IsOpen(TRUE));

    // Note: This is the call that propagates the SystemBackdrop object set on either this FlyoutBase or its
    // MenuFlyoutPresenter to the Popup. A convenient place to do this is in ForwardTargetPropertiesToPresenter, but we
    // see cases where the MenuFlyoutPresenter's SystemBackdrop property is null until it enters the tree via the
    // Popup::Open call above. So trying to propagate it before opening the popup actually finds no SystemBackdrop, and
    // the popup is left with a transparent background. Do the propagation after the popup opens instead. Windowed
    // popups support having a backdrop set after the popup is open.
    IFC_RETURN(ForwardSystemBackdropToPopup());

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::EnsurePopupAndPresenter()
{
    if (!m_tpPopup)
    {
        ctl::ComPtr<IControl> spPresenter;
        ctl::ComPtr<IFlyoutPresenter> spPresenterAsFlyoutPresenter;
        ctl::ComPtr<IUIElement> spPresenterAsUI;
        ctl::ComPtr<Popup> spPopup;

        IFC_RETURN(ctl::make(&spPopup));

        // Callback to derived class to create and initialize presenter instance.
        IFC_RETURN(FlyoutBaseGenerated::CreatePresenter(&spPresenter));

        spPresenterAsFlyoutPresenter = spPresenter.AsOrNull<IFlyoutPresenter>();

        if (spPresenterAsFlyoutPresenter)
        {
            IFC_RETURN(spPresenterAsFlyoutPresenter.Cast<FlyoutPresenter>()->put_Flyout(this));
        }

        IFC_RETURN(PreparePresenter(spPresenter.Get()));

        // Hookup presenter to popup.
        IFC_RETURN(spPresenter.As(&spPresenterAsUI));
        IFC_RETURN(spPopup->put_Child(spPresenterAsUI.Get()));

        BOOLEAN shouldConstrainToRootBounds;
        IFC_RETURN(get_ShouldConstrainToRootBounds(&shouldConstrainToRootBounds));
        IFC_RETURN(spPopup->put_ShouldConstrainToRootBounds(shouldConstrainToRootBounds));
        IFC_RETURN(spPopup->put_AssociatedFlyout(this));

        ctl::ComPtr<IUIElement> spPopupAsUI;
        IFC_RETURN(spPopup.As(&spPopupAsUI));

        IFC_RETURN(m_epPopupLostFocusHandler.AttachEventHandler(
            spPopupAsUI.Get(),
            std::bind(&FlyoutBase::OnPopupLostFocus, this, _1, _2)));

        IFC_RETURN(SetPtrValueWithQI(m_tpPresenter, spPresenter.Get()));

        // Set IsDialog property to True for popup if inherited class is Flyout
        if (ctl::is<xaml_controls::IFlyout>(this))
        {
            IFC_RETURN(DirectUI::AutomationProperties::SetIsDialogStatic(spPopup.Cast<UIElement>(), TRUE));
        }

        ctl::ComPtr<IDependencyObject> spPresenterAsDO;
        IFC_RETURN(spPresenter.As(&spPresenterAsDO));

        if (spPresenterAsDO.Cast<DependencyObject>()->GetHandle()->IsPropertyDefault(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AutomationId)) &&
            !GetHandle()->IsPropertyDefault(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AutomationId)))
        {
            wrl_wrappers::HString automationId;
            IFC_RETURN(DirectUI::AutomationProperties::GetAutomationIdStatic(this, automationId.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetAutomationIdStatic(spPresenterAsDO.Get(), automationId.Get()));
        }

        if (spPresenterAsDO.Cast<DependencyObject>()->GetHandle()->IsPropertyDefault(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Name)) &&
            !GetHandle()->IsPropertyDefault(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Name)))
        {
            wrl_wrappers::HString automationName;
            IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(this, automationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(spPresenterAsDO.Get(), automationName.Get()));
        }

        IFC_RETURN(SetPtrValueWithQI(m_tpPopup, spPopup.Get()));
    }

    ASSERT(m_tpPresenter);
    ASSERT(m_tpPopup);

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::SetPopupLightDismissBehavior()
{
    IFC_RETURN(m_tpPopup->put_IsLightDismissEnabled(m_isLightDismissOverlayEnabled));

    if (m_shouldOverlayPassThroughAllInput)
    {
        ctl::ComPtr<xaml::IDependencyObject> overlayInputPassThroughElement;
        IFC_RETURN(get_OverlayInputPassThroughElement(&overlayInputPassThroughElement));

        if (!overlayInputPassThroughElement)
        {
            ctl::ComPtr<IUIElement> rootVisual;
            IFC_RETURN(m_wrRootVisual.As(&rootVisual));

            if (rootVisual)
            {
                ctl::ComPtr<xaml::IDependencyObject> rootVisualAsDO;
                IFC_RETURN(rootVisual.As(&rootVisualAsDO));

                IFC_RETURN(put_OverlayInputPassThroughElement(rootVisualAsDO.Get()));
                m_ownsOverlayInputPassThroughElement = true;
            }
        }
    }
    else if (m_ownsOverlayInputPassThroughElement)
    {
        IFC_RETURN(put_OverlayInputPassThroughElement(nullptr));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ForwardTargetPropertiesToPresenter()
{
    ctl::ComPtr<IInspectable> spDataContext;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    ctl::ComPtr<IFrameworkElement> spPresenterAsFE;
    wrl_wrappers::HString strLanguage;
    BOOLEAN isTextScaleFactorEnabled = TRUE;
    BOOLEAN allowFocusOnInteraction = TRUE;
    BOOLEAN allowFocusWhenDisabled = FALSE;

    ASSERT(m_tpPresenter.Get() != nullptr);
    ASSERT(m_tpPlacementTarget.Get() != nullptr);

    IFC_RETURN(m_tpPresenter.As(&spPresenterAsFE));

    IFC_RETURN(m_tpPlacementTarget->get_DataContext(&spDataContext));
    IFC_RETURN(spPresenterAsFE->put_DataContext(spDataContext.Get()));

    IFC_RETURN(m_tpPlacementTarget->get_FlowDirection(&flowDirection));
    IFC_RETURN(spPresenterAsFE->put_FlowDirection(flowDirection));

    IFC_RETURN(m_tpPlacementTarget->get_Language(strLanguage.GetAddressOf()));
    IFC_RETURN(spPresenterAsFE->put_Language(strLanguage.Get()));

    IFC_RETURN(m_tpPlacementTarget.Cast<FrameworkElement>()->get_IsTextScaleFactorEnabledInternal(&isTextScaleFactorEnabled));
    IFC_RETURN(spPresenterAsFE.Cast<FrameworkElement>()->put_IsTextScaleFactorEnabledInternal(isTextScaleFactorEnabled));

    IFC_RETURN(get_AllowFocusOnInteraction(&allowFocusOnInteraction));

    // First check to see if FlyoutBase has the property set to false. If not, then pull from the placement target instead,
    // unless:
    //    - This flyout is acting as a sub-menu, in which case AllowFocusOnInteraction = true is required for keyboarding
    //      to function correctly when we open a sub-menu via pointer. Or,
    //    - This flyout is a MenuFlyout, which should always be free to take focus when it opens so that the user can use
    //      arrow keys to navigate between menu items immediately without first pressing Tab.
    //
    // Note that this value will get set on the presenter, which will allow the CPopup to set focus on its child when it
    // opens. This has the side effect of going through FocusManagerXamlIslandAdapter::SetFocus to also put Win32 focus
    // on this Xaml island's hwnd. That Win32 focus is necessary for the IInputKeyboardSource2 to raise the LostFocus
    // event, which we're relying on for light dismiss.
    if (allowFocusOnInteraction
        && !GetHandle()->OfTypeByIndex<KnownTypeIndex::MenuFlyout>())
    {
        bool isSubMenu = false;

        if (auto thisAsIMenu = ctl::query_interface_cast<IMenu>(this))
        {
            ctl::ComPtr<IMenu> parentMenu;
            IFC_RETURN(thisAsIMenu->get_ParentMenu(&parentMenu));

            isSubMenu = static_cast<bool>(parentMenu);
        }

        if (!isSubMenu)
        {
            IFC_RETURN(m_tpPlacementTarget.Cast<FrameworkElement>()->get_AllowFocusOnInteraction(&allowFocusOnInteraction));
        }
    }

    IFC_RETURN(spPresenterAsFE.Cast<FrameworkElement>()->put_AllowFocusOnInteraction(allowFocusOnInteraction))

    IFC_RETURN(get_AllowFocusWhenDisabled(&allowFocusWhenDisabled));

    //First check to see if FlyoutBase has the property set to true. If not, then pull from the placement target instead
    if (!allowFocusWhenDisabled)
    {
        IFC_RETURN(m_tpPlacementTarget.Cast<FrameworkElement>()->get_AllowFocusWhenDisabled(&allowFocusWhenDisabled));
    }

    IFC_RETURN(spPresenterAsFE.Cast<FrameworkElement>()->put_AllowFocusWhenDisabled(allowFocusWhenDisabled));

    IFC_RETURN(ForwardThemeToPresenter());
    
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ForwardThemeToPresenter()
{
    ctl::ComPtr<IFrameworkElement> spPresenterAsFE;
    IFC_RETURN(m_tpPresenter.As(&spPresenterAsFE));

    // Ensure the Flyout matches the theme of its placement target.
   const CDependencyProperty* requestedThemeProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_RequestedTheme);

    // We'll only override the requested theme on the presenter if its value hasn't been explicitly set.
    // Otherwise, we'll abide by its existing value.
    if (m_tpPresenter.Cast<Control>()->GetHandle()->IsPropertyDefault(requestedThemeProperty) ||
        m_isFlyoutPresenterRequestedThemeOverridden)
    {
        xaml::ElementTheme currentFlyoutPresenterTheme;
        xaml::ElementTheme requestedTheme = xaml::ElementTheme_Default;
        ctl::ComPtr<IDependencyObject> spCurrent;
        ctl::ComPtr<IDependencyObject> spParent;
        ctl::ComPtr<IFrameworkElement> spCurrentAsFE;

        IFC_RETURN(spPresenterAsFE->get_RequestedTheme(&currentFlyoutPresenterTheme));

        // Walk up the tree from the placement target until we find an element with a RequestedTheme.
        IFC_RETURN(m_tpPlacementTarget.As(&spCurrent));
        while (spCurrent)
        {
            IFC_RETURN(spCurrent.CopyTo(spCurrentAsFE.ReleaseAndGetAddressOf()));

            IFC_RETURN(spCurrentAsFE->get_RequestedTheme(&requestedTheme));

            if (requestedTheme != xaml::ElementTheme_Default)
            {
                break;
            }

            IFC_RETURN(VisualTreeHelper::GetParentStatic(spCurrent.Get(), spParent.ReleaseAndGetAddressOf()));
            if (spParent.AsOrNull<DirectUI::PopupRoot>())
            {
                // If the target is in a Popup and the Popup is in the Visual Tree, we want to inherrit the theme
                // from that Popup's parent. Otherwise we will get the App's theme, which might not be what
                // is expected.
                IFC_RETURN(spCurrentAsFE->get_Parent(&spParent));
            }

            spCurrent = spParent;
        }

        if (requestedTheme != currentFlyoutPresenterTheme)
        {
            IFC_RETURN(spPresenterAsFE->put_RequestedTheme(requestedTheme));
            m_isFlyoutPresenterRequestedThemeOverridden = true;
        }

        // Also set the popup's theme. If there is a SystemBackdrop on the menu, it'll be watching the theme on the
        // popup itself rather than the presenter set as the popup's child.
        IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->put_RequestedTheme(requestedTheme));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::ForwardSystemBackdropToPopup()
{
    // Propagate the SystemBackdrop on the FlyoutBasePresenter to the underlying Popup. The FlyoutBase.SystemBackdrop property
    // is just a convenient place to store the SystemBackdrop object for flyouts. Popup is the place that actually
    // implements IXP's ICompositionSupportsSystemBackdrop interface to interop with IXP's SystemBackdropControllers and
    // to kick off calls to SystemBackdrop.OnTargetConnected/OnTargetDisconnected.
    ctl::ComPtr<ISystemBackdrop> flyoutSystemBackdrop = GetSystemBackdrop();
    ctl::ComPtr<ISystemBackdrop> popupSystemBackdrop;
    IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->get_SystemBackdrop(&popupSystemBackdrop));
    if (flyoutSystemBackdrop.Get() != popupSystemBackdrop.Get())
    {
        IFC_RETURN(m_tpPopup.Cast<DirectUI::PopupGenerated>()->put_SystemBackdrop(flyoutSystemBackdrop.Get()));
    }

    return S_OK;
}

ctl::ComPtr<ISystemBackdrop> FlyoutBase::GetSystemBackdrop()
{
    //
    // There are multiple places that have a SystemBackdrop property:
    //
    //   1. Popups
    //
    //      Popups are the place that actually implement IXP's ICompositionSupportsSystemBackdrop interface to interop
    //      with IXP's SystemBackdropControllers and calls SystemBackdrop.OnTargetConnected/OnTargetDisconnected.
    //
    //   2. FlyoutBase
    //
    //      FlyoutBase.SystemBackdrop is a convenient place to store the SystemBackdrop object for flyouts, and is meant
    //      to be used in a code-behind. However, FlyoutBase can't be styled, which means we can't set the
    //      SystemBackdrop property from generic.xaml. For that, we need...
    //
    //   3. Flyout presenters (e.g. MenuFlyoutPresenter)
    //
    //      The presenter inside the flyout is the control that can actually be styled, so it has a SystemBackdrop
    //      property as well.
    //
    // The FlyoutBase.SystemBackdrop property (set via a code-behind) takes precedence over the Flyout presenter's
    // SystemBackdrop property (set via generic.xaml or via a style). Ultimately we take the SystemBackdrop set on
    // either the flyout or the presenter and set it on the underlying Popup.
    //
    ctl::ComPtr<ISystemBackdrop> systemBackdrop;
    ctl::ComPtr<MenuFlyoutPresenter> menuFlyoutPresenter;

    IFCFAILFAST(get_SystemBackdrop(&systemBackdrop));
    if (!systemBackdrop)
    {
        if (SUCCEEDED(m_tpPresenter.As(&menuFlyoutPresenter)))
        {
            IFCFAILFAST(menuFlyoutPresenter->get_SystemBackdrop(&systemBackdrop));
        }
    }
    return systemBackdrop;
}

_Check_return_ HRESULT FlyoutBase::ValidateAndSetParameters(
    _In_ IFrameworkElement* pPlacementTarget,
    _Out_ bool *shouldOpen)
{
    xaml_primitives::FlyoutPlacementMode placementMode = g_defaultPlacementMode;
    ctl::ComPtr<IFrameworkElement> spPlacementTarget;

    IFC_RETURN(GetEffectivePlacement(&placementMode));
    m_preferredJustification = GetJustificationFromPlacementMode(placementMode);
    auto majorplacementMode = GetMajorPlacementFromPlacement(placementMode);
    IFC_RETURN(ValidateFlyoutPlacementMode(placementMode));

    spPlacementTarget = pPlacementTarget;

    if (!spPlacementTarget)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    m_majorPlacementMode = majorplacementMode;
    IFC_RETURN(SetPlacementTarget(pPlacementTarget));

    // Find out placement target bounds when it's guaranteed to be in visual tree in case if it goes away
    // (as would be the case in flyout invoked by element from another flyout).
    IFC_RETURN(CalculatePlacementTargetBoundsPrivate(
          &m_majorPlacementMode,
          spPlacementTarget.Get(),
          &m_placementTargetBounds,
          &m_allowPlacementFallbacks));

    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(spPlacementTarget.AsOrNull<DependencyObject>()->GetHandle());

    m_inputDeviceTypeUsedToOpen = contentRoot->GetInputManager().GetLastInputDeviceType();

    // If the placement target isn't in the live tree, then there's nothing to anchor ourselves on.
    // In that case, we'll silently not open the flyout.
    *shouldOpen = spPlacementTarget.Cast<FrameworkElement>()->IsInLiveTree();
    return S_OK;
}

_Check_return_ HRESULT
FlyoutBase::CheckAndHandleOpenFlyout(_Out_ bool* shouldOpen)
{
    *shouldOpen = true;

    ctl::ComPtr<FlyoutMetadata> flyoutMetadata;
    IFC_RETURN(GetFlyoutMetadata(&flyoutMetadata));

    ctl::ComPtr<IFlyoutBase> openFlyout;
    ctl::ComPtr<IFrameworkElement> openFlyoutPlacementTarget;
    IFC_RETURN(flyoutMetadata->GetOpenFlyout(&openFlyout, &openFlyoutPlacementTarget));

    if (openFlyout)
    {
        *shouldOpen = false;

        bool isSameFlyoutAsOpen = ctl::are_equal(openFlyout.Get(), this);
        bool isSameTargetAsOpen = ctl::are_equal(openFlyoutPlacementTarget.Get(), m_tpPlacementTarget.Get());

        const bool isSamePosition =
            (!m_isTargetPositionSet && !m_wasTargetPositionSet) ||
            (m_isTargetPositionSet && m_wasTargetPositionSet &&
                m_targetPoint.X == m_lastTargetPoint.X &&
                m_targetPoint.Y == m_lastTargetPoint.Y);

        if (isSameFlyoutAsOpen && isSameTargetAsOpen && isSamePosition)
        {
            // Calling ShowAt on open FlyoutBase with the same placement target and at the same location.  Ignoring.
        }
        else
        {
            ctl::ComPtr<IFlyoutBase> stagedFlyout;
            ctl::ComPtr<IFrameworkElement> stagedFlyoutPlacementTarget;
            IFC_RETURN(flyoutMetadata->GetStagedFlyout(&stagedFlyout, &stagedFlyoutPlacementTarget));

            if (stagedFlyout)
            {
                // There is an open AND staged FlyoutBase.  Since we honor the latest request,
                // update staged FlyoutBase to this.
                flyoutMetadata->SetStagedFlyout(this, m_tpPlacementTarget.Get());

                // Clear the staged flyout's parent reference to no longer mark it as a child flyout.
                stagedFlyout.Cast<FlyoutBase>()->m_tpParentFlyout.Clear();
            }
            else
            {
                // Either this is a different FlyoutBase or it is the same, but placement target is
                // different AND there is no outstanding request.  Hide previous instance and
                // queue this one to be shown.
                ctl::ComPtr<IControl> presenter = openFlyout.Cast<FlyoutBase>()->GetPresenter();

                IFC_RETURN(openFlyout->Hide());
                flyoutMetadata->SetStagedFlyout(this, m_tpPlacementTarget.Get());
            }
        }
    }

    m_wasTargetPositionSet = m_isTargetPositionSet;
    m_lastTargetPoint = m_targetPoint;

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::PerformPlacement(
    wf::Size presenterSize,
    DOUBLE bottomAppBarVerticalCorrection,
    bool preparePopupTheme)
{
    HRESULT hr = S_OK;

    wf::Rect availableWindowRect = {};
    wf::Rect presenterRect = {};
    MajorPlacementMode effectivePlacementMode = GetMajorPlacementFromPlacement(g_defaultPlacementMode);
    xaml::FlowDirection placementTargetFlowDirection = xaml::FlowDirection_LeftToRight;
    Control* pPresenterAsControl = nullptr;
    Popup* pPopupNoRef = m_tpPopup.Cast<Popup>();
    BOOLEAN isPlacementPropertyLocal = FALSE;

    ASSERT(m_tpPopup.Get() != nullptr);
    ASSERT(m_tpPresenter.Get() != nullptr);

    if (!m_tpPlacementTarget)
    {
        // Placement target has been reset, which means request can be ignored.
        goto Cleanup;
    }

    pPresenterAsControl = m_tpPresenter.Cast<Control>();

    // Get available area to display Flyout, excluding space occupied by AppBars and IHM.
    IFC(CalculateAvailableWindowRect(
        m_tpPopup.Cast<Popup>(),
        m_tpPlacementTarget.Cast<FrameworkElement>(),
        m_isTargetPositionSet,
        m_targetPoint,
        m_majorPlacementMode == MajorPlacementMode::Full,
        &availableWindowRect));

    // If placement target is in live tree, get its coordinates.  If not, reuse ones cached when FlyoutBase
    // ShowAt was first called (support for scenario where flyout is invoked from element in another flyout).

    if (m_tpPlacementTarget.Cast<FrameworkElement>()->IsInLiveTree())
    {
        IFC(CalculatePlacementTargetBoundsPrivate(
            &m_majorPlacementMode,
            m_tpPlacementTarget.Get(),
            &m_placementTargetBounds,
            &m_allowPlacementFallbacks,
            bottomAppBarVerticalCorrection));
    }

    IFC(m_tpPlacementTarget->get_FlowDirection(&placementTargetFlowDirection));

    // Reverse meaning of left and right for RTL.
    // Popup's coordinates (0, 0) are always in upper-left hand corner.
    GetEffectivePlacementMode(
        m_majorPlacementMode,
        placementTargetFlowDirection,
        &effectivePlacementMode);

    IFC(IsPropertyLocal(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FlyoutBase_Placement),
        &isPlacementPropertyLocal));

    if (!isPlacementPropertyLocal)
    {
        IFC(AutoAdjustPlacement(&effectivePlacementMode));
    }

    {
        wf::Size minPresenterSize = {};
        wf::Size maxPresenterSize = {};

        IFC(GetMinimumSize(pPresenterAsControl, &minPresenterSize));
        IFC(GetMaximumSize(pPresenterAsControl, &maxPresenterSize));

        IFC(CalculatePlacementPrivate(
            &effectivePlacementMode,
            m_preferredJustification,
            m_allowPlacementFallbacks,
            m_allowPresenterResizing,
            m_placementTargetBounds,
            presenterSize,
            minPresenterSize,
            maxPresenterSize,
            availableWindowRect,
            IsWindowedPopup(),
            &presenterRect));
    }

    // Position popup.
    if (m_isTargetPositionSet)
    {
        double showAtVerticalOffset;
        IFC(m_tpPopup->get_VerticalOffset(&showAtVerticalOffset));
        IFC(UpdateTargetPosition(availableWindowRect, presenterSize, &presenterRect));

        // If this flyout has a target position set, the placement is only used
        // to determine the direction of the MenuFlyout transition which can animate
        // either from Top or Bottom.
        effectivePlacementMode =
            (showAtVerticalOffset > static_cast<double>(presenterRect.Y)) ?
            FlyoutBase::MajorPlacementMode::Top :
            FlyoutBase::MajorPlacementMode::Bottom;
    }
    else
    {
        if (placementTargetFlowDirection == xaml::FlowDirection_LeftToRight)
        {
            IFC(m_tpPopup->put_HorizontalOffset(presenterRect.X));
        }
        else
        {
            IFC(m_tpPopup->put_HorizontalOffset(presenterRect.X + presenterRect.Width));
        }

        IFC(m_tpPopup->put_VerticalOffset(presenterRect.Y));
    }


    if (presenterSize.Width != presenterRect.Width)
    {
        IFC(pPresenterAsControl->put_Width(presenterRect.Width));
        m_presenterResized = true;
    }

    if (presenterSize.Height != presenterRect.Height)
    {
        IFC(pPresenterAsControl->put_Height(presenterRect.Height));
        m_presenterResized = true;
    }

    // We don't want our popup to grab the focus since we will be focusing the flyout ourselves.
    IFC(CoreImports::Popup_SetShouldTakeFocus(pPopupNoRef->GetHandle(), FALSE));

    if (preparePopupTheme)
    {
        // Note: transitions already respect flow direction, so we undo the flow-direction change to
        // effective placement before setting up the transition.
        GetEffectivePlacementMode(
            effectivePlacementMode,
            placementTargetFlowDirection,
            &effectivePlacementMode);

        IFC(PreparePopupTheme(
            pPopupNoRef,
            effectivePlacementMode,
            m_tpPlacementTarget.Get()));
    }

    IFC(static_cast<CFlyoutBase*>(GetHandle())->OnPlacementUpdated(effectivePlacementMode));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::PreparePresenter(
    _In_ IControl* pPresenter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spPresenterAsFE;
    ctl::ComPtr<IUIElement> spPresenterAsUIE;

    ASSERT(pPresenter);

    IFC(ctl::do_query_interface(spPresenterAsFE, pPresenter));
    IFC(ctl::do_query_interface(spPresenterAsUIE, pPresenter));

    IFC(m_epPresenterSizeChangedHandler.AttachEventHandler(
        spPresenterAsFE.Get(),
        std::bind(&FlyoutBase::OnPresenterSizeChanged, this, _1, _2)));

    IFC(m_epPresenterLoadedHandler.AttachEventHandler(
        spPresenterAsFE.Get(),
        std::bind(&FlyoutBase::OnPresenterLoaded, this, _1, _2)));

    IFC(m_epPresenterUnloadedHandler.AttachEventHandler(spPresenterAsFE.Get(),
        [this](IInspectable *pSender, IRoutedEventArgs *pArgs)
        {
            HRESULT hr = S_OK;

            // Peg for the duration of the call.
            if (auto pegged = ctl::try_make_autopeg(this))
            {
                hr = OnPresenterUnloaded(pSender, pArgs);
            }

            return hr;
        }));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::PreparePopupTheme(
    _In_ Popup* pPopup,
    MajorPlacementMode placementMode,
    _In_ xaml::IFrameworkElement* pPlacementTarget)
{
    BOOLEAN areOpenCloseAnimationsEnabled = FALSE;
    IFC_RETURN(get_AreOpenCloseAnimationsEnabled(&areOpenCloseAnimationsEnabled));

    if (!m_tpThemeTransition && areOpenCloseAnimationsEnabled)
    {
        ctl::ComPtr<ITransition> spTransition;
        ctl::ComPtr<TransitionCollection> spTransitionCollection;

        IFC_RETURN(ctl::make(&spTransitionCollection));

        if (m_usePickerFlyoutTheme)
        {
            ctl::ComPtr<PickerFlyoutThemeTransition> spPickerFlyoutThemeTransition;

            IFC_RETURN(ctl::make(&spPickerFlyoutThemeTransition));
            IFC_RETURN(spPickerFlyoutThemeTransition.As(&spTransition));
        }
        else
        {
            ctl::ComPtr<PopupThemeTransition> spPopupThemeTransition;
            IFC_RETURN(ctl::make(&spPopupThemeTransition));
            IFC_RETURN(spPopupThemeTransition.As(&spTransition));

            ctl::ComPtr<xaml::IFrameworkElement> overlayElement;
            IFC_RETURN(pPopup->get_OverlayElement(&overlayElement));
            spPopupThemeTransition->SetOverlayElement(overlayElement.Get());

            IFC_RETURN(SetTransitionParameters(spTransition.Get(), placementMode));
        }

        IFC_RETURN(spTransitionCollection->Append(spTransition.Get()));

        // Deliverable 19819460: Allow LTEs to target Popup.Child when Popup is windowed
        // ThemeTransition can't be applied to child of a windowed popup, targeting grandchild here.
        if (IsWindowedPopup())
        {
            ctl::ComPtr<IUIElement> popupChild;
            IFC_RETURN(pPopup->get_Child(&popupChild));
            if (popupChild)
            {
                ctl::ComPtr<xaml::IDependencyObject> popupGrandChildAsDO;
                IFC_RETURN(VisualTreeHelper::GetChildStatic(static_cast<UIElement*>(popupChild.Get()), 0, &popupGrandChildAsDO));
                if (popupGrandChildAsDO)
                {
                    ctl::ComPtr<IUIElement> popupGrandChildAsUIE;
                    IFC_RETURN(popupGrandChildAsDO.As(&popupGrandChildAsUIE));
                    IFC_RETURN(popupGrandChildAsUIE->put_Transitions(spTransitionCollection.Get()));
                    IFC_RETURN(popupGrandChildAsUIE->InvalidateMeasure());
                }
            }
        }
        else
        {
            IFC_RETURN(pPopup->put_ChildTransitions(spTransitionCollection.Get()));
        }

        SetPtrValue(m_tpThemeTransition, spTransition.Get());
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::SetTransitionParameters(
    _In_ ITransition* pTransition,
    MajorPlacementMode placementMode)
{
    HRESULT hr = S_OK;

    ASSERT(pTransition);

    ctl::ComPtr<ITransition> spTransition(pTransition);
    ctl::ComPtr<IPopupThemeTransition> spTransitionAsPopupThemeTransition;

    spTransitionAsPopupThemeTransition = spTransition.AsOrNull<IPopupThemeTransition>();

    if (spTransitionAsPopupThemeTransition)
    {
        switch (placementMode)
        {
            case FlyoutBase::MajorPlacementMode::Full:
                IFC(spTransitionAsPopupThemeTransition->put_FromHorizontalOffset(0.0));
                IFC(spTransitionAsPopupThemeTransition->put_FromVerticalOffset(0.0));
                break;

            case FlyoutBase::MajorPlacementMode::Top:
                IFC(spTransitionAsPopupThemeTransition->put_FromHorizontalOffset(0.0));
                IFC(spTransitionAsPopupThemeTransition->put_FromVerticalOffset(g_entranceThemeOffset));
                break;

            case FlyoutBase::MajorPlacementMode::Bottom:
                IFC(spTransitionAsPopupThemeTransition->put_FromHorizontalOffset(0.0));
                IFC(spTransitionAsPopupThemeTransition->put_FromVerticalOffset(-g_entranceThemeOffset));
                break;

            case FlyoutBase::MajorPlacementMode::Left:
                IFC(spTransitionAsPopupThemeTransition->put_FromHorizontalOffset(g_entranceThemeOffset));
                IFC(spTransitionAsPopupThemeTransition->put_FromVerticalOffset(0.0));
                break;

            case FlyoutBase::MajorPlacementMode::Right:
                IFC(spTransitionAsPopupThemeTransition->put_FromHorizontalOffset(-g_entranceThemeOffset));
                IFC(spTransitionAsPopupThemeTransition->put_FromVerticalOffset(0.0));
                break;

            default:
                ASSERT(FALSE, L"Unsupported MajorPlacementMode");
                IFC(E_UNEXPECTED);
                break;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::RaiseOpenedEvent()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::RaiseOpenedEvent()");

    HRESULT hr = S_OK;
    ctl::ComPtr<EventArgs> spEventArgs;
    OpenedEventSourceType* pEventSource = nullptr;

    IFC(ctl::make(&spEventArgs));
    IFC(FlyoutBaseGenerated::GetOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spEventArgs.Get())));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FlyoutBase::OnClosing(bool* cancel)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnClosing()");

    *cancel = false;

    ctl::ComPtr<FlyoutBaseClosingEventArgs> eventArgs;
    IFC_RETURN(ctl::make(&eventArgs));

    ClosingEventSourceType* eventSource = nullptr;
    IFC_RETURN(FlyoutBaseGenerated::GetClosingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(this, eventArgs.Get()));

    BOOLEAN cancelRequested = FALSE;
    IFC_RETURN(eventArgs->get_Cancel(&cancelRequested));

    *cancel = !!cancelRequested;

    if (!cancelRequested)
    {
        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Hide, this));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::OnClosed()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnClosed()");

    m_isTargetPositionSet = FALSE;
    IFC_RETURN(put_InputDevicePrefersPrimaryCommands(FALSE));

    // Now that the flyout has closed, reset its m_hasPlacementOverride flag so that
    // it will be valid the next time it opens again.
    m_hasPlacementOverride = false;

    // Also reset the m_isLightDismissOverlayEnabled flag to its default TRUE value
    // so that the light dismiss overlay is recreated as needed when reopening.
    m_isLightDismissOverlayEnabled = TRUE;

    IFC_RETURN(RaiseClosedEvent());

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::RaiseClosedEvent()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::RaiseClosedEvent()");

    HRESULT hr = S_OK;
    ctl::ComPtr<EventArgs> spEventArgs;
    ClosedEventSourceType* pEventSource = nullptr;

    IFC(ctl::make(&spEventArgs));
    IFC(FlyoutBaseGenerated::GetClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spEventArgs.Get())));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::OnOpening()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnOpening()");

    switch (m_inputDeviceTypeUsedToOpen)
    {
    case DirectUI::InputDeviceType::None:
    case DirectUI::InputDeviceType::Mouse:
    case DirectUI::InputDeviceType::Keyboard:
    case DirectUI::InputDeviceType::GamepadOrRemote:
        IFC_RETURN(put_InputDevicePrefersPrimaryCommands(FALSE));
        break;
    case DirectUI::InputDeviceType::Touch:
    case DirectUI::InputDeviceType::Pen:
        IFC_RETURN(put_InputDevicePrefersPrimaryCommands(TRUE));
        break;
    }

    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Show, this));

    IFC_RETURN(RaiseOpeningEvent());

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::RaiseOpeningEvent()
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::RaiseOpeningEvent()");

    m_openingCanceled = false;

    ctl::ComPtr<EventArgs> spEventArgs;
    OpeningEventSourceType* pEventSource = nullptr;

    IFC_RETURN(ctl::make(&spEventArgs));
    IFC_RETURN(FlyoutBaseGenerated::GetOpeningEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spEventArgs.Get())));

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::OnPresenterSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wf::Size presenterSize = {};

    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnPresenterSizeChanged()");

    // Perform placement when size of presenter is known.
    IFC(pArgs->get_NewSize(&presenterSize));

    if (!m_presenterResized)
    {
        IFC(PerformPlacement(presenterSize));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::OnPresenterLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnPresenterLoaded()");

    BOOLEAN allowFocusOnInteraction = TRUE;

    IFC_RETURN(get_AllowFocusOnInteraction(&allowFocusOnInteraction));
    CDependencyObject* target = GetHandle();

    // First check to see if FlyoutBase has the property set to false. If not, then pull from the presenter instead,
    // if we have one - in rare occasions we may not, if OnPresenterLoaded() is called immediately after OnPresenterUnloaded()
    // without any call to ShowAt(), and we don't want to crash in such a circumstance.
    if (allowFocusOnInteraction && m_tpPlacementTarget)
    {
        target = m_tpPlacementTarget.Cast<FrameworkElement>()->GetHandle();
    }

    if (m_shouldTakeFocus)
    {
        // Propagate focus to the first focusable child.
        DirectUI::FocusState focusState = CFocusManager::GetFocusStateFromInputDeviceType(m_inputDeviceTypeUsedToOpen);

        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(target);
        CInputManager& inputManager = contentRoot->GetInputManager();

        // In some islands scenarios (e.g., Win+X menu) the app calls ShowAt in response to input that XAML is not aware of (because
        // it's happening outside the island)  Since XAML won't know then what the last input device _really_ was, we update XAML's
        // notion of last input device with what the system has here, and open the Flyout with the appropriate FocusState.
        // We only care about this for windowed popups.
        if (m_tpPopup && contentRoot->GetType() == CContentRoot::XamlIslandRoot)
        {
            CPopup* corePopup {static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())};
            wrl::ComPtr<ixp::IIslandInputSitePartner> islandInputSite = corePopup->GetIslandInputSite();
            if (nullptr != islandInputSite && corePopup->IsWindowed())
            {
                boolean showFocusRectangles{ false };
                IFCFAILFAST(islandInputSite->get_ShouldShowFocusRectangles(&showFocusRectangles));
                const bool shouldShowKeyboardIndicators = static_cast<bool>(showFocusRectangles);
                if (shouldShowKeyboardIndicators && focusState != DirectUI::FocusState::Keyboard)
                {
                    inputManager.SetLastInputDeviceType(DirectUI::InputDeviceType::Keyboard);
                    focusState = CFocusManager::GetFocusStateFromInputDeviceType(inputManager.GetLastInputDeviceType());

                }
                else if (!shouldShowKeyboardIndicators && focusState == DirectUI::FocusState::Keyboard)
                {
                    inputManager.SetLastInputDeviceType(DirectUI::InputDeviceType::None);
                    focusState = CFocusManager::GetFocusStateFromInputDeviceType(inputManager.GetLastInputDeviceType());
                }
            }
        }

        // If the user is using a UIA client such as Narrator, we want to treat things as if the device used to open
        // the flyout was a keyboard. Otherwise, AllowFocusOnInteraction being false might result in focus not moving into the
        // flyout for a Narrator user.
        if (inputManager.GetWasUIAFocusSetSinceLastInput())
        {
            focusState = DirectUI::FocusState::Keyboard;
        }

        // Note: Normally we check whether the target has AllowFocusOnInteraction set before having the flyout take
        // focus. The exception is MenuFlyout, which is allowed to take focus regardless of what its target's
        // AllowFocusOnInteraction value is. We do this so the user can immediately navigate between menu items with the
        // arrow keys after opening it. This covers cases like MenuFlyouts opening from AppBarButtons, where the target
        // AppBarButton will have AllowFocusOnInteraction="False" in the default template.
        if (Focus::FocusSelection::ShouldUpdateFocus(target, focusState)
            || (allowFocusOnInteraction && GetHandle()->OfTypeByIndex<KnownTypeIndex::MenuFlyout>()))
        {
            BOOLEAN childFocused = FALSE;
            IFC_RETURN(m_tpPresenter.AsOrNull<xaml::IUIElement>()->Focus(
                static_cast<xaml::FocusState>(focusState),
                &childFocused));

            if (!childFocused)
            {
                // If we failed to focus the flyout, focus the popup itself.
                // If the focused child or the flyout itself has AllowFocusOnInteraction set to false, do not try and set focus
                CPopup *PopupAsNative = static_cast<CPopup*>((m_tpPopup.Cast<Popup>()->GetHandle()));
                IFC_RETURN(PopupAsNative->SetFocus(focusState));
            }
        }
    }

    IFC_RETURN(RaiseOpenedEvent());

    return S_OK;
}

// Raise closed event and open staged Flyout in case one exists.
_Check_return_ HRESULT FlyoutBase::OnPresenterUnloaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnPresenterUnloaded()");

    bool wasTargetPositionSet = m_isTargetPositionSet;
    wf::Point targetPoint{ m_targetPoint.X, m_targetPoint.Y };

    IFC_RETURN(OnClosed());

    ctl::ComPtr<FlyoutMetadata> flyoutMetadata;
    IFC_RETURN(GetFlyoutMetadata(&flyoutMetadata));

    flyoutMetadata->SetOpenFlyout(nullptr, nullptr);

    ctl::ComPtr<IFlyoutBase> stagedFlyout;
    ctl::ComPtr<IFrameworkElement> stagedFlyoutPlacementTarget;
    IFC_RETURN(flyoutMetadata->GetStagedFlyout(&stagedFlyout, &stagedFlyoutPlacementTarget));

    // If our presenter was unloaded as part of tearing down the island, then don't try to reopen the staged flyout.
    VisualTree* stagedFlyoutVisualTree = stagedFlyout ? stagedFlyout.Cast<FlyoutBase>()->GetHandle()->GetVisualTree() : nullptr;

    if (stagedFlyout && stagedFlyoutVisualTree)
    {
        // If not reopening, clear the reference to placement target.  If it is reopening, it will be cleared later.
        // This state can be reached if open flyout is re-opened with a different placement target.
        if (!ctl::are_equal(stagedFlyout.Get(), this))
        {
            IFC_RETURN(SetPlacementTarget(nullptr));

            // Clear our parent reference to no longer mark this flyout as a child flyout.
            m_tpParentFlyout.Clear();
        }
        else
        {
            // If we're reopening a flyout, preserve target position
            ASSERT(targetPoint.X == m_targetPoint.X && targetPoint.Y == m_targetPoint.Y);
            m_isTargetPositionSet = wasTargetPositionSet;
        }

        flyoutMetadata->SetStagedFlyout(nullptr, nullptr);
        IFC_RETURN(stagedFlyout.Cast<FlyoutBase>()->Open());
    }
    else
    {
        IFC_RETURN(SetPlacementTarget(nullptr));

        // Clear our parent reference to no longer mark this flyout as a child flyout.
        m_tpParentFlyout.Clear();
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::OnPopupLostFocus(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::OnPopupLostFocus()");

    BOOLEAN isOpen = FALSE;

    if (m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(&isOpen));
    }

    if (isOpen)
    {
        ctl::ComPtr<DependencyObject> focusedElement;
        IFC_RETURN(GetFocusedElement(&focusedElement));

        // We only want to close when we lose focus in the case where the newly focused element
        // does not reside in any other flyout - otherwise, this will conflict with
        // the functionality of child flyouts.
        // Note that tap-and-hold can cause the root scroll viewer to take focus temporarily,
        // which we don't want to respond to, so we'll ignore that.
        if (focusedElement && (!m_isLightDismissOverlayEnabled || !focusedElement.AsOrNull<RootScrollViewer>()))
        {
            CDependencyObject *currentObject = focusedElement->GetHandle();
            CUIElement *currentElement = do_pointer_cast<CUIElement>(currentObject);

            // If the focused element is a text element (which is not a UI element),
            // then we need to get its containing element.
            if (!currentElement)
            {
                CTextElement *currentTextElement = do_pointer_cast<CTextElement>(currentObject);

                if (currentTextElement)
                {
                    currentElement = currentTextElement->GetContainingFrameworkElement();
                }
            }

            CPopup* popupAncestor = nullptr;
            bool popupRootExists = false;

            while (currentElement)
            {
                if (currentElement->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
                {
                    popupRootExists = true;
                    currentElement = do_pointer_cast<CUIElement>(currentElement->GetLogicalParentNoRef());
                    continue;
                }

                if (auto elementCast = do_pointer_cast<CPopup>(currentElement))
                {
                    popupAncestor = elementCast;
                    break;
                }

                currentElement = currentElement->GetUIElementAdjustedParentInternal(FALSE);
            }

            // In the case of XamlIslandRoots, it may be the case that we can't retrieve a popup object.
            // In that case, we'll assume that it was in a popup if we found the popup root.
            // We can't tell if it was a light-dismiss popup in that case, but we'll err on the side
            // of not hiding in that circumstance.
            if (!popupRootExists && (!popupAncestor || !popupAncestor->IsSelfOrAncestorLightDismiss()))
            {
                IFC_RETURN(Hide());
            }
        }
    }

    return S_OK;
}

// Hides the flyout in TransientWithDismissOnPointerMoveAway mode when the pointer moves more than 80 pixels away.
_Check_return_ HRESULT FlyoutBase::OnRootVisualPointerMoved(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
#ifdef DBG
    ASSERT(m_tpPopup);
    BOOLEAN isOpen = FALSE;
    IGNOREHR(m_tpPopup->get_IsOpen(&isOpen));
    ASSERT(isOpen);
#endif

    // The flyout is hidden when the pointer is moved more than 80 pixels
    // away from its bounds. This is the square of that threshold distance.
    const double hidingThreshold = 6400;

    ctl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFC_RETURN(pArgs->GetCurrentPoint(nullptr, &pointerPoint));

    wf::Point pointerPosition = {};

    IFC_RETURN(pointerPoint->get_Position(&pointerPosition));

    wf::Size presenterSize = {};
    IFC_RETURN(GetPresenterSize(&presenterSize));

    ctl::ComPtr<xaml_media::IGeneralTransform> transformToRoot;
    IFC_RETURN(m_tpPresenter.Cast<Control>()->TransformToVisual(nullptr, &transformToRoot));

    wf::Rect presenterBounds = { 0, 0, presenterSize.Width, presenterSize.Height };
    wf::Rect rootPresenterBounds = {};

    IFC_RETURN(transformToRoot->TransformBounds(presenterBounds, &rootPresenterBounds));

    double left = rootPresenterBounds.X;
    double top = rootPresenterBounds.Y;
    double right = left + rootPresenterBounds.Width;
    double bottom = top + rootPresenterBounds.Height;
    double xDistance = 0;
    double yDistance = 0;

    if (pointerPosition.X < left)
    {
        xDistance = left - pointerPosition.X;
    }
    else if (pointerPosition.X > right)
    {
        xDistance = pointerPosition.X - right;
    }

    if (pointerPosition.Y < top)
    {
        yDistance = top - pointerPosition.Y;
    }
    else if (pointerPosition.Y > bottom)
    {
        yDistance = pointerPosition.Y - bottom;
    }

    if (xDistance * xDistance + yDistance * yDistance > hidingThreshold)
    {
        IFC_RETURN(Hide());
    }

    return S_OK;
}

// Perform placement on a control using fallbacks.
_Check_return_ HRESULT FlyoutBase::CalculatePlacementPrivate(
    _Inout_ MajorPlacementMode* pMajorPlacement,
    FlyoutBase::PreferredJustification justification,
    BOOLEAN allowFallbacks,
    bool allowPresenterResizing,
    const wf::Rect& placementTargetBounds,
    const wf::Size& controlSize,
    const wf::Size& minControlSize,
    const wf::Size& maxControlSize,
    const wf::Rect& containerRect,
    bool isWindowed,
    _Out_ wf::Rect* pControlRect)
{
    HRESULT hr = S_OK;

    // Full placement is not relative to the placement target, so there's no need
    // to check for fallbacks or make any further adjustments.
    if (*pMajorPlacement == FlyoutBase::MajorPlacementMode::Full)
    {
        pControlRect->X = containerRect.X;
        pControlRect->Y = containerRect.Y;
        pControlRect->Width = containerRect.Width;
        pControlRect->Height = containerRect.Height;

        // if the presenterRect.Width is greater than maxWidth, the final width will be cut to
        // maxWidth and then a bigger margin will appear on the right.
        // Move the presenter to center. Same logic applies for Height.
        if (maxControlSize.Width < pControlRect->Width)
        {
            pControlRect->X += (pControlRect->Width - static_cast<FLOAT>(maxControlSize.Width)) / 2;
            pControlRect->Width = static_cast<FLOAT>(maxControlSize.Width);
        }
        if (maxControlSize.Height < pControlRect->Height)
        {
            pControlRect->Y += (pControlRect->Height - static_cast<FLOAT>(maxControlSize.Height)) / 2;
            pControlRect->Height = static_cast<FLOAT>(maxControlSize.Height);
        }
    }
    else
    {
        wf::Size controlSizeWithMargins = {};
        wf::Size minControlSizeWithMargins = {};
        MajorPlacementMode placementOrder[4];
        wf::Point controlPos = {};

        pControlRect->X = 0.0;
        pControlRect->Y = 0.0;
        pControlRect->Width = controlSize.Width;
        pControlRect->Height = controlSize.Height;

        controlSizeWithMargins.Width = static_cast<FLOAT>(controlSize.Width);
        controlSizeWithMargins.Height = static_cast<FLOAT>(controlSize.Height);

        minControlSizeWithMargins.Width = static_cast<FLOAT>(minControlSize.Width);
        minControlSizeWithMargins.Height = static_cast<FLOAT>(minControlSize.Height);

        switch (*pMajorPlacement)
        {
            case FlyoutBase::MajorPlacementMode::Top:
                placementOrder[0] = FlyoutBase::MajorPlacementMode::Top;
                placementOrder[1] = FlyoutBase::MajorPlacementMode::Bottom;
                placementOrder[2] = FlyoutBase::MajorPlacementMode::Left;
                placementOrder[3] = FlyoutBase::MajorPlacementMode::Right;
                break;

            case FlyoutBase::MajorPlacementMode::Bottom:
                placementOrder[0] = FlyoutBase::MajorPlacementMode::Bottom;
                placementOrder[1] = FlyoutBase::MajorPlacementMode::Top;
                placementOrder[2] = FlyoutBase::MajorPlacementMode::Left;
                placementOrder[3] = FlyoutBase::MajorPlacementMode::Right;
                break;

            case FlyoutBase::MajorPlacementMode::Left:
                placementOrder[0] = FlyoutBase::MajorPlacementMode::Left;
                placementOrder[1] = FlyoutBase::MajorPlacementMode::Right;
                placementOrder[2] = FlyoutBase::MajorPlacementMode::Top;
                placementOrder[3] = FlyoutBase::MajorPlacementMode::Bottom;
                break;

            case FlyoutBase::MajorPlacementMode::Right:
                placementOrder[0] = FlyoutBase::MajorPlacementMode::Right;
                placementOrder[1] = FlyoutBase::MajorPlacementMode::Left;
                placementOrder[2] = FlyoutBase::MajorPlacementMode::Top;
                placementOrder[3] = FlyoutBase::MajorPlacementMode::Bottom;
                break;

            default:
                ASSERT(FALSE, L"Unsupported MajorPlacementMode");
                IFC(E_UNEXPECTED);
                break;
        }

        if (!PerformPlacementWithFallback(
                placementTargetBounds,
                controlSizeWithMargins,
                containerRect,
                placementOrder,
                (allowFallbacks) ? ARRAYSIZE(placementOrder) : 1,
                justification,
                &controlPos,
                pMajorPlacement))
        {
            if (allowPresenterResizing)
            {
                ResizeToFit(
                    pMajorPlacement,
                    placementTargetBounds,
                    containerRect,
                    minControlSizeWithMargins,
                    &controlPos,
                    &controlSizeWithMargins);
            }
            pControlRect->Width = static_cast<FLOAT>(DoubleUtil::Max(0.0, controlSizeWithMargins.Width));
            pControlRect->Height = static_cast<FLOAT>(DoubleUtil::Max(0.0, controlSizeWithMargins.Height));
        }

        pControlRect->X = controlPos.X;
        pControlRect->Y = controlPos.Y;

        // We want there to be a margin between the flyout and the UI element it's attached to.  To accomplish
        // that, we add a margin only in the direction in which the flyout is appearing.
        switch (*pMajorPlacement)
        {
            case FlyoutBase::MajorPlacementMode::Top:
                pControlRect->Y -= FlyoutMargin;
                break;

            case FlyoutBase::MajorPlacementMode::Bottom:
                pControlRect->Y += FlyoutMargin;
                break;

            case FlyoutBase::MajorPlacementMode::Left:
                pControlRect->X -= FlyoutMargin;
                break;

            case FlyoutBase::MajorPlacementMode::Right:
                pControlRect->X += FlyoutMargin;
                break;

            default:
                ASSERT(FALSE, L"Unsupported MajorPlacementMode");
                IFC(E_UNEXPECTED);
                break;
        }

        // If the flyout is windowed, then a xy-position less than 0 is acceptable - that means we're displaying above or to the left of the window.
        if (!isWindowed)
        {
            pControlRect->X = static_cast<FLOAT>(DoubleUtil::Max(0.0, pControlRect->X));
            pControlRect->Y = static_cast<FLOAT>(DoubleUtil::Max(0.0, pControlRect->Y));
        }
    }

Cleanup:
    RRETURN(hr);

}

// Calculate bounding rectangle of the placement target taking into consideration whether target is on AppBar.
_Check_return_ HRESULT FlyoutBase::CalculatePlacementTargetBoundsPrivate(
    _Inout_ MajorPlacementMode* pPlacement,
    _In_ IFrameworkElement* pPlacementTarget,
    _Out_ wf::Rect* pPlacementTargetBounds,
    _Out_ BOOLEAN* pAllowFallbacks,
    DOUBLE bottomAppBarVerticalCorrection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IAppBar> spAppBar;
    DOUBLE placementTargetWidth = 0.0;
    DOUBLE placementTargetHeight = 0.0;
    AppBarMode appBarMode = AppBarMode_Floating;
    bool isTopOrBottomAppBar = false;

    *pAllowFallbacks = FALSE;
    pPlacementTargetBounds->X = 0.0;
    pPlacementTargetBounds->Y = 0.0;
    pPlacementTargetBounds->Width = 0.0;
    pPlacementTargetBounds->Height = 0.0;

    IFC(pPlacementTarget->get_ActualWidth(&placementTargetWidth));
    IFC(pPlacementTarget->get_ActualHeight(&placementTargetHeight));

    pPlacementTargetBounds->Width = static_cast<FLOAT>(placementTargetWidth);
    pPlacementTargetBounds->Height = static_cast<FLOAT>(placementTargetHeight);

    IFC(FindParentAppBar(pPlacementTarget, &spAppBar));

    if (spAppBar)
    {
        appBarMode = spAppBar.Cast<AppBar>()->GetMode();
        isTopOrBottomAppBar = (appBarMode == AppBarMode_Top || appBarMode == AppBarMode_Bottom);
    }

    if (!isTopOrBottomAppBar || (*pPlacement == FlyoutBase::MajorPlacementMode::Full))
    {
        // Non-AppBar case and non-Top/BottomAppBar and FullPlacementMode, transform target to root screen coordinates.
        ctl::ComPtr<xaml_media::IGeneralTransform> spTargetToRootTransform;
        IFC(static_cast<FrameworkElement*>(pPlacementTarget)->TransformToVisual(nullptr, &spTargetToRootTransform));
        IFC(spTargetToRootTransform->TransformBounds(*pPlacementTargetBounds, pPlacementTargetBounds));
        *pAllowFallbacks = TRUE;
    }
    else
    {
        // For Top/Bottom AppBars, expand target bounds to AppBar size and transform to root screen coordinates.
        ctl::ComPtr<xaml_media::IGeneralTransform> spTargetToAppBarTransform;
        ctl::ComPtr<xaml_media::IGeneralTransform> spAppBarToRootTransform;
        wf::Rect placementTargetBoundsInAppBar = {};
        DOUBLE appBarHeight = 0.0;

        IFC(static_cast<FrameworkElement*>(pPlacementTarget)->TransformToVisual(spAppBar.Cast<AppBar>(), &spTargetToAppBarTransform));
        IFC(spTargetToAppBarTransform->TransformBounds(*pPlacementTargetBounds, &placementTargetBoundsInAppBar));

        IFC(spAppBar.Cast<AppBar>()->get_ActualHeight(&appBarHeight));
        placementTargetBoundsInAppBar.Y = 0.0;
        placementTargetBoundsInAppBar.Height = static_cast<FLOAT>(appBarHeight);

        IFC(spAppBar.Cast<AppBar>()->TransformToVisual(nullptr, &spAppBarToRootTransform));
        IFC(spAppBarToRootTransform->TransformBounds(placementTargetBoundsInAppBar, pPlacementTargetBounds));

        // Explicitly override placement mode for AppBar contained targets.
        if (appBarMode == AppBarMode_Top)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Bottom;
        }
        else if (appBarMode == AppBarMode_Bottom)
        {
            *pPlacement = FlyoutBase::MajorPlacementMode::Top;

            // When the InputPaneState changes to Hidden, the bottom AppBar is still laid out at its
            // old position above where the InputPane was, and thus the PerformPlacement() in
            // NotifyInputPaneStateChanged() must correct for the height of the InputPane.
            pPlacementTargetBounds->Y += static_cast<FLOAT>(bottomAppBarVerticalCorrection);
        }
        else
        {
            ASSERT(false, L"Unsupported AppBarMode");
            IFC(E_UNEXPECTED);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::CloseOpenFlyout(_In_opt_ CFlyoutBase* parentFlyoutCore)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::CloseOpenFlyout()");

    ctl::ComPtr<FlyoutMetadata> flyoutMetadata;

    if (parentFlyoutCore == nullptr)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetFlyoutMetadata(&flyoutMetadata));
    }
    else
    {
        ctl::ComPtr<DependencyObject> parentFlyout;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(parentFlyoutCore, &parentFlyout));
        ASSERT(ctl::is<IFlyoutBase>(parentFlyout));

        flyoutMetadata = parentFlyout.Cast<FlyoutBase>()->m_childFlyoutMetadata;
    }

    if (flyoutMetadata)
    {
        // Clear the staged flyout before we hide the open flyout to make sure we don't
        // end up just opening a new flyout.
        flyoutMetadata->SetStagedFlyout(nullptr, nullptr);

        ctl::ComPtr<IFlyoutBase> openFlyout;
        IFC_RETURN(flyoutMetadata->GetOpenFlyout(&openFlyout, nullptr));

        if (openFlyout)
        {
            IFC_RETURN(openFlyout->Hide());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::HideFlyout(_In_ CFlyoutBase* flyout)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::HideFlyout()");

    ctl::ComPtr<DependencyObject> peer;
    ctl::ComPtr<IFlyoutBase> flyouBaseObject;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(flyout, &peer));
    IFC_RETURN(peer.As(&flyouBaseObject));
    if (flyouBaseObject)
    {
        IFC_RETURN(flyouBaseObject->Hide());
    }

    return S_OK;
}

/* static */ _Check_return_ HRESULT FlyoutBase::CalculateAvailableWindowRect(
    _In_ Popup* popup,
    _In_opt_ FrameworkElement* placementTarget,
    bool hasTargetPosition,
    wf::Point targetPosition,
    bool isFull,
    _Out_ wf::Rect* pAvailableRect)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();

    // If the popup is windowed, then the available window rect is the monitor that it will appear in.
    // The exception is if we're opening in full mode, in which case we want it to overlay the content area,
    // not the entire monitor area.
    if (!isFull && static_cast<CPopup*>(popup->GetHandle())->IsWindowed())
    {
        wf::Point popupPosition = {};

        if (hasTargetPosition)
        {
            popupPosition = targetPosition;
        }
        else
        {
            ASSERT(placementTarget);

            DOUBLE actualWidth;
            DOUBLE actualHeight;
            IFC_RETURN(placementTarget->get_ActualWidth(&actualWidth));
            IFC_RETURN(placementTarget->get_ActualHeight(&actualHeight));

            wf::Point centerOfTarget = { static_cast<float>(actualWidth / 2.0), static_cast<float>(actualHeight / 2.0) };
            ctl::ComPtr<xaml::Media::IGeneralTransform> transformToRoot;

            placementTarget->TransformToVisual(nullptr, &transformToRoot);
            IFC_RETURN(transformToRoot->TransformPoint(centerOfTarget, &centerOfTarget));

            popupPosition = centerOfTarget;
        }

        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(popup, popupPosition, pAvailableRect));
    }
    else
    {
        bool isTopAppBarOpen = false;
        bool isTopAppBarSticky = false;
        XFLOAT topAppBarWidth = 0.0f;
        XFLOAT topAppBarHeight = 0.0f;
        bool isBottomAppBarOpen = false;
        bool isBottomAppBarSticky = false;
        XFLOAT bottomAppBarWidth = 0.0f;
        XFLOAT bottomAppBarHeight = 0.0f;

        pAvailableRect->X = 0.0;
        pAvailableRect->Y = 0.0;
        pAvailableRect->Width = 0.0;
        pAvailableRect->Height = 0.0;

        wf::Rect windowBounds;
        IFC_RETURN(dxamlCore->GetContentBoundsForElement(popup->GetHandle(), &windowBounds));

        // Apply the visible bounds for calculating the available bounds area
        // that excludes the system tray and soft steering wheel navigation bar.
        IFC_RETURN(dxamlCore->GetVisibleContentBoundsForElement(true /*ignoreIHM*/, false /*inDesktopCoordinates*/, popup->GetHandle(), pAvailableRect));

        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(popup))
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            if (applicationBarService)
            {
                IFC_RETURN(applicationBarService->GetAppBarStatus(&isTopAppBarOpen, &isTopAppBarSticky,
                            &topAppBarWidth, &topAppBarHeight, &isBottomAppBarOpen, &isBottomAppBarSticky,
                            &bottomAppBarWidth, &bottomAppBarHeight));
            }
        }

        if (isTopAppBarOpen)
        {
            FLOAT clampedTopAppBarHeight = static_cast<FLOAT>(DoubleUtil::Min(topAppBarHeight, pAvailableRect->Height));
            pAvailableRect->Y = clampedTopAppBarHeight;
            pAvailableRect->Height = pAvailableRect->Height - clampedTopAppBarHeight;
        }

        if (isBottomAppBarOpen)
        {
            pAvailableRect->Height = pAvailableRect->Height - bottomAppBarHeight;
        }

        wf::Rect inputPaneOccludeRectInDips = {};
        wf::Rect intersectionRect = {};

        IFC_RETURN(DXamlCore::GetCurrent()->GetInputPaneOccludeRect(popup, &inputPaneOccludeRectInDips));

        intersectionRect = *pAvailableRect;

        IFC_RETURN(RectUtil::Intersect(intersectionRect, inputPaneOccludeRectInDips));

        if (intersectionRect.Height > 0)
        {
            pAvailableRect->Height = pAvailableRect->Height - intersectionRect.Height;
        }

        pAvailableRect->Height = static_cast<FLOAT>(DoubleUtil::Max(0.0, pAvailableRect->Height));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::SetPresenterStyle(
    _In_ IControl* pPresenter,
    _In_opt_ IStyle* pStyle)
{
    HRESULT hr = S_OK;

    ASSERT(pPresenter);

    if (pStyle)
    {
        IFC(static_cast<Control*>(pPresenter)->put_Style(pStyle));
    }
    else
    {
        IFC(static_cast<Control*>(pPresenter)->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Style)));
    }

Cleanup:
    RRETURN(hr);
}

// React to the IHM (touch keyboard) showing or hiding.
// Flyout should always be shown above the IHM.
_Check_return_ HRESULT
FlyoutBase::NotifyInputPaneStateChange(
    InputPaneState inputPaneState,
    XRECTF inputPaneBounds)
{
    DOUBLE bottomAppBarVerticalCorrection = 0.0;
    const float zoomScale = RootScale::GetRasterizationScaleForElement(GetHandle());
    wf::Size presenterSize = {};

    IFC_RETURN(GetPresenterSize(&presenterSize));

    if (inputPaneState == InputPaneState::InputPaneShowing)
    {
        m_cachedInputPaneHeight = inputPaneBounds.Height;
        bottomAppBarVerticalCorrection = -m_cachedInputPaneHeight / zoomScale;
    }
    else if (inputPaneState == InputPaneState::InputPaneHidden)
    {
        bottomAppBarVerticalCorrection = m_cachedInputPaneHeight / zoomScale;

        if (m_presenterResized && m_majorPlacementMode != FlyoutBase::MajorPlacementMode::Full)
        {
            // The presenter's size was reduced by us to fit it on screen.
            // Now that the IHM is going away, let it size itself again.
            // Reset forced resizing of width/height to get content's auto dimensions.
            //
            // This is not needed for the case where the InputPane is being shown
            // since the Flyout can only get smaller in that case and the PerformPlacement()
            // call below will size it appropriately.
            IFC_RETURN(m_tpPresenter.Cast<Control>()->put_Width(DoubleUtil::NaN));
            IFC_RETURN(m_tpPresenter.Cast<Control>()->put_Height(DoubleUtil::NaN));
            m_presenterResized = false;
        }
    }

    IFC_RETURN(PerformPlacement(presenterSize, bottomAppBarVerticalCorrection));

    return S_OK;
}

// Helper method to set/clear m_tpPlacementTarget and maintain associated state and event handlers.
_Check_return_ HRESULT
FlyoutBase::SetPlacementTarget(_In_opt_ IFrameworkElement* pPlacementTarget)
{
    // Clear the old placement target if it exists.
    if (m_tpPlacementTarget)
    {
        IFC_RETURN(DetachHandler(m_epPlacementTargetUnloadedHandler, m_tpPlacementTarget));
        IFC_RETURN(DetachHandler(m_epPlacementTargetActualThemeChangedHandler, m_tpPlacementTarget));
        m_tpPlacementTarget.Clear();
        IFC_RETURN(put_Target(nullptr));
    }

    if (pPlacementTarget)
    {
        IFC_RETURN(put_Target(pPlacementTarget));
        SetPtrValue(m_tpPlacementTarget, pPlacementTarget);

        xaml_primitives::FlyoutPlacementMode placement = xaml_primitives::FlyoutPlacementMode_Top;
        IFC_RETURN(GetEffectivePlacement(&placement));

        // Hook up an event handler that will dismiss the flyout if the placement target gets unloaded.
        IFC_RETURN(m_epPlacementTargetUnloadedHandler.AttachEventHandler(m_tpPlacementTarget.Cast<FrameworkElement>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                RRETURN(Hide());
            }));

        // Hook up an event handler that will forward any theme changes to the flyout.
        IFC_RETURN(m_epPlacementTargetActualThemeChangedHandler.AttachEventHandler(m_tpPlacementTarget.Cast<FrameworkElement>(),
            [this](auto&&, auto&&)
            {
                RRETURN(ForwardThemeToPresenter());
            }));
    }
    return S_OK;
}

/* static */ _Check_return_ HRESULT FlyoutBase::GetPlacementTargetNoRef(
    _In_ CFlyoutBase* flyout,
    _Outptr_ CFrameworkElement** placementTarget)
{
    ctl::ComPtr<DependencyObject> flyoutPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(flyout, &flyoutPeer));
    ASSERT(ctl::is<IFlyoutBase>(flyoutPeer));

    *placementTarget = static_cast<CFrameworkElement*>(flyoutPeer.Cast<FlyoutBase>()->m_tpPlacementTarget.Cast<FrameworkElement>()->GetHandle());
    return S_OK;
}

// Private interface Implementation
_Check_return_ HRESULT FlyoutBase::get_UsePickerFlyoutThemeImpl(
    _Out_ BOOLEAN* pValue)
{
    *pValue = m_usePickerFlyoutTheme;
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::put_UsePickerFlyoutThemeImpl(
    BOOLEAN value)
{
    m_usePickerFlyoutTheme = value;
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::PlaceFlyoutForDateTimePickerImpl(wf::Point point)
{
    wf::Size presenterSize = {};
    ctl::ComPtr<PickerFlyoutThemeTransition> spTransitionAsPickerFlyoutThemeTransition;

    m_isPositionedForDateTimePicker = true;

    IFC_RETURN(ForwardPopupFlowDirection());
    SetTargetPosition(point);
    IFC_RETURN(ApplyTargetPosition());
    IFC_RETURN(GetPresenterSize(&presenterSize));
    IFC_RETURN(PerformPlacement(presenterSize));

    spTransitionAsPickerFlyoutThemeTransition = m_tpThemeTransition.AsOrNull<PickerFlyoutThemeTransition>();

    if (spTransitionAsPickerFlyoutThemeTransition)
    {
        DOUBLE popupOffset = 0.0;
        wf::Point centerOfTarget = { 0, 0 };

        IFC_RETURN(m_tpPopup->get_VerticalOffset(&popupOffset));

        if (m_tpPlacementTarget)
        {
            DOUBLE actualHeight = 0.0;
            ctl::ComPtr<xaml::Media::IGeneralTransform> spTransformFromTargetToWindow;

            m_tpPlacementTarget.Cast<FrameworkElement>()->TransformToVisual(nullptr, &spTransformFromTargetToWindow);
            IFC_RETURN(spTransformFromTargetToWindow->TransformPoint(centerOfTarget, &centerOfTarget));

            IFC_RETURN(m_tpPlacementTarget->get_ActualHeight(&actualHeight));
            centerOfTarget.Y += static_cast<float>(actualHeight / 2.0);
        }

        DOUBLE center = popupOffset + presenterSize.Height / 2;
        DOUBLE offsetFromCenter = centerOfTarget.Y - center;
        IFC_RETURN(spTransitionAsPickerFlyoutThemeTransition->put_OpenedLength(presenterSize.Height));
        IFC_RETURN(spTransitionAsPickerFlyoutThemeTransition->put_OffsetFromCenter(offsetFromCenter));
    }

    return S_OK;
}

// Hooks up the root visual's PointerMoved event so the flyout can be automatically
// hidden in TransientWithDismissOnPointerMoveAway mode.
_Check_return_ HRESULT FlyoutBase::AddRootVisualPointerMovedHandler()
{
    if (!m_rootVisualPointerMovedToken.value)
    {
        ctl::ComPtr<IUIElement> rootVisual;
        IFC_RETURN(m_wrRootVisual.As(&rootVisual));

        ctl::ComPtr<IPointerEventHandler> rootVisualPointerMovedHandler;
        rootVisualPointerMovedHandler.Attach(
            new ClassMemberEventHandler<
            FlyoutBase,
            xaml_primitives::IFlyoutBase,
            xaml_input::IPointerEventHandler,
            IInspectable,
            xaml_input::IPointerRoutedEventArgs>
            (this, &FlyoutBase::OnRootVisualPointerMoved));

        IFC_RETURN(rootVisual->add_PointerMoved(rootVisualPointerMovedHandler.Get(), &m_rootVisualPointerMovedToken));
    }
    return S_OK;
}

// Unhooks the root visual's PointerMoved event.
_Check_return_ HRESULT FlyoutBase::RemoveRootVisualPointerMovedHandler()
{
    if (m_rootVisualPointerMovedToken.value)
    {
        ctl::ComPtr<IUIElement> rootVisual;
        IFC_RETURN(m_wrRootVisual.As(&rootVisual));

        if (rootVisual)
        {
            IFC_RETURN(rootVisual->remove_PointerMoved(m_rootVisualPointerMovedToken));
        }
        m_rootVisualPointerMovedToken.value = 0;
    }

    return S_OK;
}

//------------------------------------------------------------------------
// FlyoutBaseFactory
//------------------------------------------------------------------------

_Check_return_ HRESULT FlyoutBaseFactory::ShowAttachedFlyoutImpl(
    _In_ IFrameworkElement* pFlyoutOwner)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFlyoutBase> spFlyoutBase;

    IFC(GetAttachedFlyout(pFlyoutOwner, &spFlyoutBase));

    if (spFlyoutBase)
    {
        IFC(spFlyoutBase->ShowAt(pFlyoutOwner));
    }
    else
    {
        IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_FLYOUTBASE_NO_ATTACHED_FLYOUT));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutBase::ForwardPopupFlowDirection()
{
    if (m_tpPopup)
    {
        ctl::ComPtr<IFrameworkElement> spPopupAsFE;
        xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;

        IFC_RETURN(m_tpPopup.As(&spPopupAsFE));
        // Propagate the target flow direction to the Popup
        IFC_RETURN(m_tpPlacementTarget->get_FlowDirection(&flowDirection));
        IFC_RETURN(spPopupAsFE->put_FlowDirection(flowDirection));
    }

    return S_OK;
}

void FlyoutBase::SetTargetPosition(
    wf::Point targetPoint)
{
    m_isTargetPositionSet = TRUE;
    m_targetPoint = targetPoint;
}

_Check_return_ HRESULT FlyoutBase::ApplyTargetPosition()
{
    if (m_isTargetPositionSet && m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->put_HorizontalOffset(m_targetPoint.X));
        IFC_RETURN(m_tpPopup->put_VerticalOffset(m_targetPoint.Y));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::SetIsWindowedPopup()
{
    HRESULT hr = S_OK;

    IFC(EnsurePopupAndPresenter());

    // Set popup to be windowed, if we can, in order to support rendering the popup out of the XAML window.
    if (CPopup::DoesPlatformSupportWindowedPopup(DXamlCore::GetCurrent()->GetHandle()))
    {
        // Set popup to be windowed, if we can, in order to support rendering the popup out of the XAML window.
        if (m_tpPopup && !static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())->IsWindowed())
        {
            CPopup *popup = static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle());

            if (!popup->WasEverOpened())
            {
                IFC(popup->SetIsWindowed());
                ASSERT(popup->IsWindowed());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

bool FlyoutBase::IsWindowedPopup()
{
    bool areWindowedPopupsSupported = CPopup::DoesPlatformSupportWindowedPopup(DXamlCore::GetCurrent()->GetHandle());
    bool isPopupWindowed = m_tpPopup ? static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())->IsWindowed() : false;

    return areWindowedPopupsSupported && (isPopupWindowed || m_openingWindowedInProgress);
}

bool IsRightHandedHandedness()
{
    DWORD dwHandedness = HANDEDNESS_LEFT;
    BOOL fRet = ::SystemParametersInfo(SPI_GETHANDEDNESS, 0, reinterpret_cast<LPVOID>(&dwHandedness), 0);
    if (!fRet)
    {
#ifdef DBG
        Trace(L"Failed to fetch SPI_GETHANDEDNESS. Falling back to left-handed menu.");
        WCHAR szTrace[32];
        VERIFYHR(swprintf_s(szTrace, 32, L"Last Error: 0x%08X", HRESULT_FROM_WIN32(GetLastError())));
        Trace(szTrace);
#endif
        dwHandedness = HANDEDNESS_LEFT;
    }
    return dwHandedness == HANDEDNESS_RIGHT;
}

_Check_return_ HRESULT FlyoutBase::UpdateTargetPosition(
    wf::Rect availableWindowRect,
    wf::Size presenterSize,
    _Out_ wf::Rect* presenterRect)
{
    DOUBLE horizontalOffset = 0.0;
    DOUBLE verticalOffset = 0.0;
    DOUBLE maxWidth = DoubleUtil::NaN;
    DOUBLE maxHeight = DoubleUtil::NaN;
    ctl::ComPtr<IFrameworkElement> spPopupAsFE;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    xaml::FlowDirection targetFlowDirection = xaml::FlowDirection_LeftToRight;
    bool isMenuFlyout = ctl::is<xaml_controls::IMenuFlyout>(this);
    bool preferTopPlacement = false;

    ASSERT(m_tpPopup);
    ASSERT(m_isTargetPositionSet);
    IFCPTR_RETURN(presenterRect);

    horizontalOffset = m_targetPoint.X;
    verticalOffset = m_targetPoint.Y;

    xaml_primitives::FlyoutPlacementMode placementMode;
    IFC_RETURN(GetEffectivePlacement(&placementMode));

    // We want to preserve existing MenuFlyout behavior - it will continue to ignore the Placement property.
    // We also don't want to adjust anything if we've been positioned for a DatePicker or TimePicker -
    // in those cases, we've already been put at exactly the position we want to be at.
    if (!isMenuFlyout && !m_isPositionedForDateTimePicker)
    {
        switch (placementMode)
        {
            case xaml_primitives::FlyoutPlacementMode_Top:
                horizontalOffset -= presenterSize.Width / 2;
                verticalOffset -= presenterSize.Height;
                break;
            case xaml_primitives::FlyoutPlacementMode_Bottom:
                horizontalOffset -= presenterSize.Width / 2;
                break;
            case xaml_primitives::FlyoutPlacementMode_Left:
                horizontalOffset -= presenterSize.Width;
                verticalOffset -= presenterSize.Height / 2;
                break;
            case xaml_primitives::FlyoutPlacementMode_Right:
                verticalOffset -= presenterSize.Height / 2;
                break;
            case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedLeft:
            case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedBottom:
                verticalOffset -= presenterSize.Height;
                break;
            case xaml_primitives::FlyoutPlacementMode_TopEdgeAlignedRight:
            case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedBottom:
                horizontalOffset -= presenterSize.Width;
                verticalOffset -= presenterSize.Height;
                break;
            case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedLeft:
            case xaml_primitives::FlyoutPlacementMode_RightEdgeAlignedTop:
                // Nothing changes in this case - we want the point to be the top-left corner of the flyout,
                // which it already is.
                break;
            case xaml_primitives::FlyoutPlacementMode_BottomEdgeAlignedRight:
            case xaml_primitives::FlyoutPlacementMode_LeftEdgeAlignedTop:
                horizontalOffset -= presenterSize.Width;
                break;
        }
    }

    preferTopPlacement = (m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch) && isMenuFlyout;
    bool useHandednessPlacement = (m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Pen) && isMenuFlyout;

    if (preferTopPlacement)
    {
        verticalOffset -= presenterSize.Height;
    }

    FlyoutBase::MajorPlacementMode majorPlacementMode = preferTopPlacement
        ? FlyoutBase::MajorPlacementMode::Top
        : GetMajorPlacementFromPlacement(placementMode);

    IFC_RETURN(m_tpPopup.As(&spPopupAsFE));
    IFC_RETURN(spPopupAsFE->get_FlowDirection(&flowDirection));
    if (m_isPositionedAtPoint)
    {
        IFC_RETURN(m_tpPlacementTarget->get_FlowDirection(&targetFlowDirection));
        ASSERT(flowDirection == targetFlowDirection);
    }

    GetEffectivePlacementMode(
        majorPlacementMode,
        flowDirection,
        &majorPlacementMode);

    FlyoutBase::MajorPlacementMode originalMajorPlacementMode = majorPlacementMode;

    // If the desired placement of the flyout is inside the exclusion area, we'll shift it in the direction of the placement direction
    // so that it no longer is inside that area.
    auto accountForExclusionRect = [&]()
    {
        if (!RectUtil::AreDisjoint(m_exclusionRect, { static_cast<float>(horizontalOffset), static_cast<float>(verticalOffset), presenterSize.Width, presenterSize.Height }))
        {
            switch (majorPlacementMode)
            {
            case FlyoutBase::MajorPlacementMode::Top:
                verticalOffset = m_exclusionRect.Y - presenterSize.Height;
                break;
            case FlyoutBase::MajorPlacementMode::Bottom:
                verticalOffset = m_exclusionRect.Y + m_exclusionRect.Height;
                break;
            case FlyoutBase::MajorPlacementMode::Left:
                horizontalOffset = m_exclusionRect.X - presenterSize.Width;
                break;
            case FlyoutBase::MajorPlacementMode::Right:
                horizontalOffset = m_exclusionRect.X + m_exclusionRect.Width;
                break;
            }

            // (0, 0) is at the right side in RTL rather than the left side,
            // so we'll add on the presenter width in that case to the offset.
            if ((majorPlacementMode == FlyoutBase::MajorPlacementMode::Left ||
                majorPlacementMode == FlyoutBase::MajorPlacementMode::Right) &&
                flowDirection == xaml::FlowDirection_RightToLeft)
            {
                horizontalOffset += presenterSize.Width;
            }
        }
    };

    accountForExclusionRect();

    bool isRTL = (flowDirection == xaml::FlowDirection_RightToLeft);
    bool shiftLeftForRightHandedness = useHandednessPlacement && (IsRightHandedHandedness() != isRTL);
    if (shiftLeftForRightHandedness)
    {
        if (!isRTL)
        {
            horizontalOffset -= presenterSize.Width;
        }
        else
        {
            horizontalOffset += presenterSize.Width;
        }
    }

    // Get the current presenter max width/height
    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_MaxWidth(&maxWidth));
    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_MaxHeight(&maxHeight));

    wf::Point targetPoint = { static_cast<FLOAT>(horizontalOffset), static_cast<FLOAT>(verticalOffset) };
    wf::Rect availableRect = {};

    const bool useMonitorBounds = IsWindowedPopup();

    if (useMonitorBounds)
    {
        // Calculate the available monitor bounds to set the target position within the monitor bounds
        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(m_tpPopup.Cast<Popup>(), targetPoint, &availableRect));
    }
    else
    {
        availableRect = availableWindowRect;
    }

    // Set the max width and height with the available bounds
    IFC_RETURN(m_tpPresenter.Cast<Control>()->put_MaxWidth(
        DoubleUtil::IsNaN(maxWidth) ? availableRect.Width : DoubleUtil::Min(maxWidth, availableRect.Width)));
    IFC_RETURN(m_tpPresenter.Cast<Control>()->put_MaxHeight(
        DoubleUtil::IsNaN(maxHeight) ? availableRect.Height : DoubleUtil::Min(maxHeight, availableRect.Height)));

    // Adjust the target position if the current target is out of bounds
    if (flowDirection == xaml::FlowDirection_LeftToRight)
    {
        if (targetPoint.X + presenterSize.Width > (availableRect.X + availableRect.Width))
        {
            if (useMonitorBounds)
            {
                // Update the target horizontal position if the target is out of the available bounds.
                // If the presenter width is greater than the current target left point from the screen,
                // the menu target left position is set to the begin of the screen position.
                horizontalOffset -= DoubleUtil::Min(
                    presenterSize.Width,
                    DoubleUtil::Max(0, targetPoint.X - availableRect.X));
            }
            else
            {
                if (m_isPositionedAtPoint)
                {
                    // Update the target horizontal position if the target is out of the available rect
                    horizontalOffset -= DoubleUtil::Min(presenterSize.Width, horizontalOffset);
                }
                else
                {
                    // Used for date and time picker flyouts
                    horizontalOffset = availableRect.X + availableRect.Width - presenterSize.Width;
                    horizontalOffset = DoubleUtil::Max(availableRect.X, horizontalOffset);
                }
            }

            // If we were positioned on the right, we're now positioned on the left.
            if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Right)
            {
                majorPlacementMode = FlyoutBase::MajorPlacementMode::Left;
            }
        }
    }
    else
    {
        if (targetPoint.X - availableRect.X < presenterSize.Width)
        {
            if (useMonitorBounds)
            {
                // Update the target horizontal position if the target is outside the available monitor
                // if the presenter width is greater than the current target right point from the screen,
                // the menu target left position is set to the end of the screen position.
                horizontalOffset += DoubleUtil::Min(
                    presenterSize.Width,
                    DoubleUtil::Max(0, availableRect.Width - targetPoint.X + availableRect.X));
            }
            else
            {
                // Adjust the target position if the current target is out of the Xaml window bounds
                if (m_isPositionedAtPoint)
                {
                    // Update the target horizontal position if the target is out of the available rect
                    horizontalOffset += DoubleUtil::Min(presenterSize.Width, (availableRect.Width + availableRect.X - horizontalOffset));
                }
                else
                {
                    // Used for date and time picker flyouts
                    horizontalOffset = presenterSize.Width + availableRect.X;
                    horizontalOffset = DoubleUtil::Min(availableRect.Width + availableRect.X, horizontalOffset);
                }
            }

            // If we were positioned on the left, we're now positioned on the right.
            if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Left)
            {
                majorPlacementMode = FlyoutBase::MajorPlacementMode::Right;
            }
        }
    }

    // If we couldn't actually fit to the left, flip back to show right.
    if (shiftLeftForRightHandedness)
    {
        if (!isRTL && targetPoint.X < availableRect.X)
        {
            horizontalOffset += presenterSize.Width;
            targetPoint.X += presenterSize.Width;

            // If we were positioned on the left, we're now positioned on the right.
            if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Left)
            {
                majorPlacementMode = FlyoutBase::MajorPlacementMode::Right;
            }
        }
        else if (isRTL && targetPoint.X + presenterSize.Width >= availableRect.Width)
        {
            horizontalOffset -= presenterSize.Width;
            targetPoint.X -= presenterSize.Width;

            // If we were positioned on the right, we're now positioned on the left.
            if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Right)
            {
                majorPlacementMode = FlyoutBase::MajorPlacementMode::Left;
            }
        }
    }

    // If opening up would cause the flyout to get clipped, we fall back to opening down
    if (preferTopPlacement && targetPoint.Y < availableRect.Y)
    {
        verticalOffset += presenterSize.Height;
        targetPoint.Y += presenterSize.Height;

        // If we were positioned on the top, we're now positioned on the bottom.
        if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Top)
        {
            majorPlacementMode = FlyoutBase::MajorPlacementMode::Bottom;
        }
    }

    if (targetPoint.Y + presenterSize.Height > availableRect.Y + availableRect.Height)
    {
        if (useMonitorBounds)
        {
            // Update the target vertical position if the target is out of the available monitor.
            // If the presenter height is greater than the current target top point from the screen,
            // the menu target top position is set to the begin of the screen position.
            if (verticalOffset > availableRect.Y)
            {
                verticalOffset = verticalOffset - DoubleUtil::Min(
                    presenterSize.Height,
                    DoubleUtil::Max(0, targetPoint.Y - availableRect.Y));
            }
            else // if it spans two monitors, make it start at the second.
            {
                verticalOffset = availableRect.Y;
            }
        }
        else
        {
            // Update the target vertical position if the target is out of the available rect
            if (m_isPositionedAtPoint)
            {
                verticalOffset -= DoubleUtil::Min(presenterSize.Height, verticalOffset);
            }
            else
            {
                verticalOffset = availableRect.Y + availableRect.Height - presenterSize.Height;
            }
        }

        // If we were positioned on the bottom, we're now positioned on the top.
        if (majorPlacementMode == FlyoutBase::MajorPlacementMode::Bottom)
        {
            majorPlacementMode = FlyoutBase::MajorPlacementMode::Top;
        }
    }

    if (!useMonitorBounds)
    {
        verticalOffset = DoubleUtil::Max(availableRect.Y, verticalOffset);
    }

    // The above accounting may have shifted our position, so we'll account for the exclusion rect again.
    accountForExclusionRect();

    // Accounting for the exclusion rect is important, but keeping the popup fully in view is more important, if possible.
    // If we are still not completely in view at this point, we'll fall back to our original positioning and then
    // nudge the popup into full view, as long as it will fit on screen.
    const bool presenterIsOffScreenHorizontally = isRTL ?
        (horizontalOffset - presenterSize.Width < availableRect.X ||
            horizontalOffset > availableRect.X + availableRect.Width) :
        (horizontalOffset < availableRect.X ||
            horizontalOffset + presenterSize.Width > availableRect.X + availableRect.Width);

    const bool presenterIsOffScreenVertically =
        verticalOffset < availableRect.Y ||
        verticalOffset + presenterSize.Height > availableRect.Y + availableRect.Height;

    // First we'll put the positioning back and re-account for the exclusion rect.
    if ((presenterSize.Width <= availableRect.Width && presenterIsOffScreenHorizontally) ||
        (presenterSize.Height <= availableRect.Height && presenterIsOffScreenVertically))
    {
        majorPlacementMode = originalMajorPlacementMode;
        accountForExclusionRect();
    }

    /// Now we'll nudge the popup into view.
    if (presenterSize.Width <= availableRect.Width)
    {
        // In RTL, the horizontal offset represents the top-right corner instead of the top-left corner,
        // so we need to adjust our calculations accordingly.
        if (isRTL)
        {
            if (horizontalOffset - presenterSize.Width < availableRect.X)
            {
                horizontalOffset = availableRect.X + presenterSize.Width;
            }
            else if (horizontalOffset > availableRect.X + availableRect.Width)
            {
                horizontalOffset = availableRect.X + availableRect.Width;
            }
        }
        else
        {
            if (horizontalOffset < availableRect.X)
            {
                horizontalOffset = availableRect.X;
            }
            else if (horizontalOffset + presenterSize.Width > availableRect.X + availableRect.Width)
            {
                horizontalOffset = availableRect.X + availableRect.Width - presenterSize.Width;
            }
        }
    }

    if (presenterSize.Height <= availableRect.Height)
    {
        if (verticalOffset < availableRect.Y)
        {
            verticalOffset = availableRect.Y;
        }
        else if (verticalOffset + presenterSize.Height > availableRect.Y + availableRect.Height)
        {
            verticalOffset = availableRect.Y + availableRect.Height - presenterSize.Height;
        }
    }

    IFC_RETURN(m_tpPopup->put_HorizontalOffset(horizontalOffset));
    IFC_RETURN(m_tpPopup->put_VerticalOffset(verticalOffset));

    DOUBLE leftMostEdge = (flowDirection == xaml::FlowDirection_LeftToRight) ? horizontalOffset : horizontalOffset - presenterSize.Width;

    presenterRect->X = static_cast<FLOAT>(leftMostEdge);
    presenterRect->Y = static_cast<FLOAT>(verticalOffset);
    presenterRect->Width = presenterSize.Width;
    presenterRect->Height = presenterSize.Height;

    return S_OK;
}

_Check_return_ HRESULT
FlyoutBase::GetFlyoutMetadata(_Out_ FlyoutMetadata** metadata)
{
    if (m_tpParentFlyout)
    {
        IFC_RETURN(m_tpParentFlyout.Cast<FlyoutBase>()->m_childFlyoutMetadata.CopyTo(metadata));
    }
    else
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetFlyoutMetadata(metadata));
    }

    return S_OK;
}

/* static */
bool FlyoutBase::GetIsAncestorOfTraverseThroughPopups(
    _In_opt_ CUIElement* pParentElem,
    _In_opt_ CUIElement* pMaybeDescendant)
{
    if (pParentElem == nullptr)
    {
        return false;
    }

    while ((pMaybeDescendant != nullptr) && (pMaybeDescendant != pParentElem))
    {
        pMaybeDescendant = pMaybeDescendant->GetUIElementAdjustedParentInternal(FALSE);
    }

    return pParentElem == pMaybeDescendant;
}

_Check_return_ HRESULT
FlyoutBase::FindParentFlyoutFromElement(
    _In_ xaml::IFrameworkElement* element,
    _Out_ FlyoutBase** parentFlyout
    )
{
    *parentFlyout = nullptr;

    // If the element is a CommandBar element, then we'll find the parent CommandBar first -
    // this element might be in the CommandBar's popup and not be a visual child of the flyout
    // that the CommandBar is a child of.
    ctl::ComPtr<FrameworkElement> elementLocal(static_cast<FrameworkElement*>(element));
    ctl::ComPtr<ICommandBarElement> elementAsCommandBarElement = elementLocal.AsOrNull<ICommandBarElement>();

    if (elementAsCommandBarElement)
    {
        ctl::ComPtr<ICommandBar> parentCommandBar;
        IFC_RETURN(CommandBar::FindParentCommandBarForElement(elementAsCommandBarElement.Get(), &parentCommandBar));

        if (parentCommandBar)
        {
            IFC_RETURN(parentCommandBar.As(&elementLocal));
        }
    }

    VisualTree* visualTree = VisualTree::GetForElementNoRef(elementLocal->GetHandle());

    ctl::ComPtr<wfc::IVectorView<xaml_primitives::Popup*>> popups;
    IFC_RETURN(VisualTreeHelper::GetOpenPopupsStatic(visualTree, &popups));

    unsigned int numPopups = 0;
    IFC_RETURN(popups->get_Size(&numPopups));

    // Iterate over the open popups, and for the ones that are associated with a flyout
    // check to see if the given element is a descendent of that flyout's presenter.
    for (unsigned int i = 0; i < numPopups; ++i)
    {
        ctl::ComPtr<xaml_primitives::IPopup> popup;
        IFC_RETURN(popups->GetAt(i, &popup));

        ctl::ComPtr<xaml_primitives::IFlyoutBase> flyout;
        IFC_RETURN(popup.Cast<Popup>()->get_AssociatedFlyout(&flyout));
        if (flyout)
        {
            // If the flyout is open, it should have a presenter.
            ASSERT(flyout.Cast<FlyoutBase>()->m_tpPresenter);

            auto pUIElemPopup = popup.Cast<Popup>()->GetHandle();
            auto pUIElemLocal = elementLocal->GetHandle();
            const bool isAncestorOf = GetIsAncestorOfTraverseThroughPopups(pUIElemPopup, pUIElemLocal);

            if (isAncestorOf || ctl::are_equal(popup.Get(), elementLocal.Get()))
            {
                IFC_RETURN(flyout.CopyTo(parentFlyout));
                break;
            }
        }
    }

    return S_OK;
}

// Callback for ShowAt() from core layer
_Check_return_ HRESULT FlyoutBase::ShowAtStatic(
    _In_ CFlyoutBase* pCoreFlyoutBase,
    _In_ CFrameworkElement* pCoreTarget)
{
    ASSERT(pCoreFlyoutBase);
    ASSERT(pCoreTarget);

    ctl::ComPtr<DependencyObject> flyout;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pCoreFlyoutBase, &flyout));
    ASSERT(ctl::is<IFlyoutBase>(flyout));

    ctl::ComPtr<DependencyObject> target;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pCoreTarget, &target));
    ASSERT(ctl::is<IFrameworkElement>(target));

    IFC_RETURN(flyout.Cast<FlyoutBase>()->ShowAt(target.Cast<FrameworkElement>()));

    return S_OK;
}

// Callback for ShowAt() from core layer
_Check_return_ HRESULT FlyoutBase::ShowAtStatic(
    _In_ CFlyoutBase* pFlyoutBase,
    _In_ CFrameworkElement* pTarget,
    wf::Point point,
    wf::Rect exclusionRect,
    xaml_primitives::FlyoutShowMode flyoutShowMode)
{
    ASSERT(pFlyoutBase);
    ASSERT(pTarget);

    ctl::ComPtr<DependencyObject> flyout;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pFlyoutBase, &flyout));
    ASSERT(ctl::is<IFlyoutBase>(flyout));

    ctl::ComPtr<DependencyObject> target;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pTarget, &target));

    ctl::ComPtr<FlyoutShowOptions> showOptions;
    IFC_RETURN(ctl::make(&showOptions));

    ctl::ComPtr<wf::IReference<wf::Point>> pointReference;
    IFC_RETURN(PropertyValue::CreateTypedReference<wf::Point>(point, &pointReference));
    IFC_RETURN(showOptions->put_Position(pointReference.Get()));

    ctl::ComPtr<wf::IReference<wf::Rect>> exclusionRectReference;
    IFC_RETURN(PropertyValue::CreateTypedReference<wf::Rect>(exclusionRect, &exclusionRectReference));
    IFC_RETURN(showOptions->put_ExclusionRect(exclusionRectReference.Get()));

    IFC_RETURN(showOptions->put_ShowMode(flyoutShowMode));

    ctl::ComPtr<IFlyoutShowOptions> showOptionsAsI;
    IFC_RETURN(showOptions.As(&showOptionsAsI));
    IFC_RETURN(flyout.Cast<FlyoutBase>()->ShowAtWithOptions(target.Get(), showOptionsAsI.Get()));
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::get_IsLightDismissOverlayEnabledImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_isLightDismissOverlayEnabled;

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::put_IsLightDismissOverlayEnabledImpl(BOOLEAN value)
{
    m_isLightDismissOverlayEnabled = value;

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::get_IsConstrainedToRootBoundsImpl(_Out_ BOOLEAN* pValue)
{
    IFC_RETURN(EnsurePopupAndPresenter());
    IFC_RETURN(m_tpPopup.Cast<Popup>()->get_IsConstrainedToRootBounds(pValue));
    return S_OK;
}

_Check_return_ HRESULT
FlyoutBase::OnClosingStatic(_In_ CFlyoutBase* object, _Out_ bool* cancel)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(object, &peer));
    if (peer)
    {
        ASSERT(ctl::is<IFlyoutBase>(peer));
        IFC_RETURN(peer.Cast<FlyoutBase>()->OnClosing(cancel));
    }

    return S_OK;
}

_Check_return_ HRESULT
FlyoutBase::ConfigurePopupOverlay()
{
    ASSERT(m_tpPopup);

    bool isOverlayVisible = false;
    IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(this, &isOverlayVisible));

    BOOLEAN isLightDismissEnabled = FALSE;
    IFC_RETURN(m_tpPopup->get_IsLightDismissEnabled(&isLightDismissEnabled));

    // Some modes of flyout configure their popup as not light-dismissible;
    // for those cases don't allow the overlay to be visible.
    isOverlayVisible &= !!isLightDismissEnabled;

    if (isOverlayVisible)
    {
        IFC_RETURN(m_tpPopup.Cast<Popup>()->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_On));

        // Configure the overlay with the correct theme brush.
        xstring_ptr themeBrush;
        if (ctl::is<xaml_controls::IMenuFlyout>(this))
        {
            IFC_RETURN(xstring_ptr::CloneBuffer(L"MenuFlyoutLightDismissOverlayBackground", &themeBrush));
        }
        else if (ctl::is<xaml_controls::IDatePickerFlyout>(this))
        {
            IFC_RETURN(xstring_ptr::CloneBuffer(L"DatePickerLightDismissOverlayBackground", &themeBrush));
        }
        else if (ctl::is<xaml_controls::ITimePickerFlyout>(this))
        {
            IFC_RETURN(xstring_ptr::CloneBuffer(L"TimePickerLightDismissOverlayBackground", &themeBrush));
        }
        else
        {
            // CalendarDatePicker doesn't have it's own flyout type, so we check for whether
            // the placement target has a templated parent that is a CalendarDatePicker.
            ctl::ComPtr<DependencyObject> templatedParent;
            if (m_tpPlacementTarget)
            {
                IFC_RETURN(m_tpPlacementTarget.Cast<FrameworkElement>()->get_TemplatedParent(&templatedParent));
            }

            if (templatedParent && ctl::is<xaml_controls::ICalendarDatePicker>(templatedParent.Get()))
            {
                IFC_RETURN(xstring_ptr::CloneBuffer(L"CalendarDatePickerLightDismissOverlayBackground", &themeBrush));
            }
            else
            {
                IFC_RETURN(xstring_ptr::CloneBuffer(L"FlyoutLightDismissOverlayBackground", &themeBrush));
            }
        }

        // Hook up our brush to the popup's overlay using theme resources.
        IFC_RETURN(static_cast<CPopup*>(m_tpPopup.Cast<Popup>()->GetHandle())->SetOverlayThemeBrush(themeBrush));
    }
    else
    {
        IFC_RETURN(m_tpPopup.Cast<Popup>()->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_Off));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::TryInvokeKeyboardAcceleratorImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    IFC_RETURN(OnProcessKeyboardAcceleratorsProtected(pArgs));
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::GetPresenterSize(_Out_ wf::Size* value)
{
    DOUBLE actualWidth = 0.0;
    DOUBLE actualHeight = 0.0;

    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_ActualWidth(&actualWidth));
    IFC_RETURN(m_tpPresenter.Cast<Control>()->get_ActualHeight(&actualHeight));

    value->Width = static_cast<FLOAT>(actualWidth);
    value->Height = static_cast<FLOAT>(actualHeight);

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::UpdateStateToShowMode(xaml_primitives::FlyoutShowMode showMode)
{
    if (showMode == xaml_primitives::FlyoutShowMode_Auto)
    {
        showMode = xaml_primitives::FlyoutShowMode_Standard;
    }

    IFC_RETURN(put_ShowMode(showMode));

    bool oldShouldHideIfPointerMovesAway = m_shouldHideIfPointerMovesAway;

    switch (showMode)
    {
    case xaml_primitives::FlyoutShowMode_Standard:
        m_shouldTakeFocus = true;
        m_shouldHideIfPointerMovesAway = false;
        m_shouldOverlayPassThroughAllInput = false;
        break;
    case xaml_primitives::FlyoutShowMode_Transient:
        m_shouldTakeFocus = false;
        m_shouldHideIfPointerMovesAway = false;
        m_shouldOverlayPassThroughAllInput = true;
        break;
    case xaml_primitives::FlyoutShowMode_TransientWithDismissOnPointerMoveAway:
        m_shouldTakeFocus = false;
        m_shouldHideIfPointerMovesAway = true;
        m_shouldOverlayPassThroughAllInput = true;
        break;
    default:
        ASSERT(FALSE, L"Unsupported FlyoutShowMode");
        break;
    }

    if (m_tpPopup)
    {
        BOOLEAN isOpen = FALSE;
        IFC_RETURN(m_tpPopup->get_IsOpen(&isOpen));
        if (isOpen)
        {
            IFC_RETURN(SetPopupLightDismissBehavior());
            IFC_RETURN(ConfigurePopupOverlay());

            if (m_shouldHideIfPointerMovesAway && !oldShouldHideIfPointerMovesAway)
            {
                IFC_RETURN(AddRootVisualPointerMovedHandler());
            }
            else if (!m_shouldHideIfPointerMovesAway && oldShouldHideIfPointerMovesAway)
            {
                IFC_RETURN(RemoveRootVisualPointerMovedHandler());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::GetEffectivePlacement(_Out_ xaml_primitives::FlyoutPlacementMode* effectivePlacement)
{
    if (m_hasPlacementOverride)
    {
        *effectivePlacement = m_placementOverride;
    }
    else
    {
        IFC_RETURN(get_Placement(effectivePlacement));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::IsOpen(_In_ CFlyoutBase* flyoutBase, _Out_ bool& isOpen)
{
    DBG_FLYOUT_TRACE(L">>> FlyoutBase::IsOpen()");

    ASSERT(flyoutBase);

    ctl::ComPtr<DependencyObject> flyoutDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(flyoutBase, &flyoutDO));

    BOOLEAN isOpenImpl = false;
    IFC_RETURN(flyoutDO.Cast<FlyoutBase>()->get_IsOpenImpl(&isOpenImpl));

    isOpen = !!isOpenImpl;
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::get_IsOpenImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;

    if (m_tpPopup)
    {
        IFC_RETURN(m_tpPopup->get_IsOpen(pValue));
    }

    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue)
{
    ctl::ComPtr<xaml::IXamlRoot> xamlRoot = XamlRoot::GetForElementStatic(this);
    *ppValue = xamlRoot.Detach();
    return S_OK;
}

_Check_return_ HRESULT FlyoutBase::put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue)
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this).Get();
    if(pValue == xamlRoot)
    {
        return S_OK;
    }

    if( xamlRoot != nullptr )
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_CANNOT_SET_XAMLROOT_WHEN_NOT_NULL));
    }

    IFC_RETURN(XamlRoot::SetForElementStatic(this, pValue));
    return S_OK;
}

// When it was initially added, the Target property had no dependency property backing it.
// In a later version, we added a dependency property, which requires us to separate the
// associated dependency property from the property itself in code-gen.  This, in turn,
// requires us to manually define the getter for the dependency property.
_Check_return_ HRESULT FlyoutBaseFactory::get_TargetPropertyImpl(_Outptr_result_maybenull_ xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::FlyoutBase_Target, ppValue));
}
