// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "XamlDiagnosticsLauncher.h"
#include "XamlDiagnosticsTap.h"
#include <map>
#include <wil\resource.h>
#include "VisualTreeServiceCallback.h"
#include <unordered_set>
#include <XamlTailored.h>
#include <TestEvent.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

        class XamlDiagnosticsHelper
        {
        public:
            XamlDiagnosticsHelper()
            {
            }

            wrl::ComPtr<IXamlDiagnosticsTap> Connect()
            {
                LOG_OUTPUT(L"*** XamlDiagnosticsTap Connecting ***");
                wil::unique_hmodule diagModule(LoadLibrary(L"XamlDiagnosticsTap.dll"));

                WEX::Common::Throw::IfNull(diagModule.get(), L"Failed to load XamlDiagnosticsTap.dll");

                pfnGetClassObject GetClassObject = (pfnGetClassObject)GetProcAddress(diagModule.get(), "DllGetClassObject");

                wrl::ComPtr<IXamlDiagnosticsLauncherFactory> classFactory;
                LogThrow_IfFailedWithMessage(
                    GetClassObject(CLSID_XamlDiagnosticsLauncher, __uuidof(IXamlDiagnosticsLauncherFactory), reinterpret_cast<void**>(classFactory.GetAddressOf())),
                    L"Failed to get the class factory for the launcher"
                    );

                wrl::ComPtr<IXamlDiagnosticsLauncher> launcher;
                LogThrow_IfFailedWithMessage(
                    classFactory->GetLauncher(&launcher),
                    L"Failed to create the launcher from class factory");

                wrl::ComPtr<IXamlDiagnosticsTap> connectedTap;
                std::shared_ptr<Event> connectionEvent = std::make_shared<Event>();
                LogThrow_IfFailed(launcher->ConnectToVisualTree(L"Microsoft.Internal.FrameworkUdk.dll", [&connectedTap, &connectionEvent](IXamlDiagnosticsTap* tap)
                {
                    connectedTap = tap;
                    connectionEvent->Set();
                }));
                LOG_OUTPUT(L"*** Waiting for connection ***");
                connectionEvent->WaitForDefault();

                m_tap = connectedTap;
                return m_tap;
            }

            wrl::ComPtr<VisualTreeServiceCallback> Advise()
            {
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                LogThrow_IfFailed(wrl::MakeAndInitialize<VisualTreeServiceCallback>(&callback));
                LogThrow_IfFailedWithMessage(m_tap->AdviseVisualTreeChange(callback.Get()), L"Failed to Advise");

                return callback;
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper Advise(Platform::String^ xamlString, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                callback = Advise();
                return AdviseInternal(xamlString, callback);
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper AdviseOnMainQueue(Platform::String^ xamlString, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                callback = Advise();
                return AdviseInternalOnMainQueue(xamlString, callback);
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper Advise(::Windows::Foundation::Uri^ xamlUri, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                callback = Advise();
                return AdviseInternal(xamlUri, callback);
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper Advise(const std::function<UIElement ^ ()> func, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                callback = Advise();
                return AdviseInternal(func, callback);
            }

            void OnTestComplete(wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                if (m_logAllHandles)
                {
                    DumpTree(callback->GetTree());
                }
                callback->ValidateTreeState();

                m_logAllHandles = false;
                LOG_OUTPUT(L"*** Verifying Cleanup ***");
                test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

                RunOnUIThread([&]() {
                    LogThrow_IfFailed(m_tap->UnadviseVisualTreeChange(callback.Get()));
                });

                auto cleanupData = callback->VerifyTestCleanup();
                WEX::Common::Throw::IfFalse(cleanupData.second, E_UNEXPECTED, L"The cache should be empty after tearing down the tree");
            }

            void SetLogAllHandles(bool log)
            {
                m_logAllHandles = log;
            }

        private:
            typedef HRESULT(__stdcall *pfnGetClassObject)(REFCLSID, REFIID, void**);

            Microsoft::UI::Dispatching::DispatcherQueue^ GetMainQueue()
            {
                Event operationCompleted;
                Microsoft::UI::Dispatching::DispatcherQueue^ dispatcherQueue = nullptr;

                wrl::ComPtr<IInspectable> dispatcherInsp;
                LogThrow_IfFailed(m_tap->GetDispatcher(&dispatcherInsp));
                auto dispatcher = safe_cast<::Windows::UI::Core::CoreDispatcher^>(reinterpret_cast<Platform::Object^>(dispatcherInsp.Get()));

                // We always want to run this on the main ui thread regardless if we've advised after a secondary one has been created or not.
                dispatcher->RunAsync(
                    ::Windows::UI::Core::CoreDispatcherPriority::Normal, ref new ::Windows::UI::Core::DispatchedHandler([&]()
                {
                    dispatcherQueue = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
                    operationCompleted.Set();
                }));
                operationCompleted.WaitForDefault();

                return dispatcherQueue;
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper AdviseInternal(::Windows::Foundation::Uri^ xamlUri, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                // We always want to run this on the main ui thread regardless if we've advised after a secondary one has been created or not.

                RunOnUIThread([&]
                {
                    Microsoft::UI::Xaml::Controls::Page^ rootPage = ref new Microsoft::UI::Xaml::Controls::Page();
                    Microsoft::UI::Xaml::Application::LoadComponent(
                        rootPage,
                        xamlUri,
                        Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);

                    VERIFY_IS_NOT_NULL(rootPage);

                    test_infra::TestServices::WindowHelper->WindowContent = rootPage;
                    // In tests, we need to active the app since there isn't a call to DesignerAppManager.CreateNewViewAsync
                    if (::Windows::ApplicationModel::DesignMode::DesignMode2Enabled)
                    {
                        Microsoft::UI::Xaml::Window::Current->Activate();
                    }
                });

                test_infra::TestServices::WindowHelper->WaitForIdle();
                callback->ValidateTreeState();
                return Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper([&] {
                    OnTestComplete(callback);
                });
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper AdviseInternal(Platform::String^ xamlString, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                RunOnUIThread([&]
                {
                    test_infra::TestServices::WindowHelper->WindowContent = dynamic_cast<UIElement^>(xaml_markup::XamlReader::Load(xamlString));
                    // In tests, we need to active the app since there isn't a call to DesignerAppManager.CreateNewViewAsync
                    if (::Windows::ApplicationModel::DesignMode::DesignMode2Enabled)
                    {
                        Microsoft::UI::Xaml::Window::Current->Activate();
                    }
                });

                test_infra::TestServices::WindowHelper->WaitForIdle();
                callback->ValidateTreeState();
                return Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper([&] {
                    OnTestComplete(callback);
                });
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper AdviseInternalOnMainQueue(Platform::String^ xamlString, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                Microsoft::UI::Xaml::Tests::Common::RunOnDispatcherThread(GetMainQueue(), true, [&]()
                {
                    test_infra::TestServices::WindowHelper->WindowContent = dynamic_cast<UIElement^>(xaml_markup::XamlReader::Load(xamlString));
                    // In tests, we need to active the app since there isn't a call to DesignerAppManager.CreateNewViewAsync
                    if (::Windows::ApplicationModel::DesignMode::DesignMode2Enabled)
                    {
                        Microsoft::UI::Xaml::Window::Current->Activate();
                    }
                });

                test_infra::TestServices::WindowHelper->WaitForIdle();
                callback->ValidateTreeState();
                return Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper([&] {
                    OnTestComplete(callback);
                });
            }

            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper AdviseInternal(const std::function<UIElement ^ ()> func, wrl::ComPtr<VisualTreeServiceCallback>& callback)
            {
                UIElement^ uielement = func();

                RunOnUIThread([&]
                {
                    test_infra::TestServices::WindowHelper->WindowContent = uielement;
                    // In tests, we need to active the app since there isn't a call to DesignerAppManager.CreateNewViewAsync
                    if (::Windows::ApplicationModel::DesignMode::DesignMode2Enabled)
                    {
                        Microsoft::UI::Xaml::Window::Current->Activate();
                    }
                });

                test_infra::TestServices::WindowHelper->WaitForIdle();
                callback->ValidateTreeState();

                return Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper([&] {
                    OnTestComplete(callback);
                });
            }

            void DumpTree(const std::multimap<InstanceHandle, VisualElement>& tree)
            {
                LOG_OUTPUT(L" >> DUMPING TREE BEFORE TEAR DOWN");
                auto roots = tree.equal_range(0u);
                for (auto& iter = roots.first; iter != roots.second; ++iter)
                {
                    const VisualElement& element = iter->second;
                    LOG_OUTPUT(L"Type: %s, Handle, 0x%X, Name: %s", element.Type, element.Handle, element.Name);
                    DumpChildren(element.Handle, tree, 1u);
                }
            }

            void DumpChildren(InstanceHandle parent, const std::multimap<InstanceHandle, VisualElement>& tree, size_t depth)
            {
                std::wstring prepend(depth, L' ');
                auto children = tree.equal_range(parent);
                for (auto& iter = children.first; iter != children.second; ++iter)
                {
                    const VisualElement& element = iter->second;
                    LOG_OUTPUT(L"%s Type: %s, Handle, 0x%I64X, Name: %s", prepend.c_str(), element.Type, element.Handle, element.Name);
                    DumpChildren(element.Handle, tree, depth+1);
                }
            }


            bool m_logAllHandles = false;
            wil::unique_hmodule m_tapModule;
            wrl::ComPtr<IXamlDiagnosticsTap> m_tap;
        };
}}}}}