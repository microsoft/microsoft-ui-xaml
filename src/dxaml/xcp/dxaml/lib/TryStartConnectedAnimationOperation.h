// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DXamlAsyncBase.h"
#include "ListViewBase.g.h"


namespace DirectUI
{
    typedef wf::IAsyncOperationCompletedHandler<bool> TryStartConnectedAnimationOperationCompletedHandler;
    class TryStartConnectedAnimationOperation:
        public DXamlAsyncBaseImpl<TryStartConnectedAnimationOperationCompletedHandler, wf::IAsyncOperation<bool>>
    {
        InspectableClass(wf::IAsyncOperation<bool>::z_get_rc_name_impl(), BaseTrust);

        private:
            // Used to assign unique ids to AsyncActions
            static ULONG z_ulUniqueAsyncActionId;

            EventRegistrationToken m_eventToken;
            ctl::ComPtr<xaml_animation::IConnectedAnimation> m_animation;
            ctl::ComPtr<IInspectable> m_item;
            ctl::ComPtr<ListViewBase> m_listview;
            HSTRING m_elementName{};
            boolean m_result = false;

        public:
            TryStartConnectedAnimationOperation();

            IFACEMETHOD(put_Completed)(_In_ TryStartConnectedAnimationOperationCompletedHandler *pCompletedHandler) override
            {
                return __super::PutOnComplete(pCompletedHandler);
            }

            IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_  TryStartConnectedAnimationOperationCompletedHandler **ppCompletedHandler) override
            {
                return AsyncBase::GetOnComplete(ppCompletedHandler);
            }

            IFACEMETHOD(GetResults) (_Out_ boolean *results) override;

            _Check_return_ HRESULT InitAndStart(_In_ ListViewBase* listView, _In_ xaml_animation::IConnectedAnimation* animation, _In_ IInspectable* item, _In_ HSTRING elementName);

            STDMETHOD(RuntimeClassInitialize)(void)
            {
                auto id = InterlockedIncrement(&z_ulUniqueAsyncActionId);
                IFC_RETURN(AsyncBase::put_Id(id));
                return S_OK;
            }

        protected:
            // wf::IAsyncAction overrides
            HRESULT OnStart(void) override;
            void OnClose(void) override;
            void OnCancel(void) override;


        private:
            _Check_return_ HRESULT Clear();
            _Check_return_ HRESULT GetAnimationElement(_Outptr_ xaml::IUIElement ** animationElement);
            _Check_return_ HRESULT OnContainerContentChanging(_In_ xaml_controls::IListViewBase* sender, _In_ xaml_controls::IContainerContentChangingEventArgs* args);
    };

}
