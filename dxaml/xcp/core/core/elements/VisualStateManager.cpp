// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MetadataAPI.h"

#include "Storyboard.h"
#include "Animation.h"
#include "DynamicTimeline.h"
#include "TimelineCollection.h"
#include "Duration.h"

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <vsm\inc\DynamicTimelineHelper.h>
#include <vsm\inc\CVisualStateManager2.h>
#include <vsm\inc\DynamicTransitionStoryboardGenerator.h>
#include <VisualStateCollection.h>
#include <VisualStateManagerDataSource.h>

using namespace DirectUI;
using namespace RuntimeFeatureBehavior;

_Check_return_ HRESULT
CVisualStateManager::GoToState(
    _In_ CDependencyObject *pObject,
    _In_z_ const WCHAR *pStateName,
    _In_ bool useTransitions,
    _Out_opt_ bool* pSuccess)
{
    return GoToState(pObject, pStateName, nullptr, nullptr, useTransitions, pSuccess);
}

_Check_return_ HRESULT
CVisualStateManager::GoToState(
    _In_ CDependencyObject *pObject,
    _In_z_ const WCHAR *pStateName,
    _In_ CVisualState* pVisualState,
    _In_ CVisualStateGroup* pVisualStateGroup,
    _In_ bool useTransitions,
    _Out_opt_ bool* pSuccess)
{
    if (pSuccess)
    {
        *pSuccess = false;
    }

    CControl *pControl = do_pointer_cast<CControl>(pObject);
    CFrameworkElement *pImplControl = do_pointer_cast<CFrameworkElement>(pControl->GetImplementationRoot());

    // Disable control state transitions for tests to reduce execution time.
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::DisableTransitionsForTest))
    {
        useTransitions = false;
    }

    xref_ptr<CVisualStateGroupCollection> groups;
    if (pImplControl && (groups = pImplControl->GetVisualStateGroupsNoCreate()))
    {
        bool succeeded = false;

        // Transition to the NULL state
        if(!pStateName && !pVisualState)
        {
            int groupIndex = -1;
            IFC_RETURN(groups->IndexOf(pVisualStateGroup, &groupIndex));
            if (groupIndex == -1) return E_FAIL;
            IFC_RETURN(CVisualStateManager2::ResetVisualStateGroupToNullState(pControl, groupIndex));
            succeeded = true;
        }
        // Transition to an unnamed VisualState
        else if(!pStateName)
        {
            IFC_RETURN(CVisualStateManager2::GoToStateOptimized(
                do_pointer_cast<CControl>(pObject), do_pointer_cast<CVisualState>(pVisualState)->GetVisualStateToken(), useTransitions, &succeeded));
        }
        // Transition to a named VisualState
        else
        {
            IFC_RETURN(CVisualStateManager2::GoToStateOptimized(
                do_pointer_cast<CControl>(pObject), pStateName, useTransitions, &succeeded));
        }
 
        if (pSuccess)
        {
            *pSuccess = succeeded;
        }
    }
    return S_OK;
}

HRESULT
_Check_return_
CVisualStateManager::FindVisualState(
    _In_ CControl* pControl,
    _In_opt_ CFrameworkElement* pImplControl,
    _In_z_ const WCHAR* pStateName,
    _Outptr_opt_result_maybenull_ CVisualStateGroup** ppStateGroup,
    _Outptr_opt_result_maybenull_ CVisualState** ppState,
    _Out_ bool *pFound)
{
    IFCPTR_RETURN(pFound);
    IFCPTR_RETURN(pControl);
    IFCPTR_RETURN(pStateName);

    *pFound = FALSE;
    if (ppStateGroup)
    {
        *ppStateGroup = nullptr;
    }
    if (ppState)
    {
        *ppState = nullptr;
    }

    // Check if we need to find the rool element or if it has been passed
    if (!pImplControl)
    {
        // we can expect only controls to be using this for now (as per spec)
        // however, due to the special casing for attached properties, we need
        // to keep the collection in the FrameworkElement.  It doesn't mean
        // every FrameworkElement can use VSM, as there are guards elsewhere
        // to only allow controls to use us
        IFC_RETURN(DoPointerCast(pImplControl, pControl->GetImplementationRoot()));
    }

    xref_ptr<CVisualStateGroupCollection> groups;
    if (pImplControl && (groups = pImplControl->GetVisualStateGroupsNoCreate()))
    {
        auto dataSource = CreateVisualStateManagerDataSource(groups.get());
        int stateIndex = -1;
        int groupIndex = -1;

        if (dataSource->TryGetVisualState(pStateName, &stateIndex, &groupIndex))
        {
            ASSERT(stateIndex != -1 && groupIndex != -1);
            xref_ptr<CVisualStateGroup> stateGroup;
            xref_ptr<CVisualState> state;
            stateGroup.attach(do_pointer_cast<CVisualStateGroup>(groups->GetItemDOWithAddRef(groupIndex)));

            // The stateIndex we get from the dataSource is actually the index of the desired state
            // within the set of all states, rather than its index within its parent group
            // Therefore, we need to figure out what the stateIndex is of the first state within our parent
            // group in order to figure out which state corresponds to our desired stateIndex
            unsigned int index = 0;
            for (int i = 0; i <= groupIndex; i++)
            {
                xref_ptr<CVisualStateGroup> group;
                group.attach(do_pointer_cast<CVisualStateGroup>(groups->GetItemDOWithAddRef(i)));

                CValue stateCollectionValue;
                IFC_RETURN(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
                xref_ptr<CVisualStateCollection> stateCollection;
                stateCollection.attach(do_pointer_cast<CVisualStateCollection>(stateCollectionValue.DetachObject()));

                if (i != groupIndex)
                {
                    index += stateCollection->GetCount();
                }
                else
                {
                    ASSERT(((unsigned int)stateIndex - index) < stateCollection->GetCount());
                    state.attach(do_pointer_cast<CVisualState>(stateCollection->GetItemDOWithAddRef(stateIndex - index)));
                }

                stateCollection.detach();
            }

            *ppStateGroup = stateGroup.detach();
            *ppState = state.detach();

            *pFound = TRUE;
        }
        else
        {
            // If no state matches the desired name, then we can just exit early
            // without having to fault in the VSM
            *pFound = FALSE;
        }
    }

    return S_OK;
}

#pragma region Legacy VSM Code
// None of this code is used in XBFv2-based VSM implementations. It is considered dead code
// only kept for strict back-compat reasons. No VSM written with XBFv2 will use these code paths,
// only VSMs created using text parsing or from XBFv1 will use this code.

// This function judges the transitions to find the best transition based on the to/from values
// and returns the best transition
_Check_return_ HRESULT
CVisualStateManager::GetTransition(
_In_ CVisualStateGroup *pVisualStateGroup,
_In_opt_ CVisualState *pFrom,
_In_ CVisualState *pTo,
_Outptr_result_maybenull_ CVisualTransition **ppVisualTransition)
{
    CVisualTransition* pBest = nullptr;
    CVisualTransition* pDefault = nullptr;

    *ppVisualTransition = nullptr;

    const xstring_ptr& fromString = pFrom ? pFrom->m_strName : xstring_ptr::NullString();
    const xstring_ptr& toString = pTo ? pTo->m_strName : xstring_ptr::NullString();
    // Use a scoring system:
    //      1 point if it matches the from state
    //      2 points for matching the to state (3 points for both)
    // Highest score wins (else use default transition)
    int bestScore = -1;
    if (pVisualStateGroup->m_pTransitions)
    {
        for (auto transition : *pVisualStateGroup->m_pTransitions)
        {
            auto pTransition = static_cast<CVisualTransition*>(transition);

            if (pTransition->GetIsDefault())
            {
                pDefault = pTransition;
            }
            else
            {
                int score = -1;
                bool shouldSkip = false;

                if (fromString.Equals(pTransition->m_strFrom, xstrCompareCaseInsensitive))
                {
                    score += 1;
                }
                else if (!pTransition->m_strFrom.IsNullOrEmpty())
                {
                    // If, upon a transition having a state string that doesn't match the current state we're in,
                    // we only disqualify it if it matches NO states in the VisualStateGroup.
                    xref_ptr<CVisualState> foundState;
                    IFCFAILFAST(pVisualStateGroup->FindStateByName(pTransition->m_strFrom.GetBuffer(), foundState.ReleaseAndGetAddressOf()));
                    shouldSkip = !!foundState;
                }

                if (toString.Equals(pTransition->m_strTo, xstrCompareCaseInsensitive))
                {
                    score += 2;
                }
                else if (!pTransition->m_strTo.IsNullOrEmpty())
                {
                    xref_ptr<CVisualState> foundState;
                    IFCFAILFAST(pVisualStateGroup->FindStateByName(pTransition->m_strTo.GetBuffer(), foundState.ReleaseAndGetAddressOf()));
                    shouldSkip = !!foundState;
                }

                if (!shouldSkip && score > bestScore)
                {
                    bestScore = score;
                    pBest = pTransition;
                }
            }
        }

        if (pBest)
        {
            *ppVisualTransition = pBest;
            (*ppVisualTransition)->AddRef();
        }
        else if (pDefault)
        {
            *ppVisualTransition = pDefault;
            (*ppVisualTransition)->AddRef();
        }
    }
    return S_OK;
}

static inline bool IsZeroDuration(
    _In_opt_ const DurationVO::Wrapper* pDuration)
{
    return (!pDuration || pDuration->Value().GetTimeSpanInSec() == 0.0);
}

static inline bool IsZeroDuration(
    _In_ CStoryboard* pStoryboard)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool bIsZeroDuration = false;
    XFLOAT rDuration;
    DurationType durationType;

    if (pStoryboard == NULL)
    {
        bIsZeroDuration = TRUE;
        goto Cleanup;
    }

    IFC(pStoryboard->GetDuration(&durationType, &rDuration));
    bIsZeroDuration = ((0.0f == rDuration) ? TRUE : FALSE);

Cleanup:
    return bIsZeroDuration;
}

#pragma endregion
