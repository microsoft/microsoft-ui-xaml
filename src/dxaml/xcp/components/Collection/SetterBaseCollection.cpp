// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <CPropertyPath.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include "ICollectionChangeCallback.h"
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
#include <TargetPropertyPath.h>
#include <MultiParentShareableDependencyObject.h>
#include <Layouttransition.h>
#include <TransitionCollection.h>
#include <ItemsControl.h>
#include <SetterBaseCollection.h>
#include <SetterBase.h>
#include <Setter.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

using namespace RuntimeFeatureBehavior;

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CSetterBaseCollection object
//
//------------------------------------------------------------------------

CSetterBaseCollection::~CSetterBaseCollection()
{
    XUINT32 uiCount = GetCount();

    while(uiCount--)
    {
        CDependencyObject* pObject = static_cast<CDependencyObject*>(RemoveAt(uiCount));
        ReleaseInterface(pObject);
    }
}

//------------------------------------------------------------------------
//
//  Method:   Append
//
//  Synopsis:
//      Append a reference to an object to the collection.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CSetterBaseCollection::Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex)
{
    // Append will call Validate internally. This verifies
    // that our DO derives from at least CSetterBase before
    // we put it in the collection.
    IFC_RETURN(CDOCollection::Append(pObject, pnIndex));

    CSetterBase* pSetterBase = nullptr;
    IFC_RETURN(DoPointerCast(pSetterBase, pObject));

    bool shouldSealSetter = true;
    CSetter* pSetter = nullptr;
    if (SUCCEEDED(DoPointerCast(pSetter, pSetterBase)))
    {
        if (!pSetter->IsStyleSetter())
        {
            // Don't seal non-Style Setters so that they can get x:Bind updates
            shouldSealSetter = false;
        }
    }
    
    if (shouldSealSetter)
    {
        pSetterBase->SetIsSealed();
    }
    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ValidateItem
//
//  Synopsis:
//      Ensures that the property in the Setter is legitimate for the 
//      type specified by m_pTargetType.
// 
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSetterBaseCollection::ValidateItem(_In_ CDependencyObject *pObject)
{
    if (m_bIsSealed)
    {
        IFC_RETURN(E_FAIL);
    }
    
    IFC_RETURN(CDOCollection::ValidateItem(pObject));

    // First check if it is derived from CSetter
    CSetter *pSetter = nullptr;
    IFC_RETURN(DoPointerCast(pSetter, pObject));
    
    if (pSetter)
    {
        // If it's really derived from CSetter verify the properties.

        // Setter must be valid, unless we are inserting this setter into the collection while diagnostics is enabled
        IFCEXPECT_RETURN(m_allowInvalidSetter || IsSetterValid(pSetter));

        if (pSetter->m_pDependencyPropertyProxy && pSetter->GetTargetPropertyPath())
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_SETTER_AMBIGUOUS_TARGET));
        }
    }

    return S_OK;
}

bool CSetterBaseCollection::IsSetterValid(_In_ CSetter* setter) const
{
    const bool isValueValid = IsSetterValueValid(setter);
    // We expect either Setter.Property or Setter.Target to have been set
    const bool hasTargetOrProperty = (setter->m_target || setter->m_pDependencyPropertyProxy);
    return isValueValid && hasTargetOrProperty;
}

bool CSetterBaseCollection::IsSetterValueValid(_In_ CSetter* setter) const
{
    bool isValueValid = true;
        // Setter.Value can't be unset if it's for a Style, unless the Setter is mutable (i.e. has an x:Bind on it)
    if (setter->IsStyleSetter() && !setter->GetIsValueMutable())
    {
        const CDependencyProperty* pdp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Setter_Value);
        isValueValid = !setter->IsPropertyDefault(pdp);
    }
    return isValueValid;
}

// Only process 'Enter' for non-Style setters to support bindings
_Check_return_ HRESULT CSetterBaseCollection::ChildEnter(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params,
    bool fCanProcessEnterLeave
) 
{
    CSetterBase* pSetterBase = nullptr;
    IFC_RETURN(DoPointerCast(pSetterBase, pChild));
    
    CSetter* pSetter = nullptr;
    if (SUCCEEDED(DoPointerCast(pSetter, pSetterBase)))
    {
        if (pSetter->IsStyleSetter())
        {
            // Do not process ChildEnter/ChildLeave on style setters
            return S_OK;
        }
    }

    return CDOCollection::ChildEnter(pChild, pNamescopeOwner, params, fCanProcessEnterLeave);
}

// Only process 'Leave' for non-Style setters
_Check_return_ HRESULT CSetterBaseCollection::ChildLeave(
    _In_ CDependencyObject *pChild,
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params,
    bool fCanProcessEnterLeave
) 
{
    CSetterBase* pSetterBase = nullptr;
    IFC_RETURN(DoPointerCast(pSetterBase, pChild));

    CSetter* pSetter = nullptr;
    if (SUCCEEDED(DoPointerCast(pSetter, pSetterBase)))
    {
        if (pSetter->IsStyleSetter())
        {
            // Do not process ChildEnter/ChildLeave on style setters
            return S_OK;
        }
    }

    return CDOCollection::ChildLeave(pChild, pNamescopeOwner, params, fCanProcessEnterLeave);
};


//------------------------------------------------------------------------
//
//  Method:   GetSetterIndexByPropertyId
//
//  Synopsis:
//      Given a property id, it returns the index of the corresponding setter 
//      in the collection via the index parameter. It also sets the bFound 
//      parameter to indicate if a setter was found. If a setter was not found, 
//      the index should not be used by the caller.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CSetterBaseCollection::GetSetterIndexByPropertyId(
    _In_ KnownPropertyIndex uPropertyId,
    _In_ KnownTypeIndex targetTypeIndex,
    _Out_ XUINT32& uIndex, 
    _Out_ bool& bFound)
{
    KnownPropertyIndex uItrPropertyId = KnownPropertyIndex::UnknownType_UnknownProperty;

    uIndex = 0;
    bFound = false;

    for (XUINT32 uItrSetterIndex = 0; uItrSetterIndex < GetCount(); uItrSetterIndex++)
    {
        xref_ptr<CDependencyObject> pItem;
        pItem.attach(GetItemDOWithAddRef(uItrSetterIndex));
        CSetter *pSetter = do_pointer_cast<CSetter>(pItem.get());
        IFCPTR_RETURN(pSetter);

        uItrPropertyId = KnownPropertyIndex::UnknownType_UnknownProperty;
        IFC_RETURN(pSetter->GetProperty(targetTypeIndex, &uItrPropertyId));
        if (uItrPropertyId == uPropertyId)
        {
            uIndex = uItrSetterIndex;
            bFound = true;
            break;
        }
    }

    return S_OK;
}
