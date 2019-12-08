// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SharedHelpers.h"

// To avoid having to call a potentially throwing cppwinrt api, we call directly through the abi.
// We don't want to include the entirety of the abi headers, so we just reproduce the single interface
// that we need.
namespace ABI::Windows::ApplicationModel::Core
{
    MIDL_INTERFACE("0AACF7A4-5E1D-49DF-8034-FB6A68BC5ED1")
    ICoreApplication : public IInspectable
    {
        virtual HRESULT STDMETHODCALLTYPE get_Id(HSTRING* value) = 0;
        virtual HRESULT STDMETHODCALLTYPE add_Suspending(void* handler, EventRegistrationToken* token) = 0;
        virtual HRESULT STDMETHODCALLTYPE remove_Suspending(EventRegistrationToken token) = 0;
        virtual HRESULT STDMETHODCALLTYPE add_Resuming(void* handler, EventRegistrationToken* token) = 0;
        virtual HRESULT STDMETHODCALLTYPE remove_Resuming(EventRegistrationToken token) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_Properties(void** value) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetCurrentView(void** value) = 0;
        virtual HRESULT STDMETHODCALLTYPE Run(void* viewSource) = 0;
        virtual HRESULT STDMETHODCALLTYPE RunWithActivationFactories(void* activationFactoryCallback) = 0;
    };
}

class DispatcherHelper
{
public:

    DispatcherHelper(const winrt::DependencyObject& dependencyObject = nullptr)
    {
        if (SharedHelpers::IsDispatcherQueueAvailable())
        {
            dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
        }

        if (!dispatcherQueue)
        {
            if (dependencyObject)
            {
                coreDispatcher = dependencyObject.Dispatcher();
            }
            else if (auto currentView = TryGetCoreApplicationCurrentView())
            {
                coreDispatcher = currentView.Dispatcher();
            }
        }
    }

    winrt::CoreApplicationView TryGetCoreApplicationCurrentView()
    {
        // We could call winrt::CoreApplication::GetCurrentView() here, but that can throw in some cases. Even if we catch, it will still
        // generate exception noise in the debugger.
        winrt::CoreApplicationView view{ nullptr };
        const auto coreApplication = winrt::get_activation_factory<winrt::CoreApplication, ABI::Windows::ApplicationModel::Core::ICoreApplication>();
        const auto ignorehr = coreApplication->GetCurrentView(winrt::put_abi(view));

        return view;
    }

    void RunAsync(std::function<void()> func, bool fallbackToThisThread = false) const
    {
        if (dispatcherQueue)
        {
            auto result = dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler(func));
            if (!result)
            {
                if (fallbackToThisThread)
                {
                    func();
                }
            }
        }
        else if (coreDispatcher)
        {
            auto asyncOp = coreDispatcher.TryRunAsync(winrt::CoreDispatcherPriority::Normal, winrt::DispatchedHandler(func));

            asyncOp.Completed([func, fallbackToThisThread](auto& asyncInfo, auto& asyncStatus)
            {
                bool reRunOnThisThread = false;

                if (asyncStatus == winrt::AsyncStatus::Completed)
                {
                    auto succeeded = asyncInfo.GetResults();
                    if (!succeeded)
                    {
                        if (fallbackToThisThread)
                        {
                            reRunOnThisThread = true;
                        }
                    }
                }

                if (reRunOnThisThread)
                {
                    func();
                }
            });
        }
        else
        {
            if (fallbackToThisThread)
            {
                func();
            }
        }
    }

private:
    winrt::Windows::System::DispatcherQueue dispatcherQueue{ nullptr };
    winrt::CoreDispatcher coreDispatcher{ nullptr };
};
