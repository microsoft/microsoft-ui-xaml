// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "EtwEventRecord.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {
    
    struct EventInfo 
    {
        UINT16 EventId;
        BYTE EventVersion;
    };

    class EtwEventFactory
        : public wrl::AgileActivationFactory<appanalysis::IEtwEventFactory>
    {
    public:
        // IEtwEventFactory
        IFACEMETHOD(CreateInstance)(
            _In_ UINT16 eventId,
            _In_ BYTE eventVersion,
            _In_ appanalysis::IEtwProvider* provider,
            _COM_Outptr_ appanalysis::IEtwEvent** ppInstance
            ) override;
    };

class EtwEvent 
    : public wrl::RuntimeClass<appanalysis::IEtwEvent, wrl::FtmBase>
{
        InspectableClass(
            RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwEvent,
            BaseTrust
        );
public:

    EtwEvent();
    virtual ~EtwEvent();

    HRESULT RuntimeClassInitialize(
        _In_ UINT16 EventId,
        _In_ BYTE EventVersion,
        _In_ appanalysis::IEtwProvider* provider
        );
        
    static HRESULT CreateInstance(
        _In_ const EventInfo& eventInfo,
        _In_ appanalysis::IEtwProvider* provider,
        _COM_Outptr_ EtwEvent** info
        );

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_EventId
    //
    IFACEMETHOD(get_EventId)(
        _Out_ UINT16* eventId
        ) override;
        
    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_EventVersion
    //
    IFACEMETHOD(get_EventVersion)(
        _Out_ BYTE* eventVersion
        ) override;

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwEvent::get_EventVersion
    //
    IFACEMETHOD(get_Provider)(
        _COM_Outptr_ appanalysis::IEtwProvider** provider
        ) override;

private:

    HRESULT EnsureManifestLoaded();

    // Resolves the manifest path passed ink and copies the final path into
    // a pre allocated string buffer of size MAX_PATH.
    static HRESULT ResolveFullManifestPath(
        _In_ HSTRING manifestFileName,
        _Out_writes_z_(MAX_PATH) wchar_t* manifestFullPath
    );

    EventInfo m_info;
    wrl::ComPtr<appanalysis::IEtwProvider> m_provider;
    wchar_t m_providerManifestFullPath[MAX_PATH];
};


} } }
