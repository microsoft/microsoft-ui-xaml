// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include <CDependencyObject.h>
#include "UIElement.h"
#include "DOCollection.h"
#include "CKeyboardAccelerator.h"
#include "CKeyboardAcceleratorCollection.h"
#include <KeyboardAcceleratorUtility.h>
#include <AutomationPeer.h>
#include <KeyboardAutomationInvoker.h>
#include "FxCallbacks.h"
#include <dopointercast.h>
#include "CValueBoxer.h"
#include <MetadataAPI.h>
#include <FocusProperties.h>

using namespace DirectUI;
using namespace KeyboardAcceleratorUtility;

typedef bool (*KeyboardAcceleratorPolicyFn)(const CKeyboardAccelerator* const accelerator, const CDependencyObject* const owner);

// Through TryInvokeKeyboardAccelerator mechanism we will be traversing the whole subtree to hunt for the accelerator.
// As a result we may encounter matching accelerator but with the scope not valid for this instance.
// Below utility function will validate the scope of an accelerator before raising the event.
bool IsAcceleratorLocallyScoped(
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ const CKeyboardAccelerator* const accelerator)
{
    // If unable to retrive the focused element, consider the accelerator scope as valid scope.
    if (pFocusedElement == nullptr)
    {
        return false;
    }
    if (accelerator->m_scopeOwner == nullptr)
    {
        return false; // accelerator is global, hence okay to be invoked.
    }

    // Below code will unwrap the managed object pointer for scopeOwner and returns actual pointer to compate with Owner.
    CValue value;
    VERIFYHR(accelerator->GetValueByIndex(KnownPropertyIndex::KeyboardAccelerator_ScopeOwner, &value));
    CDependencyObject* pDO = nullptr;
    VERIFYHR(CValueBoxer::UnwrapWeakRef(&value, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::KeyboardAccelerator_ScopeOwner), &pDO));
    // If the scope owner for accelerator matches with starting UIElement, accelerator is okay to be invoked.
    if (pDO == pFocusedElement)
    {
        return false;
    }

    // Check if the scope owner is one of the UIElement in the path from focused element to the root.
    // If it is then the accelerator is not locally scoped and we are good to invoke it.
    CDependencyObject* parent = pFocusedElement->GetParentInternal(false /* publicParentOnly */);
    while (parent != nullptr)
    {
        if (parent == pDO)
        {
            return false;
        }
        parent = parent->GetParentInternal(false /* publicParentOnly */);
    }

    // As we are here, this means accelerator is locally scoped and we should not call it.
    return true;
}

bool RaiseKeyboardAcceleratorInvoked(
    _In_ CKeyboardAccelerator* pKAccelerator,
    _In_ CDependencyObject*  pKAcceleratorParentElement)
{
    BOOLEAN isHandled = FALSE;
    HRESULT hr = 0;

    VERIFYHR(hr = FxCallbacks::KeyboardAccelerator_RaiseKeyboardAcceleratorInvoked(pKAccelerator, pKAcceleratorParentElement, &isHandled));
    if (FAILED(hr))
    {
        return false;
    }

    // If not handled, invoke the default action on the parent UI Element. e.g. Click on Button.
    if (!isHandled)
    {
        isHandled = KeyboardAutomationInvoker::InvokeAutomationAction(pKAcceleratorParentElement);
    }

    return isHandled;
}

bool ProcessAllLiveAccelerators(
    _In_ const wsy::VirtualKey originalKey,
    _In_ const wsy::VirtualKeyModifiers keyModifiers,
    _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators,
    _In_ CDependencyObject* const owner,
    _In_ KeyboardAcceleratorPolicyFn policyFn,
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ bool isCallFromTryInvoke)
{
    Jupiter::stack_vector<KACollectionAndRefCountPair, 25> collectionsToGC;
    for (auto& weakCollection : allLiveAccelerators)
    {
        xref_ptr<CKeyboardAcceleratorCollection> strongRef = weakCollection.first.lock();
        CKeyboardAcceleratorCollection* collection = strongRef.get();
        if (collection == nullptr)
        {
            collectionsToGC.m_vector.emplace_back(weakCollection);
            continue;
        }

        CDependencyObject* parent = collection->GetParentInternal(false /* publicParentOnly */);
        while (parent != nullptr)
        {
            if (parent->IsActive())
            {
                break;
            }
            parent = parent->GetParentInternal(false /* publicParentOnly */);
        }

        for (CDependencyObject* const accelerator : *collection)
        {
            ASSERT(accelerator->OfTypeByIndex<KnownTypeIndex::KeyboardAccelerator>());

            if (policyFn(static_cast<CKeyboardAccelerator*>(accelerator), owner) &&
                ShouldRaiseAcceleratorEvent(originalKey, keyModifiers, static_cast<CKeyboardAccelerator*>(accelerator), parent))
            {
                // The parent of the collection is the element that the collection belongs to.
                CDependencyObject* const acceleratorParentElement = collection->GetParentInternal(false /* publicParentOnly */);

                const CUIElement* const acceleratorUIElement = do_pointer_cast<CUIElement>(acceleratorParentElement);
                // If the parent is disabled search for the next accelerator with enabled parent
                if (acceleratorUIElement && !acceleratorUIElement->IsEnabled())
                {
                    continue;
                }
                // Now  it's time to check if accelerator is locally scoped accelerator,
                // in which case it will be skipped.
                CKeyboardAccelerator* const pKAccelerator = static_cast<CKeyboardAccelerator*>(accelerator);
                if (isCallFromTryInvoke && IsAcceleratorLocallyScoped(pFocusedElement, pKAccelerator))
                {
                    continue; // check for another accelerator.
                }

                // We found an accelerator to try invoking - even if it wasn't handled, we don't need to look any further
                return RaiseKeyboardAcceleratorInvoked(pKAccelerator, acceleratorParentElement);
            }
        }
    }

    // Clean up zombie KA collections
    allLiveAccelerators.erase(
        std::remove_if(
            allLiveAccelerators.begin(),
            allLiveAccelerators.end(),
            [&collectionsToGC](KACollectionAndRefCountPair& weakCollection)
                {
                    auto it = std::find(collectionsToGC.m_vector.begin(), collectionsToGC.m_vector.end(), weakCollection);
                    return it != collectionsToGC.m_vector.end();
                }
            ),
        allLiveAccelerators.end()
        );

    return false;
}

_Check_return_ HRESULT KeyboardAcceleratorUtility::ProcessKeyboardAccelerators(
    _In_ const wsy::VirtualKey originalKey,
    _In_ const wsy::VirtualKeyModifiers keyModifiers,
    _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators,
    _In_ CDependencyObject* const pElement,
    _Out_ BOOLEAN* pHandled,
    _Out_ BOOLEAN* pHandledShouldNotImpedeTextInput,
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ bool isCallFromTryInvoke)
{
    // The order of things should be as follows:
    // 1) Try to process local accelerators defined on this element
    // 2) Try to process accelerators that are 'owned' by this element (KeyboardAccelerator.ScopeOwner==this)
    // 3) Give the element a chance to handle its own accelerators or forward on to attached ui (like a flyout) by calling the
    //    OnProcessKeyboardAccelerators protected virtual.
    // 4) Raise the public ProcessKeyboardAccelerators event
    //
    // If any of these gets handled, we don't need to do the rest - we just mark this as handled and return.  The caller should
    // copy the handled flag into the KeyRoutedEventArgs' handled property.

    *pHandled = FALSE;
    *pHandledShouldNotImpedeTextInput = FALSE;

    if (ProcessLocalAccelerators(originalKey, keyModifiers, pElement, pFocusedElement, isCallFromTryInvoke) ||
        ProcessOwnedAccelerators(originalKey, keyModifiers, allLiveAccelerators, pElement, pFocusedElement, isCallFromTryInvoke))
    {
        *pHandled = TRUE;
    }
    else
    {
        CUIElement *pElementAsUIE = do_pointer_cast<CUIElement>(pElement);
        if (pElementAsUIE)
        {
            // Here, we try calling the OnProcessKeyboardAccelerators protected virtual,
            // after which we raise the public ProcessKeyboardAccelerators event
            IFC_RETURN(FxCallbacks::UIElement_RaiseProcessKeyboardAccelerators(
                pElementAsUIE,
                originalKey,
                keyModifiers,
                pHandled,
                pHandledShouldNotImpedeTextInput));
        }
    }
    return S_OK;
}

bool KeyboardAcceleratorUtility::ProcessLocalAccelerators(
    _In_ wsy::VirtualKey originalKey,
    _In_ wsy::VirtualKeyModifiers keyModifiers,
    _In_ CDependencyObject* const pElement,
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ bool isCallFromTryInvoke)
{
    // If the element is disabled, none of its accelerators are considered invocable anyway, so we can bail out early in that case
    const CUIElement* const pUIElement = do_pointer_cast<CUIElement>(pElement);
    if (pUIElement && !pUIElement->IsEnabled())
    {
        return false;
    }

    if (false == pElement->IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_KeyboardAccelerators))
    {
        return false;
    }

    CValue acceleratorCollectionValue;
    HRESULT hr = 0;
    VERIFYHR(hr = pElement->GetValueByIndex(KnownPropertyIndex::UIElement_KeyboardAccelerators, &acceleratorCollectionValue));
    if (FAILED(hr))
    {
        return false;
    }

    xref_ptr<CDependencyObject> value = acceleratorCollectionValue.DetachObject();
    if (value == nullptr)
    {
        return false;
    }

    CKeyboardAcceleratorCollection* const pCollection = do_pointer_cast<CKeyboardAcceleratorCollection>(value.get());
    CDependencyObject* collectionParent = pCollection->GetParentInternal(false /*public parent only*/);
    for (CDependencyObject* const accelerator : *pCollection)
    {
        if (ShouldRaiseAcceleratorEvent(originalKey, keyModifiers, static_cast<CKeyboardAccelerator*>(accelerator), collectionParent))
        {
            // Now  it's time to check if accelerator is not locally scoped accelerator.
            // in which case it will be skipped.
            if (isCallFromTryInvoke && IsAcceleratorLocallyScoped(pFocusedElement, static_cast<CKeyboardAccelerator*>(accelerator)))
            {
                continue; // check for another accelerator.
            }

            // We found an accelerator to try invoking - even if it wasn't handled, we don't need to look any further
            return RaiseKeyboardAcceleratorInvoked(static_cast<CKeyboardAccelerator*>(accelerator), collectionParent);
        }
    }

    return false;
}

bool AcceleratorIsOwnedPolicy(
    _In_ const CKeyboardAccelerator* const accelerator,
    _In_ const CDependencyObject* const owner)
{
    // no need to proceed if accelerator we are processing is global.
    if (accelerator->m_scopeOwner == nullptr)
    {
        return false;
    }
    //Below code will unwrap the managed object pointer for scopeOwner and returns actual pointer to compate with Owner.
    CValue value;
    VERIFYHR(accelerator->GetValueByIndex(KnownPropertyIndex::KeyboardAccelerator_ScopeOwner, &value));

    CDependencyObject* pDO = nullptr;
    VERIFYHR(CValueBoxer::UnwrapWeakRef(&value, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::KeyboardAccelerator_ScopeOwner), &pDO));

    return pDO == owner;
}

bool KeyboardAcceleratorUtility::ProcessOwnedAccelerators(
    _In_ wsy::VirtualKey originalKey,
    _In_ wsy::VirtualKeyModifiers keyModifiers,
    _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators,
    _In_ CDependencyObject* const pElement,
    _In_ const CDependencyObject* const pFocusedElement,
    _In_ bool isCallFromTryInvoke)
{
    return ProcessAllLiveAccelerators(originalKey, keyModifiers, allLiveAccelerators, pElement, AcceleratorIsOwnedPolicy, pFocusedElement, isCallFromTryInvoke);
}

bool AcceleratorIsGlobalPolicy(
    _In_ const CKeyboardAccelerator* const accelerator,
    _In_ const CDependencyObject* const owner)
{
    return accelerator->m_scopeOwner == nullptr;
}

bool KeyboardAcceleratorUtility::ProcessGlobalAccelerators(
    _In_ wsy::VirtualKey originalKey,
    _In_ wsy::VirtualKeyModifiers keyModifiers,
    _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators)
{
    return ProcessAllLiveAccelerators(originalKey, keyModifiers, allLiveAccelerators, nullptr, AcceleratorIsGlobalPolicy, nullptr, false);
}

bool KeyboardAcceleratorUtility::IsKeyValidForAccelerators(_In_ const wsy::VirtualKey originalKey, _In_ const XUINT32 modifierKeys)
{
    const bool isAlphaNumeric = (wsy::VirtualKey_Number0 <= originalKey) && (originalKey <= wsy::VirtualKey_Z);
    const bool isNumpadKey = (wsy::VirtualKey_NumberPad0 <= originalKey) && (originalKey <= wsy::VirtualKey_Divide);
    const bool isFunctionKey = (wsy::VirtualKey_F1 <= originalKey) && (originalKey <= wsy::VirtualKey_F24);

    //The following VK Codes are in winuser.w but not in Windows.System.VirtualKey
    //#define VK_BROWSER_FAVORITES   0xAB
    //#define VK_BROWSER_HOME        0xAC
    //#define VK_VOLUME_MUTE         0xAD
    //#define VK_VOLUME_DOWN         0xAE
    //#define VK_VOLUME_UP           0xAF
    //#define VK_MEDIA_NEXT_TRACK    0xB0
    //#define VK_MEDIA_PREV_TRACK    0xB1
    //#define VK_MEDIA_STOP          0xB2
    //#define VK_MEDIA_PLAY_PAUSE    0xB3
    //#define VK_LAUNCH_MAIL         0xB4
    //#define VK_LAUNCH_MEDIA_SELECT 0xB5
    //#define VK_LAUNCH_APP1         0xB6
    //#define VK_LAUNCH_APP2         0xB7
    //#define VK_APPCOMMAND_LAST     0xB7
    //#define VK_OEM_1          0xBA   // ';:' for US
    //#define VK_OEM_PLUS       0xBB   // '+' any country/region
    //#define VK_OEM_COMMA      0xBC   // ',' any country/region
    //#define VK_OEM_MINUS      0xBD   // '-' any country/region
    //#define VK_OEM_PERIOD     0xBE   // '.' any country/region
    //#define VK_OEM_2          0xBF   // '/?' for US
    //#define VK_OEM_3          0xC0   // '`~' for US

    // We only want the 'symbol' keys from the extended VK set to be valid keyboard accelerators.
    const bool isVKOEMSymbolKey = VK_OEM_1 <= originalKey && originalKey < VK_OEM_3;

    //Valid non symbol / aplha-numeric accesskeys are Enter, Esc, Backspace, Space, PageUp, PageDown, End, Home, Left, Up, Right,
    //Down, Snapshot, Insert, and Delete
    const bool isValidNonSymbolAccessKey = (originalKey == wsy::VirtualKey_Enter)
                        || (originalKey == wsy::VirtualKey_Escape)
                        || (originalKey == wsy::VirtualKey_Back)
                        || ((modifierKeys & KEY_MODIFIER_CTRL) && (originalKey == wsy::VirtualKey_Tab))
                        || ((wsy::VirtualKey_Space <= originalKey) && (originalKey <= wsy::VirtualKey_Down))
                        || ((originalKey >= wsy::VirtualKey_Snapshot) && (originalKey <= wsy::VirtualKey_Delete));

    return isAlphaNumeric || isNumpadKey || isFunctionKey || isVKOEMSymbolKey || isValidNonSymbolAccessKey;
}

bool KeyboardAcceleratorUtility::TextInputHasPriorityForKey(_In_ const wsy::VirtualKey originalKey, bool isCtrlPressed, bool isAltPressed)
{
    if (isCtrlPressed || isAltPressed)
    {
        return false;
    }

    if ((wsy::VirtualKey_F1 <= originalKey) && (originalKey <= wsy::VirtualKey_F24))
    {
        return false;
    }

    if (originalKey == wsy::VirtualKey_Escape || originalKey == wsy::VirtualKey_Snapshot)
    {
        return false;
    }

    return true;
}

bool KeyboardAcceleratorUtility::ShouldRaiseAcceleratorEvent(
    _In_ wsy::VirtualKey key,
    _In_ wsy::VirtualKeyModifiers keyModifiers,
    _In_ const CKeyboardAccelerator* const pAccelerator,
    _In_ CDependencyObject* const pParent)
{
    return pAccelerator->m_isEnabled &&
        key == static_cast<wsy::VirtualKey>(pAccelerator->m_key) &&
        keyModifiers == static_cast<wsy::VirtualKeyModifiers>(pAccelerator->m_keyModifiers) &&
        FocusProperties::IsVisible(pParent) &&
        FocusProperties::AreAllAncestorsVisible(pParent);
}

// enum class wsy::VirtualKeyModifiers defined in windows.system.input.h and integer modifiers defined in paltypes.h are different in values
// for Control and Alt. They are swapped as shown below.
//                                                      VS
// VirtualKeyModifiers_None     = 0                     |
// VirtualKeyModifiers_Control  = 0x1                   |           #define KEY_MODIFIER_ALT            0x0001
// VirtualKeyModifiers_Menu     = 0x2                   |           #define KEY_MODIFIER_CTRL           0x0002
// VirtualKeyModifiers_Shift    = 0x4                   |           #define KEY_MODIFIER_SHIFT          0x0004
// VirtualKeyModifiers_Windows  = 0x8                   |           #define KEY_MODIFIER_WINDOWS        0x0008
//
// This function maps VirtualKeyModifiers_Control and VirtualKeyModifiers_Menu to corresponding integer values
const XUINT32 KeyboardAcceleratorUtility::MapVirtualKeyModifiersToIntegersModifiers( _In_ const wsy::VirtualKeyModifiers virtualKeyModifiers )
{
    XUINT32 keyModifiers = 0;
    if(virtualKeyModifiers & wsy::VirtualKeyModifiers_Control)
    {
        keyModifiers |= KEY_MODIFIER_CTRL;
    }
    if(virtualKeyModifiers & wsy::VirtualKeyModifiers_Menu)
    {
        keyModifiers = KEY_MODIFIER_ALT;
    }
    if (virtualKeyModifiers & wsy::VirtualKeyModifiers_Shift)
    {
        keyModifiers |= KEY_MODIFIER_SHIFT;
    }
    if (virtualKeyModifiers & wsy::VirtualKeyModifiers_Windows)
    {
        keyModifiers |= KEY_MODIFIER_WINDOWS;
    }

    return keyModifiers;
}
