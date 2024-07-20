// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "InputServices.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

const INT ModernCollectionBasePanel::s_cNavigationIndexFirst = 0;
const INT ModernCollectionBasePanel::s_cNavigationIndexLast = std::numeric_limits<INT>::max();
const INT ModernCollectionBasePanel::s_cNavigationIndexNone = -1;

_Check_return_ HRESULT ModernCollectionBasePanel::SupportsKeyNavigationAction(
    _In_ xaml_controls::KeyNavigationAction action,
    _Out_ BOOLEAN* pSupportsAction)
{
    // We support everything!
    *pSupportsAction = TRUE;

    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetTargetIndexFromNavigationAction(
    _In_ UINT sourceIndex,
    _In_ xaml_controls::ElementType sourceType,
    _In_ xaml_controls::KeyNavigationAction action,
    _In_ BOOLEAN allowWrap,
    _In_ UINT itemIndexHintForHeaderNavigation,
    _Out_ UINT* computedTargetIndex,
    _Out_ xaml_controls::ElementType* computedTargetElementType,
    _Out_ BOOLEAN* actionValidForSourceIndex)
{
    HRESULT hr = S_OK;
    UINT targetIndex = sourceIndex;
    auto targetType = xaml_controls::ElementType_ItemContainer;


    *computedTargetIndex = 0;
    *actionValidForSourceIndex = FALSE;

    if (action == xaml_controls::KeyNavigationAction_First)
    {
        targetIndex = 0;
    }
    else if (action == xaml_controls::KeyNavigationAction_Last)
    {
        targetIndex = m_cacheManager.GetTotalItemCount() - 1;
    }
    else if (action == xaml_controls::KeyNavigationAction_Next)
    {
        IFC(GetTargetIndexFromNavigationActionByPage(sourceIndex, TRUE /*isPageDown*/, allowWrap, &targetIndex));
    }
    else if (action == xaml_controls::KeyNavigationAction_Previous)
    {
        IFC(GetTargetIndexFromNavigationActionByPage(sourceIndex, FALSE /*isPageDown*/, allowWrap, &targetIndex));
    }
    else if (action == xaml_controls::KeyNavigationAction_Left ||
        action == xaml_controls::KeyNavigationAction_Right ||
        action == xaml_controls::KeyNavigationAction_Up ||
        action == xaml_controls::KeyNavigationAction_Down)
    {
        int sourceIndexSigned = static_cast<int>(sourceIndex);
        int targetIndexSigned = 0;

        // The layout strategies deal with layout indices, so if this
        // is a group header, translate the given index to a
        // layout index index.
        if (sourceType == xaml_controls::ElementType_GroupHeader)
        {
            sourceIndexSigned = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, sourceIndexSigned);
        }

        IFC(m_spLayoutStrategy->GetTargetIndexFromNavigationAction(
            sourceType,
            sourceIndexSigned,
            action,
            m_windowState.GetRealizationWindow(),
            static_cast<int>(itemIndexHintForHeaderNavigation),
            &targetType,
            &targetIndexSigned));

        // The layout strategies deal with layout indices, so if this
        // is a group header, translate the resulting index to a
        // data index index.
        if (targetType == xaml_controls::ElementType_GroupHeader)
        {
            targetIndexSigned = m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, targetIndexSigned);
        }

        targetIndex = static_cast<unsigned int>(targetIndexSigned);

        if (!allowWrap && sourceType == targetType && sourceIndex != targetIndex)
        {
            ctl::ComPtr<xaml::IDependencyObject> oldContainer;
            ctl::ComPtr<xaml::IUIElement> oldContainerAsUIE;
            ctl::ComPtr<xaml::IDependencyObject> newContainer;
            ctl::ComPtr<xaml::IUIElement> newContainerAsUIE;

            if (sourceType == xaml_controls::ElementType_ItemContainer)
            {
                IFC(ContainerFromIndex(sourceIndex, &oldContainer));
                IFC(ContainerFromIndex(targetIndex, &newContainer));
            }
            else
            {
                IFC(HeaderFromIndex(sourceIndex, &oldContainer));
                IFC(HeaderFromIndex(targetIndex, &newContainer));
            }

            if (oldContainer && newContainer)
            {
                oldContainerAsUIE = oldContainer.AsOrNull<xaml::IUIElement>();
                newContainerAsUIE = newContainer.AsOrNull<xaml::IUIElement>();
            }

            if (oldContainerAsUIE && newContainerAsUIE)
            {
                UIElement::VirtualizationInformation* pOldContainerVirtualizationInformation =
                    oldContainerAsUIE.Cast<UIElement>()->GetVirtualizationInformation();

                UIElement::VirtualizationInformation* pNewContainerVirtualizationInformation =
                    newContainerAsUIE.Cast<UIElement>()->GetVirtualizationInformation();

                ASSERT(pOldContainerVirtualizationInformation && pNewContainerVirtualizationInformation);

                // If we aren't allowing wrapping and we're about to wrap, then we'll set the target index
                // equal to the source index to indicate that this operation is not allowed.
                if (((action == xaml_controls::KeyNavigationAction_Left &&
                        pOldContainerVirtualizationInformation->GetBounds().X <= pNewContainerVirtualizationInformation->GetBounds().X) ||
                    (action == xaml_controls::KeyNavigationAction_Up &&
                        pOldContainerVirtualizationInformation->GetBounds().Y <= pNewContainerVirtualizationInformation->GetBounds().Y) ||
                    (action == xaml_controls::KeyNavigationAction_Right &&
                        pOldContainerVirtualizationInformation->GetBounds().X >= pNewContainerVirtualizationInformation->GetBounds().X) ||
                    (action == xaml_controls::KeyNavigationAction_Down &&
                        pOldContainerVirtualizationInformation->GetBounds().Y >= pNewContainerVirtualizationInformation->GetBounds().Y)))
                {
                    targetIndex = sourceIndex;
                }
            }
        }
    }

    *computedTargetIndex = targetIndex;
    *computedTargetElementType = targetType;
    *actionValidForSourceIndex = sourceIndex != targetIndex || sourceType != targetType;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetTargetHeaderIndexFromNavigationAction(
    _In_ UINT sourceIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _Out_ UINT* computedTargetIndex,
    _Out_ BOOLEAN* pActionHandled)
{
    HRESULT hr = S_OK;

    *pActionHandled = FALSE;
    *computedTargetIndex = sourceIndex;

    if (action == xaml_controls::KeyNavigationAction_Next ||
        action == xaml_controls::KeyNavigationAction_Previous)
    {
        // Nothing to do for PgUp/PgDown
    }
    else
    {
        if (action == xaml_controls::KeyNavigationAction_First)
        {
            // Search for the first non-hidden header
            for (INT32 candidateGroupIndex = 0; candidateGroupIndex < m_cacheManager.GetTotalGroupCount(); ++candidateGroupIndex)
            {
                if (ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, candidateGroupIndex))
                {
                    *computedTargetIndex = candidateGroupIndex;
                    break;
                }
            }
        }
        else if (action == xaml_controls::KeyNavigationAction_Last)
        {
            // Search for the last non-hidden header
            for (INT32 candidateGroupIndex = m_cacheManager.GetTotalGroupCount() - 1; candidateGroupIndex >= 0; --candidateGroupIndex)
            {
                if (ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, candidateGroupIndex))
                {
                    *computedTargetIndex = candidateGroupIndex;
                    break;
                }
            }
        }
        else if (action == xaml_controls::KeyNavigationAction_Left ||
            action == xaml_controls::KeyNavigationAction_Right ||
            action == xaml_controls::KeyNavigationAction_Up ||
            action == xaml_controls::KeyNavigationAction_Down)
        {
            IFC(GetTargetHeaderIndexFromNavigationActionByItem(
                sourceIndex,
                action,
                computedTargetIndex));
        }

        *pActionHandled = (sourceIndex != *computedTargetIndex);
    }

Cleanup:
    RRETURN(hr);
}

// Given an index, calculate the index of the item one page from it.
// WARNING: This method will potentially realize containers to figure
// things out. Call with caution.
// Note: computedTargetIndex will be set to sourceIndex if we are unable to
// find a target index.
_Check_return_ HRESULT ModernCollectionBasePanel::GetTargetIndexFromNavigationActionByPage(
    _In_ UINT sourceIndex,
    _In_ BOOLEAN isPageDown,
    _In_ BOOLEAN allowWrap,
    _Out_ UINT* computedTargetIndex)
{
    HRESULT hr = S_OK;

    // The usual behavior for PgDn is to jump to the last visible item.
    // If we're already on the last fully visible item (or after it), move
    // one page over then jump to the last visible item.
    // Sometimes, however, the last visible item is irrelevant to
    // the process and should never be jumped to. This is the case
    // if the source index isn't in the visible window or if the source
    // index container isn't realized.
    // In this case, things get complicated - we must bring the source
    // container into view on the appropriate view window side, realize one
    // more page in the appropriate direction, then find the last/first
    // visible container. The latter is the target index.
    ctl::ComPtr<IDependencyObject> spSourceObject;
    BOOLEAN viewIsRelevantForCalculation = FALSE;

    // We're going to mess with the visible window quite a bit trying to figure out
    // what a "page" is. Save off the visible window.
    const wf::Rect originalVisibleWindow = m_windowState.GetVisibleWindow();
    BOOLEAN modifiedVisibleWindow = FALSE;
    *computedTargetIndex = sourceIndex;

    IFC(ContainerFromIndexImpl(sourceIndex, &spSourceObject));

    // Make sure the source container is in the visible window.
    if (spSourceObject)
    {
        ctl::ComPtr<IUIElement> spSourceObjectAsUI;

        IFC(spSourceObject.As<IUIElement>(&spSourceObjectAsUI));
        wf::Rect sourceObjectBounds = GetBoundsFromElement(spSourceObjectAsUI);

        viewIsRelevantForCalculation = !RectUtil::AreDisjoint(sourceObjectBounds, m_windowState.GetVisibleWindow());
    }

    // See if we're the last visible container in the view.
    if (viewIsRelevantForCalculation)
    {
        UINT lastVisibleItemIndex = 0;

        IFC(GetLastVisibleContainer(isPageDown, TRUE, &lastVisibleItemIndex, nullptr));

        // We must scroll the page if sourceIndex is at or after the last fully visible item index.
        const BOOLEAN isAtOrAfterLastFullyVisibleItem = isPageDown ? (sourceIndex >= lastVisibleItemIndex) : (sourceIndex <= lastVisibleItemIndex);
        const BOOLEAN isAtLastItem = isPageDown ? (sourceIndex == m_cacheManager.GetTotalItemCount() - 1) : (sourceIndex == 0);

        // get last input device type. If it is gamepad, we do not want
        // to jump to the last visible index in the same page.
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
        const bool isGamepadInput = contentRoot->GetInputManager().GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote;

        if ((isAtOrAfterLastFullyVisibleItem || isGamepadInput) && !isAtLastItem)
        {
            // We must scroll the view, run virtualization, and see what the last visible item is.
            // Then return that item's index.
            IFC(SynchronousScrollByPage(isPageDown));
            IFC(GetLastVisibleContainer(isPageDown, TRUE, &lastVisibleItemIndex, nullptr));
            if (sourceIndex == lastVisibleItemIndex)
            {
                // In this case, we've entered a case where items are big enough to remain
                // the only fully visible item despite scrolling by a page. Manually add or
                // subtract one to the index depending on the direction.
                if (isPageDown)
                {
                    if ((lastVisibleItemIndex + 1) < static_cast<UINT>(m_cacheManager.GetTotalItemCount()))
                    {
                        ++lastVisibleItemIndex;
                    }
                }
                else if (lastVisibleItemIndex > 0)
                {
                    --lastVisibleItemIndex;
                }
            }
            modifiedVisibleWindow = TRUE;
            *computedTargetIndex = lastVisibleItemIndex;
        }
        else
        {
            // We're not on the last visible container.
            // Just jump to the last visible container.
            *computedTargetIndex = lastVisibleItemIndex;
        }
    }
    else if (sourceIndex < static_cast<UINT>(m_cacheManager.GetTotalItemCount()))
    {
        // We're not in view.
        // 1- Bring the source index into view, aligned to the edge of the window we're paging towards.
        // 2- Scroll one page in the appropriate direction.
        // Find the last visible container in the direction appropriate.
        IFC(ScrollItemIntoView(sourceIndex, xaml_controls::ScrollIntoViewAlignment_Leading, 0.0, TRUE /* forceSynchronous */));
        IFC(SynchronousScrollByPage(isPageDown));
        IFC(GetLastVisibleContainer(isPageDown, TRUE, computedTargetIndex, nullptr));
        modifiedVisibleWindow = TRUE;
    }

    // If we messed with the visible window, restore it here.
    // NOTE: This isn't just to preserve state. Over the course of
    // the above code, our window may have been set out of the real
    // panel extent. It's difficult to detect this. So, we let the
    // offset be corrected by the next measure pass.
    if (modifiedVisibleWindow)
    {
        IFC(ScrollRectIntoView(originalVisibleWindow, FALSE /* forceSynchronous */ ));
    }

Cleanup:
    RRETURN(hr);
}

// Calculates the next group header index for a given selection index and item navigation action.
_Check_return_ HRESULT ModernCollectionBasePanel::GetTargetHeaderIndexFromNavigationActionByItem(
    _In_ UINT sourceIndex,
    _In_ xaml_controls::KeyNavigationAction action,
    _Out_ UINT* computedTargetIndex)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;

    *computedTargetIndex = sourceIndex;

    IFC(m_spLayoutStrategy->GetVirtualizationDirection(&orientation));

    if ((orientation == xaml_controls::Orientation_Horizontal && (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Right)) ||
        (orientation == xaml_controls::Orientation_Vertical && (action == xaml_controls::KeyNavigationAction_Up || action == xaml_controls::KeyNavigationAction_Down)))
    {
        // Handle only actions along axis of virtualization.
        INT step = (action == xaml_controls::KeyNavigationAction_Left || action == xaml_controls::KeyNavigationAction_Up) ? -1 : 1;

        // Search for the next non-hidden header
        for (INT tempTargetIndex = sourceIndex + step; 0 <= tempTargetIndex && tempTargetIndex < m_cacheManager.GetTotalGroupCount(); tempTargetIndex += step)
        {
            if (ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, tempTargetIndex))
            {
                *computedTargetIndex = tempTargetIndex;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Calculates the prior visible group header index from the provided index.
// Returns the provided index if no visible smaller index is found.
_Check_return_ HRESULT ModernCollectionBasePanel::GetPreviousHeaderIndex(
    _In_ UINT sourceIndex,
    _Out_ UINT* pComputedPreviousIndex)
{
    *pComputedPreviousIndex = sourceIndex;

    // Search for the prior non-hidden header
    for (INT tempPreviousIndex = sourceIndex - 1; 0 <= tempPreviousIndex && tempPreviousIndex < m_cacheManager.GetTotalGroupCount(); tempPreviousIndex--)
    {
        if (ShouldElementBeVisible(xaml_controls::ElementType_GroupHeader, tempPreviousIndex))
        {
            *pComputedPreviousIndex = tempPreviousIndex;
            break;
        }
    }

    return S_OK;
}

// Called when attempting to animate from the current group header (currentGroupIndex)
// to another group header (newGroupIndex).
// When animating down/right to a larger index, returns the last item's index in the visible
// group prior to the target group.
// When animating up/left to a smaller index, returns the first item's index in the target group.
_Check_return_ HRESULT ModernCollectionBasePanel::GetNeighboringItemIndex(
    _In_ UINT currentGroupIndex,
    _In_ UINT newGroupIndex,
    _Out_ UINT* pNeighboringItemIndex)
{
    *pNeighboringItemIndex = 0;

    INT32 startItemIndex = 0;
    INT32 itemCountInGroup = 0;

    if (newGroupIndex < currentGroupIndex)
    {
        // Attempting to animate up to a smaller group index.
        // Identify the first item index of that target group.
        IFC_RETURN(m_cacheManager.GetGroupInformationFromGroupIndex(newGroupIndex, &startItemIndex, &itemCountInGroup));
        *pNeighboringItemIndex = static_cast<UINT>(startItemIndex);
    }
    else
    {
        ASSERT(newGroupIndex > currentGroupIndex);
        // Attempting to animate down to a larger group index.
        // Identify the last item index of the visible group just above that target group.
        UINT previousGroupIndex = 0;
        IFC_RETURN(GetPreviousHeaderIndex(newGroupIndex, &previousGroupIndex));
        IFC_RETURN(m_cacheManager.GetGroupInformationFromGroupIndex(previousGroupIndex, &startItemIndex, &itemCountInGroup));
        *pNeighboringItemIndex = static_cast<UINT>(startItemIndex + itemCountInGroup - 1);
    }

    return S_OK;
}