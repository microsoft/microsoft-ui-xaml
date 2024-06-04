// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CCOMObjectWrapper class.
//    PAL-friendly wrapper for an IUnknown COM interface implementation.

#pragma once

class CCOMObjectWrapper final : public CXcpObjectBase<IObject>
{
public:
    virtual XUINT32 AddRef()
    {
        ASSERT(m_pCOMObject);
        m_pCOMObject->AddRef();
        return CXcpObjectBase::AddRef();
    }

    virtual XUINT32 Release()
    {
        ASSERT(m_pCOMObject);
        m_pCOMObject->Release();
        return CXcpObjectBase::Release();
    }

    IUnknown* GetCOMObjectNoRef()
    {
        return m_pCOMObject;
    }

    static _Check_return_ HRESULT Create(
        _In_ IUnknown* pCOMObject, _Outptr_ CCOMObjectWrapper **ppCOMObjectWrapper)
    {
        HRESULT hr = S_OK;

        ASSERT(ppCOMObjectWrapper);

        *ppCOMObjectWrapper = new CCOMObjectWrapper(pCOMObject);
        RRETURN(hr);//RRETURN_REMOVAL
    }

// Private Methods
private:
    // Constructor
    CCOMObjectWrapper(_In_ IUnknown* pCOMObject)
        : m_pCOMObject(pCOMObject)
    {
        ASSERT(pCOMObject);
        pCOMObject->AddRef();
    }

    // Destructor
    ~CCOMObjectWrapper()
    {
    }

// Private Fields
private:
    IUnknown* m_pCOMObject;
};
