// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <functional>
#include <wrl\module.h>
#include <UIAutomation.h>
#include "AutomationClientManager.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationClient {

    // This is base class for various Automation Events handlers. This provides basic implementations of IUnknown.
    // Also it follows the pattern which auto removes the handler once the task is done. Event handlers can override
    // Attach and Remove and do their specific task and most of the baisc stuff is already baked for them here.
    template <class IFACE>
    class AutomationEventHandlerBase : public IFACE
    {
    public:
        AutomationEventHandlerBase(std::shared_ptr<AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope)
            : m_refCount(1)
             ,m_spEvent(spEvent)
             ,m_spAClientManager(spAClientManager)
             ,m_treeScope(tScope)
             ,m_spUIAutomationElement(nullptr)
             ,m_hrForHandler(S_OK)
        {
        }

        virtual void RemoveEventHandler()
        {
        }

        virtual void AttachEventHandler()
        {
        }

        void Confirm()
        {
            LOG_OUTPUT(L"Confirm or wait for the Event Handler.");
            if (m_spEvent)
            {
                m_spEvent->WaitForDefault();
            }
        }

        void ConfirmAndUnregister()
        {
            HRESULT hr = S_OK;
            Confirm();
            RemoveEventHandler();

            hr = m_hrForHandler;
            m_hrForHandler = S_OK;

            LogThrow_IfFailedWithMessage(hr, L"AutomationEventHandlerBase::ConfirmAndUnregister:Handler Failed");
        }

        void ConfirmNegativeAndUnregister()
        {
            HRESULT hr = S_OK;

            LOG_OUTPUT(L"Confirm negative or wait for the Event Handler.");
            if (m_spEvent)
            {
                m_spEvent->WaitForNoThrow(std::chrono::milliseconds(1000));
            }

            // We expect the event won't fire.
            if (m_spEvent->HasFired())
            {
                hr = E_UNEXPECTED;
            }

            RemoveEventHandler();

            LogThrow_IfFailedWithMessage(hr, L"AutomationEventHandlerBase::ConfirmNegativeAndUnregister Failed");
        }

        // IUnknown methods
        IFACEMETHODIMP_(ULONG) AddRef()
        {
            return ++m_refCount;
        }

        IFACEMETHODIMP_(ULONG) Release()
        {
            if (--m_refCount <= 0)
            {
                delete this;
                return 0;
            }
            return m_refCount;
        }

        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppInterface)
        {
            IUnknown *pResult = NULL;
            *ppInterface = nullptr;

            if (riid == __uuidof(IUnknown))
            {
                pResult = static_cast<IUnknown*>(this);
            }
            else if (riid == __uuidof(IFACE))
            {
                pResult = static_cast<IFACE*>(this);
            }
            else
            {
                return E_NOINTERFACE;
            }

            pResult->AddRef();
            *ppInterface = (void*) pResult;
            return S_OK;
        }

    protected:
        ULONG m_refCount;
        std::shared_ptr<AutomationClientManager> m_spAClientManager;
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> m_spEvent;
        wrl::ComPtr<IUIAutomationElement> m_spUIAutomationElement;
        TreeScope m_treeScope;
        HRESULT m_hrForHandler;
    };


    // Basic Implementation of Structure Changed EventHandler.
    // Similar model can be used to write other basic Event Handlers like, PropertyChange EventHandler or any AutomationEvent Handler.
    // Refer to InvokePatternTests for Invoke Event Handler.
    class AutomationStructureChangeHandler : public AutomationEventHandlerBase<IUIAutomationStructureChangedEventHandler>
    {
    public:
        AutomationStructureChangeHandler(std::shared_ptr<AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_fStructureChangedEventHandlerEnabled(false)
        {
        }

        void Init(StructureChangeType eventType)
        {
            m_spEvent->Reset();
            m_EventType = eventType;
        }

        HRESULT STDMETHODCALLTYPE HandleStructureChangedEvent(
            __RPC__in_opt IUIAutomationElement* /*sender*/,
            enum StructureChangeType eventType,
            __RPC__in SAFEARRAY* runtimeId)
        {
            m_hrForHandler = E_UNEXPECTED;

            if (eventType == m_EventType)
            {
                // According to the documentation, the runtime IDs correspond to the child elements
                // of the provider node where the tree change occurred. This parameter is used only
                // when the type is StructureChangeType_ChildRemoved; it is NULL for all other
                // StructureChanged events.
                if (m_EventType != StructureChangeType::StructureChangeType_ChildRemoved
                    || runtimeId != nullptr)
                {
                    m_hrForHandler = S_OK;
                }

                m_spEvent->Set();
            }

            return m_hrForHandler;
        }

        void AttachEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);
            LogThrow_IfFailedWithMessage(spAutomation->AddStructureChangedEventHandler(m_spUIAutomationElement.Get(), m_treeScope, spCacheRequest.Get(), this), L"AutomationStructureChangeHandler::AttachEventHandler: Failed in attaching StructureChange EventHandler.");

            m_fStructureChangedEventHandlerEnabled = true;
        }

        void RemoveEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            m_spAClientManager->GetAutomation(&spAutomation);

            if (m_fStructureChangedEventHandlerEnabled)
            {
                LogThrow_IfFailedWithMessage(spAutomation->RemoveStructureChangedEventHandler(m_spUIAutomationElement.Get(), this), L"AutomationStructureChangeHandler::RemoveEventHandler: Failed while removing StructureChange EventHandler.");
            }

            m_fStructureChangedEventHandlerEnabled = false;
        }

    private:
        bool m_fStructureChangedEventHandlerEnabled;
        StructureChangeType m_EventType;
    };

    // Basic Implementation of Property Changed EventHandler. This handler can handle all of the generic UIA Property change events.
    template <std::size_t Size>
    class AutomationPropertyChangedHandler : public AutomationEventHandlerBase<IUIAutomationPropertyChangedEventHandler>
    {
    public:
        AutomationPropertyChangedHandler(std::shared_ptr<AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope, std::array<PROPERTYID, Size> saPropertyIds)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_fPropertyChangedEventHandlerEnabled(false)
            , m_saPropertyIds(saPropertyIds)
        {
        }

        HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(
            __RPC__in_opt IUIAutomationElement* /*sender*/,
            PROPERTYID propertyId,
            VARIANT /*newValue*/)
        {
            m_hrForHandler = E_UNEXPECTED;

            for (auto& handledPropertyId : m_saPropertyIds)
            {
                if (propertyId == handledPropertyId)
                {
                    m_hrForHandler = S_OK;
                    break;
                }
            }

            m_spEvent->Set();
            return m_hrForHandler;
        }

        void AttachEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);

            Common::AutoSafeArray<VT_I4> propertyIdsAutoSafeArray(static_cast<ULONG>(m_saPropertyIds.size()));

            for (auto& propertyId : m_saPropertyIds)
            {
                propertyIdsAutoSafeArray.AddElement(&propertyId);
            }

            LogThrow_IfFailedWithMessage(spAutomation->AddPropertyChangedEventHandler(m_spUIAutomationElement.Get(), m_treeScope, spCacheRequest.Get(), this, propertyIdsAutoSafeArray.Get()), L"AutomationPropertyChangedHandler::AttachEventHandler: Failed in attaching PropertyChanged EventHandler.");

            m_fPropertyChangedEventHandlerEnabled = true;
        }

        void RemoveEventHandler()
        {
            if (m_fPropertyChangedEventHandlerEnabled)
            {
                wrl::ComPtr<IUIAutomation> spAutomation;
                m_spAClientManager->GetAutomation(&spAutomation);
                LogThrow_IfFailedWithMessage(spAutomation->RemovePropertyChangedEventHandler(m_spUIAutomationElement.Get(), this), L"AutomationPropertyChangedHandler::RemoveEventHandler: Failed while removing PropertyChanged EventHandler.");
            }

            m_fPropertyChangedEventHandlerEnabled = false;
        }

    private:
        bool m_fPropertyChangedEventHandlerEnabled;
        std::array<PROPERTYID, Size> m_saPropertyIds;
    };

    class AutomationEventHandler : public AutomationClient::AutomationEventHandlerBase<IUIAutomationEventHandler>
    {
    public:
        AutomationEventHandler(std::shared_ptr<AutomationClient::AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope, EVENTID id)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_fAutomationEventHandlerEnabled(false)
            , m_eventId(id)
        {
        }

        void RemoveEventHandler() override
        {
            wrl::ComPtr<IUIAutomation> spAutomation;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);

            if (m_fAutomationEventHandlerEnabled)
            {
                LogThrow_IfFailedWithMessage(spAutomation->RemoveAutomationEventHandler(m_eventId, m_spUIAutomationElement.Get(), this), L"AutomationEventPatternHandler::RemoveEventHandler: Failed while removing EventHandler.");
            }

            m_fAutomationEventHandlerEnabled = false;
        }

        void AttachEventHandler() override
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);
            LogThrow_IfFailedWithMessage(spAutomation->AddAutomationEventHandler(m_eventId, m_spUIAutomationElement.Get(), m_treeScope, spCacheRequest.Get(), this), L"AutomationEventPatternHandler::AttachEventHandler:Failed in attaching EventHandler.");
            m_fAutomationEventHandlerEnabled = true;
        }

        HRESULT STDMETHODCALLTYPE HandleAutomationEvent(
            __RPC__in_opt IUIAutomationElement*,
            EVENTID eventId)
        {
            if (m_eventId == eventId)
            {
                m_hrForHandler = S_OK;
            }
            else
            {
                m_hrForHandler = E_UNEXPECTED;
            }

            m_spEvent->Set();
            return m_hrForHandler;
        }

    private:
        EVENTID m_eventId;
        bool m_fAutomationEventHandlerEnabled;
    };

    // Basic Implementation of Focus Changed EventHandler.
    // Similar model can be used to write other basic Event Handlers like, PropertyChange EventHandler or any AutomationEvent Handler.
    // Refer to InvokePatternTests for Invoke Event Handler.
    class AutomationFocusChangeHandler : public AutomationEventHandlerBase<IUIAutomationFocusChangedEventHandler>
    {
    public:
        AutomationFocusChangeHandler(std::shared_ptr<AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_lastFocusedElement(nullptr)
            , m_focusChangedEventHandlerEnabled(false)
        {
        }

        void Init()
        {
            m_spEvent->Reset();
        }

        HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(
            __RPC__in_opt IUIAutomationElement* sender)
        {
            m_hrForHandler = S_OK;
            m_lastFocusedElement = sender;

            if (focusChangedCallback)
            {
                focusChangedCallback();
            }

            m_spEvent->Set();

            return m_hrForHandler;
        }

        void AttachEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);
            LogThrow_IfFailedWithMessage(spAutomation->AddFocusChangedEventHandler(spCacheRequest.Get(), this), L"AutomationFocusChangeHandler::AttachEventHandler: Failed in attaching FocusChange EventHandler.");

            m_focusChangedEventHandlerEnabled = true;
        }

        void RemoveEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            m_spAClientManager->GetAutomation(&spAutomation);

            if (m_focusChangedEventHandlerEnabled)
            {
                LogThrow_IfFailedWithMessage(spAutomation->RemoveFocusChangedEventHandler(this), L"AutomationFocusChangeHandler::RemoveEventHandler: Failed while removing FocusChange EventHandler.");
            }

            m_focusChangedEventHandlerEnabled = false;
            m_lastFocusedElement = nullptr;
        }

        IUIAutomationElement* GetLastFocusedElement()
        {
            return m_lastFocusedElement.Get();
        }

        std::function<void()> focusChangedCallback;

    private:
        wrl::ComPtr<IUIAutomationElement> m_lastFocusedElement;
        bool m_focusChangedEventHandlerEnabled;
    };

    class AutomationNotificationHandler : public AutomationEventHandlerBase<IUIAutomationNotificationEventHandler>
    {
    public:
        AutomationNotificationHandler(
            std::shared_ptr<AutomationClientManager> automationClientManager,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> notifyEvent,
            TreeScope scope)
            : AutomationEventHandlerBase(automationClientManager, notifyEvent, scope)
        {
        }

        void Init(
            NotificationKind expectedNotificationKind,
            NotificationProcessing expectedNotificationProcessing,
            _In_opt_z_ const wchar_t* expectedDisplayString,
            _In_z_ const wchar_t* expectedActivityId)
        {
            m_spEvent->Reset();

            m_expectedNotificationKind = expectedNotificationKind;
            m_expectedNotificationProcessing = expectedNotificationProcessing;
            if (expectedDisplayString)
            {
                m_expectedDisplayString = expectedDisplayString;
            }
            else
            {
                m_expectedDisplayString.clear();
            }
            m_expectedActivityId = expectedActivityId;
        }

        IFACEMETHODIMP HandleNotificationEvent(
            _In_ IUIAutomationElement* /*sender*/,
            NotificationKind notificationKind,
            NotificationProcessing notificationProcessing,
            _In_opt_ BSTR displayString,
            _In_ BSTR activityId)
        {
            if ((m_expectedNotificationKind == notificationKind) &&
                (m_expectedNotificationProcessing == notificationProcessing) &&
                ((m_expectedDisplayString.length() == 0) || (m_expectedDisplayString == displayString)) &&
                (m_expectedActivityId == activityId))
            {
                m_spEvent->Set();
                return S_OK;
            }
            else
            {
                return E_UNEXPECTED;
            }
        }

        void AttachEventHandler()
        {
            wrl::ComPtr<IUIAutomation> automation;
            wrl::ComPtr<IUIAutomation5> automation5;
            wrl::ComPtr<IUIAutomationCacheRequest> cacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&automation);
            LogThrow_IfFailed(automation.As(&automation5));
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&cacheRequest);
            LogThrow_IfFailedWithMessage(automation5->AddNotificationEventHandler(m_spUIAutomationElement.Get(), m_treeScope, cacheRequest.Get(), this), L"AutomationNotificationHandler::AttachEventHandler: Failed in attaching Notification EventHandler.");

            m_handlerEnabled = true;
        }

        void RemoveEventHandler()
        {
            wrl::ComPtr<IUIAutomation> automation;
            wrl::ComPtr<IUIAutomation5> automation5;
            m_spAClientManager->GetAutomation(&automation);
            LogThrow_IfFailed(automation.As(&automation5));

            if (m_handlerEnabled)
            {
                LogThrow_IfFailedWithMessage(automation5->RemoveNotificationEventHandler(m_spUIAutomationElement.Get(), this), L"AutomationNotificationHandler::RemoveEventHandler: Failed while removing Notification EventHandler.");
            }

            m_handlerEnabled = false;
        }

    private:
        bool m_handlerEnabled = false;
        NotificationKind m_expectedNotificationKind;
        NotificationProcessing m_expectedNotificationProcessing;
        std::wstring m_expectedDisplayString;
        std::wstring m_expectedActivityId;
    };

} } } } } } // namespace Microsoft::UI::Xaml::Tests::Automation::AutomationClient