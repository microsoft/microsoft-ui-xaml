// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EtwEventInfo.h"
#include "pathcch.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
    
    IFACEMETHODIMP
        EtwEventFactory::CreateInstance(
        _In_ UINT16 eventId,
        _In_ BYTE eventVersion,
        _In_ appanalysis::IEtwProvider* provider,
        _COM_Outptr_ appanalysis::IEtwEvent** ppInstance
        )
    {
        wrl::ComPtr<EtwEvent> eventInfo;
        IFC_RETURN(wrl::MakeAndInitialize<EtwEvent>(&eventInfo, eventId, eventVersion, provider));

        *ppInstance = eventInfo.Detach();
        return S_OK;
    }

    EtwEvent::EtwEvent()
        : m_info({ 0 })
    {
        ZeroMemory(m_providerManifestFullPath, _countof(m_providerManifestFullPath) * sizeof(wchar_t));
    }

    EtwEvent::~EtwEvent()
    {
        TdhUnloadManifest(m_providerManifestFullPath);
    }

    HRESULT EtwEvent::RuntimeClassInitialize(
        _In_ UINT16 eventId,
        _In_ BYTE eventVersion,
        _In_ appanalysis::IEtwProvider* provider
        )
    {
        m_info.EventId = eventId;
        m_info.EventVersion = eventVersion;
        m_provider = provider;

        // make sure this is a valid event the user has declared
        IFC_RETURN(EnsureManifestLoaded());

        return S_OK;
    }

    HRESULT EtwEvent::CreateInstance(
        _In_ const EventInfo& eventInfo,
        _In_ appanalysis::IEtwProvider* provider,
        _COM_Outptr_ EtwEvent** instance)
    {
        wrl::ComPtr<EtwEvent> info;
        IFC_RETURN(wrl::MakeAndInitialize<EtwEvent>(&info, eventInfo.EventId, eventInfo.EventVersion, provider));

        *instance = info.Detach();
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_EventId
    //
    IFACEMETHODIMP EtwEvent::get_EventId(
        _Out_ UINT16* eventId
        )
    {
        *eventId = m_info.EventId;
        return S_OK;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_EventVersion
    //
    IFACEMETHODIMP EtwEvent::get_EventVersion(
        _Out_ BYTE* eventVersion
        )
    {
        *eventVersion = m_info.EventVersion;
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_Provider
    //
    IFACEMETHODIMP EtwEvent::get_Provider(
        _COM_Outptr_ appanalysis::IEtwProvider** provider
        )
    {
        IFC_RETURN(m_provider.CopyTo(provider));
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    HRESULT EtwEvent::EnsureManifestLoaded()
    {
        // If this is a classic provider we can just exit early since we dont'
        // need to try and load the manifest
        appanalysis::ProviderType providerType = appanalysis::ProviderType_Manifest;
        IFC_RETURN(m_provider->get_ProviderType(&providerType));
        if (providerType == appanalysis::ProviderType_Kernel)
        {
            return S_OK;
        }

        EVENT_DESCRIPTOR EventDescriptor = { 0 };
        EventDescriptor.Id = m_info.EventId;
        EventDescriptor.Version = m_info.EventVersion;

        GUID ProviderId = GUID_NULL;
        IFC_RETURN(m_provider->get_ID(&ProviderId));

        ULONG cbBuffer = 0;
        DWORD dwStatus = TdhGetManifestEventInformation(&ProviderId, &EventDescriptor, nullptr, &cbBuffer);
        if (dwStatus == ERROR_NOT_FOUND || dwStatus == ERROR_FILE_NOT_FOUND)
        {
            // Need to load the manifest associated with this provider, event or event version.
            // If no manifest, then this is a classic provider and we can exit early
            wil::unique_hstring providerManifestPath;
            IFC_RETURN(m_provider->get_Manifest(&providerManifestPath));

            ASSERT(!WindowsIsStringEmpty(providerManifestPath.get()));

            GUID ProviderId = GUID_NULL;

            IFC_RETURN(EtwEvent::ResolveFullManifestPath(providerManifestPath.get(), m_providerManifestFullPath));

            dwStatus = TdhLoadManifest(m_providerManifestFullPath);
            IFCSTATUS_RETURN(dwStatus);

            dwStatus = TdhGetManifestEventInformation(&ProviderId, &EventDescriptor, nullptr, &cbBuffer);
            // no check for dwStatus here, fall over.
        }

        if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
        {
            // EventId and Version match, we're good to go.
            return S_OK;
        }
        IFCSTATUS_RETURN(dwStatus);

        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    HRESULT EtwEvent::ResolveFullManifestPath(
            _In_ HSTRING manifestFileName,
            _Out_writes_z_(MAX_PATH) wchar_t* manifestFullPath
        )
    {
        *manifestFullPath = { 0 };

        // We need to get the path to AppAnalyis.dll, so we can load the manifest. We can be guaranteed that the manifest will
        // be placed next to the .dll in the SDK.
        wil::unique_hmodule appAnalysisModule;
        IFCW32_RETURN(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)&EtwEvent::ResolveFullManifestPath, &appAnalysisModule));

        wchar_t appAnalysisModulePath[MAX_PATH] = { 0 };
        IFCW32_RETURN(GetModuleFileName(appAnalysisModule.get(), appAnalysisModulePath, _countof(appAnalysisModulePath)));

        // remove the file name (AppAnalysis.dll) from the path so we can combine the path and filename together into the output string
        IFC_RETURN(PathCchRemoveFileSpec(appAnalysisModulePath, _countof(appAnalysisModulePath)));
        IFC_RETURN(PathCchCombine(manifestFullPath, MAX_PATH, appAnalysisModulePath, WindowsGetStringRawBuffer(manifestFileName, nullptr)));

        return S_OK;
    }

} } }