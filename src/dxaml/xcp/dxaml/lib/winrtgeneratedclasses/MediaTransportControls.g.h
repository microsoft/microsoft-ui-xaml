// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "Control.g.h"

#define __MediaTransportControls_GUID "34b383f2-569a-4b9c-840b-d8869a8ccdbc"

#pragma region forwarders
namespace ctl
{
    template<typename impl_type>
    class interface_forwarder< ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls, impl_type> final
        : public ctl::iinspectable_forwarder_base< ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls, impl_type>
    {
        impl_type* This() { return this->This_helper<impl_type>(); }
        IFACEMETHOD(get_FastPlayFallbackBehaviour)(_Out_ ABI::Microsoft::UI::Xaml::Media::FastPlayFallbackBehaviour* pValue) override { return This()->get_FastPlayFallbackBehaviour(pValue); }
        IFACEMETHOD(put_FastPlayFallbackBehaviour)(ABI::Microsoft::UI::Xaml::Media::FastPlayFallbackBehaviour value) override { return This()->put_FastPlayFallbackBehaviour(value); }
        IFACEMETHOD(get_IsCompact)(_Out_ BOOLEAN* pValue) override { return This()->get_IsCompact(pValue); }
        IFACEMETHOD(put_IsCompact)(BOOLEAN value) override { return This()->put_IsCompact(value); }
        IFACEMETHOD(get_IsFastForwardButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsFastForwardButtonVisible(pValue); }
        IFACEMETHOD(put_IsFastForwardButtonVisible)(BOOLEAN value) override { return This()->put_IsFastForwardButtonVisible(value); }
        IFACEMETHOD(get_IsFastForwardEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsFastForwardEnabled(pValue); }
        IFACEMETHOD(put_IsFastForwardEnabled)(BOOLEAN value) override { return This()->put_IsFastForwardEnabled(value); }
        IFACEMETHOD(get_IsFastRewindButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsFastRewindButtonVisible(pValue); }
        IFACEMETHOD(put_IsFastRewindButtonVisible)(BOOLEAN value) override { return This()->put_IsFastRewindButtonVisible(value); }
        IFACEMETHOD(get_IsFastRewindEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsFastRewindEnabled(pValue); }
        IFACEMETHOD(put_IsFastRewindEnabled)(BOOLEAN value) override { return This()->put_IsFastRewindEnabled(value); }
        IFACEMETHOD(get_IsNextTrackButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsNextTrackButtonVisible(pValue); }
        IFACEMETHOD(put_IsNextTrackButtonVisible)(BOOLEAN value) override { return This()->put_IsNextTrackButtonVisible(value); }
        IFACEMETHOD(get_IsPlaybackRateButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsPlaybackRateButtonVisible(pValue); }
        IFACEMETHOD(put_IsPlaybackRateButtonVisible)(BOOLEAN value) override { return This()->put_IsPlaybackRateButtonVisible(value); }
        IFACEMETHOD(get_IsPlaybackRateEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsPlaybackRateEnabled(pValue); }
        IFACEMETHOD(put_IsPlaybackRateEnabled)(BOOLEAN value) override { return This()->put_IsPlaybackRateEnabled(value); }
        IFACEMETHOD(get_IsPreviousTrackButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsPreviousTrackButtonVisible(pValue); }
        IFACEMETHOD(put_IsPreviousTrackButtonVisible)(BOOLEAN value) override { return This()->put_IsPreviousTrackButtonVisible(value); }
        IFACEMETHOD(get_IsRepeatButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsRepeatButtonVisible(pValue); }
        IFACEMETHOD(put_IsRepeatButtonVisible)(BOOLEAN value) override { return This()->put_IsRepeatButtonVisible(value); }
        IFACEMETHOD(get_IsRepeatEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsRepeatEnabled(pValue); }
        IFACEMETHOD(put_IsRepeatEnabled)(BOOLEAN value) override { return This()->put_IsRepeatEnabled(value); }
        IFACEMETHOD(get_IsSeekBarVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSeekBarVisible(pValue); }
        IFACEMETHOD(put_IsSeekBarVisible)(BOOLEAN value) override { return This()->put_IsSeekBarVisible(value); }
        IFACEMETHOD(get_IsSeekEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSeekEnabled(pValue); }
        IFACEMETHOD(put_IsSeekEnabled)(BOOLEAN value) override { return This()->put_IsSeekEnabled(value); }
        IFACEMETHOD(get_IsSkipBackwardButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSkipBackwardButtonVisible(pValue); }
        IFACEMETHOD(put_IsSkipBackwardButtonVisible)(BOOLEAN value) override { return This()->put_IsSkipBackwardButtonVisible(value); }
        IFACEMETHOD(get_IsSkipBackwardEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSkipBackwardEnabled(pValue); }
        IFACEMETHOD(put_IsSkipBackwardEnabled)(BOOLEAN value) override { return This()->put_IsSkipBackwardEnabled(value); }
        IFACEMETHOD(get_IsSkipForwardButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSkipForwardButtonVisible(pValue); }
        IFACEMETHOD(put_IsSkipForwardButtonVisible)(BOOLEAN value) override { return This()->put_IsSkipForwardButtonVisible(value); }
        IFACEMETHOD(get_IsSkipForwardEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsSkipForwardEnabled(pValue); }
        IFACEMETHOD(put_IsSkipForwardEnabled)(BOOLEAN value) override { return This()->put_IsSkipForwardEnabled(value); }
        IFACEMETHOD(get_IsStopButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsStopButtonVisible(pValue); }
        IFACEMETHOD(put_IsStopButtonVisible)(BOOLEAN value) override { return This()->put_IsStopButtonVisible(value); }
        IFACEMETHOD(get_IsStopEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsStopEnabled(pValue); }
        IFACEMETHOD(put_IsStopEnabled)(BOOLEAN value) override { return This()->put_IsStopEnabled(value); }
        IFACEMETHOD(get_IsVolumeButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsVolumeButtonVisible(pValue); }
        IFACEMETHOD(put_IsVolumeButtonVisible)(BOOLEAN value) override { return This()->put_IsVolumeButtonVisible(value); }
        IFACEMETHOD(get_IsVolumeEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsVolumeEnabled(pValue); }
        IFACEMETHOD(put_IsVolumeEnabled)(BOOLEAN value) override { return This()->put_IsVolumeEnabled(value); }
        IFACEMETHOD(get_IsZoomButtonVisible)(_Out_ BOOLEAN* pValue) override { return This()->get_IsZoomButtonVisible(pValue); }
        IFACEMETHOD(put_IsZoomButtonVisible)(BOOLEAN value) override { return This()->put_IsZoomButtonVisible(value); }
        IFACEMETHOD(get_IsZoomEnabled)(_Out_ BOOLEAN* pValue) override { return This()->get_IsZoomEnabled(pValue); }
        IFACEMETHOD(put_IsZoomEnabled)(BOOLEAN value) override { return This()->put_IsZoomEnabled(value); }
        IFACEMETHOD(get_ShowAndHideAutomatically)(_Out_ BOOLEAN* pValue) override { return This()->get_ShowAndHideAutomatically(pValue); }
        IFACEMETHOD(put_ShowAndHideAutomatically)(BOOLEAN value) override { return This()->put_ShowAndHideAutomatically(value); }
        IFACEMETHOD(Hide)() override { return This()->Hide(); }
        IFACEMETHOD(Show)() override { return This()->Show(); }
        IFACEMETHOD(add_ThumbnailRequested)(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::MediaTransportControls*, ABI::Microsoft::UI::Xaml::Media::MediaTransportControlsThumbnailRequestedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override { return This()->add_ThumbnailRequested(pValue, pToken); }
        IFACEMETHOD(remove_ThumbnailRequested)(EventRegistrationToken token) override { return This()->remove_ThumbnailRequested(token); }
    };
}
#pragma endregion

namespace DirectUI
{
    class MediaTransportControls;

    class __declspec(novtable) MediaTransportControlsGenerated:
        public DirectUI::Control
        , public ctl::forwarder_holder< ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls, MediaTransportControlsGenerated >
        , public ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener
    {
        friend class DirectUI::MediaTransportControls;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.MediaTransportControls");

        BEGIN_INTERFACE_MAP(MediaTransportControlsGenerated, DirectUI::Control)
            INTERFACE_ENTRY(MediaTransportControlsGenerated, ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls)
            INTERFACE_ENTRY(MediaTransportControlsGenerated, ABI::Microsoft::Internal::FrameworkUdk::IBackButtonPressedListener)
        END_INTERFACE_MAP(MediaTransportControlsGenerated, DirectUI::Control)

    public:
        MediaTransportControlsGenerated();
        ~MediaTransportControlsGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::MediaTransportControls*, ABI::Microsoft::UI::Xaml::Media::MediaTransportControlsThumbnailRequestedEventArgs*>, ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls, ABI::Microsoft::UI::Xaml::Media::IMediaTransportControlsThumbnailRequestedEventArgs> ThumbnailRequestedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::MediaTransportControls;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::MediaTransportControls;
        }

        // Properties.
        _Check_return_ HRESULT STDMETHODCALLTYPE get_FastPlayFallbackBehaviour(_Out_ ABI::Microsoft::UI::Xaml::Media::FastPlayFallbackBehaviour* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_FastPlayFallbackBehaviour(ABI::Microsoft::UI::Xaml::Media::FastPlayFallbackBehaviour value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsCompact(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsCompact(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsFastForwardButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsFastForwardButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsFastForwardEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsFastForwardEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsFastRewindButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsFastRewindButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsFastRewindEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsFastRewindEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsNextTrackButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsNextTrackButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsPlaybackRateButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsPlaybackRateButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsPlaybackRateEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsPlaybackRateEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsPreviousTrackButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsPreviousTrackButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsRepeatButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsRepeatButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsRepeatEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsRepeatEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSeekBarVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSeekBarVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSeekEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSeekEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSkipBackwardButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSkipBackwardButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSkipBackwardEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSkipBackwardEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSkipForwardButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSkipForwardButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsSkipForwardEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsSkipForwardEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsStopButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsStopButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsStopEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsStopEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsVolumeButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsVolumeButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsVolumeEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsVolumeEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsZoomButtonVisible(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsZoomButtonVisible(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsZoomEnabled(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_IsZoomEnabled(BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_ShowAndHideAutomatically(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE put_ShowAndHideAutomatically(BOOLEAN value);

        // Events.
        _Check_return_ HRESULT GetThumbnailRequestedEventSourceNoRef(_Outptr_ ThumbnailRequestedEventSourceType** ppEventSource);
        virtual _Check_return_ HRESULT STDMETHODCALLTYPE add_ThumbnailRequested(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::MediaTransportControls*, ABI::Microsoft::UI::Xaml::Media::MediaTransportControlsThumbnailRequestedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken);
        virtual _Check_return_ HRESULT STDMETHODCALLTYPE remove_ThumbnailRequested(EventRegistrationToken token);

        // Methods.
        _Check_return_ HRESULT STDMETHODCALLTYPE Hide();
        _Check_return_ HRESULT STDMETHODCALLTYPE OnBackButtonPressed(_Out_ BOOLEAN* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE Show();


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        _Check_return_ HRESULT EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override;
        _Check_return_ HRESULT EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler) override;

    private:

        // Fields.
    };
}

#include "MediaTransportControls_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) MediaTransportControlsFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsStatics
    {
        BEGIN_INTERFACE_MAP(MediaTransportControlsFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(MediaTransportControlsFactory, ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsFactory)
            INTERFACE_ENTRY(MediaTransportControlsFactory, ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControlsStatics)
        END_INTERFACE_MAP(MediaTransportControlsFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IMediaTransportControls** ppInstance);

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_IsZoomButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsZoomEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsFastForwardButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsFastForwardEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsFastRewindButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsFastRewindEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsStopButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsStopEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsVolumeButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsVolumeEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsPlaybackRateButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsPlaybackRateEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSeekBarVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSeekEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsCompactProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSkipForwardButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSkipForwardEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSkipBackwardButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsSkipBackwardEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsNextTrackButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsPreviousTrackButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_FastPlayFallbackBehaviourProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ShowAndHideAutomaticallyProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsRepeatEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsRepeatButtonVisibleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::MediaTransportControls;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
