// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <AutomationClient\AutomationEventHandler.h>
#include <UIAutomationHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Patterns {

    class InvokePatternHandler : public AutomationClient::AutomationEventHandlerBase<IUIAutomationEventHandler>
    {
    public:
        InvokePatternHandler(std::shared_ptr<AutomationClient::AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_fInvokeAutomationEventHandlerEnabled(false)
            , m_eventId(UIA_Invoke_InvokedEventId)
        {
        }

        InvokePatternHandler(std::shared_ptr<AutomationClient::AutomationClientManager> spAClientManager) 
            : AutomationEventHandlerBase(spAClientManager, nullptr, TreeScope_Descendants)
            , m_fInvokeAutomationEventHandlerEnabled(false)
            , m_eventId(UIA_Invoke_InvokedEventId)
        {
        }

        void Invoke()
        {
            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;

            m_spAClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern), L"InvokePatternHandler::Invoke: Failed in retreiving Invoke Pattern.");
            WEX::Common::Throw::IfNull(spInvokePattern.Get(), L"InvokePatternHandler::Invoke: This Button doesn't support Invoke Pattern which is required.");
            LogThrow_IfFailedWithMessage(spInvokePattern->Invoke(), L"InvokePatternHandler::Invoke: Failed to Invoke.");
        }

        void RemoveEventHandler() override
        {
            wrl::ComPtr<IUIAutomation> spAutomation;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);

            if (m_fInvokeAutomationEventHandlerEnabled)
            {
                UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                {
                    LogThrow_IfFailedWithMessage(spAutomation->RemoveAutomationEventHandler(m_eventId, m_spUIAutomationElement.Get(), this), L"InvokePatternHandler::RemoveEventHandler: Failed while removing Invoke EventHandler.");
                });
            }

            m_fInvokeAutomationEventHandlerEnabled = false;
        }

        void AttachEventHandler() override
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                LogThrow_IfFailedWithMessage(spAutomation->AddAutomationEventHandler(m_eventId, m_spUIAutomationElement.Get(), m_treeScope, spCacheRequest.Get(), this), L"InvokePatternHandler::AttachEventHandler:Failed in attaching EventHandler for Invoke Event.");
            });            

            m_fInvokeAutomationEventHandlerEnabled = true;
        }

        HRESULT STDMETHODCALLTYPE HandleAutomationEvent(
            __RPC__in_opt IUIAutomationElement* /*sender*/,
            EVENTID eventId)
        {
            if (m_eventId == eventId)
            {
                m_hrForHandler = S_OK;
                // Possibly compare Sender too.
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
        bool m_fInvokeAutomationEventHandlerEnabled;
    };

} } } } } }
