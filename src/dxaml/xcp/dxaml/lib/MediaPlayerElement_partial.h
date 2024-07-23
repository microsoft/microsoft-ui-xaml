// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MediaPlayerElement.g.h"
#include "TimedTextSource.h"

namespace DirectUI
{
    #define EVENT_DECL(EventName) \
        _Check_return_ IFACEMETHOD(add_##EventName)(_In_ EventName##EventSourceType::HandlerType* pValue, _Out_ EventRegistrationToken* ptToken) override; \
        _Check_return_ IFACEMETHOD(remove_##EventName)(_In_ EventRegistrationToken tToken) override;

    PARTIAL_CLASS(MediaPlayerElement)
    {
    public:

        MediaPlayerElement();
        ~MediaPlayerElement() override;

        _Check_return_ HRESULT get_SourceImpl(_Outptr_result_maybenull_ wmp::IMediaPlaybackSource** ppValue);
        _Check_return_ HRESULT put_SourceImpl(_In_opt_ wmp::IMediaPlaybackSource* pValue);
        _Check_return_ HRESULT get_AutoPlayImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_AutoPlayImpl(_In_ BOOLEAN value);
        _Check_return_ HRESULT get_TransportControlsImpl(_Outptr_result_maybenull_ xaml_controls::IMediaTransportControls** ppValue);
        _Check_return_ HRESULT put_TransportControlsImpl(_In_opt_ xaml_controls::IMediaTransportControls* pValue);
        _Check_return_ HRESULT SetMediaPlayerImpl(_In_ wmp::IMediaPlayer* pMediaPlayer);

        static _Check_return_ HRESULT ResolveLocalSourceUri(_In_ wf::IUriRuntimeClass* pUri, _Outptr_result_maybenull_ wf::IUriRuntimeClass **ppFileUri);

        _Check_return_ IFACEMETHOD(OnApplyTemplate)() override;

        // Prepares object's state
        _Check_return_ HRESULT PrepareState() override;
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        void CleanupDeviceRelatedResources(const bool cleanupDComp);

        // Customized methods.
        _Check_return_ HRESULT BeginInitImpl();
        _Check_return_ HRESULT EndInitImpl(_In_opt_ XamlServiceProviderContext*);

        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding
        ) final;

        _Check_return_ HRESULT OnMTCVisible(_In_ double);

        EVENT_DECL(KeyUp)
        EVENT_DECL(KeyDown)
        EVENT_DECL(GotFocus)
        EVENT_DECL(LostFocus)
        EVENT_DECL(DragStarting)
        EVENT_DECL(DropCompleted)
        EVENT_DECL(DragEnter)
        EVENT_DECL(DragLeave)
        EVENT_DECL(DragOver)
        EVENT_DECL(Drop)
        EVENT_DECL(PointerPressed)
        EVENT_DECL(PointerMoved)
        EVENT_DECL(PointerReleased)
        EVENT_DECL(PointerEntered)
        EVENT_DECL(PointerExited)
        EVENT_DECL(PointerCaptureLost)
        EVENT_DECL(PointerCanceled)
        EVENT_DECL(PointerWheelChanged)
        EVENT_DECL(Tapped)
        EVENT_DECL(DoubleTapped)
        EVENT_DECL(Holding)
        EVENT_DECL(ContextRequested)
        EVENT_DECL(ContextCanceled)
        EVENT_DECL(RightTapped)
        EVENT_DECL(ManipulationStarting)
        EVENT_DECL(ManipulationInertiaStarting)
        EVENT_DECL(ManipulationStarted)
        EVENT_DECL(ManipulationDelta)
        EVENT_DECL(ManipulationCompleted)

    protected:
        _Check_return_ HRESULT LeaveImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset) override;

    private:
        _Check_return_ HRESULT UpdateTransportControlsState();

        _Check_return_ HRESULT UpdateTimedTextSource();

        _Check_return_ HRESULT EnableTransportControls();
        _Check_return_ HRESULT DisableTransportControls();

        _Check_return_ HRESULT UpdateIsFullWindow();
        _Check_return_ HRESULT UpdateTransportControlsPresenter();

        _Check_return_ HRESULT AddToFullWindowMediaRoot();
        _Check_return_ HRESULT RemoveFromFullWindowMediaRoot();

        _Check_return_ HRESULT UpdateMediaTransportBounds();

        _Check_return_ HRESULT CreateDefaultMediaPlayer();
        _Check_return_ HRESULT UpdateSource();
        _Check_return_ HRESULT UpdateMediaPlayer();
        _Check_return_ HRESULT UpdateAutoPlay();

        _Check_return_ HRESULT SetFocusAfterEnteringFullWindowMode();

        _Check_return_ HRESULT RedirectEvents();
        _Check_return_ HRESULT RestoreEventsRedirection();

        _Check_return_ HRESULT CloseMediaPlayer(_In_opt_ wmp::IMediaPlayer* pOldMediaPlayer);

        _Check_return_ HRESULT UpdatePosterImageVisibility();

        bool IsPropertySet(KnownPropertyIndex propertyIndex);

        EventRegistrationToken m_tokLayoutBoundsChanged{0};

        ctl::ComPtr<wmp::IMediaPlayer> m_spMediaPlayer;
        ctl::ComPtr<CTimedTextSource> m_spTimedTextSource;
        bool m_bInit;
        bool m_bOwnsMediaPlayer;
    };
}
