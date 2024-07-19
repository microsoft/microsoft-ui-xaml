// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "OptimizedVisualStateManagerDataSource.h"
#include "VisualStateSetterHelper.h"
#include <VisualStateGroupCollection.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>

#include <TimelineCollection.h>
#include <DynamicTimeline.h>
#include <VisualTransitionCollection.h>
#include <CDependencyObject.h>
#include <DOPointerCast.h>
#include <animation.h>
#include <VisualTransition.h>
#include <storyboard.h>
#include <Setter.h>
#include <StateTriggerBase.h>
#include <QualifierFactory.h>
#include <AdaptiveTrigger.h>
#include <StateTriggerCollection.h>
#include <SetterBaseCollection.h>
#include <ThemeResource.h>

OptimizedVisualStateManagerDataSource::OptimizedVisualStateManagerDataSource(CVisualStateGroupCollection* pGroupCollection)
    : m_objectCreator(
        NameScopeRegistrationMode::SkipRegistration,
        pGroupCollection->GetCustomRuntimeContext())
    , VisualStateManagerDataSource(pGroupCollection)
{
    // Resize the group context vector such that every group index maps to a context.
    size_t visualStateGroupCount = m_pGroupCollection->GetCustomRuntimeData()->GetVisualStateGroupCount();
    if (m_pGroupCollection->GetGroupContext().size() <
        visualStateGroupCount)
    {
        m_pGroupCollection->GetGroupContext().resize(visualStateGroupCount);
    }
}

_Success_(return) _Must_inspect_result_
bool OptimizedVisualStateManagerDataSource::TryGetVisualStateImpl(_In_z_ const WCHAR* stateName,
    _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    return VisualStateManagerDataSource::TryGetVisualStateImpl(stateName, stateIndex, groupIndex);
}

_Success_(return) _Must_inspect_result_
bool OptimizedVisualStateManagerDataSource::TryGetVisualStateImpl(_In_ VisualStateToken vsToken,
    _Out_ int* stateIndex, _Out_ int* groupIndex)
{
    *stateIndex = -1;
    *groupIndex = -1;

    auto vsCount = GetVisualStateCount();
    for(int vsIndex = 0; vsIndex < vsCount; ++vsIndex)
    {
        if(vsToken == VisualStateToken(vsIndex))
        {
            // Get the group index and VisualState index
            *groupIndex = m_pGroupCollection->GetCustomRuntimeData()->GetGroupIndex(vsIndex);
            *stateIndex = vsIndex;

            return true;
        }
    }

    return false;
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::TryGetOrCreateTransitionImpl(_In_ int fromIndex, _In_ int toIndex, _Out_ std::shared_ptr<CVisualTransition>* pTransition)
{
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    StreamOffsetToken transitionToken;

    if (customRuntimeData->TryGetVisualTransition(fromIndex, toIndex, &transitionToken))
    {
        std::shared_ptr<CDependencyObject> transition;
        xref_ptr<CThemeResource> unused;
        IFC_RETURN(m_objectCreator.CreateInstance(transitionToken, &transition, &unused));
        *pTransition = std::static_pointer_cast<CVisualTransition>(transition);
    }
    else
    {
        *pTransition = std::shared_ptr<CVisualTransition>();
    }
    return S_OK;
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::TryGetOrCreateStoryboardForVisualStateImpl(_In_ int index, _Out_ std::shared_ptr<CStoryboard>* pStoryboard)
{
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    if (customRuntimeData->HasStoryboard(index))
    {
        auto storyboardToken = customRuntimeData->GetStoryboard(index);
        std::shared_ptr<CDependencyObject> storyboard;
        xref_ptr<CThemeResource> unused;
        IFC_RETURN(m_objectCreator.CreateInstance(storyboardToken, &storyboard, &unused));
        *pStoryboard = std::static_pointer_cast<CStoryboard>(storyboard);
    }
    else
    {
        *pStoryboard = std::shared_ptr<CStoryboard>();
    }
    return S_OK;
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::TryGetOrCreatePropertySettersForVisualStateImpl(_In_ int index, _Out_ std::vector<std::shared_ptr<CSetter>>* pSetterVector)
{
    std::vector<std::shared_ptr<CSetter>> setterVector;
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    if (customRuntimeData->HasPropertySetters(index))
    {
        for (const auto& token : customRuntimeData->GetPropertySetterTokens(index))
        {
            std::shared_ptr<CDependencyObject> result;
            xref_ptr<CThemeResource> unused;
            IFC_RETURN(m_objectCreator.CreateInstance(token, &result, &unused));
            std::shared_ptr<CSetter> setter = std::static_pointer_cast<CSetter>(result);
            setterVector.push_back(setter);
        }
    }
    *pSetterVector = setterVector;
    return S_OK;
}

void OptimizedVisualStateManagerDataSource::ClearActiveStoryboardsImpl(_In_ int groupIndex)
{
    VisualStateManagerDataSource::ClearActiveStoryboardsImpl(groupIndex);
}

void OptimizedVisualStateManagerDataSource::AddActiveStoryboardImpl(_In_ int groupIndex, xref_ptr<CStoryboard> storyboard, VisualStateGroupContext::StoryboardType type)
{
    if (storyboard->GetParent())
    {
        // The only reason an optimized VSM DataSource's Storyboard will
        // already have a parent is if it belongs to a VisualTransition.
        ASSERT(static_cast<CDependencyObject*>(storyboard->GetParent())->OfTypeByIndex(
            KnownTypeIndex::VisualTransition));
        ASSERT(type == VisualStateGroupContext::StoryboardType::Transition);
    }
    VisualStateManagerDataSource::AddActiveStoryboardImpl(groupIndex, std::move(storyboard), type);
}

void OptimizedVisualStateManagerDataSource::RemoveActiveStoryboardImpl(_In_ int groupIndex, CStoryboard* storyboard)
{
    VisualStateManagerDataSource::RemoveActiveStoryboardImpl(groupIndex, storyboard);
}

void OptimizedVisualStateManagerDataSource::ClearActiveTransitionsImpl(_In_ int groupIndex)
{
    auto& activeTransitions = m_pGroupCollection->GetGroupContext()[groupIndex].ActiveTransitions();
    CValue cValueWrapper;
    IFCFAILFAST(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_ActiveTransitions, &cValueWrapper));

    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();

    do_pointer_cast<CVisualTransitionCollection>(value.get())->remove_if([&activeTransitions](CDependencyObject* obj) {
        return obj->OfTypeByIndex<KnownTypeIndex::VisualTransition>() &&
            std::find(activeTransitions.begin(), activeTransitions.end(),
                static_cast<CVisualTransition*>(obj)) != activeTransitions.end();
    });

}

void OptimizedVisualStateManagerDataSource::AddActiveTransitionImpl(_In_ int groupIndex, xref_ptr<CVisualTransition> transition)
{
    // No VisualTransition should be parented by default in the Optimized model because they are all
    // created on demand.
    ASSERT(!transition->GetParent());
    CValue cValueWrapper;
    IFCFAILFAST(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_ActiveTransitions, &cValueWrapper));

    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();

    do_pointer_cast<CVisualTransitionCollection>(value.get())->push_back(
        static_sp_cast<CDependencyObject>(transition));
}

void OptimizedVisualStateManagerDataSource::RemoveActiveTransitionImpl(_In_ int groupIndex, CVisualTransition* transition)
{
    CValue cValueWrapper;
    IFCFAILFAST(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_ActiveTransitions, &cValueWrapper));
    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
    do_pointer_cast<CVisualTransitionCollection>(value.get())->remove(transition);
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::TryGetOrCreateStateTriggerVariantMapImpl(int vsIndex, _Out_ std::shared_ptr<StateTriggerVariantMap>* pResult)
{
    auto groupIndex = m_pGroupCollection->GetCustomRuntimeData()->GetGroupIndex(vsIndex);
    auto& groupContext = m_pGroupCollection->GetGroupContext();

    std::shared_ptr<StateTriggerVariantMap> variantMap;
    if (groupContext[groupIndex].GetStateTriggerVariantMap() == nullptr)
    {
        variantMap = std::make_shared<StateTriggerVariantMap>();
        groupContext[groupIndex].GetStateTriggerVariantMap() = variantMap;
        m_pGroupCollection->GetStateTriggerVariantMaps().push_back(variantMap);
    }
    else
    {
        variantMap = groupContext[groupIndex].GetStateTriggerVariantMap();
    }

    *pResult = variantMap;
    return S_OK;
};

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::ParentDeferredStateTriggerImpl(xref_ptr<CStateTriggerBase> stateTrigger)
{
    ASSERT(stateTrigger);
    CValue cValueWrapper;
    IFC_RETURN(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_DeferredStateTriggers, &cValueWrapper));
    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
    IFC_RETURN(do_pointer_cast<CStateTriggerCollection>(value.get())->Append(stateTrigger.get()));
    return S_OK;
}

int OptimizedVisualStateManagerDataSource::GetVisualStateCountImpl()
{
    return static_cast<int>(m_pGroupCollection->GetCustomRuntimeData()->GetVisualStateCount());
}

// Creates qualifiers for deferred triggers
_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::GetQualifiersFromStateTriggersImpl(int index, OnQualifierCreatedCallback onCreated)
{
    ASSERT(onCreated);
    IFC_RETURN(GetQualifiersFromStateTriggerValues(index, onCreated));
    IFC_RETURN(GetQualifiersFromStateTriggerTokens(index, onCreated));
    GetQualifiersFromStaticResourceTriggerTokens(index, onCreated);
    return S_OK;
};

// Creates qualifiers for deferred AdaptiveTriggers from trigger values saved in XBF
_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::GetQualifiersFromStateTriggerValues(int index, OnQualifierCreatedCallback onCreated)
{
    ASSERT(onCreated);
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();
    if(!customRuntimeData->HasStateTriggers(index)) return S_OK;

    const auto stateTriggerValues = customRuntimeData->GetStateTriggers(index);
    for (auto& qualifier : stateTriggerValues)
    {
        auto pQualifier = QualifierFactory::Create(qualifier);
        ASSERT(pQualifier);
        IFC_RETURN(onCreated(VisualStateToken(index), pQualifier, nullptr));
    }
    return S_OK;
};

// Creates qualifiers for deferred extensible triggers from saved stream offset tokens
_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::GetQualifiersFromStateTriggerTokens(int index, OnQualifierCreatedCallback onCreated)
{
    ASSERT(onCreated);
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    const auto extensibleStateTriggerTokens = customRuntimeData->GetExtensibleStateTriggerTokens(index);
    for (auto& token : extensibleStateTriggerTokens)
    {
        std::shared_ptr<CDependencyObject> stateTriggerBase;
        xref_ptr<CThemeResource> unused;
        IFC_RETURN(m_objectCreator.CreateInstance(token, &stateTriggerBase, &unused));
        auto pStateTriggerBase = std::static_pointer_cast<CStateTriggerBase>(stateTriggerBase);
        if(!pStateTriggerBase) return S_OK;

        auto pQualifier = QualifierFactory::Create(&pStateTriggerBase->m_triggerState);
        ASSERT(pQualifier);
        IFC_RETURN(onCreated(VisualStateToken(index), pQualifier, xref_ptr<CStateTriggerBase>(pStateTriggerBase.get())));
        IFC_RETURN(ParentDeferredStateTrigger(xref_ptr<CStateTriggerBase>(pStateTriggerBase.get())));
    }
    return S_OK;
};

// Creates qualifiers for deferred triggers, declared as a static resource, from saved stream offset tokens
_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::GetQualifiersFromStaticResourceTriggerTokens(int index, OnQualifierCreatedCallback onCreated)
{
    ASSERT(onCreated);
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    const auto staticResourceTriggerTokens = customRuntimeData->GetStaticResourceTriggerTokens(index);
    for (auto& token : staticResourceTriggerTokens)
    {
        std::shared_ptr<CDependencyObject> triggerDO;
        IFC_RETURN(m_objectCreator.LookupStaticResourceValue(token, &triggerDO));
        auto trigger = std::static_pointer_cast<CStateTriggerBase>(triggerDO);
        if(!trigger) return S_OK;

        std::shared_ptr<IQualifier> pQualifier;
        if(trigger->GetTypeIndex() == KnownTypeIndex::AdaptiveTrigger)
        {
            pQualifier = QualifierFactory::Create(
                    static_cast<int>(std::static_pointer_cast<CAdaptiveTrigger>(trigger)->m_minWindowWidth),
                    static_cast<int>(std::static_pointer_cast<CAdaptiveTrigger>(trigger)->m_minWindowHeight));
        }
        else
        {
            pQualifier = QualifierFactory::Create(&trigger->m_triggerState);
        }

        ASSERT(pQualifier);
        IFC_RETURN(onCreated(VisualStateToken(index), pQualifier, xref_ptr<CStateTriggerBase>(trigger.get())));
        IFC_RETURN(ParentDeferredStateTrigger(xref_ptr<CStateTriggerBase>(trigger.get())));
    }
    return S_OK;
};

std::vector<std::shared_ptr<StateTriggerVariantMap>> OptimizedVisualStateManagerDataSource::GetStateTriggerVariantMapsImpl()
{
    return m_pGroupCollection->GetStateTriggerVariantMaps();
}

// Always return NULL because VisualStateGroups have not been faulted in and VisualStates have not been created.
xref_ptr<CDependencyObject> OptimizedVisualStateManagerDataSource::GetVisualStateImpl(VisualStateToken token)
{
    return nullptr;
}

xstring_ptr OptimizedVisualStateManagerDataSource::GetVisualStateNameImpl(VisualStateToken token)
{
    auto customRuntimeData = m_pGroupCollection->GetCustomRuntimeData();

    int index = -1;
    int group = -1;
    if(TryGetVisualStateImpl(token, &index, &group))
    {
        ASSERT(index != -1);
        ASSERT(group != -1);

        return customRuntimeData->GetVisualStateName(index);
    }

    return xstring_ptr();
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::ParentDeferredSetterImpl(xref_ptr<CSetter> setter)
{
    ASSERT(setter);
    CValue cValueWrapper;
    IFC_RETURN(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_DeferredSetters, &cValueWrapper));
    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
    do_pointer_cast<CSetterBaseCollection>(value.get())->push_back(setter);
    return S_OK;
}

_Check_return_ HRESULT OptimizedVisualStateManagerDataSource::UnparentDeferredSetterImpl(xref_ptr<CSetter> setter)
{
    ASSERT(setter);
    CValue cValueWrapper;
    IFC_RETURN(m_pGroupCollection->GetValueByIndex(KnownPropertyIndex::VisualStateGroupCollection_DeferredSetters, &cValueWrapper));
    xref_ptr<CDependencyObject> value = cValueWrapper.DetachObject();
    do_pointer_cast<CSetterBaseCollection>(value.get())->remove(setter);
    return S_OK;
}

