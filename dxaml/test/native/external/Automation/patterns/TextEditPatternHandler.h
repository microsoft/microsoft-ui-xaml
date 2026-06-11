// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <AutomationClient\AutomationEventHandler.h>

using namespace  Microsoft::UI::Xaml::Tests::Automation::AutomationClient;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Patterns {

    class TextEditTextChangedEventHandler : public AutomationEventHandlerBase<IUIAutomationTextEditTextChangedEventHandler>
    {
    public:
        TextEditTextChangedEventHandler(const std::shared_ptr<AutomationClient::AutomationClientManager> &spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope, TextEditChangeType textEditChangeType)
            : AutomationEventHandlerBase(spAClientManager, spEvent, tScope)
            , m_fTextEditTextChangedEventHandlerEnabled(false)
            , m_textEditChangeType(textEditChangeType)
        {
        }

        HRESULT  STDMETHODCALLTYPE HandleTextEditTextChangedEvent(
            __RPC__in_opt IUIAutomationElement* /*sender*/,
        enum TextEditChangeType textEditChangeType,
            __RPC__in SAFEARRAY * /*eventStrings*/)
        {
            m_hrForHandler = E_UNEXPECTED;
            if (textEditChangeType == m_textEditChangeType)
            {
                m_hrForHandler = S_OK;
            }

            m_spEvent->Set();
            return m_hrForHandler;
        }

        void AttachEventHandler()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomation3> spAutomation3;
            wrl::ComPtr<IUIAutomationCacheRequest> spCacheRequest;

            WEX::Common::Throw::IfNull(m_spAClientManager.get());
            m_spAClientManager->GetAutomation(&spAutomation);
            spAutomation.As(&spAutomation3);
            m_spAClientManager->GetCurrentUIAutomationElement(&m_spUIAutomationElement);
            m_spAClientManager->GetCurrentCacheRequest(&spCacheRequest);

            LogThrow_IfFailedWithMessage(spAutomation3->AddTextEditTextChangedEventHandler(m_spUIAutomationElement.Get(), m_treeScope, m_textEditChangeType, spCacheRequest.Get(), this),
                    L"AutomationTextEditTextChangedEventHandler::AddTextEditTextChangedEventHandler: Failed in attaching TextEditTextChanged EventHandler.");

            m_fTextEditTextChangedEventHandlerEnabled = true;
        }

        void RemoveEventHandler()
        {
            if (m_fTextEditTextChangedEventHandlerEnabled)
            {
                wrl::ComPtr<IUIAutomation> spAutomation;
                m_spAClientManager->GetAutomation(&spAutomation);
                wrl::ComPtr<IUIAutomation3> spAutomation3;
                spAutomation.As(&spAutomation3);
                LogThrow_IfFailedWithMessage(spAutomation3->RemoveTextEditTextChangedEventHandler(m_spUIAutomationElement.Get(), this),
                    L"AutomationTextEditTextChangedEventHandler::RemoveTextEditTextChangedEventHandler: Failed while removing TextEditTextChanged EventHandler.");
            }

            m_fTextEditTextChangedEventHandlerEnabled = false;
        }

    private:
        bool m_fTextEditTextChangedEventHandlerEnabled;
        TextEditChangeType m_textEditChangeType;
    };

} } } } } }
