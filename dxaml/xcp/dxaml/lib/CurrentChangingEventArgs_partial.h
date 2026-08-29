// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CurrentChangingEventArgs.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(CurrentChangingEventArgs)
    {
        public:

            CurrentChangingEventArgs()
            {
                m_fCancel = false;
                m_fIsCancelable = true;
            }

        public:

            // ICollectionChangingEventArgs implementation
            _Check_return_ HRESULT get_CancelImpl(_Out_ BOOLEAN *value) 
            {
                HRESULT hr = S_OK;

                IFCPTR(value);
                *value = m_fCancel;

            Cleanup:

                RRETURN(hr);
            }
            
            _Check_return_ HRESULT put_CancelImpl(_In_ BOOLEAN value) 
            {
                HRESULT hr = S_OK;

                IFCEXPECT(m_fIsCancelable);

                m_fCancel = value;

            Cleanup:

                RRETURN(hr);
            }

            _Check_return_ HRESULT get_IsCancelableImpl(_Out_ BOOLEAN *value) 
            {
                HRESULT hr = S_OK;

                IFCPTR(value);
                *value = m_fIsCancelable;

            Cleanup:

                RRETURN(hr);
            }

        public:

            static _Check_return_ HRESULT CreateInstance(_In_ BOOLEAN isCancelable, _Outptr_ CurrentChangingEventArgs **ppArgs);

        private:

            BOOLEAN m_fCancel;
            BOOLEAN m_fIsCancelable;

            friend class CurrentChangingEventArgsFactory;
    };
}
