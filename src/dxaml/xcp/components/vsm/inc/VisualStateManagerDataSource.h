// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xvector.h>

#include "VisualStateGroupContext.h"
#include <ccontrol.h>
#include "VisualStateToken.h"

class CSetter;
class CStoryboard;
class CVisualTransition;
class CVisualStateGroupCollection;

class VisualStateManagerDataSource
{
public:
    _Success_(return) _Must_inspect_result_
    bool TryGetVisualState(_In_z_ const WCHAR* stateName,
        _Out_ int* stateIndex, _Out_ int* groupIndex);

    bool TryGetVisualState(_In_ const VisualStateToken vsToken,
        _Out_ int* stateIndex, _Out_ int* groupIndex);

    int GetGroupCount() const;
    VisualStateGroupContext& GetGroupContext(_In_ int groupIndex);

    _Check_return_ HRESULT TryGetOrCreateTransition(_In_ int fromIndex, _In_ int toIndex, _Out_ std::shared_ptr<CVisualTransition>* pTransition);
    _Check_return_ HRESULT TryGetOrCreateStoryboardForVisualState(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard);
    _Check_return_ HRESULT TryGetOrCreatePropertySettersForVisualState(_In_ int index, _Out_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection* pResolvedSetters);

    const std::vector<std::pair<CStoryboard*, VisualStateGroupContext::StoryboardType>>& GetActiveStoryboards(_In_ int groupIndex) const;
    void ClearActiveStoryboards(_In_ int groupIndex);
    void RemoveActiveStoryboard(_In_ int groupIndex, CStoryboard* storyboard);
    void AddActiveStoryboard(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type);

    _Check_return_ HRESULT SetAndApplyActivePropertySetters(_In_ int groupIndex, _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters, _In_ const bool forceDeferOperation = false);
    _Check_return_ HRESULT UnapplyPropertySetters(_In_ int groupIndex, _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters, _In_ const bool forceDeferOperation = false);
    const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& GetActivePropertySetters(_In_ int groupIndex) const;
    const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& GetPendingPropertySetters(_In_ int groupIndex) const;
    void AddPendingPropertySetters(_In_ int groupIndex, _In_ VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters);
    void ClearPendingPropertySetters(_In_ int groupIndex);

    const std::vector<CVisualTransition*>& GetActiveTransitions(_In_ int groupIndex = -1) const;
    void ClearActiveTransitions(_In_ int groupIndex);
    void RemoveActiveTransition(_In_ int groupIndex, CVisualTransition* transition);
    void AddActiveTransition(_In_ int groupIndex, xref_ptr<CVisualTransition> transition);

    void SetPendingStoryboard(_In_ int groupIndex, _In_opt_ CStoryboard* storyboard);
    _Ret_maybenull_ CStoryboard* GetPendingStoryboard(_In_ int groupIndex);

    void IncrementPendingStoryboardCompletionCallbackCounter(_In_ int groupIndex);
    int DecrementPendingStoryboardCompletionCallbackCounter(_In_ int groupIndex);

    _Check_return_ HRESULT BeginTransitionToState(_In_ int groupIndex, _In_ int stateIndex);
    _Check_return_ HRESULT CompleteTransitionToState(_In_ int groupIndex);

    _Check_return_ HRESULT TransitionToNullState(_In_ int groupIndex);

    bool IsAnimationEnabled() const;

    virtual ~VisualStateManagerDataSource() {};

    using OnQualifierCreatedCallback = std::function<HRESULT(VisualStateToken token, std::shared_ptr<IQualifier>, xref_ptr<CStateTriggerBase>)>;

    _Check_return_ HRESULT ParentDeferredStateTrigger(xref_ptr<CStateTriggerBase> stateTrigger);
    int GetVisualStateCount();
    _Check_return_ HRESULT TryGetOrCreateStateTriggerVariantMap(int index, _Out_ std::shared_ptr<StateTriggerVariantMap>* pResult);
    _Check_return_ HRESULT GetQualifiersFromStateTriggers(int index, OnQualifierCreatedCallback);
    std::vector<std::shared_ptr<StateTriggerVariantMap>> GetStateTriggerVariantMaps();
    xref_ptr<CDependencyObject> GetVisualState(VisualStateToken token);
    xstring_ptr GetVisualStateName(VisualStateToken token);

protected:
    VisualStateManagerDataSource(CVisualStateGroupCollection* pGroupCollection);

    _Check_return_ HRESULT ParentDeferredSetter(xref_ptr<CSetter> setter);
    _Check_return_ HRESULT UnparentDeferredSetter(xref_ptr<CSetter> setter);
    _Success_(return) _Must_inspect_result_
    virtual bool TryGetVisualStateImpl(_In_z_ const WCHAR* stateName, 
        _Out_ int* stateIndex, _Out_ int* groupIndex) = 0;
    virtual bool TryGetVisualStateImpl(_In_ VisualStateToken vsToken, 
        _Out_ int* stateIndex, _Out_ int* groupIndex) = 0;
    virtual void ClearActiveStoryboardsImpl(_In_ int groupIndex) = 0;
    virtual void RemoveActiveStoryboardImpl(_In_ int groupIndex, CStoryboard* storyboard) = 0;
    virtual void AddActiveStoryboardImpl(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type) = 0;
    virtual void ClearActiveTransitionsImpl(_In_ int groupIndex) = 0;
    virtual void RemoveActiveTransitionImpl(_In_ int groupIndex, CVisualTransition* transition) = 0;
    virtual void AddActiveTransitionImpl(_In_ int groupIndex, xref_ptr<CVisualTransition> transition) = 0;
    virtual _Check_return_ HRESULT TryGetOrCreateTransitionImpl(_In_ int fromIndex, _In_ int toIndex, _Out_ std::shared_ptr<CVisualTransition>* pTransition) = 0;
    virtual _Check_return_ HRESULT TryGetOrCreateStoryboardForVisualStateImpl(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard) = 0;
    virtual _Check_return_ HRESULT TryGetOrCreatePropertySettersForVisualStateImpl(_In_ int index, _Out_ std::vector<std::shared_ptr<CSetter>>* pSetterVector) = 0;

    virtual _Check_return_ HRESULT OnBeginTransitionToState(_In_ int groupIndex, _In_ int stateIndex);
    virtual  _Check_return_ HRESULT OnCompleteTransitionToState(_In_ int groupIndex);

    virtual _Check_return_ HRESULT ParentDeferredStateTriggerImpl(xref_ptr<CStateTriggerBase> stateTrigger) = 0;
    virtual int GetVisualStateCountImpl() = 0;
    virtual _Check_return_ HRESULT TryGetOrCreateStateTriggerVariantMapImpl(int index, _Out_ std::shared_ptr<StateTriggerVariantMap>* pResult) = 0;
    virtual _Check_return_ HRESULT GetQualifiersFromStateTriggersImpl(int index, OnQualifierCreatedCallback) = 0;
    virtual std::vector<std::shared_ptr<StateTriggerVariantMap>> GetStateTriggerVariantMapsImpl() = 0;
    virtual xref_ptr<CDependencyObject> GetVisualStateImpl(VisualStateToken token) = 0;
    virtual xstring_ptr GetVisualStateNameImpl(VisualStateToken token) = 0;
    virtual _Check_return_ HRESULT  ParentDeferredSetterImpl(xref_ptr<CSetter> setter) = 0;
    virtual _Check_return_ HRESULT UnparentDeferredSetterImpl(xref_ptr<CSetter> setter) = 0;

    CVisualStateGroupCollection* m_pGroupCollection = nullptr;

private:
    _Check_return_ HRESULT ApplyPropertySetters(_In_ int groupIndex, _In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& propertySetters, _In_ const bool forceDeferOperation = false);
};

std::unique_ptr<VisualStateManagerDataSource>
CreateVisualStateManagerDataSource(_In_ CVisualStateGroupCollection* pSourceCollection);
