// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <wil/result_macros.h>

#define MakeAgileDispatcherCallback ::Microsoft::WRL::Callback<::Microsoft::WRL::Implements<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>, ::ABI::Microsoft::UI::Dispatching::IDispatcherQueueHandler, ::Microsoft::WRL::FtmBase>>

template<typename... Interfaces>
class TrackerHelperBase
    : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix | Microsoft::WRL::RuntimeClassType::InhibitWeakReference>,
    IReferenceTrackerExtension,
    Interfaces...>
{
public:
    IFACEMETHOD_(ULONG, Release)() override final
    {
        auto refCount = RuntimeClassT::InternalRelease();

        if (refCount == 0)
        {
            bool deleteThis = true;

            // Safeguard against reentrancy
            RuntimeClassT::InternalAddRef();

            if (m_pTrackerOwnerInnerNoRef)
            {
                // Try to queue the RutimeClassT::Release over to the UI thread.
                // (This will return !queued if we're already on the UI thread)

                auto queued = false;
                LOG_IF_FAILED(TryQueueForFinalRelease(&queued));
                if (queued)
                {
                    // We are queued for finalization on the UI thread.
                    // We already AddRef'ed ourselves back to a ref count of 1.
                    deleteThis = false;
                }
            }


            if (deleteThis)
            {
                // If we're truly ready to delete, proceed with the default behavior
                refCount = RuntimeClassT::Release();
            }
        }
        return refCount;
    }

protected:
    template<typename FactoryInterface>
    _Check_return_ HRESULT SetComposableBasePointers(_In_ IInspectable* base, _In_ FactoryInterface* baseFactory)
    {
        RETURN_IF_FAILED(RuntimeClassT::SetComposableBasePointers(base, baseFactory));

        WRL::ComPtr<ITrackerOwner> spTrackerOwnerInner;
        RETURN_IF_FAILED(GetComposableBase().As(&spTrackerOwnerInner));

        m_pTrackerOwnerInnerNoRef = spTrackerOwnerInner.Get();
        return S_OK;
    }

    _Check_return_ HRESULT CreateTrackerHandle(_Outptr_ TrackerHandle* pValue)
    {
        return m_pTrackerOwnerInnerNoRef->CreateTrackerHandle(pValue);
    }

    _Check_return_ HRESULT DeleteTrackerHandle(_In_ TrackerHandle value)
    {
        return m_pTrackerOwnerInnerNoRef->DeleteTrackerHandle(value);
    }

    _Check_return_ HRESULT SetTrackerValue(_In_ TrackerHandle handle, _In_opt_ IUnknown* pValue)
    {
        return m_pTrackerOwnerInnerNoRef->SetTrackerValue(handle, pValue);
    }

    _Check_return_ _Success_(return != false) boolean TryGetTrackerValue(_In_ TrackerHandle handle, _COM_Outptr_result_maybenull_ IUnknown** ppValue)
    {
        return m_pTrackerOwnerInnerNoRef->TryGetSafeTrackerValue(handle, ppValue);
    }


private:


    // Post a call to RuntimeClassT::Release to the UI thread.  If we're already on the UI thread, then just
    // return false.  If we're off the UI thread but can't get to it, then do the Release here (asynchronously).
    _Check_return_ HRESULT TryQueueForFinalRelease( _Out_ bool* queued )
    {
        *queued = false;

        WRL::ComPtr<ABI::Microsoft::UI::Xaml::IDependencyObject> trackerOwnerDO;
        WRL::ComPtr<ABI::Microsoft::UI::Dispatching::IDispatcherQueue> dispatcherQueue;
        WRL::ComPtr<ABI::Microsoft::UI::Dispatching::IDispatcherQueue2> dispatcherQueue2;
        boolean hasThreadAccess;

        // See if we're on the UI thread
        RETURN_IF_FAILED(m_pTrackerOwnerInnerNoRef->QueryInterface(IID_PPV_ARGS(&trackerOwnerDO)));
        RETURN_IF_FAILED(trackerOwnerDO->get_DispatcherQueue(&dispatcherQueue));
        RETURN_IF_FAILED(dispatcherQueue.As(&dispatcherQueue2));
        RETURN_IF_FAILED(dispatcherQueue2->get_HasThreadAccess(&hasThreadAccess));

        if(!hasThreadAccess)
        {
            // We're not on the UI thread

            auto handler = MakeAgileDispatcherCallback([this]() -> HRESULT
            {
                // This will run on the UI thread
                this->RuntimeClassT::Release();
                return S_OK;
            });

            boolean enqueueResult;
            RETURN_IF_FAILED(dispatcherQueue->TryEnqueue(handler.Get(), &enqueueResult));
            *queued = true;

            // If the post asynchronously fails (because the UI thread is gone), do the Release here.
            if (!enqueueResult)
            {
                RuntimeClassT::Release();
            }
        }

        return S_OK;
    }

private:
    ITrackerOwner* m_pTrackerOwnerInnerNoRef = nullptr;
};
