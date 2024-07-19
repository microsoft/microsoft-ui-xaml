// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <ImageProviderInterfaces.h>

class ImageCopyParams;

class AsyncImageFactory final : public CXcpObjectBase< IAsyncImageFactory >
{
    public:
        static _Check_return_ HRESULT Create(
            _In_ IPALWorkItemFactory *pWorkItemFactory,
            _In_ ImageTaskDispatcher* pDispatcher,
            _Outptr_ AsyncImageFactory** ppAsyncImageFactory
            );

        _Check_return_ HRESULT CopyAsync(
            _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
            _In_ const xref_ptr<IImageDecodeCallback>& spCallback,
            _Outptr_ IPALWorkItem** ppWork
            ) override;

        _Check_return_ HRESULT Shutdown(
            );

    protected:
        AsyncImageFactory(
            );

        ~AsyncImageFactory(
            ) override;

        _Check_return_ HRESULT Initialize(
            _In_ IPALWorkItemFactory *pWorkItemFactory,
            _In_ ImageTaskDispatcher* pDispatcher
            );

        static HRESULT WorkCallback(
            _In_opt_ IObject *pData
            );

        bool m_continueDecode;

        ImageTaskDispatcher* m_pDispatcher;

        IPALWorkItemFactory *m_pWorkItemFactory;
};
