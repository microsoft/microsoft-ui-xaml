// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <VisualStateManagerDataSource.h>
#include <VisualStateGroupContext.h>
#include <CustomWriterRuntimeObjectCreator.h>

class VisualStateGroupContext;
class CStoryboard;
class CVisualTransition;
class CVisualStateGroupCollection;

class OptimizedVisualStateManagerDataSource
    : public VisualStateManagerDataSource
{
public:
    OptimizedVisualStateManagerDataSource(CVisualStateGroupCollection* pGroupCollection);

protected:
    _Success_(return) _Must_inspect_result_
    bool TryGetVisualStateImpl(_In_z_ const WCHAR* stateName, 
        _Out_ int* stateIndex, _Out_ int* groupIndex) override;

    _Success_(return) _Must_inspect_result_
    bool TryGetVisualStateImpl(_In_ VisualStateToken vsToken, 
        _Out_ int* stateIndex, _Out_ int* groupIndex) override;

    _Check_return_ HRESULT TryGetOrCreateTransitionImpl(_In_ int fromIndex, _In_ int toIndex, _Out_ std::shared_ptr<CVisualTransition>* pTransition) override;
    _Check_return_ HRESULT TryGetOrCreateStoryboardForVisualStateImpl(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard) override;
    _Check_return_ HRESULT TryGetOrCreatePropertySettersForVisualStateImpl(_In_ int index, _Out_ std::vector<std::shared_ptr<CSetter>>* pSetterVector) override;

    void ClearActiveStoryboardsImpl(_In_ int groupIndex) override;
    void RemoveActiveStoryboardImpl(_In_ int groupIndex, CStoryboard* storyboard) override;
    void AddActiveStoryboardImpl(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type) override;

    void ClearActiveTransitionsImpl(_In_ int groupIndex) override;
    void RemoveActiveTransitionImpl(_In_ int groupIndex, CVisualTransition* transition) override;
    void AddActiveTransitionImpl(_In_ int groupIndex, xref_ptr<CVisualTransition> transition) override;

    _Check_return_ HRESULT ParentDeferredStateTriggerImpl(xref_ptr<CStateTriggerBase> stateTrigger) override;
    int GetVisualStateCountImpl() override;
    _Check_return_ HRESULT TryGetOrCreateStateTriggerVariantMapImpl(int index, _Out_ std::shared_ptr<StateTriggerVariantMap>* pResult) override;
    _Check_return_ HRESULT GetQualifiersFromStateTriggersImpl(int index, OnQualifierCreatedCallback) override;
    std::vector<std::shared_ptr<StateTriggerVariantMap>> GetStateTriggerVariantMapsImpl() override; 
    xref_ptr<CDependencyObject> GetVisualStateImpl(VisualStateToken token) override; 
    xstring_ptr GetVisualStateNameImpl(VisualStateToken token) override; 
    _Check_return_ HRESULT ParentDeferredSetterImpl(xref_ptr<CSetter> setter) override;
    _Check_return_ HRESULT UnparentDeferredSetterImpl(xref_ptr<CSetter> setter) override;

private:
    CustomWriterRuntimeObjectCreator m_objectCreator;
    _Check_return_ HRESULT GetQualifiersFromStateTriggerTokens(int index, OnQualifierCreatedCallback onQualifierCreated);
    _Check_return_ HRESULT GetQualifiersFromStateTriggerValues(int index, OnQualifierCreatedCallback onQualifierCreated);
    _Check_return_ HRESULT GetQualifiersFromStaticResourceTriggerTokens(int index, OnQualifierCreatedCallback onCreated);
};
