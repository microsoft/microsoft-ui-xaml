// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EtwProvider.h"
#include "EventProcessor.h"

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis {

    IFACEMETHODIMP 
    EtwProviderFactory::CreateKernelProvider(
        _In_ GUID providerId,
        _In_ UINT32 enableFlags,
        _COM_Outptr_ appanalysis::IEtwProvider** instance
        )
    {
        wrl::ComPtr<appanalysis::IEtwProvider> provider;
        IFC_RETURN(wrl::MakeAndInitialize<EtwProvider>(&provider, providerId, enableFlags));

        *instance = provider.Detach();

        return S_OK;
    }

    IFACEMETHODIMP 
    EtwProviderFactory::Create(
        _In_ GUID providerId,
        _In_ HSTRING manifest,
        _COM_Outptr_ appanalysis::IEtwProvider** instance
        )
    {
        wrl::ComPtr<appanalysis::IEtwProvider> provider;
        IFC_RETURN(wrl::MakeAndInitialize<EtwProvider>(&provider, providerId, manifest));

        *instance = provider.Detach();
        return S_OK;
    }

    IFACEMETHODIMP 
    EtwProviderFactory::ActivateInstance(
        _COM_Outptr_ IInspectable **
        )
    {
        return E_NOTIMPL;
    }

    HRESULT EtwProvider::RuntimeClassInitialize(
        _In_ GUID providerId,
        _In_ HSTRING manifestPath
        )
    {
        m_providerType = appanalysis::ProviderType_Manifest;
        m_id = providerId;
        
        // we need a manifest!
        if (!!WindowsIsStringEmpty(manifestPath))
        {
            return E_INVALIDARG;
        }

        IFC_RETURN(WindowsDuplicateString(manifestPath, &m_manifest));

        return S_OK;
    }

    HRESULT EtwProvider::RuntimeClassInitialize(
        _In_ GUID providerId,
        _In_ UINT32 enableFlags
        )
    {
        m_providerType = appanalysis::ProviderType_Kernel;
        m_enableFlags = enableFlags;
        m_id = providerId;

        return S_OK;
    }

    HRESULT EtwProvider::CreateInstance(
        _In_ const ProviderInfo& providerInfo,
        _COM_Outptr_ appanalysis::IEtwProvider** provider
        )
    {
        
        switch (providerInfo.ProviderType)
        {
        case appanalysis::ProviderType_Kernel:
        {
            if (providerInfo.ManifestPath != nullptr)
            {
                return E_INVALIDARG;
            }
            IFC_RETURN(wrl::MakeAndInitialize<EtwProvider>(provider, providerInfo.ProviderId, providerInfo.EnableFlags));
            break;
        }
        case appanalysis::ProviderType_Manifest:
        {
            if (providerInfo.EnableFlags > 0)
            {
                return E_INVALIDARG;
            }
            IFC_RETURN(wrl::MakeAndInitialize<EtwProvider>(provider, providerInfo.ProviderId, StringRef(providerInfo.ManifestPath)));
            break;
        }
        default:
        {
            return E_INVALIDARG;
        }
        }

        return S_OK;
    }
        
    ////////////////////////////////////////////////////////////////////////////////
    // IEtwProvider::get_ID
    //
    IFACEMETHODIMP EtwProvider::get_ID(
        _Out_ GUID* ID
        )
    {
        ARG_VALIDRETURNPOINTER(ID);
        *ID = m_id;
        return S_OK;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // IEtwProvider::get_ProviderType
    //
    IFACEMETHODIMP EtwProvider::get_ProviderType(
        _Out_ appanalysis::ProviderType* providerType
        )
    {
        ARG_VALIDRETURNPOINTER(providerType);
        *providerType = m_providerType;
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwProvider::get_Manifest
    //
    IFACEMETHODIMP EtwProvider::get_Manifest(
        _Out_ HSTRING* manifest
        )
    {
        ARG_VALIDRETURNPOINTER(manifest);

        ASSERT(m_providerType == appanalysis::ProviderType_Manifest);
        IFC_RETURN(WindowsDuplicateString(m_manifest.get(), manifest));
        return S_OK;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // IEtwProvider::get_ProviderType
    //
    IFACEMETHODIMP EtwProvider::get_EnableFlags(
        _Out_ UINT32* enableFlags
        )
    {
        ARG_VALIDRETURNPOINTER(enableFlags);

        ASSERT(m_providerType == appanalysis::ProviderType_Kernel);
        *enableFlags = m_enableFlags;
        return S_OK;
    }

} } }