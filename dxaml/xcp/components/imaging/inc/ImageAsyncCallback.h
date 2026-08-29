// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImageProviderInterfaces.h"

template< typename T >
class ImageAsyncCallback : public CXcpObjectBase< IImageTask >
{
    typedef _Check_return_ HRESULT (T::*TaskHandlerFunc)();

public:
    ImageAsyncCallback(
        _In_ T* callback,
        _In_ TaskHandlerFunc callbackFunc
        )
        : m_callback(callback)
        , m_callbackFunc(callbackFunc)
    {
    }

    // IImageTask
    uint64_t GetRequestId() const override { return 0; }
    _Check_return_ HRESULT Execute() override
    {
        return (m_callback->*m_callbackFunc)();
    }

protected:
    xref_ptr<T> m_callback;
    TaskHandlerFunc m_callbackFunc;
};
