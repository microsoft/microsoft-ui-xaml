// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <AKCommon.h>
#include <AccessKey.h>
#include <UIAEnums.h> // Core Automation peer enums
#include "MUX-ETWEvents.h"
#include "AccessKeysEvents.h"
#include "KeyboardAutomationInvoker.h"

class CDependencyObject;
class CUIElement;
class CAutomationPeer;

struct IUIAInvokeProvider;
struct IUIAToggleProvider;
struct IUIASelectionItemProvider;
struct IUIAExpandCollapseProvider;

namespace AccessKeys {
    // Represents an object or entity in the AccessKeys Scope tree that has a valid attached AccessKey
    // property and can invoke actions on the object (e.g. through automation providers)
    template<   class CDObject = CDependencyObject,
                class _CAutomationPeer = CAutomationPeer,
                class UIAInvokeProvider = IUIAInvokeProvider,
                class UIAToggleProvider = IUIAToggleProvider,
                class UIASelectionItemProvider = IUIASelectionItemProvider,
                class UIAExpandCollapseProvider = IUIAExpandCollapseProvider>
    class AKOwner
    {
    public:
        // Construct an AKOwner with a valid weak reference to a CDependencyObject and a valid AKAccessKey (Should be made with the AKParser)
        AKOwner(_In_ CDObject* const element, _In_ const AKAccessKey& accessKey) :
            owningElement(xref::get_weakref(element)),
            accessKey(accessKey)
        { }

        AKOwner() = default;

        const AKAccessKey& GetAccessKey() const { return accessKey; };
        const xref::weakref_ptr<CDObject> GetElement() const { return owningElement; };

        bool Invoke() const
        {
            TraceAccessKeyScopeInvokeBegin();
            bool eventHandled = false;
            xref_ptr<CDObject> cDObject = owningElement.lock();
            if (cDObject != nullptr)
            {
                if (cDObject->IsActive())
                {
                    eventHandled = AccessKeys::AKOwnerEvents::InvokeEvent(cDObject.get());
                }

                if (!eventHandled)
                {
                    eventHandled = KeyboardAutomationInvoker::InvokeAutomationAction<CDObject, _CAutomationPeer, UIAInvokeProvider, UIAToggleProvider, UIASelectionItemProvider, UIAExpandCollapseProvider>(cDObject.get());
                }
            }
            TraceAccessKeyScopeInvokeEnd();
            return eventHandled;
        }

        _Check_return_ HRESULT
        ShowAccessKey(_In_ const wchar_t* const pressedKeys) const
        {
            xref_ptr<CDObject> cDObject = owningElement.lock();
            if (cDObject != nullptr && cDObject->IsActive())
            {
                AK_TRACE(L"AK> ShowAccessKey: %p '%s'\n", cDObject.get(), GetAccessKey().GetAccessKeyString());

                //Note: We can run into situations where we try to fire an element before it has been added to the tree. In these
                //scenarios, firing will fail because we have not added the request to the event manager for the event to be
                //fired. When the element enters the tree, another attempt will be made to fire the event successfully
                AccessKeys::AKOwnerEvents::RaiseAccessKeyShown(cDObject.get(), pressedKeys);
            }
            return S_OK;
        }

        _Check_return_ HRESULT
        HideAccessKey() const
        {
            xref_ptr<CDObject> cDObject = owningElement.lock();
            if (cDObject != nullptr)
            {
                AK_TRACE(L"AK> HideAccessKey: %p '%s'\n", cDObject.get(), GetAccessKey().GetAccessKeyString());

                //We do not check IsActive on AccessKeyHidden because an element may
                //have already been removed from the Visual Tree and we want
                //to remove the associated Keytip
                AccessKeys::AKOwnerEvents::RaiseAccessKeyHidden(cDObject.get());
            }
            return S_OK;
        }

        bool operator==(const AKOwner& rhs) const { return (owningElement == rhs.owningElement) && (accessKey == rhs.accessKey); };
        bool operator!=(const AKOwner& rhs) const { return !operator==(rhs); };

    private:
        AKAccessKey accessKey;  // Collection of characters parsed from the CDependencyObject owner's AutomationProperties.AccessKey field. 
        xref::weakref_ptr<CDObject> owningElement;  // Weak ref back to the CDependencyObject  owner of this access key is defined.
    };

    typedef AKOwner<> Owner;
}