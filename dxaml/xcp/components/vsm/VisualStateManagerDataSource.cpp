// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <VisualStateManagerDataSource.h>
#include "OptimizedVisualStateManagerDataSource.h"
#include "StandardVisualStateManagerDataSource.h"

#include <VisualStateGroupCollection.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <VisualStateSetterHelper.h>
#include <VisualTransitionCollection.h>
#include <TimelineCollection.h>
#include <DynamicTimeline.h>
#include <StoryboardCollection.h>
#include <CDependencyObject.h>
#include <DOPointerCast.h>
#include <animation.h>
#include <VisualTransition.h>
#include <storyboard.h>
#include <corep.h>
#include <StateTriggerCollection.h>
#include <StateTriggerBase.h>
#include <Setter.h>
#include <SetterBaseCollection.h>
#include <DXamlServices.h>

VisualStateManagerDataSource::VisualStateManagerDataSource(CVisualStateGroupCollection* pGroupCollection)
    : m_pGroupCollection(pGroupCollection)
{
}

bool VisualStateManagerDataSource::IsAnimationEnabled() const
{
    return m_pGroupCollection->AreAnimationsEnabled();
}

int VisualStateManagerDataSource::GetGroupCount() const
{
    return static_cast<int>(m_pGroupCollection->GetGroupContext().size());
}

VisualStateGroupContext& VisualStateManagerDataSource::GetGroupContext(_In_ int groupIndex)
{
    return m_pGroupCollection->GetGroupContext()[groupIndex];
}

const std::vector<std::pair<CStoryboard*, VisualStateGroupContext::StoryboardType>>& VisualStateManagerDataSource::GetActiveStoryboards(_In_ int groupIndex) const
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].ActiveStoryboards();
}

const std::vector<CVisualTransition*>& VisualStateManagerDataSource::GetActiveTransitions(_In_ int groupIndex) const
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].ActiveTransitions();
}

void VisualStateManagerDataSource::SetPendingStoryboard(_In_ int groupIndex, _In_opt_ CStoryboard* storyboard)
{
    m_pGroupCollection->GetGroupContext()[groupIndex].SetPendingStoryboard(storyboard);
}

_Ret_maybenull_ CStoryboard* VisualStateManagerDataSource::GetPendingStoryboard(_In_ int groupIndex)
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].GetPendingStoryboard();
}

_Success_(return) _Must_inspect_result_
bool VisualStateManagerDataSource::TryGetVisualState(_In_z_ const WCHAR* stateName,
        _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    return TryGetVisualStateImpl(stateName, stateIndex, groupIndex);
}

_Success_(return) _Must_inspect_result_
bool VisualStateManagerDataSource::TryGetVisualState(_In_ VisualStateToken vsToken,
        _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    return TryGetVisualStateImpl(vsToken, stateIndex, groupIndex);
}

void VisualStateManagerDataSource::RemoveActiveStoryboard(_In_ int groupIndex, CStoryboard* storyboard)
{
    RemoveActiveStoryboardImpl(groupIndex, storyboard);
    auto& activeStoryboards = m_pGroupCollection->GetGroupContext()[groupIndex].ActiveStoryboards();
    activeStoryboards.erase(std::remove_if(activeStoryboards.begin(), activeStoryboards.end(),
        [storyboard](const std::pair<CStoryboard*, VisualStateGroupContext::StoryboardType>& item) {
            return (item.first) == storyboard;
    }), activeStoryboards.end());
}

void VisualStateManagerDataSource::AddActiveStoryboard(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type)
{
    AddActiveStoryboardImpl(groupIndex, storyboard, type);
    m_pGroupCollection->GetGroupContext()[groupIndex].ActiveStoryboards().push_back(std::make_pair(storyboard.get(), type));
}

void VisualStateManagerDataSource::AddPendingPropertySetters(
    _In_ int groupIndex,
    _In_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters)
{
    auto& currentPendingSetters = m_pGroupCollection->GetGroupContext()[groupIndex].PendingPropertySetters();
    currentPendingSetters.clear();
    currentPendingSetters.reserve(propertySetters.size());
    currentPendingSetters.insert(
        currentPendingSetters.cend(),
        propertySetters.begin(),
        propertySetters.end());
}

_Check_return_ HRESULT VisualStateManagerDataSource::SetAndApplyActivePropertySetters(
    _In_ int groupIndex,
    _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters,
    _In_ const bool forceDeferOperation)
{
    auto& activePropertySetters = m_pGroupCollection->GetGroupContext()[groupIndex].ActivePropertySetters();
    auto clearActiveSettersGuard = wil::scope_exit([&] { activePropertySetters.clear(); });

    activePropertySetters.clear();
    activePropertySetters.reserve(propertySetters.size());
    activePropertySetters.insert(
        activePropertySetters.cend(),
        propertySetters.begin(),
        propertySetters.end());
    activePropertySetters.shrink_to_fit();

    IFC_RETURN(ApplyPropertySetters(groupIndex, propertySetters, forceDeferOperation));

    clearActiveSettersGuard.release();

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::ApplyPropertySetters(
    _In_ int groupIndex,
    _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters,
    _In_ const bool forceDeferOperation)
{
    for (auto setter : propertySetters)
    {
        // TODO: Debugger check is (hopefully) a temporary measure that will be removed at a later date
        HRESULT hr = VisualStateSetterHelper::PerformAnimatedValueOperation(
            VisualStateSetterHelper::SetterOperation::Set,
            setter.get_TargetObject().lock(),
            setter.get_TargetProperty(),
            setter.get_Value(),
            setter.get_OriginalSetter(),
            forceDeferOperation);
        if (IsDebuggerPresent())
        {
            IFC_RETURN(hr);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::UnapplyPropertySetters(
    _In_ int groupIndex,
    _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters,
    _In_ const bool forceDeferOperation)
{
    CValue unsetValue;
    unsetValue.Unset();

    for (auto& setter : propertySetters)
    {
        // TODO: Debugger check is (hopefully) a temporary measure that will be removed at a later date
        HRESULT hr = VisualStateSetterHelper::PerformAnimatedValueOperation(
            VisualStateSetterHelper::SetterOperation::Unset,
            setter.get_TargetObject().lock(),
            setter.get_TargetProperty(),
            unsetValue,
            setter.get_OriginalSetter(),
            forceDeferOperation);
        if (IsDebuggerPresent())
        {
            IFC_RETURN(hr);
        }
    }

    return S_OK;
}

const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& VisualStateManagerDataSource::GetActivePropertySetters(_In_ int groupIndex) const
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].ActivePropertySetters();
}

const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& VisualStateManagerDataSource::GetPendingPropertySetters(_In_ int groupIndex) const
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].PendingPropertySetters();
}

void VisualStateManagerDataSource::ClearPendingPropertySetters(_In_ int groupIndex)
{
    auto& pendingPropertySetters = m_pGroupCollection->GetGroupContext()[groupIndex].PendingPropertySetters();
    pendingPropertySetters.clear();
    pendingPropertySetters.shrink_to_fit();
}

void VisualStateManagerDataSource::ClearActiveStoryboards(_In_ int groupIndex)
{
    ClearActiveStoryboardsImpl(groupIndex);
    m_pGroupCollection->GetGroupContext()[groupIndex].ActiveStoryboards().clear();
}

void VisualStateManagerDataSource::RemoveActiveTransition(_In_ int groupIndex, CVisualTransition* transition)
{
    RemoveActiveTransitionImpl(groupIndex, transition);
    auto& activeTransitions = m_pGroupCollection->GetGroupContext()[groupIndex].ActiveTransitions();
    activeTransitions.erase(std::remove(activeTransitions.begin(),
        activeTransitions.end(), transition), activeTransitions.end());
}

void VisualStateManagerDataSource::AddActiveTransition(_In_ int groupIndex, xref_ptr<CVisualTransition> transition)
{
    AddActiveTransitionImpl(groupIndex, transition);
    m_pGroupCollection->GetGroupContext()[groupIndex].ActiveTransitions().push_back(transition.get());
}

void VisualStateManagerDataSource::ClearActiveTransitions(_In_ int groupIndex)
{
    ClearActiveTransitionsImpl(groupIndex);
    m_pGroupCollection->GetGroupContext()[groupIndex].ActiveTransitions().clear();
}

_Check_return_ HRESULT VisualStateManagerDataSource::TryGetOrCreateTransition(_In_ int fromIndex, _In_ int toIndex, _Outptr_ std::shared_ptr<CVisualTransition>* pTransition)
{
    IFC_RETURN(TryGetOrCreateTransitionImpl(fromIndex, toIndex, pTransition));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::TryGetOrCreateStoryboardForVisualState(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard)
{
    IFC_RETURN(TryGetOrCreateStoryboardForVisualStateImpl(index, pStoryboard));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::TryGetOrCreatePropertySettersForVisualState(_In_ int index, _Out_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection* pResolvedSetters)
{
    const bool shouldRefSetter = DirectUI::DXamlServices::ShouldStoreSourceInformation(); //XamlDiagnostics needs a strong reference to the original CSetter for source info purposes
    xref_ptr<CDependencyObject> targetPropertyOwner;
    const CDependencyProperty* targetProperty;
    CValue setterValue;
    std::vector<std::shared_ptr<CSetter>> setters;
    IFC_RETURN(TryGetOrCreatePropertySettersForVisualStateImpl(index, &setters));

    VisualStateSetterHelper::ResolvedVisualStateSetterCollection resolvedSetters;
    resolvedSetters.reserve(setters.size());

    for (auto& setter : setters)
    {
        IFC_RETURN(ParentDeferredSetter(xref_ptr<CSetter>(setter.get())));

        auto unparentGuard = wil::scope_exit([&]()
        {
            IFCFAILFAST(UnparentDeferredSetter(xref_ptr<CSetter>(setter.get())));
        });
        // TODO: Debugger check is (hopefully) a temporary measure that will be removed at a later date
        HRESULT hr = VisualStateSetterHelper::ResolveSetterAnimationTargets(
                setter.get(),
                targetPropertyOwner,
                &targetProperty,
                setterValue);
        if (IsDebuggerPresent())
        {
            IFC_RETURN(hr);
        }

        if (SUCCEEDED(hr))
        {
            if (shouldRefSetter)
            {
                resolvedSetters.emplace_back(xref::get_weakref(targetPropertyOwner), targetProperty, std::move(setterValue), xref_ptr<CSetter>(setter.get()));
            }
            else
            {
                resolvedSetters.emplace_back(xref::get_weakref(targetPropertyOwner), targetProperty, std::move(setterValue));
            }
        }
    }

    *pResolvedSetters = resolvedSetters;
    return S_OK;
}

void VisualStateManagerDataSource::IncrementPendingStoryboardCompletionCallbackCounter(_In_ int groupIndex)
{
    m_pGroupCollection->GetGroupContext()[groupIndex].IncrementPendingStoryboardCompletionCallbackCounter();
}

int VisualStateManagerDataSource::DecrementPendingStoryboardCompletionCallbackCounter(_In_ int groupIndex)
{
    return m_pGroupCollection->GetGroupContext()[groupIndex].DecrementPendingStoryboardCompletionCallbackCounter();
}

_Check_return_ HRESULT VisualStateManagerDataSource::BeginTransitionToState(_In_ int groupIndex, _In_ int stateIndex)
{
    m_pGroupCollection->GetGroupContext()[groupIndex].BeginTransitionToState(stateIndex);
    IFC_RETURN(OnBeginTransitionToState(groupIndex, stateIndex));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::CompleteTransitionToState(_In_ int groupIndex)
{
    m_pGroupCollection->GetGroupContext()[groupIndex].CompleteTransitionToState();
    IFC_RETURN(OnCompleteTransitionToState(groupIndex));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::TransitionToNullState(_In_ int groupIndex)
{
    IFC_RETURN(BeginTransitionToState(groupIndex, -1));
    IFC_RETURN(CompleteTransitionToState(groupIndex));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::OnBeginTransitionToState(_In_ int groupIndex, _In_ int stateIndex)
{
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::OnCompleteTransitionToState(_In_ int groupIndex)
{
    return S_OK;
}

_Success_(return) _Must_inspect_result_
bool VisualStateManagerDataSource::TryGetVisualStateImpl(_In_z_ const WCHAR* stateName,
    _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    *stateIndex = 0;
    *groupIndex = 0;

    auto runtimeData = m_pGroupCollection->GetCustomRuntimeData();

    unsigned int index = 0;
    bool found = runtimeData->TryGetVisualStateIndex(stateName, &index);

    if (found)
    {
        *groupIndex = static_cast<int>(runtimeData->GetGroupIndex(index));
        *stateIndex = static_cast<int>(index);
    }

    return found;
}

void VisualStateManagerDataSource::ClearActiveStoryboardsImpl(_In_ int groupIndex)
{
    auto& activeStoryboards = m_pGroupCollection->GetGroupContext()[groupIndex].ActiveStoryboards();
    if (m_pGroupCollection->m_pActiveStoryboards)
    {
        m_pGroupCollection->m_pActiveTransitions->remove_if([&activeStoryboards](CDependencyObject* obj) {
            return obj->OfTypeByIndex<KnownTypeIndex::Storyboard>() &&
                std::find_if(activeStoryboards.begin(), activeStoryboards.end(),
                    [obj](const std::pair<CStoryboard*, VisualStateGroupContext::StoryboardType>& item) {
                        return static_cast<CStoryboard*>(obj) == item.first;
                }) != activeStoryboards.end();
        });
    }
}

void VisualStateManagerDataSource::AddActiveStoryboardImpl(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type)
{
    if (!storyboard->GetParentInternal(false))
    {
        CValue cValueWrapper;
        IFCFAILFAST(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_ActiveStoryboards, &cValueWrapper));
        xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
        do_pointer_cast<CStoryboardCollection>(value.get())->push_back(
            static_sp_cast<CDependencyObject>(storyboard));
    }
}

void VisualStateManagerDataSource::RemoveActiveStoryboardImpl(_In_ int groupIndex, CStoryboard* storyboard)
{
    ASSERT(storyboard->GetParentInternal(false));
    if (storyboard->GetParentInternal(false)->OfTypeByIndex<KnownTypeIndex::StoryboardCollection>())
    {
        CValue cValueWrapper;
        IFCFAILFAST(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_ActiveStoryboards, &cValueWrapper));
        xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
        do_pointer_cast<CStoryboardCollection>(value.get())->remove(storyboard);
    }
}

std::unique_ptr<VisualStateManagerDataSource>
CreateVisualStateManagerDataSource(_In_ CVisualStateGroupCollection* pSourceCollection)
{
    if (pSourceCollection->IsOptimizedGroupCollection())
    {
        return std::unique_ptr<VisualStateManagerDataSource>(
            new OptimizedVisualStateManagerDataSource(pSourceCollection));
    }
    else
    {
        return std::unique_ptr<VisualStateManagerDataSource>(
            new StandardVisualStateManagerDataSource(pSourceCollection));
    }
}

_Check_return_ HRESULT VisualStateManagerDataSource::ParentDeferredStateTrigger(xref_ptr<CStateTriggerBase> stateTrigger)
{
    IFC_RETURN(ParentDeferredStateTriggerImpl(stateTrigger));
    return S_OK;
}

int VisualStateManagerDataSource::GetVisualStateCount()
{
    return GetVisualStateCountImpl();
}

_Check_return_ HRESULT VisualStateManagerDataSource::TryGetOrCreateStateTriggerVariantMap(int index, _Outptr_ std::shared_ptr<StateTriggerVariantMap>* pResult)
{
    IFC_RETURN(TryGetOrCreateStateTriggerVariantMapImpl(index, pResult));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::GetQualifiersFromStateTriggers(int index, OnQualifierCreatedCallback onQualifierCreatedCallback)
{
    IFC_RETURN(GetQualifiersFromStateTriggersImpl(index, onQualifierCreatedCallback));
    return S_OK;
}

std::vector<std::shared_ptr<StateTriggerVariantMap>> VisualStateManagerDataSource::GetStateTriggerVariantMaps()
{
    return GetStateTriggerVariantMapsImpl();
}

xref_ptr<CDependencyObject> VisualStateManagerDataSource::GetVisualState(VisualStateToken token)
{
    return GetVisualStateImpl(token);
}

xstring_ptr VisualStateManagerDataSource::GetVisualStateName(VisualStateToken token)
{
    return GetVisualStateNameImpl(token);
}

_Check_return_ HRESULT VisualStateManagerDataSource::ParentDeferredSetter(xref_ptr<CSetter> setter)
{
    IFC_RETURN(ParentDeferredSetterImpl(setter));
    return S_OK;
}

_Check_return_ HRESULT VisualStateManagerDataSource::UnparentDeferredSetter(xref_ptr<CSetter> setter)
{
    IFC_RETURN(UnparentDeferredSetterImpl(setter));
    return S_OK;
}

