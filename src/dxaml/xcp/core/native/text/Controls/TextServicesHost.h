// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef TEXT_SERVICES_HOST_H
#define TEXT_SERVICES_HOST_H

#include "fontinfo.h"
#include "xamltexthost.h"

class TextBoxBase;

//---------------------------------------------------------------------------
//
//  CTextBoxBase's ITextHost implementation for the windowless RichEdit
//  control.
//
//  For the most part this class simply forwards callbacks back into
//  CTextBoxBase or CTextBoxView.  Methods that do not require state from
//  either the control or the view are implemented directly.
//
//---------------------------------------------------------------------------
class TextServicesHost final : public ITextHost2,
                         public IProvideFontInfo,
                         public IXamlTextHost,
                         public IGripperHost2
{
public:
    TextServicesHost(_In_ CTextBoxBase *pTextBox);

private:
    ~TextServicesHost();

public:
    // IUnknown overrides.
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void **ppObject
        ) final;
    virtual ULONG STDMETHODCALLTYPE AddRef() final;
    virtual ULONG STDMETHODCALLTYPE Release() final;

    // ITextHost2 overrides.
    virtual HDC TxGetDC() final;
    virtual INT TxReleaseDC(HDC hdc) final;
    virtual BOOL TxShowScrollBar(INT fnBar, BOOL fShow) final;
    virtual BOOL TxEnableScrollBar (INT fuSBFlags, INT fuArrowflags) final;
    virtual BOOL TxSetScrollRange(
        INT fnBar,
        LONG nMinPos,
        INT nMaxPos,
        BOOL fRedraw) final;
    virtual BOOL TxSetScrollPos (INT fnBar, INT nPos, BOOL fRedraw) final;
    virtual void TxInvalidateRect(LPCRECT prc, BOOL fMode) final;
    virtual void TxViewChange(BOOL fUpdate) final;
    virtual BOOL TxCreateCaret(
        _In_ HBITMAP bitmap,
        _In_ INT width,
        _In_ INT height
        ) final;
    virtual BOOL TxShowCaret(_In_ BOOL isCaretVisible) final;
    virtual BOOL TxSetCaretPos(
        _In_ INT x,
        _In_ INT y
        ) final;
    virtual BOOL TxSetTimer(
        _In_ UINT id,
        _In_ UINT duration
        ) final;
    virtual void TxKillTimer(_In_ UINT id) final;
    virtual void TxScrollWindowEx(
        INT dx,
        INT dy,
        LPCRECT lprcScroll,
        LPCRECT lprcClip,
        HRGN hrgnUpdate,
        LPRECT lprcUpdate,
        UINT fuScroll) final;
    virtual void TxSetCapture(BOOL fCapture) final;
    virtual void TxSetFocus() final;
    virtual void TxSetCursor(HCURSOR hcur, BOOL fText) final;
    virtual BOOL TxScreenToClient (_Inout_ POINT *pPoint) final;
    virtual BOOL TxClientToScreen (_Inout_ POINT *pPoint) final;
    virtual HRESULT TxActivate( LONG * plOldState ) final;
    virtual HRESULT TxDeactivate( LONG lNewState ) final;
    virtual HRESULT TxGetClientRect(_Out_ RECT *pClientRect) final;
    virtual HRESULT TxGetViewInset(_Out_ RECT *pViewInsert) final;
    virtual HRESULT TxGetCharFormat(_Outptr_ const CHARFORMATW **ppCharFormat) final;
    virtual HRESULT TxGetParaFormat(_Outptr_ const PARAFORMAT **ppPF) final;
    virtual COLORREF TxGetSysColor(_In_ INT index) final;
    virtual HRESULT TxGetBackStyle(_Out_ TXTBACKSTYLE *pStyle) final;
    virtual HRESULT TxGetMaxLength(_Out_ DWORD *pLength) final;
    virtual HRESULT TxGetScrollBars(_Out_ DWORD *pScrollBars) final;
    virtual HRESULT TxGetPasswordChar(_Out_ TCHAR *pch) final;
    virtual HRESULT TxGetAcceleratorPos(LONG *pcp) final;
    virtual HRESULT TxGetExtent(LPSIZEL lpExtent) final;
    virtual HRESULT OnTxCharFormatChange (const CHARFORMATW * pCF) final;
    virtual HRESULT OnTxParaFormatChange (const PARAFORMAT * pPF) final;
    virtual HRESULT TxGetPropertyBits(_In_ DWORD mask, _Out_ DWORD *pFlags) final;
    virtual HRESULT TxNotify(DWORD iNotify, void *pv) final;
    virtual HIMC TxImmGetContext() final;
    virtual void TxImmReleaseContext(_In_ HIMC himc ) final;
    virtual HRESULT TxGetSelectionBarWidth (_Out_ LONG *pSelectionBarWidth) final;
    virtual BOOL TxIsDoubleClickPending() final;
    virtual HRESULT TxGetWindow(_Out_ HWND *pHwnd) final;
    virtual HRESULT TxSetForegroundWindow() final;
    virtual HPALETTE TxGetPalette() final;
    virtual HRESULT TxGetEastAsianFlags(LONG *pFlags) final;
    virtual HCURSOR TxSetCursor2(
        _In_ HCURSOR cursor,
        _In_ BOOL usedForText
        ) final;
    virtual void TxFreeTextServicesNotification() final;
    virtual HRESULT TxGetEditStyle(DWORD dwItem, DWORD *pdwData) final;
    virtual HRESULT TxGetWindowStyles(
        _Out_ DWORD *pStyle,
        _Out_ DWORD *pExStyle
        ) final;
    virtual HRESULT TxShowDropCaret(BOOL fShow, HDC hdc, LPCRECT prc) final;
    virtual HRESULT TxDestroyCaret() final;
    virtual HRESULT TxGetHorzExtent(LONG *plHorzExtent) final;

    // IProvideFontInfo overrides.
    BSTR GetDefaultFont() override;

    BSTR GetSerializableFontName(
        _In_ DWORD fontFaceId
        ) override;

    IDWriteFontFace* GetFontFace(
        _In_ DWORD font_id
        ) override;

    DWORD GetRunFontFaceId(
        _In_z_ const wchar_t *pCurrentFontName,
        _In_ DWRITE_FONT_WEIGHT weight,
        _In_ DWRITE_FONT_STRETCH stretch,
        _In_ DWRITE_FONT_STYLE style,
        _In_ LCID lcid,
        _In_reads_opt_(charCount) const wchar_t *pText,
        _In_ unsigned int charCount,
        _In_ DWORD fontFaceIdCurrent,
        _Out_ unsigned int& runCount
    ) override;

    // IXamlTextHost overrides
    virtual HRESULT TxGetViewportRect(_Out_ RECT *pViewportRect) override;
    virtual HRESULT TxGetContentPadding(_Out_ RECT *pContentPadding) override;
    virtual bool TxGetShouldUseVisualPixels() override;

#if false // _ONECORETRANSFORMS_REMOVED_
    virtual UINT64  TxGetVisualReferenceId() override;
#endif

    // IGripperHost overrides
    _Check_return_ HRESULT ShowGrippers(_In_ POINT *pBeginPoint, float beginLineHeight, _In_ POINT *pEndPoint, float endLineHeight) override;
    HRESULT GetVisibleRect(_Out_ RECT *pRect) override;
    HRESULT EnsureRectVisible(_In_ const RECT &rect) override;
    HRESULT EnsureRectVisibleWithPadding(_In_ const RECT &rect) override;
    HRESULT GetGripperMetrics(_Out_ int *pnDiameter, _Out_ int *pnOffsetBelowLine) override;

    _Check_return_ HRESULT UpdateGripperColors();

private:
    // Weak reference to container to avoid circular dependency
    // TextBoxBase will always call shutdown service to RichEdit before release TextService pointer.
    CTextBoxBase *m_pTextBox;

    // Message only window used for timer callbacks.
    HWND m_timerWindow;

    XUINT32 m_refCount;

    // Mutex used to register timer window class.
    static HANDLE s_mutex;

    static ATOM s_timerWindowClass;

    BOOL m_grippersCreated;
    CRichEditGripper *m_pStartGripper;
    CRichEditGripper *m_pEndGripper;

    _Check_return_ HRESULT CreateTimerWindow();

    _Check_return_ HRESULT EnsureTimerWindowClass();

    _Check_return_ HRESULT EnsureGrippers();

    _Check_return_ HRESULT CreateGripper(RichEditGripperCommon::GripperType type);

    CRichEditGripper* GetGripper(RichEditGripperCommon::GripperType type);

    _Check_return_ HRESULT ShowGripper(_In_ CRichEditGripper *pGripper, _In_ POINT *pPoint, float lineHeight, XRECT_WH *pRectGripper, bool bForSelection);

    static LRESULT CALLBACK TimerWindowProc(
      _In_  HWND window,
      _In_  UINT message,
      _In_  WPARAM wParam,
      _In_  LPARAM lParam
    );

    // Gets the HINSTANCE of Microsoft.UI.Xaml.dll.
    static HINSTANCE GetDirectUIInstance();
};

#endif // TEXT_SERVICES_HOST_H
