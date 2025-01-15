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

#include "ContentControl.g.h"

#define __Frame_GUID "6dd83456-8b6f-4c19-9e09-501a6de803f6"

namespace DirectUI
{
    class Frame;
    class FrameNavigationOptions;
    class NavigationTransitionInfo;

    class __declspec(novtable) FrameGenerated:
        public DirectUI::ContentControl
        , public ABI::Microsoft::UI::Xaml::Controls::IFrame
        , public ABI::Microsoft::UI::Xaml::Controls::IFramePrivate
        , public ABI::Microsoft::UI::Xaml::Controls::INavigate
    {
        friend class DirectUI::Frame;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.Frame");

        BEGIN_INTERFACE_MAP(FrameGenerated, DirectUI::ContentControl)
            INTERFACE_ENTRY(FrameGenerated, ABI::Microsoft::UI::Xaml::Controls::IFrame)
            INTERFACE_ENTRY(FrameGenerated, ABI::Microsoft::UI::Xaml::Controls::IFramePrivate)
            INTERFACE_ENTRY(FrameGenerated, ABI::Microsoft::UI::Xaml::Controls::INavigate)
        END_INTERFACE_MAP(FrameGenerated, DirectUI::ContentControl)

    public:
        FrameGenerated();
        ~FrameGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Navigation::INavigationEventArgs> NavigatedEventSourceType;
        typedef CEventSource<ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventArgs> NavigatingEventSourceType;
        typedef CEventSource<ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventArgs> NavigationFailedEventSourceType;
        typedef CEventSource<ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Navigation::INavigationEventArgs> NavigationStoppedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Frame;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::Frame;
        }

        // Properties.
        IFACEMETHOD(get_BackStack)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Navigation::PageStackEntry*>** ppValue) override;
        IFACEMETHOD(get_BackStackDepth)(_Out_ INT* pValue) override;
        _Check_return_ HRESULT put_BackStackDepth(_In_ INT value);
        IFACEMETHOD(get_CacheSize)(_Out_ INT* pValue) override;
        IFACEMETHOD(put_CacheSize)(_In_ INT value) override;
        IFACEMETHOD(get_CanGoBack)(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT put_CanGoBack(_In_ BOOLEAN value);
        IFACEMETHOD(get_CanGoForward)(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT put_CanGoForward(_In_ BOOLEAN value);
        IFACEMETHOD(get_CurrentSourcePageType)(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue) override;
        _Check_return_ HRESULT put_CurrentSourcePageType(_In_ ABI::Windows::UI::Xaml::Interop::TypeName value);
        IFACEMETHOD(get_ForwardStack)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Navigation::PageStackEntry*>** ppValue) override;
        IFACEMETHOD(get_IsNavigationStackEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsNavigationStackEnabled)(_In_ BOOLEAN value) override;
        IFACEMETHOD(get_SourcePageType)(_Out_ ABI::Windows::UI::Xaml::Interop::TypeName* pValue) override;
        IFACEMETHOD(put_SourcePageType)(_In_ ABI::Windows::UI::Xaml::Interop::TypeName value) override;

        // Events.
        _Check_return_ HRESULT GetNavigatedEventSourceNoRef(_Outptr_ NavigatedEventSourceType** ppEventSource);
        IFACEMETHOD(add_Navigated)(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigatedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Navigated)(_In_ EventRegistrationToken token) override;
        _Check_return_ HRESULT GetNavigatingEventSourceNoRef(_Outptr_ NavigatingEventSourceType** ppEventSource);
        IFACEMETHOD(add_Navigating)(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigatingCancelEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Navigating)(_In_ EventRegistrationToken token) override;
        _Check_return_ HRESULT GetNavigationFailedEventSourceNoRef(_Outptr_ NavigationFailedEventSourceType** ppEventSource);
        IFACEMETHOD(add_NavigationFailed)(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigationFailedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_NavigationFailed)(_In_ EventRegistrationToken token) override;
        _Check_return_ HRESULT GetNavigationStoppedEventSourceNoRef(_Outptr_ NavigationStoppedEventSourceType** ppEventSource);
        IFACEMETHOD(add_NavigationStopped)(_In_ ABI::Microsoft::UI::Xaml::Navigation::INavigationStoppedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_NavigationStopped)(_In_ EventRegistrationToken token) override;

        // Methods.
        IFACEMETHOD(GetNavigationState)(_Out_ HSTRING* pReturnValue) override;
        IFACEMETHOD(GetNavigationTransitionInfoOverride)(_Outptr_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo** ppInfoOverride, _Out_ BOOLEAN* pIsBackNavigation, _Out_ BOOLEAN* pIsInitialPage) override;
        IFACEMETHOD(GoBack)() override;
        IFACEMETHOD(GoBackWithTransitionInfo)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pTransitionInfoOverride) override;
        IFACEMETHOD(GoForward)() override;
        IFACEMETHOD(Navigate)(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(Navigate)(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(NavigateToType)(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _In_opt_ ABI::Microsoft::UI::Xaml::Navigation::IFrameNavigationOptions* pNavigationOptions, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(NavigateWithTransitionInfo)(_In_ ABI::Windows::UI::Xaml::Interop::TypeName sourcePageType, _In_opt_ IInspectable* pParameter, _In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pInfoOverride, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(SetNavigationState)(_In_ HSTRING navigationState) override;
        IFACEMETHOD(SetNavigationStateWithNavigationControl)(_In_ HSTRING navigationState, _In_ BOOLEAN suppressNavigate) override;
        IFACEMETHOD(SetNavigationTransitionInfoOverride)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::INavigationTransitionInfo* pInfoOverride) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        _Check_return_ HRESULT EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override;
        _Check_return_ HRESULT EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler) override;

    private:

        // Fields.
    };
}

#include "Frame_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) FrameFactory:
       public ctl::BetterAggregableCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IFrameFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IFrameStatics
    {
        BEGIN_INTERFACE_MAP(FrameFactory, ctl::BetterAggregableCoreObjectActivationFactory)
            INTERFACE_ENTRY(FrameFactory, ABI::Microsoft::UI::Xaml::Controls::IFrameFactory)
            INTERFACE_ENTRY(FrameFactory, ABI::Microsoft::UI::Xaml::Controls::IFrameStatics)
        END_INTERFACE_MAP(FrameFactory, ctl::BetterAggregableCoreObjectActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IFrame** ppInstance);

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_CacheSizeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_CanGoBackProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_CanGoForwardProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_CurrentSourcePageTypeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_SourcePageTypeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_BackStackDepthProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_BackStackProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ForwardStackProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsNavigationStackEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Frame;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}