// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DXamlAsyncBase.h"


namespace DirectUI
{
    typedef wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*> TextAlternativesOperationCompletedHandler;
    class TextAlternativesOperation:
        public DXamlAsyncBaseImpl<TextAlternativesOperationCompletedHandler, wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>>
    {
        friend class TextBox;
        friend class RichEditBox;

        InspectableClass(wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>::z_get_rc_name_impl(), BaseTrust);

        private:
            // Used to assign unique ids to AsyncActions
            static ULONG z_ulUniqueAsyncActionId;

            wfc::IVectorView<HSTRING>* m_pAlternatives{};

            wrl_wrappers::HString m_compositionString;
            wrl_wrappers::HString m_prefixString;
            wrl_wrappers::HString m_postfixString;
            wrl_wrappers::HString m_currentInputMethodLanguageTag;

        public:
            IFACEMETHOD(put_Completed)(_In_ TextAlternativesOperationCompletedHandler *pCompletedHandler) override
            {
                return __super::PutOnComplete(pCompletedHandler);
            }

            IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  TextAlternativesOperationCompletedHandler **ppCompletedHandler) override
            {
                return AsyncBase::GetOnComplete(ppCompletedHandler);
            }

            IFACEMETHOD(GetResults) (
                _Inout_ wfc::IVectorView<HSTRING>* *results) override;

            TextAlternativesOperation();

            _Check_return_ HRESULT Init(_In_ CTextBoxBase* pOwner);

            void SetResults(_In_ wfc::IVectorView<HSTRING>* results);

            _Check_return_ HRESULT SetEmptyResults();

            STDMETHOD(RuntimeClassInitialize)(void)
            {
                HRESULT hr = S_OK;
                auto id = InterlockedIncrement(&z_ulUniqueAsyncActionId);
                AsyncBase::put_Id(id);
                return hr;
            }

        protected:
            // wf::IAsyncAction overrides
            HRESULT OnStart(void) override;
            void OnClose(void) override;
            void OnCancel(void) override;

        private:
            _Check_return_ HRESULT GetInputLanguage(_Outptr_ HSTRING* pLanguage);
            _Check_return_ HRESULT GetLinguisticAlternativesAsyncImpl(
                _Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** returnValue
            );
            _Check_return_ HRESULT InjectAlternateIntoFullText(_In_ HSTRING alternate, _Out_ HSTRING* concatString);
};


    class TextAlternativesAsyncCompleteHandler
        : public wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*>
        , public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(TextAlternativesAsyncCompleteHandler, ctl::ComBase)
            INTERFACE_ENTRY(TextAlternativesAsyncCompleteHandler, wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*> )
        END_INTERFACE_MAP(TextAlternativesAsyncCompleteHandler, ctl::ComBase)

        public:
            STDMETHODIMP Invoke(
                _In_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>* asyncInfo,
                _In_ wf::AsyncStatus status) override;

            void SetLinguisticCallback(
                _In_ TextAlternativesOperation* pTextAlternativesOperation);

        private:
            Microsoft::WRL::ComPtr<TextAlternativesOperation> m_spTextAlternativesOperation;


        protected:
            TextAlternativesAsyncCompleteHandler();

            ~TextAlternativesAsyncCompleteHandler() override;

            _Check_return_ HRESULT QueryInterfaceImpl(
                _In_ REFIID iid,
                _Outptr_ void** ppObject) override;
    };
}
