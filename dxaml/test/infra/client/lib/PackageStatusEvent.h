// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <windows.h>
#include <documentsource.h>
#include <documenttarget.h>

class CDocumentPackageStatusEvent : public IPrintDocumentPackageStatusEvent
{
public:
    CDocumentPackageStatusEvent();
    ~CDocumentPackageStatusEvent();

    // IUnknown members
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID iid, _Out_ void ** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(_Out_ UINT *pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, _Outptr_result_maybenull_ ITypeInfo **ppTInfo);
    virtual STDMETHODIMP
    GetIDsOfNames(
        _In_                        REFIID      riid,
        _In_reads_(cNames)          LPOLESTR *  rgszNames,
        _In_range_(0,16384)         UINT        cNames,
                                    LCID        lcid,
        _Out_writes_all_(cNames)    DISPID *    rgDispId
        );

    virtual STDMETHODIMP
    Invoke(
                DISPID          dispIdMember,
        _In_    REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
        _Inout_ DISPPARAMS *    pDispParams,
        _Out_   VARIANT *       pVarResult,
        _Out_   EXCEPINFO *     pExcepInfo,
        _Out_   UINT *          puArgErr
        );

    // IPrintDocumentPackageStatusEvent
    HRESULT STDMETHODCALLTYPE PackageStatusUpdated(
        _In_ PrintDocumentPackageStatus* packageStatus
        );

    // CDocumentPackageStatusEvent
    bool WaitForCompletion();
    PrintDocumentPackageCompletion GetCompletionFlag();

private:
    PrintDocumentPackageStatus     m_docPackageStatus;
    PrintDocumentPackageCompletion m_Completion;
    HANDLE m_completionEvent;
    ULONG m_refcount;
};
