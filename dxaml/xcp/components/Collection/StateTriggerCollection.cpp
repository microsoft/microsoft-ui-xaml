// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <CPropertyPath.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include <CValue.h>
#include <UIElement.h>
#include <double.h>
#include <Point.h>
#include <dopointercast.h>
#include <corep.h>
#include "ICollectionChangeCallback.h"
#include <NoParentShareableDependencyObject.h>
#include <Panel.h>
#include <Template.h>
#include <XAMLItemCollection.h>
#include <CControl.h>
#include <ItemsPresenter.h>
#include <MultiParentShareableDependencyObject.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <ItemsControl.h>
#include <VariantMap.h>
#include <StateTriggerBase.h>
#include <StateTriggerCollection.h>
#include <QualifierFactory.h>
#include <xref_ptr.h>
#include <VisualState.h>
#include <AdaptiveTrigger.h>


_Check_return_ HRESULT CStateTriggerCollection::Insert(XUINT32 index, _In_ CDependencyObject* object)
{
    IFC_RETURN(CDOCollection::Insert(index, object));
    IFC_RETURN(AddTriggerToVariantMap(object));

    return S_OK;
}

_Check_return_ HRESULT CStateTriggerCollection::Append(_In_ CDependencyObject* object, _Out_opt_ unsigned int* index)
{
    IFC_RETURN(CDOCollection::Append(object, index));
    IFC_RETURN(AddTriggerToVariantMap(object));

    return S_OK;
}

_Check_return_ HRESULT CStateTriggerCollection::OnClear()
{
    IFC_RETURN(CDOCollection::OnClear());

    CDependencyObject* pVisualStateDO = GetParentInternal(false);
    CVisualState* pVisualState = do_pointer_cast<CVisualState>(pVisualStateDO); 
    if(pVisualState && m_pVariantMap)
    {
        IFC_RETURN(m_pVariantMap->Clear(pVisualState->GetVisualStateToken()));
    }

    return S_OK;
}

_Check_return_ void* CStateTriggerCollection::RemoveAt(XUINT32 nIndex)
{
    CDependencyObject* pDO = static_cast<CDependencyObject*>(CDOCollection::RemoveAt(nIndex));
    if(pDO)
    {
        RemoveTriggerFromVariantMap(pDO);
    }

    return pDO;
}

_Check_return_ HRESULT CStateTriggerCollection::AddTriggerToVariantMap(_In_ CDependencyObject* trigger)
{
    CDependencyObject* pVisualStateDO = GetParentInternal(false);
    CVisualState* pVisualState = do_pointer_cast<CVisualState>(pVisualStateDO); 

    if(pVisualState && m_pVariantMap)
    {
        auto& owningVariantMaps = do_pointer_cast<CStateTriggerBase>(trigger)->m_owningVariantMaps;
        auto comparator = [&](std::weak_ptr<StateTriggerVariantMap>& i)
        {
            auto variantMap = i.lock();
            return (variantMap == m_pVariantMap);
        };

        auto iterVariantMapItem = std::find_if(owningVariantMaps.begin(), owningVariantMaps.end(), comparator);
        if(iterVariantMapItem == owningVariantMaps.end()) owningVariantMaps.push_back(m_pVariantMap);

        if(auto pQualifier = CreateQualifierFromTrigger(trigger))
        {
            IFC_RETURN(m_pVariantMap->Add(pVisualState->GetVisualStateToken(), pQualifier, xref_ptr<CDependencyObject>(trigger)));
            IFC_RETURN(m_pVariantMap->Evaluate());
        }
        else
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CStateTriggerCollection::RemoveTriggerFromVariantMap(_In_ CDependencyObject* trigger)
{
    CDependencyObject* pVisualStateDO = GetParentInternal(false);
    CVisualState* pVisualState = do_pointer_cast<CVisualState>(pVisualStateDO); 

    if(pVisualState && m_pVariantMap)
    {
        auto& owningVariantMaps = do_pointer_cast<CStateTriggerBase>(trigger)->m_owningVariantMaps;
        auto comparator = [&](std::weak_ptr<StateTriggerVariantMap>& i)
        {
            auto variantMap = i.lock();
            return (variantMap == m_pVariantMap);
        };

        owningVariantMaps.erase(std::remove_if(owningVariantMaps.begin(), owningVariantMaps.end(), comparator), owningVariantMaps.end());
        
        if(auto pQualifier = CreateQualifierFromTrigger(trigger))
        {
            IFC_RETURN(m_pVariantMap->Remove(pVisualState->GetVisualStateToken(), pQualifier));
            IFC_RETURN(m_pVariantMap->Evaluate());
        }
        else
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

std::shared_ptr<IQualifier> CStateTriggerCollection::CreateQualifierFromTrigger(_In_ CDependencyObject* trigger)
{
    CAdaptiveTrigger* adaptiveTrigger = do_pointer_cast<CAdaptiveTrigger>(trigger);
    std::shared_ptr<IQualifier> pQualifier;

    if(adaptiveTrigger)
    {
        pQualifier = QualifierFactory::Create(static_cast<int>(adaptiveTrigger->m_minWindowWidth), static_cast<int>(adaptiveTrigger->m_minWindowHeight));
    }
    else 
    {
        CStateTriggerBase* stateTriggerBase = do_pointer_cast<CStateTriggerBase>(trigger);
        if(stateTriggerBase)
        {
            pQualifier = QualifierFactory::Create(&stateTriggerBase->m_triggerState);
        }
    }

    return pQualifier;
}
