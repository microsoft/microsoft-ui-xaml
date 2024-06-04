// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "CKeyboardAccelerator.h"
#include "CKeyboardAcceleratorCollection.h"

//  Add our events to the global event request list.
//  Need to do this ourselves since we don't derive from CUIElement.
_Check_return_ HRESULT CKeyboardAccelerator::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params)
{
    IFC_RETURN(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // If there are events registered on this element, ask the
        // EventManager to extract them and create a request for every event.
        auto core = GetContext();
        if (m_pEventList)
        {
            // Get the event manager.
            IFCEXPECT_ASSERT_RETURN(core);
            CEventManager* const pEventManager = core->GetEventManager();
            IFCEXPECT_ASSERT_RETURN(pEventManager);
            IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
        }
        CKeyboardAcceleratorCollection* const pCollection = do_pointer_cast<CKeyboardAcceleratorCollection>(this->GetParentInternal(false /* publicParentOnly */));
        IFCPTR_RETURN(pCollection);
        CDependencyObject * pParentElement = pCollection->GetParentInternal(false /* publicParentOnly */);

        // Do not set tooltip if
        // 1. Parent element has disabled the keyboard accelerator tooltip.
        // 2. current keyboard accelerator is disabled.

        DirectUI::KeyboardAcceleratorPlacementMode kaPlacementMode = DirectUI::KeyboardAcceleratorPlacementMode::Auto;
        CValue kaPlacementModeValue = {};
        CUIElement* element = do_pointer_cast<CUIElement>(pParentElement);
        IFCPTR_RETURN(element);

        IFC_RETURN(element->GetValueByIndex(KnownPropertyIndex::UIElement_KeyboardAcceleratorPlacementMode, &kaPlacementModeValue));
        kaPlacementMode = static_cast<DirectUI::KeyboardAcceleratorPlacementMode>(kaPlacementModeValue.AsEnum());

        if (kaPlacementMode == DirectUI::KeyboardAcceleratorPlacementMode::Hidden
            || this->m_isEnabled == false
            || this->m_key == static_cast<DirectUI::VirtualKey>(wsy::VirtualKey_None))
        {
            // Don't show a tooltip for an accelerator, no need to make the popup
            return S_OK;
        }

        // Create and set tooltip on parent control, only if this is the first tooltip enabled accelerator in the collection.
        for (CDependencyObject* const accelerator : *pCollection)
        {
            ASSERT(accelerator->OfTypeByIndex<KnownTypeIndex::KeyboardAccelerator>());

            if (static_cast<CKeyboardAccelerator*>(accelerator)->m_isEnabled)
            {
                if (accelerator == this)
                {
                    IFC_RETURN(FxCallbacks::KeyboardAccelerator_SetToolTip(this, pParentElement));
                }
                break;
            }
        }
    }

    return S_OK;
}


//  Clear our events from the global event request list.
//  Need to do this ourselves since we don't derive from CUIElement.
_Check_return_ HRESULT CKeyboardAccelerator::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params)
{
    IFC_RETURN(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // Reverse of the Add() operation in EnterImpl().
        if (m_pEventList)
        {
            auto core = GetContext();
            IFCEXPECT_ASSERT_RETURN(core);
            CEventManager* const pEventManager = core->GetEventManager();
            IFCEXPECT_ASSERT_RETURN(pEventManager);
            IFC_RETURN(pEventManager->RemoveRequests(this, m_pEventList));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CKeyboardAccelerator::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_     CREATEPARAMETERS   *pCreate
)
{
    *ppObject = new CKeyboardAccelerator(pCreate->m_pCore);
    return S_OK;
}

CKeyboardAccelerator::~CKeyboardAccelerator()
{
    // Clear event list
    if (m_pEventList)
    {
        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = nullptr;
    }
}

_Check_return_
HRESULT
CKeyboardAccelerator::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

_Check_return_
HRESULT
CKeyboardAccelerator::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}