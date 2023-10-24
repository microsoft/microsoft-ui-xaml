// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FrameworkApplication.g.h>
#include <XamlIslandRootCollection.g.h>
#include <MetadataResetter.h>
#include "theming\inc\Theme.h"

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft::Windows::ApplicationModel::Resources {
    struct IResourceManager;
}
XAML_ABI_NAMESPACE_END

namespace XAML { namespace PLM {
    class PLMHandler;
} }

namespace DirectUI
{
    class Window;
    PARTIAL_CLASS(FrameworkApplication)
    {
        enum RequestedThemeSetState
        {
            RequestedThemeSetState_Settable,
            RequestedThemeSetState_Unsettable,
        };

    public:
        static _Check_return_ HRESULT GlobalInit();
        static void GlobalDeinit();
        static _Check_return_ HRESULT MainASTAInitialize();

        // does NOT addref the return value
        // may return NULL if no application singleton has been established
        static FrameworkApplication* GetCurrentNoRef();
        static void ReleaseCurrent();
        static bool InitializeFromPreviousApplication();

        // In cases where the app keeps an Application instance alive beyond deinitializing Xaml, this checks whether
        // the instance is still alive when Xaml is reinitialized. If it is, we can reuse it as Application.Current.
        static FrameworkApplication* GetAndClearPreviousApplicationInstance();

        static _Check_return_ HRESULT LoadComponent(
            _In_ IInspectable* pComponent,
            _In_ const xstring_ptr& strUri,
            xaml_primitives::ComponentResourceLocation resourceLocation = xaml_primitives::ComponentResourceLocation_Application);

        static _Check_return_ HRESULT OnSetRequestedTheme();

        // Returns the xaml::Application::FocusVisualKind property value for the current FrameworkApplication instance.
        static FocusVisualKind GetFocusVisualKind();

        static Theming::Theme GetApplicationRequestedTheme();

        static _Check_return_ HRESULT GetApplicationHighContrastAdjustment(_Out_ ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment);

        std::shared_ptr<MetadataResetter> GetMetadataReference();

        AppPolicyWindowingModel GetAppPolicyWindowingModel() { return m_appPolicyWindowingModel; }

        _Check_return_ HRESULT RaiseUnhandledExceptionEvent(_In_ HRESULT hrToReport, _In_opt_ HSTRING hstrMessage, _Inout_ bool* pfHandled);

        _Check_return_ HRESULT RaiseResourceManagerRequestedEvent(_Out_ wrl::ComPtr<mwar::IResourceManager>& resourceManager);

        _Check_return_ HRESULT get_ResourcesImpl(_Outptr_ xaml::IResourceDictionary** pValue);
        _Check_return_ HRESULT put_ResourcesImpl(_In_ xaml::IResourceDictionary* value);
        _Check_return_ HRESULT get_DebugSettingsImpl(_Outptr_ xaml::IDebugSettings** pvalue);

        _Check_return_ HRESULT ExitImpl();

        _Check_return_ HRESULT DispatchGenericActivation(_In_ IInspectable* args);

        _Check_return_ HRESULT OnActivatedImpl(_In_ waa::IActivatedEventArgs* args);
        _Check_return_ HRESULT OnLaunchedImpl(_In_ xaml::ILaunchActivatedEventArgs* args);
        _Check_return_ HRESULT OnFileActivatedImpl(_In_ waa::IFileActivatedEventArgs* args);
        _Check_return_ HRESULT OnSearchActivatedImpl(_In_ waa::ISearchActivatedEventArgs* args);
        _Check_return_ HRESULT OnShareTargetActivatedImpl(_In_ waa::IShareTargetActivatedEventArgs* args);
        _Check_return_ HRESULT OnFileOpenPickerActivatedImpl(_In_ waa::IFileOpenPickerActivatedEventArgs* args);
        _Check_return_ HRESULT OnFileSavePickerActivatedImpl(_In_ waa::IFileSavePickerActivatedEventArgs* args);
        _Check_return_ HRESULT OnCachedFileUpdaterActivatedImpl(_In_ waa::ICachedFileUpdaterActivatedEventArgs* args);
        _Check_return_ HRESULT OnBackgroundActivatedImpl(_In_ waa::IBackgroundActivatedEventArgs* pArgs);
        _Check_return_ HRESULT DispatchBackgroundActivated(_In_ IInspectable* pComponent, _In_ waa::IBackgroundActivatedEventArgs* pArgs);

        virtual _Check_return_ HRESULT OnWindowCreatedImpl(_In_ xaml::IWindowCreatedEventArgs* args);

        IFACEMETHOD(add_Suspending)(_In_ xaml::ISuspendingEventHandler* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_Suspending)(_In_ EventRegistrationToken tToken) override;

        IFACEMETHOD(add_Resuming)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_Resuming)(_In_ EventRegistrationToken token) override;

        IFACEMETHOD(add_LeavingBackground)(_In_ xaml::ILeavingBackgroundEventHandler* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_LeavingBackground)(_In_ EventRegistrationToken tToken) override;

        IFACEMETHOD(add_EnteredBackground)(_In_ xaml::IEnteredBackgroundEventHandler* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_EnteredBackground)(_In_ EventRegistrationToken tToken) override;

        IFACEMETHOD(add_UnhandledException)(_In_ xaml::IUnhandledExceptionEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_UnhandledException)(_In_ EventRegistrationToken token) override;

        _Check_return_ HRESULT STDMETHODCALLTYPE add_ResourceManagerRequested(_In_ wf::ITypedEventHandler<IInspectable*, xaml::ResourceManagerRequestedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        _Check_return_ HRESULT STDMETHODCALLTYPE remove_ResourceManagerRequested(_In_ EventRegistrationToken token) override;

        _Check_return_ HRESULT InvokeOnWindowCreated();
        _Check_return_ HRESULT InvokeOnLaunchActivated(_In_ waa::ILaunchActivatedEventArgs* uwpLaunchActivatedEventArgs);

        void SetRequestedThemeNotSettable()
        {
            m_isRequestedThemeSettable = false;
        }

        void SetRequiresPointerModeNotSettable()
        {
            m_isRequiresPointerModeSettable = false;
        }

        const xstring_ptr& GetAppXamlPath() const
        {
            return m_appXamlPath;
        }

        _Check_return_ HRESULT get_RequestedThemeImpl(_Out_ xaml::ApplicationTheme* pValue);
        _Check_return_ HRESULT put_RequestedThemeImpl(_In_ xaml::ApplicationTheme value);

        _Check_return_ HRESULT get_RequiresPointerModeImpl(_Out_ xaml::ApplicationRequiresPointerMode* pValue);
        _Check_return_ HRESULT put_RequiresPointerModeImpl(_In_ xaml::ApplicationRequiresPointerMode value);

        _Check_return_ HRESULT get_FocusVisualKindImpl(_Out_ xaml::FocusVisualKind* pValue);
        _Check_return_ HRESULT put_FocusVisualKindImpl(_In_ xaml::FocusVisualKind value);

        _Check_return_ HRESULT StartOnCurrentThreadImpl(_In_ xaml::IApplicationInitializationCallback* pCallback);
        _Check_return_ HRESULT CreateIslandImpl(_Outptr_ xaml_hosting::IXamlIsland** returnIsland);
        _Check_return_ HRESULT CreateIslandWithContentBridgeImpl(_In_ IInspectable* owner, _In_ IInspectable* contentBridge, _Outptr_ xaml_hosting::IXamlIsland** returnValue);
        _Check_return_ HRESULT RemoveIslandImpl(_In_ xaml_hosting::IXamlIsland* value);
        _Check_return_ HRESULT SetSynchronizationWindowImpl(UINT64 commitResizeWindow);

        _Check_return_ HRESULT get_HighContrastAdjustmentImpl(_Out_ xaml::ApplicationHighContrastAdjustment* pValue);
        _Check_return_ HRESULT put_HighContrastAdjustmentImpl(_In_ xaml::ApplicationHighContrastAdjustment value);

        _Check_return_ HRESULT get_WindowsImpl(_Outptr_result_maybenull_ wfc::IVectorView<xaml::Window*>** ppValue);

        // Initialization for WinUI UWP called exclusively by FrameworkApplication::StartImpl
        static _Check_return_ HRESULT StartUWP(_In_ xaml::IApplicationInitializationCallback* pCallback);

        // Initialization for WinUI Desktop called exclusively by FrameworkApplication::StartImpl
        static _Check_return_ HRESULT StartDesktop();

    protected:
        FrameworkApplication();
        ~FrameworkApplication() override;

        _Check_return_ HRESULT Initialize() override;

    private:
        static void RunDesktopWindowMessageLoop();

        _Check_return_ HRESULT GetPLMHandlerForCallingThread(_Outptr_ XAML::PLM::PLMHandler** ppHandler);
        _Check_return_ HRESULT HookBackgroundActivationEvents(bool fRegister);

        CFTMEventSource<xaml::IUnhandledExceptionEventHandler, xaml::IApplication, xaml::IUnhandledExceptionEventArgs> m_UnhandledExceptionEventSource;
        CFTMEventSource<wf::ITypedEventHandler<IInspectable*, xaml::ResourceManagerRequestedEventArgs*>, xaml::IApplication, xaml::IResourceManagerRequestedEventArgs> m_resourceManagerRequestedEventSource;

        DebugSettings *m_pDebugSettings { nullptr };

        // Is RequiresPointerMode settable?
        bool m_isRequiresPointerModeSettable { true };
        xaml::ApplicationRequiresPointerMode m_requiresPointerMode { xaml::ApplicationRequiresPointerMode_Auto };

        // Is RequestedTheme settable?
        bool m_isRequestedThemeSettable { true };

        // Path from which to load App.xaml
        xstring_ptr m_appXamlPath;

        XAML::PLM::PLMHandler* m_pPLMHandlerForMTA { nullptr };

        std::shared_ptr<MetadataResetter> m_metadataRef;

        // Stores the current FocusVisualKind property value.
        xaml::FocusVisualKind m_focusVisualKind { xaml::FocusVisualKind_DottedLine };

        Theming::Theme m_applicationRequestedTheme { Theming::Theme::None };

        xaml::ApplicationHighContrastAdjustment m_highContrastAdjustment { xaml::ApplicationHighContrastAdjustment_Auto };

        EventRegistrationToken m_BackgroundActivatedEventToken { };

        AppPolicyWindowingModel m_appPolicyWindowingModel { AppPolicyWindowingModel_None };
    };

    class CApplicationLock
    {
        friend class FrameworkApplication;
    public:
        CApplicationLock();
        ~CApplicationLock();
        static bool IsInitialized();

    private:
        // prohibit copying and allocating on the heap
        CApplicationLock(const CApplicationLock&);
        CApplicationLock& operator=(const CApplicationLock&);
        static void* operator new(size_t);
        static void operator delete(void*);
        static void* operator new[](size_t);
        static void operator delete[](void*);

        static bool s_fStaticCSInitialized;
    };
}
