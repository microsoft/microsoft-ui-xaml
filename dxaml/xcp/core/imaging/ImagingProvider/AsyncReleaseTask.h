// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template < typename T >
class AsyncReleaseTask : public CXcpObjectBase< IImageTask >
{
    public:
        static _Check_return_ HRESULT Create(
            _In_ T* pObject,
            _Outptr_ IImageTask** ppTask
            )
        {
            xref_ptr<AsyncReleaseTask> pNewTask;

            pNewTask.attach(new AsyncReleaseTask< T >(pObject));

            *ppTask = pNewTask.detach();

            return S_OK;
        }

        _Check_return_ HRESULT Execute(
            ) override
        {
            RRETURN(S_OK);
        }

    protected:
        AsyncReleaseTask(
            _In_ T* pObject
            )
            : m_pObject(pObject)
        {
            AddRefInterface(m_pObject);
        }

        ~AsyncReleaseTask(
            ) override
        {
            ReleaseInterface(m_pObject);
        }

        T* m_pObject;
};

template < typename T >
_Check_return_ HRESULT CreateAsyncReleaseTask(
    _In_ T* pObject,
    _Outptr_ IImageTask** ppTask
    )
{
    IFC_RETURN(AsyncReleaseTask< T >::Create(pObject, ppTask));

    return S_OK;
}
