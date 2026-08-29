// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Animation
{

    // NavigationThemeTransitionFactory

    class NavigationThemeTransitionFactory :
        public wrl::AgileActivationFactory<
            xaml_animation::INavigationThemeTransitionStatics
            >
    {
    public:
        _Check_return_ HRESULT RuntimeClassInitialize();
        static _Check_return_ HRESULT EnsureProperties();
        static void ClearProperties();

        IFACEMETHOD(ActivateInstance)(
            _Outptr_ IInspectable** ppInspectable);

        IFACEMETHOD(get_DefaultNavigationTransitionInfoProperty)(
            _Outptr_ xaml::IDependencyProperty **value);

    private:
        static _Check_return_ HRESULT InitializeDefaultNavigationTransitionInfoProperty();

        static _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyObject* pSender,
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static wrl::ComPtr<xaml::IDependencyProperty> s_spDefaultNavigationTransitionInfoProperty;
    };

    // NavigationThemeTransition

    class NavigationThemeTransition :
        public ::Microsoft::WRL::RuntimeClass<xaml_animation::ITransition, xaml_animation::ITransitionPrivate, xaml_animation::INavigationThemeTransition,
        ::Microsoft::WRL::ComposableBase<xaml_animation::ITransitionFactoryPrivate>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_NavigationThemeTransition, TrustLevel::BaseTrust);

    public:
        NavigationThemeTransition();

        _Check_return_ HRESULT RuntimeClassInitialize();

        _Check_return_ HRESULT OnPropertyChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        IFACEMETHOD(get_DefaultNavigationTransitionInfo)(_Outptr_ xaml_animation::INavigationTransitionInfo** value) override;
        IFACEMETHOD(put_DefaultNavigationTransitionInfo)(_In_ xaml_animation::INavigationTransitionInfo* value) override;

        // ITransitionPrivate members
        IFACEMETHOD(ParticipatesInTransition)(_In_ xaml::IUIElement* element, _In_ xaml::TransitionTrigger transitionTrigger, _Out_ BOOLEAN* returnValue) override;
        IFACEMETHOD(CreateStoryboard)(_In_ xaml::IUIElement* element, _In_ wf::Rect start, _In_ wf::Rect destination, _In_ xaml::TransitionTrigger transitionTrigger,  _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards, _Inout_ xaml::TransitionParent* parentForTransition) override;

    private:
        ~NavigationThemeTransition();

        _Check_return_ HRESULT GetNavigationThemeTransitionFromCollection(_In_ wfc::IVector<xaml_animation::Transition*>* transitions, _Out_ xaml_animation::INavigationThemeTransition** themeTransition);

        wrl::ComPtr<xaml_animation::INavigationTransitionInfo> m_spDefaultNavigationTransitionInfo;
    };

    ActivatableClassWithFactory(NavigationThemeTransition, NavigationThemeTransitionFactory);

    // CommonNavigationTransitionInfoFactory

    class CommonNavigationTransitionInfoFactory :
        public wrl::AgileActivationFactory<
            xaml_animation::ICommonNavigationTransitionInfoStatics
            >
    {
    public:
        _Check_return_ HRESULT RuntimeClassInitialize();
        static _Check_return_ HRESULT EnsureProperties();
        static void ClearProperties();

        IFACEMETHOD(ActivateInstance)(_Outptr_ IInspectable** ppInspectable);

        // Dependency properties.
        IFACEMETHOD(get_IsStaggeringEnabledProperty)(_Outptr_ xaml::IDependencyProperty **value);

        // Attached properties.
        IFACEMETHOD(get_IsStaggerElementProperty)(_Outptr_ xaml::IDependencyProperty** value);
        IFACEMETHOD(GetIsStaggerElement)(_In_ xaml::IUIElement* element, _Out_ boolean* value);
        IFACEMETHOD(SetIsStaggerElement)(_In_ xaml::IUIElement* element, _In_ boolean value);

        // Helpers.
        static _Check_return_ HRESULT GetStaggerElements(_Out_ std::vector<wrl::WeakRef>* elements);
        static _Check_return_ HRESULT ClearStaggerElements();

    private:
        static _Check_return_ HRESULT InitializeIsStaggeringEnabledProperty();
        static _Check_return_ HRESULT InitializeIsStaggerElementProperty();

        static _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyObject* pSender,
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static wrl::ComPtr<xaml::IDependencyProperty> s_spIsStaggeringEnabledProperty;
        static wrl::ComPtr<xaml::IDependencyProperty> s_spIsStaggerElementProperty;
        static std::vector<wrl::WeakRef> s_spStaggerElements;
    };

    // CommonNavigationTransitionInfo

    class CommonNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::ICommonNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_CommonNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        CommonNavigationTransitionInfo();

        _Check_return_ HRESULT RuntimeClassInitialize();

        _Check_return_ HRESULT OnPropertyChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        IFACEMETHOD(get_IsStaggeringEnabled)(_Out_ boolean* value) override;
        IFACEMETHOD(put_IsStaggeringEnabled)(_In_ boolean value) override;

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;

    private:
        ~CommonNavigationTransitionInfo();

        wrl::ComPtr<xaml_animation::ICommonNavigationTransitionInfoStatics> m_spTNTIStatics;
    };

    ActivatableClassWithFactory(CommonNavigationTransitionInfo, CommonNavigationTransitionInfoFactory);

    // SlideTransitionInfoFactory

    class SlideNavigationTransitionInfoFactory :
        public wrl::AgileActivationFactory<
        xaml_animation::ISlideNavigationTransitionInfoStatics
        >
    {
    public:
        _Check_return_ HRESULT RuntimeClassInitialize();
        static _Check_return_ HRESULT EnsureProperties();
        static void ClearProperties();

        IFACEMETHOD(ActivateInstance)(_Outptr_ IInspectable** ppInspectable);

        IFACEMETHOD(get_EffectProperty)(_Outptr_ xaml::IDependencyProperty **value);

    private:
        static wrl::ComPtr<xaml::IDependencyProperty> s_effectProperty;
    };

    // SlideNavigationTransitionInfo

    class SlideNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::ISlideNavigationTransitionInfo,
        xaml_animation::ISlideNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SlideNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        SlideNavigationTransitionInfo();

        _Check_return_ HRESULT RuntimeClassInitialize();

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;

        // ISlideNavigationTransitionInfo
        IFACEMETHOD(get_Effect)(_Out_ SlideNavigationTransitionEffect* value) override;
        IFACEMETHOD(put_Effect)(_In_ SlideNavigationTransitionEffect value) override;

    private:
        ~SlideNavigationTransitionInfo();
    };

    ActivatableClassWithFactory(SlideNavigationTransitionInfo, SlideNavigationTransitionInfoFactory);

    // ContinuumNavigationTransitionInfoFactory

    class ContinuumNavigationTransitionInfoFactory :
        public wrl::AgileActivationFactory<
            xaml_animation::IContinuumNavigationTransitionInfoStatics
            >
    {
    public:
        _Check_return_ HRESULT RuntimeClassInitialize();
        static _Check_return_ HRESULT EnsureProperties();
        static void ClearProperties();

        IFACEMETHOD(ActivateInstance)(_Outptr_ IInspectable** ppInspectable);

        // Dependency properties.
        IFACEMETHOD(get_ExitElementProperty)(_Outptr_ xaml::IDependencyProperty **value);

        // Attached properties.
        IFACEMETHOD(get_IsEntranceElementProperty)(_Outptr_ xaml::IDependencyProperty** value);
        IFACEMETHOD(GetIsEntranceElement)(_In_ xaml::IUIElement* element, _Out_ boolean* value);
        IFACEMETHOD(SetIsEntranceElement)(_In_ xaml::IUIElement* element, _In_ boolean value);

        IFACEMETHOD(get_IsExitElementProperty)(_Outptr_ xaml::IDependencyProperty** value);
        IFACEMETHOD(GetIsExitElement)(_In_ xaml::IUIElement* element, _Out_ boolean* value);
        IFACEMETHOD(SetIsExitElement)(_In_ xaml::IUIElement* element, _In_ boolean value);

        IFACEMETHOD(get_ExitElementContainerProperty)(_Outptr_ xaml::IDependencyProperty** value);
        IFACEMETHOD(GetExitElementContainer)(_In_ xaml_controls::IListViewBase* element, _Out_ boolean* value);
        IFACEMETHOD(SetExitElementContainer)(_In_ xaml_controls::IListViewBase* element, _In_ boolean value);

        // Helpers.
        static _Check_return_ HRESULT GetEntranceElements(_Out_ std::vector<wrl::WeakRef>* elements);
        static _Check_return_ HRESULT ClearEntranceElements();
        static _Check_return_ HRESULT GetExitElements(_Out_ std::vector<wrl::WeakRef>* elements);
        static _Check_return_ HRESULT ClearExitElements();

    private:
        static _Check_return_ HRESULT InitializeExitElementProperty();
        static _Check_return_ HRESULT InitializeIsEntranceElementProperty();
        static _Check_return_ HRESULT InitializeIsExitElementProperty();
        static _Check_return_ HRESULT InitializeExitElementContainerProperty();

        static _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyObject* pSender,
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static wrl::ComPtr<xaml::IDependencyProperty> s_spExitElementProperty;
        static wrl::ComPtr<xaml::IDependencyProperty> s_spIsEntranceElementProperty;
        static wrl::ComPtr<xaml::IDependencyProperty> s_spIsExitElementProperty;
        static wrl::ComPtr<xaml::IDependencyProperty> s_spExitElementContainerProperty;

        static std::vector<wrl::WeakRef> s_spEntranceElements;
        static std::vector<wrl::WeakRef> s_spExitElementContainers;
        static std::vector<wrl::WeakRef> s_spExitElements;
    };

    // ContinuumNavigationTransitionInfo

    class ContinuumNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::IContinuumNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ContinuumNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        ContinuumNavigationTransitionInfo();

        _Check_return_ HRESULT RuntimeClassInitialize();

        _Check_return_ HRESULT OnPropertyChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        IFACEMETHOD(get_ExitElement)(_Outptr_ xaml::IUIElement** value) override;
        IFACEMETHOD(put_ExitElement)(_In_ xaml::IUIElement* value) override;

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;
    private:
        ~ContinuumNavigationTransitionInfo();

        _Check_return_ HRESULT GetLogicalExitElement(_In_ xaml::IUIElement* page, _Outptr_result_maybenull_ xaml::IUIElement** target);

        _Check_return_ HRESULT GetLogicalEntranceElement(_In_ xaml::IUIElement* page, _Outptr_result_maybenull_ xaml::IUIElement** target);

        wrl::ComPtr<xaml_animation::IContinuumNavigationTransitionInfoStatics> m_spCNTIStatics;
    };

    ActivatableClassWithFactory(ContinuumNavigationTransitionInfo, ContinuumNavigationTransitionInfoFactory);

    // DrillInNavigationTransitionInfo

    class DrillInNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::IDrillInNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DrillInNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        DrillInNavigationTransitionInfo() { }

        _Check_return_ HRESULT RuntimeClassInitialize();

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;

    private:
        ~DrillInNavigationTransitionInfo() { }

    public:
        static const INT64 s_NavigatingAwayScaleDuration = 100;
        static const INT64 s_NavigatingAwayOpacityDuration = 100;
        static const INT64 s_NavigatingToScaleDuration = 783;
        static const INT64 s_NavigatingToOpacityDuration = 333;
        static const INT64 s_BackNavigatingAwayScaleDuration = 100;
        static const INT64 s_BackNavigatingAwayOpacityDuration = 100;
        static const INT64 s_BackNavigatingToScaleDuration = 333;
        static const INT64 s_BackNavigatingToOpacityDuration = 333;
    };

    ActivatableClass(DrillInNavigationTransitionInfo);

    // SuppressNavigationTransitionInfo

    class SuppressNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::ISuppressNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory>>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SuppressNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        SuppressNavigationTransitionInfo() { }

        _Check_return_ HRESULT RuntimeClassInitialize();

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;

    private:
        ~SuppressNavigationTransitionInfo() { }
    };

    ActivatableClass(SuppressNavigationTransitionInfo);

    // EntranceNavigationTransitionInfoFactory

    class EntranceNavigationTransitionInfoFactory :
        public wrl::AgileActivationFactory<
        xaml_animation::IEntranceNavigationTransitionInfoStatics
        >
    {
    public:
        _Check_return_ HRESULT RuntimeClassInitialize();
        static _Check_return_ HRESULT EnsureProperties();
        static void ClearProperties();

        IFACEMETHOD(ActivateInstance)(_Outptr_ IInspectable** ppInspectable);

        // Attached properties.
        IFACEMETHOD(get_IsTargetElementProperty)(_Outptr_ xaml::IDependencyProperty** value);
        IFACEMETHOD(GetIsTargetElement)(_In_ xaml::IUIElement* element, _Out_ boolean* value);
        IFACEMETHOD(SetIsTargetElement)(_In_ xaml::IUIElement* element, _In_ boolean value);

        // Helpers.
        static _Check_return_ HRESULT GetTargetElements(_Out_ std::vector<wrl::WeakRef>* elements);
        static _Check_return_ HRESULT ClearTargetElements();

    private:
        static _Check_return_ HRESULT InitializeIsTargetElementProperty();

        static _Check_return_ HRESULT OnPropertyChanged(
            _In_ xaml::IDependencyObject* pSender,
            _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        static wrl::ComPtr<xaml::IDependencyProperty> s_spIsTargetElementProperty;

        static std::vector<wrl::WeakRef> s_spTargetElements;
    };

    // EntranceNavigationTransitionInfo

    class EntranceNavigationTransitionInfo :
        public ::Microsoft::WRL::RuntimeClass<
        xaml_animation::INavigationTransitionInfo,
        xaml_animation::INavigationTransitionInfoPrivate,
        xaml_animation::INavigationTransitionInfoOverrides,
        xaml_animation::IEntranceNavigationTransitionInfo,
        ::Microsoft::WRL::ComposableBase<xaml_animation::INavigationTransitionInfoFactory >>
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EntranceNavigationTransitionInfo, TrustLevel::BaseTrust);

    public:
        EntranceNavigationTransitionInfo() { }

        _Check_return_ HRESULT RuntimeClassInitialize();

        _Check_return_ HRESULT OnPropertyChanged(_In_ xaml::IDependencyPropertyChangedEventArgs* pArgs);

        // INavigationTransitionInfoPrivate
        IFACEMETHOD(CreateStoryboards)(_In_ xaml::IUIElement* element, _In_ xaml_animation::NavigationTrigger trigger, _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards) override;

        // INavigationTransitionInfoOverrides,
        IFACEMETHOD(GetNavigationStateCore)(_Out_ HSTRING* string) override;
        IFACEMETHOD(SetNavigationStateCore)(_In_ HSTRING string) override;

    private:
        ~EntranceNavigationTransitionInfo() { }

        _Check_return_ HRESULT GetLogicalTargetElement(_In_ xaml::IUIElement* page, _Outptr_result_maybenull_ xaml::IUIElement** target);

        wrl::ComPtr<xaml_animation::IEntranceNavigationTransitionInfoStatics> m_spENTIStatics;

    public:
        static const INT64 s_Duration = 667;
    };

    ActivatableClassWithFactory(EntranceNavigationTransitionInfo, EntranceNavigationTransitionInfoFactory);

} } } } } XAML_ABI_NAMESPACE_END
