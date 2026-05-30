// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ITimedTextSourcePresenter.h>
#include <CueRenderer.h>

namespace DirectUI
{
    using namespace Microsoft::WRL;

    class MediaTransportControls;
    class TimedTextTrack;

    // This class acts as the controller for the closed captioning
    // portion of MediaPlayerElement
    class CTimedTextSource :
        public ITimedTextSourcePresenter,
        public ctl::WeakReferenceSourceNoThreadId
    {
    public:
        static _Check_return_ HRESULT Create(
            _In_ xaml::IDependencyObject* IDependencyObject,
            _In_ xaml::IUIElement* pLayoutRoot,
            _Outptr_ CTimedTextSource** ppTimedTextSource);

        CTimedTextSource();
       ~CTimedTextSource() override;
       
        void CleanupDeviceRelatedResources(const bool cleanupDComp);
        
        _Check_return_ HRESULT SetMediaPlayer(_In_opt_ wmp::IMediaPlayer* pMediaPlayer);

        _Check_return_ HRESULT AddToFullWindowMediaRoot();
        _Check_return_ HRESULT RemoveFromFullWindowMediaRoot();

        IFACEMETHOD(SetMTCOffset)(_In_ double offset) override;
        void ClearCues();

        _Check_return_ IFACEMETHOD(Reset)() override;

        static bool IsVisibleTrack(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmp::IMediaPlaybackItem* pCurrentItem);
        static bool IsSupportedTrack(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmp::IMediaPlaybackItem* pCurrentItem);

        // ICueRendererCallback
        IFACEMETHOD(ResetActiveCues)() override;
        _Check_return_ IFACEMETHOD(GetNaturalVideoSize)(_Out_ INT32* pHeight, _Out_ INT32* pWidth) override;
        _Check_return_ IFACEMETHOD(get_IsFullWindow)(_Out_ BOOLEAN* isFullWindow) override;
        _Check_return_ IFACEMETHOD(get_Stretch)(_Out_ xaml_media::Stretch* stretch) override;

    protected:
        BEGIN_INTERFACE_MAP(CTimedTextSource, ctl::WeakReferenceSourceNoThreadId)
            INTERFACE_ENTRY(CTimedTextSource, ITimedTextSourcePresenter)
        END_INTERFACE_MAP(CTimedTextSource, ctl::WeakReferenceSourceNoThreadId)

    private:
        _Check_return_ HRESULT Initialize(_In_opt_ xaml::IDependencyObject* pOwner, _In_ xaml::IUIElement* pLayoutRoot);

        _Check_return_ HRESULT AddCueCallbacks(_In_ wmc::ITimedMetadataTrack *, _In_ unsigned int trackIndex);
        _Check_return_ HRESULT RemoveAllTrackCallbacks();
        _Check_return_ HRESULT AddTrackCallback();
        _Check_return_ HRESULT UpdateCurrentItem();

        _Check_return_ HRESULT OnTimedMetadataTracksChanged(_In_ wfc::IVectorChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnCueEntered(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmc::IMediaCueEventArgs* pArgs);
        _Check_return_ HRESULT OnCueExited(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmc::IMediaCueEventArgs* pArgs);

        _Check_return_ HRESULT OnMediaPlayerSourceChanged();
        _Check_return_ HRESULT OnMediaCurrentItemChanged();

        _Check_return_ HRESULT AddMediaPlayerEventRegistrations();
        _Check_return_ HRESULT RemoveMediaPlayerEventRegistrations();

        _Check_return_ HRESULT AddTrackEventListeners();
        _Check_return_ HRESULT RemoveTrackEventListeners();

        _Check_return_ HRESULT AddCuePresentationModeChangedCallback();
        _Check_return_ HRESULT RemoveCuePresentationModeChangedCallback();

        _Check_return_ HRESULT OnCuePresentationModeChanged(_In_ wmp::ITimedMetadataPresentationModeChangedEventArgs* pArgs);

        bool IsSupportedCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue) const;

        CCueRenderer m_cueRenderer;
        ctl::ComPtr<msy::IDispatcherQueue> m_spDispatcherQueue;
        ctl::WeakRefPtr m_wrOwner;

        struct  CTrackTokens
        {
            unsigned int Track;
            ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
            EventRegistrationToken EnteredToken;
            EventRegistrationToken ExitedToken{};
        };

        std::list<std::shared_ptr<CTrackTokens>> m_tokensList;
        EventRegistrationToken m_trackAddedEventToken;
        ctl::ComPtr<wmp::IMediaPlayer> m_spMediaPlayer;
        ctl::ComPtr<wmp::IMediaPlaybackItem> m_spCurrentItem;
        ctl::ComPtr<wmp::IMediaPlaybackList> m_spMediaPlaybackList;

        EventRegistrationToken m_mediaSourceChangedToken;
        EventRegistrationToken m_mediaCurrentItemChangedToken;
        EventRegistrationToken m_presentationModeChangedToken;
    };
}
