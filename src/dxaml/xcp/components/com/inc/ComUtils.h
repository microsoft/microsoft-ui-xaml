// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace ctl
{
    template <typename T>
    class ComPtr;

    class ComBase;

    inline IUnknown *iunknown_cast(_In_opt_ IUnknown* pInterface)
    {
        return pInterface;
    }

    inline IInspectable *iinspectable_cast(_In_opt_ IInspectable* pInterface)
    {
        return pInterface;
    }

    // Allows old code to run
    // NOTE: This template is deprecated, do not use
    template <typename T>
    IUnknown* as_iunknown(_In_opt_ T* pInterface)
    {
        return iunknown_cast(pInterface);
    }

    // Allows old code to run
    // NOTE: This template is deprecated do not use
    template <typename T>
    IInspectable* as_iinspectable(_In_opt_ T* pInterface)
    {
        return iinspectable_cast(pInterface);
    }

    // NOTE: Use only with raw pointers. Assigning the output to a ComPtr with = will cause it to leak, since we already added a ref in this method.
    template <typename T, typename U>
    T *query_interface(_In_opt_ U *pOther)
    {
        HRESULT hr = S_OK;
        T *pCasted = NULL;

        if (pOther)
        {
            hr = iunknown_cast(pOther)->QueryInterface(__uuidof(T), (void **)&pCasted);
            if (SUCCEEDED(hr))
            {
                return pCasted;
            }
        }

        return NULL;
    }

    template <typename T, typename U>
    ComPtr<T> query_interface_cast(_In_ U* pIn)
    {
        HRESULT hr = S_OK;
        T* pCasted = NULL;
        ComPtr<T> result;

        if (pIn)
        {
            hr = iunknown_cast(pIn)->QueryInterface(__uuidof(T), (void **)&pCasted);
            if (SUCCEEDED(hr))
            {
                result.Attach(pCasted);
            }
        }

        return result;
    }

    template <typename T, typename U>
    T* query_interface_cast_noref(_In_ U* pIn)
    {
        ComPtr<T> result = query_interface_cast<T>(pIn);
        return result.Get();
    }

    template <typename T, typename U>
    HRESULT do_query_interface(_Out_ T*& pOut, _In_opt_ U *pIn)
    {
        HRESULT hr = S_OK;
        pOut = nullptr;
        
        if (pIn)
        {
            hr = iunknown_cast(pIn)->QueryInterface(__uuidof(T), (void **)&pOut);
        }

        return hr;
    }

    template <typename T, typename U>
    HRESULT do_query_interface(_Out_ ctl::ComPtr<T>& spOut, _In_opt_ U *pIn)
    {
        HRESULT hr = S_OK;
        T* pRawInterface = NULL;
        ComPtr<T> spInterface;

        if (pIn)
        {
            hr = iunknown_cast(pIn)->QueryInterface(__uuidof(T), (void **)&pRawInterface);
        }

        spInterface.Attach(pRawInterface);
        spOut = std::move(spInterface);

        return hr;
    }

    template <typename T>
    ULONG addref_interface(_In_opt_ T* pInstance)
    {
        ULONG count = 0;
        IUnknown *pUnk = iunknown_cast(pInstance);

        if (pUnk)
        {
            count = pUnk->AddRef();
        }

        return count;
    }

    template <class T>
    ULONG release_interface(_Inout_opt_ T*& pInterface)
    {
        ULONG count = 0;
        IUnknown *pUnk = iunknown_cast(pInterface);

        if (pUnk)
        {
            count = pUnk->Release();
        }

        pInterface = NULL;
        return count;
    }

    template <class T>
    ULONG release_interface_nonull(_In_opt_ T* pInterface)
    {
        ULONG count = 0;
        IUnknown *pUnk = iunknown_cast(pInterface);

        if (pUnk)
        {
            count = pUnk->Release();
        }

        return count;
    }

    template <typename T, typename U>
    bool is(_In_opt_ U *pInterface)
    {
        T* pOut = NULL;
        bool bReturnValue = false;
        if(pInterface)
        {
            pOut = ctl::query_interface<T>(pInterface);
            bReturnValue = pOut != NULL;
            ReleaseInterface(pOut);
        }
        return bReturnValue;
    }

    template <typename T, typename U>
    bool is(_In_ ComPtr<U>& spInterface)
    {
        return is<T>(spInterface.Get());
    }

    // Helpers for values in the IPropertyValue typed as IInspectable
    _Check_return_ HRESULT do_get_property_type(_In_ IInspectable *pIn, _Out_ wf::PropertyType* pType);
    wf::IPropertyValue* get_property_value(_In_ IInspectable *pIn);

    // NOTE: Consider using do_query_interface<T> directly.
    template <class T>
    _Check_return_ HRESULT do_get_value(_Out_ T*& pOut, _In_ IInspectable *pIn)
    {
        RRETURN(do_query_interface(pOut, pIn));
    }

    _Check_return_ HRESULT do_get_value(_Out_ wf::DateTime& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ wf::TimeSpan& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ HSTRING& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ BOOLEAN& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ INT& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ UINT& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ INT64& pOut, _In_ IInspectable *pIn);
    _Check_return_ HRESULT do_get_value(_Out_ UINT64& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ FLOAT& pOut, _In_ IInspectable *pIn);

    _Check_return_ HRESULT do_get_value(_Out_ DOUBLE& pOut, _In_ IInspectable *pIn);

    // NOTE: Consider using query_interface<T> directly.
    template <class T>
    T *get_value_as(_In_opt_ IInspectable *pIn)
    {
        return query_interface<T>(pIn);
    }

    // NOTE: Consider using is<T> directly.
    template <class T>
    bool value_is(_In_opt_ IInspectable *pIn)
    {
        return ctl::is<T>(pIn);
    }

    _Check_return_ HRESULT are_equal(
        _In_ IUnknown *pFirst,
        _In_ IUnknown *pSecond,
        _Out_ bool *pAreEqual);

    _Check_return_ bool are_equal(_In_ IUnknown* pFirst, _In_ IUnknown* pSecond);

    template <typename T, typename U>
    _Check_return_ bool are_equal(_In_ T* pFirst, _In_ U* pSecond)
    {
        return are_equal(ctl::iunknown_cast(pFirst), ctl::iunknown_cast(pSecond));
    }
}
