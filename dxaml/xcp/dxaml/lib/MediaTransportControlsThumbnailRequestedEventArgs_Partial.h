// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MediaTransportControlsThumbnailRequestedEventArgs.g.h"
#include "MediaTransportControls.g.h"

namespace DirectUI
{    
    // Implements MediaTransportControlsThumbnailRequestedEventArgs
    PARTIAL_CLASS(MediaTransportControlsThumbnailRequestedEventArgs)
    {
    public:
        MediaTransportControlsThumbnailRequestedEventArgs() { }

        _Check_return_ HRESULT SetOwner(_In_ xaml_controls::IMediaTransportControls* pOwner)
        {
            ctl::ComPtr<xaml_controls::IMediaTransportControls> spOwner(pOwner);
            IFC_RETURN(spOwner.AsWeak(&m_wkOwner));
            return S_OK;
        }

        _Check_return_ HRESULT GetDeferralImpl(_Outptr_ wf::IDeferral** returnValue)
        { 
            *returnValue = NULL;
            
            if (!m_spDeferral)
            {
                ctl::ComPtr<wf::IDeferralFactory> factory;
                
                IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Deferral).Get(), factory.ReleaseAndGetAddressOf()));
                factory->Create(wrl::Callback<wf::IDeferralCompletedHandler>(this, &MediaTransportControlsThumbnailRequestedEventArgs::Complete).Get(), &m_spDeferral);
            }
            IFC_RETURN(m_spDeferral.CopyTo(returnValue));
            return S_OK;
        }

        _Check_return_ HRESULT SetThumbnailImageImpl(_In_ wsts::IInputStream* pSource)
        {
            if (pSource == nullptr)
            {
                return E_INVALIDARG;
            }
            m_spInputStream = pSource;
            return S_OK;
        }

        HRESULT Complete()
        {
            ctl::ComPtr<xaml_controls::IMediaTransportControls> spOwner;

            if (m_wkOwner)
            {
                IFC_RETURN(m_wkOwner.As(&spOwner));
            }

            if (spOwner)
            {
                ctl::ComPtr<MediaTransportControls> spMTC = 
                    static_cast<MediaTransportControls*>(ctl::impl_cast<MediaTransportControlsGenerated>(spOwner.Get()));
                IFC_RETURN(spMTC->SetThumbnailImage(m_spInputStream.Get()));
            }

            return S_OK;
        }

    private:

        ctl::ComPtr<wsts::IInputStream> m_spInputStream;
        ctl::WeakRefPtr m_wkOwner;
        ctl::ComPtr<wf::IDeferral> m_spDeferral;
    };
}
