// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef RICH_EDIT_OLE_CALLBACK_H
#define RICH_EDIT_OLE_CALLBACK_H

class TextBoxBase;

//---------------------------------------------------------------------------
//
// Callback handler that receives events from a windowless RichEdit control.
// Used in practice to handle context menu opening events.
//
//---------------------------------------------------------------------------
class RichEditOleCallback final : public IRichEditOleCallback
{
public:
    RichEditOleCallback(_In_ CTextBoxBase *pTextBox);

    // IUnknown overrides.
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void **ppObject
        );
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IRichEditOleCallback overrides.
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    STDMETHODIMP DeleteObject(LPOLEOBJECT lpoleobj);
    STDMETHODIMP GetClipboardData(CHARRANGE *pchrg, DWORD reco, LPDATAOBJECT *plpdataobj);
    STDMETHODIMP GetContextMenu(
        _In_ WORD seltype,
        _In_ LPOLEOBJECT lpoleobj,
        _In_ CHARRANGE *pchrg,
        _In_ HMENU *phmenu
        );
    STDMETHODIMP GetDragDropEffect(BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect);
    STDMETHODIMP GetInPlaceContext(LPOLEINPLACEFRAME *plpFrame, LPOLEINPLACEUIWINDOW *plpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHODIMP GetNewStorage(LPSTORAGE *plpstg);
    STDMETHODIMP QueryAcceptData(LPDATAOBJECT lpdataobj, CLIPFORMAT *pcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict);
    STDMETHODIMP QueryInsertObject(LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp);
    STDMETHODIMP ShowContainerUI(BOOL fShow);

private:
    ~RichEditOleCallback();
    static bool ClipboardContainsBitmap();

    // Weak reference to container.
    CTextBoxBase *m_pTextBox;
    XUINT32 m_referenceCount;
};

#endif // RICH_EDIT_OLE_CALLBACK_H
