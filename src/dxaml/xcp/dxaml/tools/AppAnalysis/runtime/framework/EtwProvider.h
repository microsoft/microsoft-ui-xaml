// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "macros.h"
#include "wil_resource.h"
#include <unordered_set>
#include <windowscollections.h>
#include "helpers.h"
#include <map>
#include "EtwEventInfo.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    struct ProviderInfo
    {
        GUID ProviderId;
        LPCWSTR ManifestPath; // null if classic provider
        UINT32 EnableFlags;   // 0 if manifest provider
        appanalysis::ProviderType ProviderType;
    };

    class EtwProviderFactory
        : public wrl::AgileActivationFactory<appanalysis::IEtwProviderStatics>
    {
    public:
        // IEtwEventProviderFactory
        IFACEMETHOD(CreateKernelProvider)(
            _In_ GUID providerId,
            _In_ UINT32 enableFlags,
            _COM_Outptr_ appanalysis::IEtwProvider** instance
            ) override;

        IFACEMETHOD(Create)(
            _In_ GUID providerId,
            _In_ HSTRING manifest,
            _COM_Outptr_ appanalysis::IEtwProvider** instance
            ) override;

        IFACEMETHOD(ActivateInstance)(
            _COM_Outptr_ IInspectable **ppInspectable
            ) override;
    };


    class EtwProvider
        : public wrl::RuntimeClass<appanalysis::IEtwProvider, wrl::FtmBase>
    {
            InspectableClass(
                RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwProvider,
                BaseTrust
            );
    public:

        EtwProvider()
            : m_id(GUID_NULL)
            , m_enableFlags(0)
        {
        }

        virtual ~EtwProvider()
        {
        }

        HRESULT RuntimeClassInitialize(
            _In_ GUID ProviderId,
            _In_ HSTRING ManifestPath
        );

        HRESULT RuntimeClassInitialize(
            _In_ GUID ProviderId,
            _In_ UINT32 enableFlags
            );

        static HRESULT CreateInstance(
            _In_ const ProviderInfo& providerInfo,
            _COM_Outptr_ appanalysis::IEtwProvider** provider
            );

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwProvider::get_Id
        //
        IFACEMETHOD(get_ID)(
            _Out_ GUID* id
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwProvider::get_EventVersion
        //
        IFACEMETHOD(get_EnableFlags)(
            _Out_ UINT32* enableFlags
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwProvider::get_ProviderType
        //
        IFACEMETHOD(get_ProviderType)(
            _Out_ appanalysis::ProviderType* providerType
            ) override;

        ////////////////////////////////////////////////////////////////////////////////
        // IEtwProvider::get_Manifest
        //
        IFACEMETHOD(get_Manifest)(
            _Out_ HSTRING* manifest
            ) override;

    private:


        GUID m_id;
        wil::unique_hstring m_manifest;
        UINT32 m_enableFlags;
        appanalysis::ProviderType m_providerType{};
    };


} } }
