// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DXamlAsyncBase.h"
#include "CoreAsyncAction.h"
#include "FocusMovement.h"
#include "FocusMovementResult.g.h"

namespace DirectUI
{
    extern __declspec(selectany) const WCHAR FocusManagerTryMoveFocusAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Input.FocusManager.TryMoveFocusAsync";
    extern __declspec(selectany) const WCHAR FocusManagerTryFocusAsyncOperationName[] = L"Windows.Foundation.IAsyncAction Microsoft.UI.Xaml.Input.FocusManager.TryFocusAsync";

    template<typename TAsyncBaseOptions = wrl::AsyncOptions<>>
    class FocusAsyncActionBase :
        public DXamlAsyncBaseImpl<wf::IAsyncOperationCompletedHandler<xaml_input::FocusMovementResult*>,
                                    wf::IAsyncOperation<xaml_input::FocusMovementResult*>,
                                    TAsyncBaseOptions>,
        public ICoreAsyncFocusOperation
    {
        using RuntimeClassT = typename DXamlAsyncBaseImpl<wf::IAsyncOperationCompletedHandler<xaml_input::FocusMovementResult*>,
                                    wf::IAsyncOperation<xaml_input::FocusMovementResult*>,
                                    TAsyncBaseOptions>::RuntimeClassT;

        InspectableClass(wf::IAsyncOperation<xaml_input::FocusMovementResult*>::z_get_rc_name_impl(), BaseTrust);

        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<xaml_input::FocusMovementResult*>* completedHandler) override
        {
            return this->PutOnComplete(completedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_ wf::IAsyncOperationCompletedHandler<xaml_input::FocusMovementResult*>** completedHandler) override
        {
            return this->GetOnComplete(completedHandler);
        }

        IFACEMETHOD(GetResults)(_Outptr_result_maybenull_ xaml_input::IFocusMovementResult** focusMovementResult)
        {
            *focusMovementResult = nullptr;
            IFC_RETURN(m_focusMovementResult.CopyTo(focusMovementResult));
            return S_OK;
        }

        _Check_return_ HRESULT CoreSetResults(_In_ Focus::FocusMovementResult coreFocusMovementResult) override
        {
            if (m_focusMovementResult == nullptr)
            {
                IFC_RETURN(ctl::ComObject<FocusMovementResult>::CreateInstance(&m_focusMovementResult));
            }

            const bool wasMoved = coreFocusMovementResult.WasMoved() && coreFocusMovementResult.WasCanceled() == false;

            IFCFAILFAST(m_focusMovementResult->put_Succeeded(wasMoved));
            return S_OK;
        }

        bool CoreContinueAsyncAction() override { return this->CoreContinueAsyncActionImpl(); }

        void CoreFireCompletion() override { this->CoreFireCompletionImpl(); }

        void CoreReleaseRef(void) override { this->CoreReleaseRefImpl(); }

        void CoreSetError(HRESULT hr) override { this->CoreSetErrorImpl(hr); }

        const GUID GetCorrelationId() const final
        {
            return m_correlationId;
        }

    public:
        static wrl::ComPtr<FocusAsyncActionBase<TAsyncBaseOptions>> CreateFocusAsyncOperation(const GUID cid)
        {
            wrl::ComPtr<FocusAsyncActionBase<TAsyncBaseOptions>> spAsyncOperation;
            ULONG actionId = ::InterlockedIncrement(&m_uniqueAsyncActionId);

            // We pass in a nullptr for the dispatcher, so that the async operation can be completed
            // synchronously if possible. Passing a dispatcher would cause an async dispatch, even
            // if we are able to complete the operation synchronously.
            IFCFAILFAST(wrl::MakeAndInitialize<FocusAsyncActionBase<TAsyncBaseOptions>>(
                &spAsyncOperation,
                actionId,
                nullptr));

            spAsyncOperation->m_correlationId = cid;

            return spAsyncOperation;
        }

    private:
        ctl::ComPtr<FocusMovementResult> m_focusMovementResult;
        static ULONG m_uniqueAsyncActionId;
        GUID m_correlationId;
    };
}