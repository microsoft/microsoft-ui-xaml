// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RuleServiceProvider.h"
#include <string>
#include "helpers.h"
#include "StringDefinitions.h"
#include "EtwConsumer.h"
#include "EtwEventInfo.h"
#include "ETWRule.h"
#include "ResourceString.h"

#define MAX_FORMAT_STRING 1024

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    ////////////////////////////////////////////////////////////////////////////////
    //
    // The Rule templated class is to be used as base class for Etw based rules. 
    // common functionality that all rules can benefit from. This is an internal class
    // that implements the IRule interface and also implements ETWConsumer, which is 
    // an internal class that is able to declare events and register them with an
    // EventWatcher.
    //

    template<class T, appanalysis::RuleCategories DeclaredRuleCategories = appanalysis::RuleCategories_None>
    class EtwRuleImpl
        : public ETWConsumer<T, appanalysis::IRule>
    {

    public:

        EtwRuleImpl()
            : m_id(nullptr)
            , m_impact(0)
            , m_title(0)
            , m_linkInfoUri(nullptr)
            , m_linkInfoTitleId(0)
            , m_ruleCategories(DeclaredRuleCategories)
        {
        }

        virtual ~EtwRuleImpl()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        static HRESULT CreateInstance(
            _In_ LPCWSTR rule,
            _In_ UINT titleResourceId,
            _In_ UINT ruleImpactId,
            _In_ UINT linkTitleResourceId,
            _In_ LPCWSTR linkUri,
            _COM_Outptr_ T** ppInstance
            )
        {
            ARG_VALIDRETURNPOINTER(ppInstance);
            *ppInstance = nullptr;

            wrl::ComPtr<T> instance;
            IFC_RETURN(wrl::MakeAndInitialize<T>(instance.GetAddressOf(), rule, titleResourceId, ruleImpactId, linkTitleResourceId, linkUri));
   
            *ppInstance = instance.Detach();

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        HRESULT RuntimeClassInitialize(
            _In_ LPCWSTR ruleResourceId,
            _In_ UINT titleResourceId,
            _In_ UINT ruleImpactId,
            _In_ UINT linkTitleResourceId,
            _In_ LPCWSTR linkUri
            )
        {
            m_impact = ruleImpactId;
            m_title = titleResourceId;
            m_id = ruleResourceId;
            m_linkInfoUri = linkUri;
            m_linkInfoTitleId = linkTitleResourceId;

            // Rules must declare a category
            if (m_ruleCategories == appanalysis::RuleCategories_None)
            {
                return E_FAIL;
            }

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        HRESULT RegisterEvents(
            _COM_Outptr_ appanalysis::IEtwEventWatcher** watcher
            )
        {
            IFC_RETURN(ETWConsumer::RegisterEvents(watcher));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::GetId
        //
        IFACEMETHOD(get_Id)(
            _Out_ HSTRING* resourceId
            ) override
        {
            IFC_RETURN(WindowsDuplicateString(StringRef(m_id), resourceId));
            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::get_Title
        //
        IFACEMETHOD(get_Title)(
            _Out_ HSTRING* title
            ) override
        {
            IFC_RETURN(FormatStringInternal(m_title, nullptr, title));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::get_Impact
        //
        IFACEMETHOD(get_Impact)(
            _Out_ HSTRING* impact
            ) override
        {
            IFC_RETURN(FormatStringInternal(m_impact, nullptr, impact));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::get_LinkInfo
        //
        IFACEMETHOD(get_LinkTitle)(
                _Out_ HSTRING* linkTitle
                ) override
        {
            ARG_VALIDRETURNPOINTER(linkTitle);
            IFC_RETURN(FormatStringInternal(m_linkInfoTitleId, nullptr, linkTitle));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::get_LinkUri
        //
        IFACEMETHOD(get_LinkUri)(
            _Out_ HSTRING* linkUri
            ) override
        {
            ARG_VALIDRETURNPOINTER(linkUri);
            IFC_RETURN(WindowsCreateString(m_linkInfoUri, static_cast<UINT32>(wcslen(m_linkInfoUri)), linkUri))

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::get_RuleType
        //
        IFACEMETHOD(get_Categories)(
            _Out_ appanalysis::RuleCategories* categories
            ) override
        {
            ARG_VALIDRETURNPOINTER(categories);
            ASSERT(m_ruleCategories != appanalysis::RuleCategories_None);
            *categories = m_ruleCategories;

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::ResolveFormattedResourceString
        //
        IFACEMETHOD(FormatString)(
            _In_opt_ appanalysis::IResourceStringView* inputString,
            _Out_ HSTRING* outString
            ) override
        {
            ARG_VALIDRETURNPOINTER(outString);
            if (inputString == nullptr)
            {
                return S_OK;
            }

            *outString = nullptr;

            UINT resourceId = 0;
            IFC_RETURN(inputString->get_Identifier(&resourceId));

            wrl::ComPtr<wfc::IVectorView<HSTRING>> args;
            IFC_RETURN(AppAnalysisHelpers::As(inputString, &args));
            std::vector<PCWSTR> rawStrings;

            IFC_RETURN(ConvertToRawStringArray(args.Get(), &rawStrings));
            IFC_RETURN(FormatStringInternal(resourceId, rawStrings.data(), outString))

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::add_Triggered
        //
        IFACEMETHOD(add_Triggered)(
            _In_ wf::ITypedEventHandler<appanalysis::IRule*, appanalysis::RuleTriggeredEventArgs*> *handler,
            _Out_ EventRegistrationToken *token
            ) override
        {
            IFC_RETURN(m_notificationHandlers.Add(handler, token));
            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // IRule::remove_Triggered
        //
        IFACEMETHOD(remove_Triggered)(
            _In_ EventRegistrationToken token
            ) override
        {
            IFC_RETURN(m_notificationHandlers.Remove(token));
            return S_OK;
        }

    protected:

        void FireNotification(
            _In_ appanalysis::IRuleTriggeredEventArgs* pNotification
            )
        {
            VERIFYHR(m_notificationHandlers.InvokeAll(this, pNotification));
        }

        HRESULT CreateRuleTriggeredEventArgs(
            _In_ const RuleTriggeredEventArgs::CreateParams& params,
            _COM_Outptr_ appanalysis::IRuleTriggeredEventArgs **instance
            )
        {
            *instance = nullptr;

            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> notificationInfo;

            IFC_RETURN(RuleTriggeredEventArgs::CreateInstance(params, &notificationInfo));

            *instance = notificationInfo.Detach();

            return S_OK;
        }

        HRESULT GetRuleService(_In_ const GUID& serviceId, _COM_Outptr_ appanalysis::IRuleService** service)
        {
            wrl::ComPtr<appanalysis::IRuleServiceProviderStatics> serviceProvider;
            IFC_RETURN(RuleServiceProvider::GetProvider(&serviceProvider));
            IFC_RETURN(serviceProvider->GetService(serviceId, service));

            return S_OK;
        }

        template <typename I>
        HRESULT GetRuleService(_Inout_ wrl::Details::ComPtrRef<wrl::ComPtr<I>> service)
        {
            wrl::ComPtr<appanalysis::IRuleService> baseService;
            IFC_RETURN(GetRuleService(__uuidof(I), &baseService));

            IFC_RETURN(baseService.As(service));

            return S_OK;
        }


    private:

        HRESULT FormatStringInternal(
            _In_ UINT32 resourceId,
            _In_opt_z_ PCWSTR* args,
            _Out_ HSTRING* result)
        {

            WCHAR formattedMessage[MAX_FORMAT_STRING] = { 0 };
            wil::unique_hmodule resources;
            IFCW32_RETURN(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)&CoAllocSourceInfo, &resources));

            ULONG stringLength = 0;
            if (args)
            {
                WCHAR formatSpecifier[MAX_FORMAT_STRING] = { 0 };
                IFCW32_RETURN(LoadString(resources.get(), resourceId, formatSpecifier, static_cast<int>(_countof(formatSpecifier))));

                stringLength = FormatMessage(
                    FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    formatSpecifier,
                    0,
                    0,
                    (LPWSTR)&formattedMessage,
                    _countof(formattedMessage),
                    (va_list*)args);
            }
            else
            {
                stringLength = static_cast<ULONG>(LoadString(resources.get(), resourceId, formattedMessage, static_cast<int>(_countof(formattedMessage))));
            }

            if (resourceId != 0 && stringLength == 0)
            {
                IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
            }

            IFC_RETURN(WindowsCreateString(formattedMessage, stringLength, result));
                
            return S_OK;
        }

        HRESULT ConvertToRawStringArray(_In_ wfc::IVectorView<HSTRING>* formatStrings, _Out_ std::vector<PCWSTR>* strings)
        {
            ARG_VALIDRETURNPOINTER(strings);
            strings->clear();

            UINT count = 0;
            IFC_RETURN(formatStrings->get_Size(&count));
            for (UINT i = 0; i < count; ++i)
            {
                wil::unique_hstring arg;
                IFC_RETURN(formatStrings->GetAt(i, &arg));
                IFCSTL_RETURN(strings->push_back(WindowsGetStringRawBuffer(arg.get(), nullptr)));
            }

            return S_OK;
        }

        LPCWSTR m_id;
        UINT m_impact;
        UINT m_title;
        LPCWSTR m_linkInfoUri;
        UINT m_linkInfoTitleId;
        appanalysis::RuleCategories m_ruleCategories;
        wrl::EventSource<wf::ITypedEventHandler<appanalysis::IRule*, appanalysis::RuleTriggeredEventArgs*>, wrl::InvokeModeOptions<wrl::FireAll>> m_notificationHandlers;
    };

} } } 
