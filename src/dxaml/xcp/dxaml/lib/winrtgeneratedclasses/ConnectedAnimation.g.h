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


#define __ConnectedAnimation_GUID "dfe8acc7-3e69-485f-b327-4c4b00155c75"

namespace DirectUI
{
    class ConnectedAnimation;
    class ConnectedAnimationConfiguration;
    class UIElement;

    class __declspec(novtable) ConnectedAnimationGenerated:
        public DirectUI::DependencyObject
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IConnectedAnimation
    {
        friend class DirectUI::ConnectedAnimation;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.ConnectedAnimation");

        BEGIN_INTERFACE_MAP(ConnectedAnimationGenerated, DirectUI::DependencyObject)
            INTERFACE_ENTRY(ConnectedAnimationGenerated, ABI::Microsoft::UI::Xaml::Media::Animation::IConnectedAnimation)
        END_INTERFACE_MAP(ConnectedAnimationGenerated, DirectUI::DependencyObject)

    public:
        ConnectedAnimationGenerated();
        ~ConnectedAnimationGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Animation::ConnectedAnimation*, IInspectable*>, ABI::Microsoft::UI::Xaml::Media::Animation::IConnectedAnimation, IInspectable> CompletedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ConnectedAnimation;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ConnectedAnimation;
        }

        // Properties.
        IFACEMETHOD(get_Configuration)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::Animation::IConnectedAnimationConfiguration** ppValue) override;
        IFACEMETHOD(put_Configuration)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::IConnectedAnimationConfiguration* pValue) override;
        IFACEMETHOD(get_IsScaleAnimationEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsScaleAnimationEnabled)(_In_ BOOLEAN value) override;

        // Events.
        _Check_return_ HRESULT GetCompletedEventSourceNoRef(_Outptr_ CompletedEventSourceType** ppEventSource);
        IFACEMETHOD(add_Completed)(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Media::Animation::ConnectedAnimation*, IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Completed)(_In_ EventRegistrationToken token) override;

        // Methods.
        IFACEMETHOD(Cancel)() override;
        IFACEMETHOD(SetAnimationComponent)(_In_ ABI::Microsoft::UI::Xaml::Media::Animation::ConnectedAnimationComponent component, _In_opt_ ABI::Microsoft::UI::Composition::ICompositionAnimationBase* pAnimation) override;
        IFACEMETHOD(TryStart)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pDestination, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(TryStartWithCoordinatedElements)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pDestination, _In_ ABI::Windows::Foundation::Collections::IIterable<ABI::Microsoft::UI::Xaml::UIElement*>* pCoordinatedElements, _Out_ BOOLEAN* pReturnValue) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ConnectedAnimation_Partial.h"

