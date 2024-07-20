// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

using unique_safearray = wil::unique_any<SAFEARRAY*, decltype(&::SafeArrayDestroy), ::SafeArrayDestroy>;

template <class TElement>
class SafeArrayAccessor
{
public:
    SafeArrayAccessor() : m_pSafeArray(nullptr), m_count(0), m_pData(nullptr)
    {
    }
    SafeArrayAccessor(const SafeArrayAccessor &) = delete;
    SafeArrayAccessor& operator = (const SafeArrayAccessor &) = delete;

    ~SafeArrayAccessor()
    {
        if (m_pSafeArray)
        {
            SafeArrayUnaccessData(m_pSafeArray);
        }
    }

    HRESULT Access(_In_ SAFEARRAY * pSafeArray, VARTYPE vtExpected)
    {
        ASSERT(m_pSafeArray == nullptr);

        if (pSafeArray == nullptr)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        m_pSafeArray = pSafeArray;

        // Check that it's a 1-D array of the expected type...
        if(SafeArrayGetDim(m_pSafeArray) != 1)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        VARTYPE actualType;
        IFC_RETURN(SafeArrayGetVartype(m_pSafeArray, &actualType));
        if(actualType != vtExpected)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        LONG lb;
        LONG ub;
        IFC_RETURN(SafeArrayGetLBound(m_pSafeArray, 1, &lb));
        IFC_RETURN(SafeArrayGetUBound(m_pSafeArray, 1, &ub));
        m_count = ub - lb + 1;

        IFC_RETURN(SafeArrayAccessData(m_pSafeArray, (void **)&m_pData));
        return S_OK;
    }

    UINT Count() const
    {
        return m_count;
    }

    TElement & operator [] (UINT i)
    {
        return m_pData[i];
    }

    TElement * Ptr()
    {
        return m_pData;
    }
private:
    SAFEARRAY * m_pSafeArray;
    UINT m_count;
    TElement * m_pData;
};

template <class T>
inline
HRESULT SafeArrayToArray(_In_ SAFEARRAY * psarray, __deref_ecount(*pCount) T ** ppArray, int *pCount, VARTYPE vt)
{
    SafeArrayAccessor<T> accessor;
    IFC_RETURN(accessor.Access(psarray, vt));
    T *pArray = new T [accessor.Count()];
    std::copy_n(accessor.Ptr(), accessor.Count(), pArray);
    *pCount = accessor.Count();
    *ppArray = pArray;
    return S_OK;
}

template<class T>
inline
HRESULT ArrayToSafeArray(_In_ T *pArray, int count, VARTYPE vt, _Outptr_ SAFEARRAY ** pResult)
{
    *pResult = nullptr;
    unique_safearray safeArray(SafeArrayCreateVector(vt, 0, count));
    IFCPTR_RETURN(safeArray.get());
    SafeArrayAccessor<T> accessor;
    IFC_RETURN(accessor.Access(safeArray.get(), vt));
    std::copy_n(pArray, count, accessor.Ptr());

    *pResult = safeArray.release();
    return S_OK;
}
