// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageProviderInterfaces.h"

template< typename T >
class ImageAvailableCallback : public CXcpObjectBase< IImageAvailableCallback >
{
    typedef _Check_return_ HRESULT(T::*OnAvailableFunc)(
        _In_ IImageAvailableResponse* pResponse
        );

public:
    ImageAvailableCallback(
        _In_ T* pCallback,
        _In_opt_ OnAvailableFunc pCallbackFunc
        )
        : m_pCallbackWeakRef(xref::get_weakref(pCallback))
        , m_pCallbackFunc(pCallbackFunc)
    {
    }

    _Check_return_ HRESULT OnImageAvailable(
        _In_ IImageAvailableResponse* pResponse
        ) override
    {
        if (m_pCallbackFunc != nullptr)
        {
            if (auto callback = m_pCallbackWeakRef.lock())
            {
                IFC_RETURN((callback->*m_pCallbackFunc)(pResponse));
            }
        }

        return S_OK;
    }

private:
    xref::weakref_ptr<T> m_pCallbackWeakRef;

    OnAvailableFunc m_pCallbackFunc;
};
