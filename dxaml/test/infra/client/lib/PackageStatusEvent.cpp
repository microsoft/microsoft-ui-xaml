// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PackageStatusEvent.h"

CDocumentPackageStatusEvent::CDocumentPackageStatusEvent()
{
    PrintDocumentPackageStatus  status = {0};
    m_docPackageStatus = status;
    m_Completion = PrintDocumentPackageCompletion_InProgress;
    m_completionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_refcount = 1;
}
 

CDocumentPackageStatusEvent::~CDocumentPackageStatusEvent()
{
    if (m_completionEvent)
    {
        CloseHandle(m_completionEvent);
        m_completionEvent = NULL;
    }
}

HRESULT STDMETHODCALLTYPE CDocumentPackageStatusEvent::QueryInterface(_In_ REFIID iid, _Out_ void ** ppvObject)
{
    if (iid == __uuidof(IUnknown) ||
        iid == __uuidof(IPrintDocumentPackageStatusEvent))
    {
        *ppvObject = static_cast<IPrintDocumentPackageStatusEvent*>(this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE CDocumentPackageStatusEvent::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_refcount);
}

ULONG STDMETHODCALLTYPE CDocumentPackageStatusEvent::Release(void)
{
    ULONG res = (ULONG)InterlockedDecrement(&m_refcount);
    if (0 == res)
    {
        delete this;
    }
    return res;
}

// IDispatch
STDMETHODIMP CDocumentPackageStatusEvent::GetTypeInfoCount(_Out_ UINT *pctinfo)
{
    UNREFERENCED_PARAMETER(pctinfo);
    return E_NOTIMPL;
}

STDMETHODIMP CDocumentPackageStatusEvent::GetTypeInfo(UINT iTInfo, LCID lcid, _Outptr_result_maybenull_ ITypeInfo **ppTInfo)
{
    UNREFERENCED_PARAMETER(iTInfo);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(ppTInfo);
    return E_NOTIMPL;
}

STDMETHODIMP
CDocumentPackageStatusEvent::GetIDsOfNames(
    _In_                        REFIID      riid,
    _In_reads_(cNames)          LPOLESTR *  rgszNames,
    _In_range_(0,16384)         UINT        cNames,
                                LCID        lcid,
    _Out_writes_all_(cNames)    DISPID *    rgDispId
    )
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(rgszNames);
    UNREFERENCED_PARAMETER(cNames);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(rgDispId);
    return E_NOTIMPL;
}

STDMETHODIMP
CDocumentPackageStatusEvent::Invoke(
            DISPID          dispIdMember,
    _In_    REFIID          riid,
            LCID            lcid,
            WORD            wFlags,
    _Inout_ DISPPARAMS *    pDispParams,
    _Out_   VARIANT *       pVarResult,
    _Out_   EXCEPINFO *     pExcepInfo,
    _Out_   UINT *          puArgErr
    )
{
    UNREFERENCED_PARAMETER(dispIdMember);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(pDispParams);
    UNREFERENCED_PARAMETER(pVarResult);
    UNREFERENCED_PARAMETER(pExcepInfo);
    UNREFERENCED_PARAMETER(puArgErr);
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CDocumentPackageStatusEvent::PackageStatusUpdated(
    _In_ PrintDocumentPackageStatus* packageStatus
    )
{
    HRESULT hr = !packageStatus ? E_INVALIDARG : S_OK;

    if (SUCCEEDED(hr))
    {
        m_docPackageStatus = *packageStatus;
        m_Completion = m_docPackageStatus.Completion;

        if (m_docPackageStatus.Completion != PrintDocumentPackageCompletion_InProgress)
        {
            SetEvent(m_completionEvent);
        }
    }

    return hr;
}

bool CDocumentPackageStatusEvent::WaitForCompletion()
{
    if (m_docPackageStatus.Completion == PrintDocumentPackageCompletion_InProgress)
    {
        DWORD dwIndex;
        HRESULT hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 300000, 1, &m_completionEvent, &dwIndex);

        return SUCCEEDED(hr);
    }

    return (m_docPackageStatus.Completion != PrintDocumentPackageCompletion_InProgress);
}

PrintDocumentPackageCompletion CDocumentPackageStatusEvent::GetCompletionFlag()
{
    return m_Completion;
}
