// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // RAII for Variant, ideally it will be good to be able to use _variant_t though that is
    // defined in ComUtil which exist in comsuppw.lib, It was not clear if this is OneCore or
    // has corresponding lib on APISet. Also at this time keeping this class only as minimal wrapper.
    class AutoVariant
    {
    public:

        AutoVariant()
        {
            VariantInit(&m_vt);
        }

        ~AutoVariant()
        {
            VariantClear(&m_vt);
        }

        VARIANT* ReleaseAndGetAddressOf()
        {
            VariantClear(&m_vt);
            return &m_vt;
        }

        VARIANT& Get() { return m_vt; }

        VARIANT* Storage() { return &m_vt; }

        void SetString(const wchar_t* pSrc)
        {
            wchar_t* tmp = ::SysAllocString(pSrc);

            if (tmp == nullptr && pSrc != nullptr) {
                LogThrow_IfFailed(E_OUTOFMEMORY);
            }
            else {
                VariantClear(&m_vt);
                V_VT(&m_vt) = VT_BSTR;
                V_BSTR(&m_vt) = tmp;
            }
        }

        void SetInt(int data)
        {
            V_VT(&m_vt) = VT_I4;
            V_I4(&m_vt) = data;
        }

        void SetBool(bool data)
        {
            V_VT(&m_vt) = VT_BOOL;
            m_vt.boolVal = data ? VARIANT_TRUE : VARIANT_FALSE;
        }

    private:
        VARIANT m_vt;
    };

    // RAII for SafeArray of PROPERTYIDs and Doubles used in various UIA APIs, incrementally add support for other types as needed.
    template <VARENUM VT>
    class AutoSafeArray
    {
    public:

        AutoSafeArray()
        {
            m_psa = nullptr;
            m_index = 0;
        }

        AutoSafeArray(ULONG count)
        {
            m_psa = SafeArrayCreateVector(VT, 0, count);
            m_index = 0;
        }

        ~AutoSafeArray()
        {
            if (m_psa)
            {
                SafeArrayDestroy(m_psa);
            }
        }

        SAFEARRAY* Get() { return m_psa; }

        void Attach(SAFEARRAY* psa)
        {
            if (m_psa)
            {
                SafeArrayDestroy(m_psa);
                m_psa = nullptr;
                m_index = 0;
            }

            if (psa)
            {
                VARTYPE vt = VT_UNKNOWN;
                LogThrow_IfFailed(SafeArrayGetVartype(psa, &vt));
                WEX::Common::Throw::IfFalse(VT == vt, E_INVALIDARG);
                m_psa = psa;
                m_index = m_psa->cbElements;
            }
        }

        SAFEARRAY* Detach()
        {
            SAFEARRAY* _psa = m_psa;
            m_psa = nullptr;
            return _psa;
        }

        void AddElement(void* element)
        {
            LogThrow_IfFailed(SafeArrayPutElement(m_psa, &m_index, element));
            m_index++;
        }

    private:
        LONG m_index;
        SAFEARRAY* m_psa;
    };

} } } } }
