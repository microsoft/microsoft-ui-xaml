// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "UIAsyncBase.h"
#include "IPickerFlyoutAsyncOperation.h"
#include <functional>
#include <WRLHelper.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
#pragma region FlyoutAsyncOperationManager Declaration

    // --------------------------------------------------------------------------------------------
    //
    //  Copyright (c) Microsoft Corporation.  All rights reserved.
    //
    //  Synopsis:
    //      Manages the creation and completion of async operations for picker flyouts, ensuring
    //      that operation state stays in sync with the state of the associated flyout.
    //
    //  Notes:
    //      This class expects not to outlive the associated flyout passed to the Initialize
    //      method. An instance may only be initialized once.
    //
    //      We handle the case where the consumer of the operation starts a new operation from the
    //      completion handler of the previous operation. This is important for support of chaining
    //      of async operations. The difficulty is that an operation may be completed before the
    //      flyout associated with that operation is closed. Calling ShowAt on the flyout in this
    //      case will no-op, and we'll be left with a dangling operation. To address this, we
    //      create a "deferred" operation in this case, which simply means that the call to ShowAt
    //      for the operation is deferred until the Closed event for the associated flyout is
    //      received. Because FlyoutBase ignores ShowAt calls until after the Closed event if both
    //      the flyout and placement target are the same as the closing flyout, we actually need
    //      to post the ShowAt to the dispatcher.
    //
    //      This added async-ness combined with cancellation handling makes the state transition
    //      logic for this manager quite complex.
    //      MODIFY WITH GREAT CARE!
    //
    // --------------------------------------------------------------------------------------------
    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    class FlyoutAsyncOperationManager
    {

    public:
        typedef std::function<TResult()> CancellationValueFunction;

        FlyoutAsyncOperationManager(Private::ReferenceTrackerHelper<TTrackerRuntimeClass> referenceTrackerHelper);

        _Check_return_ HRESULT Initialize(
            _In_ xaml_primitives::IFlyoutBase* pAssociatedFlyout,
            _In_ CancellationValueFunction cancellationValueCallback);

        _Check_return_ HRESULT Start(
            _In_ xaml::IFrameworkElement* pTarget,
            _Outptr_ wf::IAsyncOperation<TResult>** ppOperation);

        _Check_return_ HRESULT Complete(_In_ TResult result);

    private:

        _Check_return_ HRESULT OnOpening(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnClosed(_In_ IInspectable* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT BeginAttemptStartDeferredOperation();
        _Check_return_ HRESULT AttemptStartDeferredOperation();

        inline void AssertInvariants();
        inline void AssertCompleteOperationPreconditions();

        template <typename T>
        _Check_return_ HRESULT SetPtrValue(_In_ Private::TrackerPtr<T>& ptr, _In_ T* value)
        {
            RRETURN(m_referenceTrackerHelper.SetPtrValue(ptr, value));
        }

    private:

        enum FlyoutState
        {
            Closed,
            Open,
            Closing,
        };

        Private::ReferenceTrackerHelper<TTrackerRuntimeClass> m_referenceTrackerHelper;
        CancellationValueFunction m_cancellationValueCallback;
        xaml_primitives::IFlyoutBase* m_pAssociatedFlyoutNoRef;
        Private::TrackerPtr<xaml::IFrameworkElement> m_tpTargetForDeferredShowAt;
        wrl::ComPtr<xaml_controls::IPickerFlyoutAsyncOperation<TResult>> m_spCurrentOperation;
        bool m_isInitialized;
        bool m_isShowAtForCurrentOperationDeferred;
        FlyoutState m_flyoutState;
        wrl::ComPtr<msy::IDispatcherQueue> m_spDispatcherQueue;
    };

#pragma endregion FlyoutAsyncOperationManager Declaration

#pragma region FlyoutAsyncOperationManager Definition

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::FlyoutAsyncOperationManager(
        Private::ReferenceTrackerHelper<TTrackerRuntimeClass> referenceTrackerHelper) :
        m_isInitialized(false),
        m_isShowAtForCurrentOperationDeferred(false),
        m_flyoutState(FlyoutState::Closed),
        m_pAssociatedFlyoutNoRef(nullptr),
        m_referenceTrackerHelper(referenceTrackerHelper)
    {}

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::Initialize(
        _In_ xaml_primitives::IFlyoutBase* pAssociatedFlyout,
        _In_ CancellationValueFunction cancellationValueCallback)
    {
        HRESULT hr = S_OK;
        EventRegistrationToken openingToken = { };
        EventRegistrationToken closedToken = { };
        wrl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;

        IFCEXPECT(!m_isInitialized);

        m_pAssociatedFlyoutNoRef = pAssociatedFlyout;
        m_cancellationValueCallback = std::move(cancellationValueCallback);

        IFC(pAssociatedFlyout->add_Opening(
            wrl::Callback<wf::IEventHandler<IInspectable*>>
            (this, &FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::OnOpening).Get(),
            &openingToken));

        IFC(pAssociatedFlyout->add_Closed(
            wrl::Callback<wf::IEventHandler<IInspectable*>>
            (this, &FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::OnClosed).Get(),
            &closedToken));

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(), &spDispatcherQueueStatics));
        IFC(spDispatcherQueueStatics->GetForCurrentThread(&m_spDispatcherQueue));

        m_isInitialized = true;

    Cleanup:
        RRETURN(hr);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::Start(
        _In_ xaml::IFrameworkElement* pTarget,
        _Outptr_ wf::IAsyncOperation<TResult>** ppOperation)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml_controls::PickerFlyoutAsyncOperation<TResult, OpName>> spAsyncOp;

        // Validation
        IFCEXPECT(m_isInitialized);
        AssertInvariants();
        if (m_spCurrentOperation)
        {
            ROERROR_LOOKUP(E_ASYNC_OPERATION_NOT_STARTED, ERR_ASYNC_ALREADY_IN_PROGRESS);
        }
        if (m_flyoutState == FlyoutState::Open)
        {
            ROERROR_LOOKUP(E_ASYNC_OPERATION_NOT_STARTED, ERR_FLYOUT_ALREADY_OPEN);
        }

        IFC((wrl::MakeAndInitialize<xaml_controls::PickerFlyoutAsyncOperation<TResult, OpName>>(&spAsyncOp)));
        IFC(spAsyncOp->StartOperation(m_pAssociatedFlyoutNoRef));

        IFC(SetPtrValue(m_tpTargetForDeferredShowAt, pTarget));
        m_isShowAtForCurrentOperationDeferred = true;
        if (m_flyoutState == FlyoutState::Closed)
        {
            // The flyout is currently closed so we can try to show now, but we have to treat this
            // as a deferred op anyway and show asynchronously in case we're being called from the
            // closed event handler, as FlyoutBase may swallow a synchronous call to ShowAt in that case.
            IFC(BeginAttemptStartDeferredOperation());
        }

        m_spCurrentOperation = static_cast<xaml_controls::IPickerFlyoutAsyncOperation<TResult>*>(spAsyncOp.Get());
        IFC(spAsyncOp.CopyTo(ppOperation));

        AssertInvariants();

    Cleanup:
        RRETURN(hr);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::Complete(_In_ TResult result)
    {
        HRESULT hr = S_OK;

        IFCEXPECT(m_isInitialized);
        AssertInvariants();

        if (m_flyoutState == FlyoutState::Open)
        {
            // If the flyout is not already closed or closing, then we're being called
            // because the user accepted a selection, and the flyout will be closed shortly
            // by PickerFlyoutBase::OnConfirmedImpl
            m_flyoutState = FlyoutState::Closing;
        }

        if (m_spCurrentOperation)
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;

            // If an operation is deferred, then it is not associated with the current
            // open/close cycle of this flyout, and we should not complete it unless it
            // has been cancelled.
            IFC(m_spCurrentOperation.As(&spAsyncInfo));
            IFC(spAsyncInfo->get_Status(&asyncStatus));
            if (!m_isShowAtForCurrentOperationDeferred || asyncStatus == wf::AsyncStatus::Canceled)
            {
                AssertCompleteOperationPreconditions();

                // nulls out m_spCurrentOperation. This is important in the case of reentrancy;
                // the consumer's CompleteOperation handler may trigger another call to Start,
                // in which case we want Start to be able to return a deferred operation
                // rather than failing.
                wrl::ComPtr<xaml_controls::IPickerFlyoutAsyncOperation<TResult>> spCurrentOperation;
                spCurrentOperation.Swap(m_spCurrentOperation);
                m_isShowAtForCurrentOperationDeferred = false;
                m_tpTargetForDeferredShowAt.Clear();
                IFC(spCurrentOperation->CompleteOperation(result));
            }
        }

        AssertInvariants();

Cleanup:
        RRETURN(hr);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::OnOpening(
        _In_ IInspectable* /*pSender*/,
        _In_ IInspectable* /*pArgs*/)
    {
        AssertInvariants();

        m_flyoutState = FlyoutState::Open;

        AssertInvariants();

        RRETURN(S_OK);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::OnClosed(
        _In_ IInspectable* /*pSender*/,
        _In_ IInspectable* /*pArgs*/)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
        wf::AsyncStatus asyncStatus = wf::AsyncStatus::Canceled;

        AssertInvariants();

        m_flyoutState = FlyoutState::Closing;

        // We loop here because it is possible for the consumer's completion handler to cause a
        // new deferred operation to begin. If that operation is synchronously canceled for some
        // reason, we will need to complete the new operation immediately, since there will not
        // be another Closed event. And as are firing another completion handler, the situation
        // repeats itself.
        while (m_spCurrentOperation && asyncStatus == wf::AsyncStatus::Canceled)
        {
            IFC(m_spCurrentOperation.As(&spAsyncInfo));
            IFC(spAsyncInfo->get_Status(&asyncStatus));
            if (!m_isShowAtForCurrentOperationDeferred || asyncStatus == wf::AsyncStatus::Canceled)
            {
                // Two cases:
                // 1. There is a non-deferred operation associated with the closing flyout. Since
                // the operation was not completed earlier, the closing of the flying triggers
                // completion of the operation.
                // 2. There is a deferred operation, but it has already been cancelled. We can
                // complete the canceled operation immediately rather than showing the flyout again.
                IFC(Complete(m_cancellationValueCallback()));
            }

            if (m_isShowAtForCurrentOperationDeferred)
            {
                IFC(m_spCurrentOperation.As(&spAsyncInfo));
                IFC(spAsyncInfo->get_Status(&asyncStatus));
                if (asyncStatus != wf::AsyncStatus::Canceled)
                {
                    // Consumer attempted to start an operation while the flyout was closing.
                    // Show the flyout again, and clear the deferral info. We need to do this
                    // asynchronously because FlyoutBase will not accept another ShowAt until
                    // the flyout is fully closed, which happens after the Closed event.
                    IFC(BeginAttemptStartDeferredOperation());
                }
            }
        }

        if (m_flyoutState == FlyoutState::Closing)
        {
            m_flyoutState = FlyoutState::Closed;
        }

        AssertInvariants();

    Cleanup:
        RRETURN(hr);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::BeginAttemptStartDeferredOperation()
    {
        HRESULT hr = S_OK;
        boolean enqueued;

        IFC(m_spDispatcherQueue->TryEnqueue(
            WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([this]() -> HRESULT
            {
                RRETURN(this->AttemptStartDeferredOperation());
            }).Get(),
            &enqueued));
        IFCEXPECT(enqueued);

    Cleanup:
        RRETURN(hr);
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    _Check_return_ HRESULT FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::AttemptStartDeferredOperation()
    {
        HRESULT hr = S_OK;

        AssertInvariants();

        if (m_flyoutState == FlyoutState::Closed)
        {
            wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;
            wf::AsyncStatus asyncStatus = wf::AsyncStatus::Started;

            IFC(m_spCurrentOperation.As(&spAsyncInfo));
            IFC(spAsyncInfo->get_Status(&asyncStatus));

            if (asyncStatus == wf::AsyncStatus::Canceled)
            {
                IFC(Complete(m_cancellationValueCallback()));
            }
            else
            {
                ASSERT(asyncStatus == wf::AsyncStatus::Started);
                IFC(m_pAssociatedFlyoutNoRef->ShowAt(m_tpTargetForDeferredShowAt.Get()));
                m_isShowAtForCurrentOperationDeferred = false;
                m_tpTargetForDeferredShowAt.Clear();
            }
        }
        // else something else reopened the flyout before this callback had a
        // chance to run, possibly with a different placement target. In this case
        // just keep the deferred operation active and wait again for the
        // current flyout to close.

        AssertInvariants();

    Cleanup:
        RRETURN(hr);
    }

#pragma region Validation Helpers

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    inline void FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::AssertInvariants()
    {
#ifdef DBG

        ASSERT(m_spCurrentOperation || (!m_tpTargetForDeferredShowAt && !m_isShowAtForCurrentOperationDeferred), "State saved for deferred operation, but operation is null.");
        ASSERT(m_pAssociatedFlyoutNoRef || !m_isInitialized, "Operation must have an associated flyout");

#endif
    }

    template<typename TResult, typename TTrackerRuntimeClass, PCWSTR OpName>
    inline void FlyoutAsyncOperationManager<TResult, TTrackerRuntimeClass, OpName>::AssertCompleteOperationPreconditions()
    {
#ifdef DBG
        wf::AsyncStatus status;
        wrl::ComPtr<wf::IAsyncInfo> spAsyncInfo;

        ASSERT(m_spCurrentOperation, "Attempting to complete a null operation");

        ASSERTSUCCEEDED(m_spCurrentOperation.As(&spAsyncInfo));
        ASSERTSUCCEEDED(spAsyncInfo->get_Status(&status));
        ASSERT(status != wf::AsyncStatus::Completed, "Attempting to complete an operation that's already been completed.");

        ASSERT(!m_isShowAtForCurrentOperationDeferred || status == wf::AsyncStatus::Canceled, "Attempting to complete an operation but still waiting to show flyout");
#endif
    }

#pragma endregion Validation Helpers

#pragma endregion FlyoutAsyncOperationManager Definition

}}}} XAML_ABI_NAMESPACE_END


