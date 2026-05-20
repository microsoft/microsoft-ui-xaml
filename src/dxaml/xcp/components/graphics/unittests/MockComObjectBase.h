// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>
#include "d3d11_2.h"
#include "MockDxgi.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    //  MockComObjectBase - Base class for Mock objects that derive from IUnknown.

    template<class T>
    class MockComObjectBase : public T
    {
    public:
        MockComObjectBase() : m_refCount(1)
        {
        }

        virtual ~MockComObjectBase()
        {
        }

        // Override this method to provide additional interfaces, but don't forget
        // to delegate back the base implemenation on an unknown interface.
        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(T)) return static_cast<T*>(this);
            if (iid == __uuidof(IUnknown)) return static_cast<IUnknown*>(this);

            // Identity to get back to the implementation object.  Use with care.
            if (iid == GUID_NULL) return this;
            return nullptr;
        }

        // IUnknown
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID *pVoid)
        {
            if (!pVoid) return E_POINTER;
            *pVoid = CastTo(iid);
            if (*pVoid == nullptr) return E_NOINTERFACE;
            AddRef();
            return S_OK;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef()
        {
            return InterlockedIncrement(&m_refCount);
        }

        virtual ULONG STDMETHODCALLTYPE Release()
        {
            ULONG newRefCount = InterlockedDecrement(&m_refCount);
            if (newRefCount == 0)
            {
                delete this;
            }
            return newRefCount;
        }
       
    private:
        ULONG m_refCount;
    };

    //  MockAlsoImplementsComInterface - templated wrapper class for alternate COM interfaces to help 
    //                                   with the multiple inheritance issues of IUnknown.

    template<class T>
    class MockAlsoImplementsComInterface : public T
    {
    private:
        IUnknown* m_delegateIUnknown;
    public:
        MockAlsoImplementsComInterface(IUnknown* delegateIUnknown) : m_delegateIUnknown(delegateIUnknown)
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID *pVoid)
        {
            return m_delegateIUnknown->QueryInterface(iid, pVoid);
        }

        virtual ULONG STDMETHODCALLTYPE AddRef()
        {
            return m_delegateIUnknown->AddRef();
        }

        virtual ULONG STDMETHODCALLTYPE Release()
        {
            return m_delegateIUnknown->Release();
        }
    };

} } } } }
