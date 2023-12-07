// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "StandardVisualStateManagerDataSource.h"
#include <VisualStateGroupCollection.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>

#include <Indexes.g.h>

#include <TimelineCollection.h>
#include <DynamicTimeline.h>
#include <DOCollection.h>
#include <CDependencyObject.h>
#include <DOPointerCast.h>
#include <animation.h>
#include <VisualTransition.h>
#include <VisualStateCollection.h>
#include <VisualStateGroup.h>
#include <VisualStateManager.h>
#include <VisualState.h>
#include <storyboard.h>
#include <ccontrol.h>
#include <StateTriggerCollection.h>
#include <AdaptiveTrigger.h>
#include <StateTriggerBase.h>
#include <QualifierFactory.h>
#include <Setter.h>
#include <SetterBaseCollection.h>
#include <StateTriggerCollection.h>

namespace {
    _Check_return_ HRESULT GetStateFromIndex(_In_ CVisualStateGroupCollection* groupCollection, _In_ int index, _Out_ xref_ptr<CVisualState>* ppResult)
    {
        *ppResult = nullptr;

        int stateIndex = 0;
        for (auto group : *groupCollection)
        {
            CValue stateCollectionValue;
            IFC_RETURN(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(
                KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
            xref_ptr<CDependencyObject> value = stateCollectionValue.DetachObject();

            if (value != nullptr)
            {
                ASSERT(value->OfTypeByIndex<KnownTypeIndex::VisualStateCollection>());
                for (auto state : *(static_cast<CVisualStateCollection*>(value.get())))
                {
                    if (stateIndex == index)
                    {
                        *ppResult = static_cast<CVisualState*>(state);
                        return S_OK;
                    }
                    stateIndex++;
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT GetGroupFromStateIndex(_In_ CVisualStateGroupCollection* groupCollection, _In_ int index, _Out_ xref_ptr<CVisualStateGroup>* ppResult)
    {
        *ppResult = nullptr;
        int stateIndex = 0;
        for (auto group : *groupCollection)
        {
            CValue stateCollectionValue;
            IFC_RETURN(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(
                KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
            xref_ptr<CDependencyObject> value = stateCollectionValue.DetachObject();

            int stateCount = static_cast<int>(value ?
                static_cast<CVisualStateCollection*>(value.get())->size() : 0);

            stateIndex += stateCount;
            if (index < stateIndex)
            {
                *ppResult = static_cast<CVisualStateGroup*>(group);
                return S_OK;
            }
        }

        return S_OK;
    }

    xref_ptr<CVisualStateGroup> GetGroupFromGroupIndex(_In_ CVisualStateGroupCollection* groupCollection, _In_ int index)
    {
        if ((index >= 0 ) && (index < static_cast<int>(groupCollection->GetCount())) && (groupCollection->GetCount() < INT_MAX))
        {
            auto visualStateGroup = static_cast<CVisualStateGroup*>(groupCollection->GetItemDOWithAddRef(index));
            xref_ptr<CVisualStateGroup> spVSGroup;
            spVSGroup.attach(visualStateGroup);
            return spVSGroup;
        }
        else
        {
            return nullptr;
        }
    }

    _Check_return_ HRESULT GetTransition(_In_ CVisualStateGroupCollection* groupCollection, _In_ int toIndex, _In_ int fromIndex, _Out_ xref_ptr<CVisualTransition>* ppTransition)
    {
        xref_ptr<CVisualState> toState = nullptr;
        xref_ptr<CVisualState> fromState = nullptr;
        if (toIndex != -1)
        {
            IFC_RETURN(GetStateFromIndex(groupCollection, toIndex, &toState));
        }

        if (fromIndex != -1)
        {
            IFC_RETURN(GetStateFromIndex(groupCollection, fromIndex, &fromState));
        }
        xref_ptr<CVisualStateGroup> group;
        IFC_RETURN(GetGroupFromStateIndex(groupCollection, toIndex, &group));

        xref_ptr<CVisualTransition> transition;
        IFC_RETURN(CVisualStateManager::GetTransition(group.get(), fromState.get(), toState.get(), transition.ReleaseAndGetAddressOf()));

        *ppTransition = transition;
        return S_OK;
    }

    _Success_(return) _Must_inspect_result_
    bool TryFindVisualState(
        _In_ CVisualStateGroupCollection* groupCollection,
        _In_z_ const WCHAR* stateName,
        _Out_ int* stateIndex,
        _Out_ int* groupIndex)
    {
        *stateIndex = -1;
        *groupIndex = -1;

        int returnStateIndex = 0;
        int returngroupIndex = 0;
        for (auto group : *groupCollection)
        {
            CValue stateCollectionValue;
            IFCFAILFAST(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(
                KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
            xref_ptr<CDependencyObject> value = stateCollectionValue.DetachObject();

            if (value != nullptr)
            {
                ASSERT(value->OfTypeByIndex<KnownTypeIndex::VisualStateCollection>());
                for (auto state : *(static_cast<CVisualStateCollection*>(value.get())))
                {
                    CValue cValueStateName;
                    IFCFAILFAST(state->GetValueByIndex(KnownPropertyIndex::DependencyObject_Name, &cValueStateName));

                    if (!cValueStateName.IsNullOrUnset() && cValueStateName.AsString().Equals(stateName))
                    {
                        *stateIndex = returnStateIndex;
                        *groupIndex = returngroupIndex;
                        return true;
                    }
                    returnStateIndex++;
                }
            }

            returngroupIndex++;
        }

        return false;
    }
}

StandardVisualStateManagerDataSource::StandardVisualStateManagerDataSource(CVisualStateGroupCollection* pGroupCollection)
    : VisualStateManagerDataSource(pGroupCollection)
{
    // Resize the group context vector such that every group index maps to a context.
    size_t visualStateGroupCount = m_pGroupCollection->size();

    if (m_pGroupCollection->GetGroupContext().size() < visualStateGroupCount)
    {
        m_pGroupCollection->GetGroupContext().resize(visualStateGroupCount);
    }
}

_Success_(return) _Must_inspect_result_
bool StandardVisualStateManagerDataSource::TryGetVisualStateImpl(_In_z_ const WCHAR* stateName,
    _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    return TryFindVisualState(m_pGroupCollection, stateName, stateIndex, groupIndex);
}

_Success_(return) _Must_inspect_result_
bool StandardVisualStateManagerDataSource::TryGetVisualStateImpl(_In_ VisualStateToken vsToken,
    _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    *stateIndex = -1;
    *groupIndex = -1;

    int vsIndex = 0;
    int vsGroupIndex = 0;

    for (auto group : *m_pGroupCollection)
    {
        CValue stateCollectionValue;
        IFCFAILFAST(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(
            KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
        xref_ptr<CDependencyObject> value = stateCollectionValue.DetachObject();

        if (value != nullptr)
        {
            ASSERT(value->OfTypeByIndex<KnownTypeIndex::VisualStateCollection>());
            for (auto state : *(static_cast<CVisualStateCollection*>(value.get())))
            {
                CVisualState* vs = static_cast<CVisualState*>(state);
                if(vsToken == vs->GetVisualStateToken())
                {
                    *stateIndex = vsIndex;
                    *groupIndex = vsGroupIndex;

                    return true;
                }
                ++vsIndex;
            }
        }
        ++vsGroupIndex;
    }

    return false;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::TryGetOrCreateTransitionImpl(_In_ int fromIndex, _In_ int toIndex, _Out_ std::shared_ptr<CVisualTransition>* pTransition)
{
    xref_ptr<CVisualTransition> transition;
    IFC_RETURN(GetTransition(m_pGroupCollection, toIndex, fromIndex, &transition));
    *pTransition = std::shared_ptr<CVisualTransition>(transition.detach(), [](CVisualTransition* owned) { if (owned) owned->Release(); });
    return S_OK;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::TryGetOrCreateStoryboardForVisualStateImpl(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard)
{
    xref_ptr<CVisualState> visualState;
    IFC_RETURN(GetStateFromIndex(m_pGroupCollection, index, &visualState));

    // A null visualstate represents the default visualstate, which we can be in only
    // on control load or when VSM triggers can't find a matching visualstate; this is
    // represented internally with a visualstate index of -1.
    // We don't expect to ever be in this method if the desired visualstate is null.
    // Therefore, assert so we catch it in CHK builds (in case of coding error),
    // but do a null check so we don't AV in production.
    ASSERT(visualState);
    if (visualState)
    {
        CValue storyboardCValue;
        IFC_RETURN(visualState->GetValueByIndex(KnownPropertyIndex::VisualState_Storyboard, &storyboardCValue));
        xref_ptr<CDependencyObject> value = storyboardCValue.DetachObject();
        *pStoryboard =  std::shared_ptr<CStoryboard>(static_cast<CStoryboard*>(value.detach()), [](CStoryboard* owned) { if (owned) owned->Release(); });
    }
    else
    {
        *pStoryboard = std::shared_ptr<CStoryboard>();
    }
    return S_OK;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::TryGetOrCreatePropertySettersForVisualStateImpl(_In_ int index, _Out_ std::vector<std::shared_ptr<CSetter>>* pSetterVector)
{
    xref_ptr<CVisualState> visualState;
    IFC_RETURN(GetStateFromIndex(m_pGroupCollection, index, &visualState));

    // A null visualstate represents the default visualstate, which we can be in only
    // on control load or when VSM triggers can't find a matching visualstate; this is
    // represented internally with a visualstate index of -1.
    // Refreshing VSM setters in the event of a theme change is done blindly for every
    // VisualStateGroup, so check to make sure that the VSG's current state is actually
    // non-null.
    if (visualState)
    {
        CValue settersCollection;
        IFC_RETURN(visualState->GetValueByIndex(KnownPropertyIndex::VisualState_Setters, &settersCollection));
        for (auto& item : (do_pointer_cast<CSetterBaseCollection>(settersCollection.DetachObject().get())->GetCollection()))
        {
            // Setters come out from the collection as raw pointers, so we need to AddRef before wrapping them in std::shared_ptr
            item->AddRef();
            pSetterVector->push_back(std::shared_ptr<CSetter>(do_pointer_cast<CSetter>(item), [](CSetter* owned) { if (owned) owned->Release(); }));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::OnBeginTransitionToState(_In_ int groupIndex, _In_ int stateIndex)
{
    auto group = GetGroupFromGroupIndex(m_pGroupCollection, groupIndex);
    xref_ptr<CVisualState> newState;
    IFC_RETURN(GetStateFromIndex(m_pGroupCollection, stateIndex, &newState));
    xref_ptr<CVisualState> currentState;
    IFC_RETURN(group->GetCurrentVisualState(currentState.ReleaseAndGetAddressOf()));

    auto owningControl = m_pGroupCollection->GetOwningControl();
    IFC_RETURN(group->NotifyVisualStateEvent(currentState.get(), newState.get(),
        CVisualStateGroup::VisualStateGroupEvent::VisualStateEventChanging,
        owningControl.get()));
    return S_OK;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::OnCompleteTransitionToState(_In_ int groupIndex)
{
    int stateIndex = m_pGroupCollection->GetGroupContext()[groupIndex].CurrentVisualStateIndex();
    auto group = GetGroupFromGroupIndex(m_pGroupCollection, groupIndex);
    xref_ptr<CVisualState> newState;
    IFC_RETURN(GetStateFromIndex(m_pGroupCollection, stateIndex, &newState));
    xref_ptr<CVisualState> currentState;
    IFC_RETURN(group->GetCurrentVisualState(currentState.ReleaseAndGetAddressOf()));
    IFC_RETURN(group->SetCurrentVisualState(newState.get()));

    auto owningControl = m_pGroupCollection->GetOwningControl();
    IFC_RETURN(group->NotifyVisualStateEvent(currentState.get(), newState.get(),
        CVisualStateGroup::VisualStateGroupEvent::VisualStateEventChanged,
        owningControl.get()));
    return S_OK;
}

// In the standard VSM storyboards are permanently parented to their VisualState or VisualTransition.
// The only storyboards that aren't parented this way are dynamic timelines, which for correctness
// and simplicitly, we still add to the m_pActiveStoryboard collection.
void StandardVisualStateManagerDataSource::ClearActiveStoryboardsImpl(_In_ int groupIndex)
{
    VisualStateManagerDataSource::ClearActiveStoryboardsImpl(groupIndex);
}

void StandardVisualStateManagerDataSource::RemoveActiveStoryboardImpl(_In_ int groupIndex, CStoryboard* storyboard)
{
    VisualStateManagerDataSource::RemoveActiveStoryboardImpl(groupIndex, storyboard);
}

void StandardVisualStateManagerDataSource::AddActiveStoryboardImpl(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type)
{
    if (storyboard->GetParent())
    {
        ASSERT(
                static_cast<CDependencyObject*>(storyboard->GetParent())->OfTypeByIndex<KnownTypeIndex::VisualTransition>()
            ||
                static_cast<CDependencyObject*>(storyboard->GetParent())->OfTypeByIndex<KnownTypeIndex::VisualState>()
            );
        ASSERT(type == VisualStateGroupContext::StoryboardType::Transition || type == VisualStateGroupContext::StoryboardType::State);
    }
    VisualStateManagerDataSource::AddActiveStoryboardImpl(groupIndex, std::move(storyboard), type);
}

// No-op: In the standard VSM VisualTransitions are always created and always in the live tree.
void StandardVisualStateManagerDataSource::ClearActiveTransitionsImpl(_In_ int /* groupIndex */)
{}

void StandardVisualStateManagerDataSource::RemoveActiveTransitionImpl(_In_ int /* groupIndex */, CVisualTransition* /* transition */)
{}

void StandardVisualStateManagerDataSource::AddActiveTransitionImpl(_In_ int /* groupIndex */, xref_ptr<CVisualTransition> /* transition */)
{}

int StandardVisualStateManagerDataSource::GetVisualStateCountImpl()
{
    int stateIndex = 0;
    for (auto group : *m_pGroupCollection)
    {
        CValue stateCollectionValue;
        IFCFAILFAST(static_cast<CVisualStateGroup*>(group)->GetValueByIndex(
            KnownPropertyIndex::VisualStateGroup_States, &stateCollectionValue));
        xref_ptr<CDependencyObject> value = stateCollectionValue.DetachObject();

        if (value != nullptr)
        {
            ASSERT(value->OfTypeByIndex<KnownTypeIndex::VisualStateCollection>());
            stateIndex += static_cast<int>(static_cast<CVisualStateCollection*>(value.get())->size());
        }
    }

    return stateIndex;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::TryGetOrCreateStateTriggerVariantMapImpl(int vsIndex, _Out_ std::shared_ptr<StateTriggerVariantMap>* pResult)
{
    xref_ptr<CVisualStateGroup> stateGroup;
    IFC_RETURN(GetGroupFromStateIndex(m_pGroupCollection, vsIndex, &stateGroup));
    if(!stateGroup->m_pStateTriggerVariantMap)
    {
        stateGroup->m_pStateTriggerVariantMap = std::make_shared<StateTriggerVariantMap>();
        m_pGroupCollection->GetStateTriggerVariantMaps().push_back(stateGroup->m_pStateTriggerVariantMap);
    }

    *pResult = stateGroup->m_pStateTriggerVariantMap;
    return S_OK;
}

_Check_return_ HRESULT StandardVisualStateManagerDataSource::GetQualifiersFromStateTriggersImpl(int index, OnQualifierCreatedCallback onCreated)
{
    ASSERT(onCreated);
    xref_ptr<CVisualState> visualState = nullptr;
    IFC_RETURN(GetStateFromIndex(m_pGroupCollection, index, &visualState));
    CStateTriggerCollection* pTriggers = visualState->m_pStateTriggerCollection;
    if (!pTriggers) return S_OK;

    std::shared_ptr<StateTriggerVariantMap> variantMap;
    IFC_RETURN(TryGetOrCreateStateTriggerVariantMap(index, &variantMap));
    pTriggers->m_pVariantMap = variantMap;

    for(auto& item : *pTriggers)
    {
        // Create qualifiers for AdaptiveTriggers
        CAdaptiveTrigger* pTrigger = do_pointer_cast<CAdaptiveTrigger>(item);
        if(pTrigger)
        {
            auto qualifier = QualifierFactory::Create(static_cast<int>(pTrigger->m_minWindowWidth),static_cast<int>(pTrigger->m_minWindowHeight));
            ASSERT(qualifier);
            IFC_RETURN(onCreated(visualState->GetVisualStateToken(), qualifier, xref_ptr<CStateTriggerBase>(pTrigger)));

            continue;
        }

        // Create qualifiers for extensible triggers
        CStateTriggerBase* pUserTrigger = do_pointer_cast<CStateTriggerBase>(item);
        if(pUserTrigger)
        {
            auto qualifier = QualifierFactory::Create(&pUserTrigger->m_triggerState);
            ASSERT(qualifier);
            IFC_RETURN(onCreated(visualState->GetVisualStateToken(), qualifier, xref_ptr<CStateTriggerBase>(pUserTrigger)));
        }
    }
    return S_OK;
}

std::vector<std::shared_ptr<StateTriggerVariantMap>> StandardVisualStateManagerDataSource::GetStateTriggerVariantMapsImpl()
{
    std::vector<std::shared_ptr<StateTriggerVariantMap>> variantMaps;

    for (auto group : *m_pGroupCollection)
    {
        if ((static_cast<CVisualStateGroup*>(group)->m_pStateTriggerVariantMap))
        {
            variantMaps.push_back(static_cast<CVisualStateGroup*>(group)->m_pStateTriggerVariantMap);
        }
    }

    return variantMaps;
}

xref_ptr<CDependencyObject> StandardVisualStateManagerDataSource::GetVisualStateImpl(VisualStateToken token)
{
    int index = -1;
    int group = -1;
    if(TryGetVisualStateImpl(token, &index, &group))
    {
        ASSERT(index != -1);
        ASSERT(group != -1);
        xref_ptr<CVisualState> state;
        IFCFAILFAST(GetStateFromIndex(m_pGroupCollection, index, &state));
        return state;
    }

    return nullptr;
}

xstring_ptr StandardVisualStateManagerDataSource::GetVisualStateNameImpl(VisualStateToken token)
{
    int index = -1;
    int group = -1;
    if(TryGetVisualStateImpl(token, &index, &group))
    {
        ASSERT(index != -1);
        ASSERT(group != -1);

        xref_ptr<CVisualState> visualState;
        IFCFAILFAST(GetStateFromIndex(m_pGroupCollection, index, &visualState));

        return visualState->m_strName;
    }

    return xstring_ptr();
}

// No-op for standard load path: setters will already be parented to VisualState.Setters by the parser
_Check_return_ HRESULT StandardVisualStateManagerDataSource::ParentDeferredSetterImpl(xref_ptr<CSetter> setter)
{
    return S_OK;
}

// No-op for standard load path: setters will already be parented to VisualState.Setters by the parser
_Check_return_ HRESULT StandardVisualStateManagerDataSource::UnparentDeferredSetterImpl(xref_ptr<CSetter> setter)
{
    return S_OK;
}

// No-op for standard load path: triggers will already be parented to VisualState.StateTriggers by the parser
_Check_return_ HRESULT StandardVisualStateManagerDataSource::ParentDeferredStateTriggerImpl(xref_ptr<CStateTriggerBase> stateTrigger)
{
    return S_OK;
}
