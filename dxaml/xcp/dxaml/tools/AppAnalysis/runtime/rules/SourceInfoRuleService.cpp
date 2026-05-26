// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "ETWRuleService.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
    class SourceInfoRuleService
        : public ETWRuleService<SourceInfoRuleService, appanalysis::ISourceInfoRuleService>
    {
    public:
        SourceInfoRuleService()
        {
        }

        virtual ~SourceInfoRuleService()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////
        // ISourceInfoRuleService::GetSourceInfo
        //
        IFACEMETHOD(GetSourceInfo)(
            _In_ UINT64 elementId,
            _Out_ appanalysis::SourceInfo *sourceInfo)
        {
            ARG_VALIDRETURNPOINTER(sourceInfo);

            *sourceInfo = { 0 };

            // It's ok if source info isn't found since retail builds of applications won't contain source info.
            SourceInfoMap::iterator iter = m_sourceInfoCache.find(elementId);
            if (iter == m_sourceInfoCache.end())
            {
                return S_FALSE;
            }

            IFC_RETURN(::CoAllocSourceInfo(iter->second, sourceInfo));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // ISourceInfoRuleService::GetVisualTreeId
        //
        IFACEMETHOD(GetVisualTreeId)(
            _In_ UINT64 elementId,
            _Out_ UINT64 *visualTreeId)
        {
            ARG_VALIDRETURNPOINTER(visualTreeId);

            *visualTreeId =  0;

            // It's ok if source info isn't found since retail builds of applications won't contain source info.
            PeerMap::iterator iter = m_peerMap.find(elementId);
            if (iter == m_peerMap.end())
            {
                return S_FALSE;
            }

            *visualTreeId = iter->second;

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // ISourceInfoRuleService::GetElementName
        //
        IFACEMETHOD(GetElementName)(
            _In_ UINT64 elementId,
            _Out_ HSTRING *elementName)
        {
            ARG_VALIDRETURNPOINTER(elementName);

            // not all elements require a name
            ElementStringMap::iterator iter = m_elementNameCache.find(elementId);
            if (iter == m_elementNameCache.end())
            {
                return S_FALSE;
            }

            IFC_RETURN(WindowsDuplicateString(iter->second.get(), elementName));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // ISourceInfoRuleService::GetElementTypeName
        //
        IFACEMETHOD(GetElementTypeName)(
            _In_ UINT64 elementId,
            _Out_ HSTRING *elementTypeName)
        {
            ARG_VALIDRETURNPOINTER(elementTypeName);

            ElementStringMap::iterator iter = m_elementTypeCache.find(elementId);
            if (iter == m_elementTypeCache.end())
            {
                return E_NOTFOUND;
            }

            IFC_RETURN(WindowsDuplicateString(iter->second.get(), elementTypeName));

            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // SourceInfoRuleService::ProcessElementCreated
        //
        HRESULT ProcessElementCreated(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            InstanceHandle elementId = 0;
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));
            IFC_RETURN(ProcessElementAddedInternal(pEvent, elementId));

            wil::shared_hstring name;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"ClassName"), &name));

            IFCSTL_RETURN(m_elementTypeCache.emplace(elementId, name));
            return S_OK;
        }
    
        HRESULT ProcessPeerCreated(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            InstanceHandle elementId = 0;
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));
            InstanceHandle peerId = 0;
            // we are reusing a generic ETW template, so the peer is considered the "parent"
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ParentId"), &peerId));

            IFCSTL_RETURN(m_peerMap.emplace(elementId, peerId));
            return S_OK;
        }
    
        HRESULT ProcessResourceDictionaryAdd(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            InstanceHandle resourceId = 0;
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ResourceId"), &resourceId));
            IFC_RETURN(ProcessElementAddedInternal(pEvent, resourceId));
        
            return S_OK;
        }
    
        HRESULT ProcessElementAddedInternal(
            _In_ appanalysis::IEtwEventRecord* pEvent,
            _In_ InstanceHandle elementId
            )
        {
            wil::unique_sourceinfo info;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"FileURI"), &info.FileName));
            IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"LineNumber"), &info.LineNumber));
            IFC_RETURN(pEvent->GetUInt32Property(StringRef(L"ColumnNumber"), &info.ColumnNumber));
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"FileHash"), &info.FileHash));

            IFCSTL_RETURN(m_sourceInfoCache.emplace(elementId, std::move(info)));

            return S_OK;
        }

        HRESULT ProcessSetNameInfo(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            wil::shared_hstring name;
            IFC_RETURN(pEvent->GetUnicodeStringProperty(StringRef(L"Name"), &name));

            InstanceHandle elementId = 0;
            IFC_RETURN(pEvent->GetUInt64Property(StringRef(L"ElementId"), &elementId));

            IFCSTL_RETURN(m_elementNameCache.emplace(elementId, name));

            return S_OK;
        }

    private:
        using SourceInfoMap = std::map<InstanceHandle, wil::unique_sourceinfo>;
        using PeerMap = std::map<InstanceHandle, InstanceHandle>;
        using ElementStringMap = std::map<InstanceHandle, wil::shared_hstring>;

        SourceInfoMap m_sourceInfoCache;
        PeerMap m_peerMap;
        ElementStringMap m_elementNameCache;
        ElementStringMap m_elementTypeCache;
    };

    HRESULT SourceInfoRuleService_CreateInstance(_COM_Outptr_ appanalysis::IRuleService** ppService)
    {
        IFC_RETURN(SourceInfoRuleService::CreateInstance(ppService));
        return S_OK;
    }

    BEGIN_PROVIDERS(SourceInfoRuleService)
        DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
        DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
    END_PROVIDERS()

    BEGIN_CALLBACKS(SourceInfoRuleService)
        DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ElementCreatedWithSourceInfo_value, EventVersion_1, &SourceInfoRuleService::ProcessElementCreated)
        DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, PeerCreatedInfo_value, EventVersion_0, &SourceInfoRuleService::ProcessPeerCreated)
        DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, ResourceDictionaryAddWithSourceInfo_value, EventVersion_0, &SourceInfoRuleService::ProcessResourceDictionaryAdd)
        DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ElementSetNameInfo_value, EventVersion_0, &SourceInfoRuleService::ProcessSetNameInfo)
    END_CALLBACKS()

} } }
