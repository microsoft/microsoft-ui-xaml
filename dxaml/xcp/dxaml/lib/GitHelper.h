// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines GITHelper, which manages obtaining and releasing a pointer
//      to the GIT.

#pragma once

namespace DirectUI
{
    class GITHelper
    {
    public:
        GITHelper() :
            m_pGIT(NULL)
        {
            IGNOREHR(CoCreateInstance(
                CLSID_StdGlobalInterfaceTable,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IGlobalInterfaceTable,
                reinterpret_cast<void**>(&m_pGIT)));
        }

        ~GITHelper()
        {
            ReleaseInterface(m_pGIT);
        }

        _Check_return_ HRESULT RegisterInterfaceInGlobal( 
            _In_  IUnknown *pUnk,
            _In_  REFIID riid,
            _Out_  DWORD *pdwCookie)
        {
            if (!m_pGIT)
            {
                return E_UNEXPECTED;
            }
            return m_pGIT->RegisterInterfaceInGlobal(pUnk, riid, pdwCookie);
        }
        
        _Check_return_ HRESULT RevokeInterfaceFromGlobal( 
            _In_  DWORD dwCookie)
        {
            if (!m_pGIT)
            {
                return E_UNEXPECTED;
            }
            return m_pGIT->RevokeInterfaceFromGlobal(dwCookie);
        }
        
        _Check_return_ HRESULT GetInterfaceFromGlobal( 
            _In_  DWORD dwCookie,
            _In_  REFIID riid,
            _Outptr_  void **ppv)
        {
            if (!m_pGIT)
            {
                return E_UNEXPECTED;
            }
            return m_pGIT->GetInterfaceFromGlobal(dwCookie, riid, ppv);
        }
        
    private:
        IGlobalInterfaceTable* m_pGIT;
        
        // prohibit copying
        GITHelper(const GITHelper&);
        GITHelper& operator=(const GITHelper&);        
    };
}
