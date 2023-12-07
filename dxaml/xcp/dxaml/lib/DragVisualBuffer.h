// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class DragVisualBuffer : public Microsoft::WRL::RuntimeClass<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
            wsts::IBuffer,
            ::Windows::Storage::Streams::IBufferByteAccess,
            Microsoft::WRL::FtmBase>
{
    HRESULT STDMETHODCALLTYPE Buffer(_Outptr_ byte **value) override
    {
        *value = _myBuffer;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(_Outptr_ HSTRING *name) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetTrustLevel(_Outptr_ TrustLevel *trustLevel) override
    {    
        if(!trustLevel)
        {
            return E_INVALIDARG;
        }

        *trustLevel = TrustLevel::BaseTrust;    
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_Capacity(_Outptr_ UINT32 *capacity) override
    {
        *capacity = _capacity;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_Length(_Outptr_ UINT32 *length) override
    {
        // This is a fixed buffer, so length is capacity.
        *length = _capacity;
        return S_OK;
    }
            
    HRESULT STDMETHODCALLTYPE put_Length(_In_ UINT32 length) override
    {
        // This is a fixed buffer that does not support updating the length.
        return E_NOTIMPL;
    }

private:
    byte* _myBuffer;
    UINT32 _capacity;

public:
    DragVisualBuffer(byte* buffer, UINT32 capacity) : _myBuffer(buffer), _capacity(capacity){};
};

