// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma region Templated Method Helpers

template <class T>
HRESULT
XamlDiagnosticsHelpers::do_query_interface(
    _In_opt_ IUnknown *pIn,
    _Outptr_ T** pOut)
{
    *pOut = nullptr;

    if (pIn)
    {
        return pIn->QueryInterface(__uuidof(T), (void **)pOut);
    }

    return S_OK;
}

template <>
HRESULT
XamlDiagnosticsHelpers::do_query_interface(
    _In_opt_ IUnknown *pIn,
    _Outptr_ HSTRING* pOut)
{
    *pOut = nullptr;

    if (pIn)
    {
        wrl::ComPtr<wf::IPropertyValue> keyAsPropValue;
        wf::PropertyType type = wf::PropertyType_Empty;
        IFC_RETURN(pIn->QueryInterface(__uuidof(wf::IPropertyValue), &keyAsPropValue));
        IFC_RETURN(keyAsPropValue->get_Type(&type));

        if (type != wf::PropertyType_String)
        {
            return E_NOINTERFACE;
        }

        return keyAsPropValue->GetString(pOut);
    }

    return S_OK;
}

template <class T>
wrl::ComPtr<T>
XamlDiagnosticsHelpers::as_or_null(
    _In_opt_ IUnknown *pIn)
{
    wrl::ComPtr<T> returnVal;

    if (pIn)
    {
        IGNOREHR(pIn->QueryInterface(__uuidof(T), (void **)returnVal.GetAddressOf()));
    }

    return returnVal;
}

template <typename T, typename U>
bool
XamlDiagnosticsHelpers::is(
    _In_opt_ U *pInterface)
{
    if (pInterface)
    {
        wrl::ComPtr<T> spOut;
        if (SUCCEEDED(do_query_interface<T>(pInterface, &spOut)) && (spOut != nullptr))
        {
            return true;
        }
    }

    return false;
}

template <class T>
HRESULT
XamlDiagnosticsHelpers::WinRTCreateInstance(
    _In_ PCWSTR szType,
    _Outptr_ T** pOut)
{
    wrl::ComPtr<IActivationFactory> spActivationFactory;
    wrl_wrappers::HString strType;

    IFC_RETURN(strType.Set(szType));
    IFC_RETURN(wf::GetActivationFactory(strType.Get(), &spActivationFactory));
    IFC_RETURN(do_query_interface(spActivationFactory.Get(), pOut));

    return S_OK;
}

#pragma endregion

#pragma region Inlines and Template Implementations for Collection

template <class T>
HRESULT
Collection<T>::Initialize(
    _In_ const std::vector<T>& other)
{
    m_vector = other;

    return S_OK;
}

template <class T>
HRESULT
Collection<T>::Append(
_In_ const T& item)
{
    m_vector.push_back(item);

    return S_OK;
}

template <class T>
HRESULT
Collection<T>::Append(
    _In_ T&& item)
{
    m_vector.push_back(std::forward<T>(item));

    return S_OK;
}

// Get the underlying vector as a view.
template <class T>
inline const std::vector<T>&
Collection<T>::GetVectorView() const
{
    return m_vector;
}

// Get the underlying vector
template <class T>
inline std::vector<T>&
Collection<T>::GetVector()
{
    return m_vector;
}


template <class T>
inline size_t
Collection<T>::GetSize() const
{
    // std::list::size() has a no-throw guarantee.
    return m_vector.size();
}

template <class T>
inline void
Collection<T>::Clear()
{
    // clear() has a no-throw guarantee.
    m_vector.clear();
}

#pragma endregion

#pragma region SafeArrayWrapper

template <class T>
SafeArrayPtr<T>::SafeArrayPtr(_In_ size_t size)
    : m_ptr(nullptr)
{
    m_ptr = SafeArrayCreateVector(SafeArrayAutomationType<T>::type, 0, size);
    InternalLock();
}

template <class T>
SafeArrayPtr<T>::~SafeArrayPtr()
{
    InternalDestroy();
}

template <class T>
T&
SafeArrayPtr<T>::operator[](size_t index)
{
    return InternalGetAt(index);
}

template <class T>
SAFEARRAY*
SafeArrayPtr<T>::Detach()
{
    auto ptr = m_ptr;

    InternalUnlock();

    m_ptr = nullptr;

    return ptr;
}

template <class T>
void
SafeArrayPtr<T>::InternalLock()
{
    SafeArrayLock(m_ptr);
}

template <class T>
void
SafeArrayPtr<T>::InternalUnlock()
{
    SafeArrayUnlock(m_ptr);
}

template <class T>
void 
SafeArrayPtr<T>::InternalDestroy()
{
    InternalUnlock();
    SafeArrayDestroy(m_ptr);
}

template <class T>
T&
SafeArrayPtr<T>::InternalGetAt(size_t index)
{
    return (static_cast<T*>(m_ptr->pvData))[index];
}

#pragma endregion
