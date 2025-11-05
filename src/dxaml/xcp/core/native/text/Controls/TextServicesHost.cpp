// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditPal.h"
#include "TextServicesHost.h"
#include "TextBoxBase.h"
#include "GripperPopup.h"
#include <CColor.h>
#include <XamlOneCoreTransforms.h>

HANDLE TextServicesHost::s_mutex = 0;

ATOM TextServicesHost::s_timerWindowClass = 0;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Instantiates a new TextServicesHost instance.
//
//---------------------------------------------------------------------------
TextServicesHost::TextServicesHost(_In_ CTextBoxBase *pTextBox) :
    m_pTextBox(pTextBox),
    m_timerWindow(0),
    m_refCount(1),
    m_pStartGripper(nullptr),
    m_pEndGripper(nullptr),
    m_grippersCreated(FALSE)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases allocated resources.
//
//---------------------------------------------------------------------------
TextServicesHost::~TextServicesHost()
{
    if (m_timerWindow != 0)
    {
        DestroyWindow(m_timerWindow);
    }
    if (nullptr != m_pStartGripper)
    {
        m_pStartGripper->m_pPopupChild->SetPeer(nullptr);
    }
    if (nullptr != m_pEndGripper)
    {
        m_pEndGripper->m_pPopupChild->SetPeer(nullptr);
    }
    ReleaseInterface(m_pStartGripper);
    ReleaseInterface(m_pEndGripper);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE TextServicesHost::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void **ppObject
    )
{
    HRESULT hr = S_OK;
    *ppObject = NULL;

    // {13E670F4-1A5A-11cf-ABEB-00AA00B65EA1}
    static const GUID IID_ITextHost =
    { 0x13e670f4, 0x1a5a, 0x11cf, { 0xab, 0xeb, 0x0, 0xaa, 0x0, 0xb6, 0x5e, 0xa1 } };

    // {13E670F5-1A5A-11cf-ABEB-00AA00B65EA1}
    static const GUID IID_ITextHost2 =
    { 0x13e670f5, 0x1a5a, 0x11cf, { 0xab, 0xeb, 0x0, 0xaa, 0x0, 0xb6, 0x5e, 0xa1 } };

#if DBG
    // The above IID values are available as exports from WinUIEdit.dll. They are hardcoded
    // above for performance and convenience. Validate on debug builds that the IID values
    // are correct. (They shouldn't change, but this will catch if they ever do change.)
    static bool s_verifiedIIDs = false;
    if (!s_verifiedIIDs)
    {
        s_verifiedIIDs = true;
        auto winUIEdit = ::GetModuleHandle(L"WinUIEdit.dll");
        ASSERT(winUIEdit);

        auto pIID_ITextHost = (GUID*)::GetProcAddress(winUIEdit, "IID_ITextHost");
        ASSERT(pIID_ITextHost);
        ASSERT(*pIID_ITextHost == IID_ITextHost);

        auto pIID_ITextHost2 = (GUID*)::GetProcAddress(winUIEdit, "IID_ITextHost2");
        ASSERT(pIID_ITextHost2);
        ASSERT(*pIID_ITextHost2 == IID_ITextHost2);
    }
#endif

    if (riid == IID_IUnknown ||
        riid == IID_ITextHost)
    {
        SetInterface(*ppObject, static_cast<ITextHost *>(this));
    }
    else if (riid == IID_ITextHost2)
    {
        SetInterface(*ppObject, static_cast<ITextHost2 *>(this));
    }
    else if (riid == __uuidof(IProvideFontInfo))
    {
        SetInterface(*ppObject, static_cast<IProvideFontInfo *>(this));
    }
    else if (riid == __uuidof(IXamlTextHost))
    {
        SetInterface(*ppObject, static_cast<IXamlTextHost*>(this));
    }
    else if (riid == __uuidof(IGripperHost2))
    {
        // We need to expose the IGripperHost2 interface to provide
        // support for rendering the selection grippers for RichEdit and for
        // providing it with the gripper touch events.
        SetInterface(*ppObject, static_cast<IGripperHost2 *>(this));
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE TextServicesHost::AddRef()
{
    return ++m_refCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE TextServicesHost::Release()
{
    ASSERT(m_refCount != 0);
    ULONG refCount = --m_refCount;

    if (refCount == 0)
    {
        delete this;
    }

    return refCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HDC TextServicesHost::TxGetDC()
{
    return GetDC(NULL);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
INT TextServicesHost::TxReleaseDC(HDC hdc)
{
    return ReleaseDC(NULL, hdc);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxShowScrollBar(INT fnBar, BOOL fShow)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxSetScrollRange(
    INT fnBar,
    LONG nMinPos,
    INT nMaxPos,
    BOOL fRedraw)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw)
{
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxInvalidateRect(LPCRECT prc, BOOL fMode)
{
    if (m_pTextBox->GetView() != NULL)
    {
        m_pTextBox->GetView()->TxInvalidateRect(reinterpret_cast<const XRECT_RB*>(prc), fMode);
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxViewChange(BOOL fUpdate)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight)
{
    return m_pTextBox->GetView()->TxCreateCaret(hbmp, xWidth, yHeight);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxShowCaret(BOOL fShow)
{
    return m_pTextBox->GetView()->TxShowCaret(fShow);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxSetCaretPos(INT x, INT y)
{
    bool result;

    if (y < 0)
    {
        // y should logically never be zero, but RichEdit will use -32000 when it thinks the caret
        // is hidden, which it will for brief periods of time (before calling with a real value).
        // Filter out the hidden carets.
        result = TRUE;
    }
    else
    {
        result = m_pTextBox->GetView()->TxSetCaretPos(x, y);
    }

    return result;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxSetTimer(
    _In_ UINT id,
    _In_ UINT duration
    )
{
    HRESULT hr = S_OK;

    // Demand create a message only window for timer callbacks.
    // TODO: workitem 101629: We should only allocate at most one window per thread.
    //       The difficult part is ensuring that we don't let any timer ids collide.
    if (0 == m_timerWindow)
    {
        IFC(CreateTimerWindow());
    }

    if (!SetTimer(m_timerWindow, id, duration, NULL))
    {
        IFC(E_FAIL);
    }

Cleanup:
    return SUCCEEDED(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxKillTimer(_In_ UINT id)
{
    // RichEdit will regularly ask us to kill timers it has not yet asked for.  Thus we need to
    // consider the case where we haven't yet lazy allocated a timer window.
    if (m_timerWindow != 0)
    {
        KillTimer(m_timerWindow, id);
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override. Scroll the view by specified x and y delta.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxScrollWindowEx(
    INT dx,
    INT dy,
    LPCRECT lprcScroll,
    LPCRECT lprcClip,
    HRGN hrgnUpdate,
    LPRECT lprcUpdate,
    UINT fuScroll)
{
    IGNOREHR(m_pTextBox->GetView()->ScrollView(static_cast<XFLOAT>(dx),
                                               static_cast<XFLOAT>(dy)));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxSetCapture(BOOL fCapture)
{
    m_pTextBox->TxSetCapture(fCapture);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxSetFocus()
{
    m_pTextBox->TxSetFocus();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxSetCursor(HCURSOR hcur, BOOL fText)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxScreenToClient(_Inout_ LPPOINT lppt)
{
    if (TxGetShouldUseVisualPixels())
    {
        // In XamlOneCoreTransforms mode we use client coordinates rather than screen coordinates
        XPOINTF pointf = { static_cast<float>(lppt->x), static_cast<float>(lppt->y) };
        if (SUCCEEDED(m_pTextBox->ClientToTextBox(&pointf)))
        {
            *lppt = { static_cast<LONG>(pointf.x), static_cast<LONG>(pointf.y) };
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        return SUCCEEDED(m_pTextBox->ScreenToTextBox(reinterpret_cast<XPOINT *>(lppt)));
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxClientToScreen(_Inout_ LPPOINT lppt)
{
    if (TxGetShouldUseVisualPixels())
    {
        return SUCCEEDED(m_pTextBox->TextBoxToClient(reinterpret_cast<XPOINT*>(lppt)));
    }
    else
    {
        return SUCCEEDED(m_pTextBox->TextBoxToScreen(reinterpret_cast<XPOINT*>(lppt)));
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxActivate(LONG * plOldState )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxDeactivate(LONG lNewState )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetClientRect(_Out_ RECT *pClientRect)
{
    return m_pTextBox->GetView()->TxGetClientRect(reinterpret_cast<XRECT_RB *>(pClientRect));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IXamlTextHost override to report the true viewport rect.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetViewportRect(_Out_ RECT *pViewportRect)
{
    RRETURN(m_pTextBox->GetView()->TxGetViewportRect(reinterpret_cast<XRECT_RB *>(pViewportRect)));
}

HRESULT TextServicesHost::TxGetContentPadding(_Out_ RECT *pContentPadding)
{
    RRETURN(m_pTextBox->GetView()->TxGetContentPadding(reinterpret_cast<XRECT_RB *>(pContentPadding)));
}

bool TextServicesHost::TxGetShouldUseVisualPixels()
{
    return(m_pTextBox->ShouldUseVisualPixels());
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetViewInset(_Out_ RECT *pViewInsert)
{
    // RichEdit has a strange design feature in that when you call TxGetNaturalSize, it reserves a
    // single pixel on the right for the insert bar/caret.  However, when you draw, it doesn't.  So what
    // happens is that for some widths, TxGetNaturalSize will wrap one pixel sooner and potentially
    // cause it to compute a height that is one line bigger than it will need when it actually
    // paints.  However, if you have defined a right inset, then TxGetNaturalSize assumes that
    // the text bar can go there and doesn't reserve space for it.  So, by always indicating that
    // our right edit is inset by one pixel TxGetNaturalSize and TxDraw function the same way.
    pViewInsert->left = 0;
    pViewInsert->top = 0;
    pViewInsert->right = 1;
    pViewInsert->bottom = 0;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetCharFormat(_Outptr_ const CHARFORMATW **ppCharFormat)
{
    return m_pTextBox->GetView()->TxGetCharFormat(reinterpret_cast<const WCHARFORMAT **>(ppCharFormat));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetParaFormat(_Outptr_ const PARAFORMAT **ppParaFormat)
{
    return m_pTextBox->GetView()->TxGetParaFormat(reinterpret_cast<const XPARAFORMAT **>(ppParaFormat));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
COLORREF TextServicesHost::TxGetSysColor(_In_ INT index)
{
    COLORREF sysColor = GetSysColor(index);

    if (!UseHighContrastSelection(m_pTextBox->GetContext()))
    {
        switch (index)
        {
            case COLOR_HIGHLIGHT:
            {
                CSolidColorBrush* selectionHighlightColorNoRef = m_pTextBox->GetSelectionHighlightColorNoRef();
                if (selectionHighlightColorNoRef)
                {
                    BYTE red = static_cast<BYTE>(selectionHighlightColorNoRef->m_rgb >> 16 & 0xff);
                    BYTE green = static_cast<BYTE>(selectionHighlightColorNoRef->m_rgb >> 8 & 0xff);
                    BYTE blue = static_cast<BYTE>(selectionHighlightColorNoRef->m_rgb & 0xff);
                    return RGB(red, green, blue);
                }
            }

            case COLOR_HIGHLIGHTTEXT:
                return RGB(0xFF, 0xFF, 0xFF);

            default:
                break;
        }
    }

    return sysColor;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetBackStyle(_Out_ TXTBACKSTYLE *pStyle)
{
    *pStyle = TXTBACK_TRANSPARENT;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetMaxLength(_Out_ DWORD *pLength)
{
    XUINT32 length;

    IFC_RETURN(m_pTextBox->TxGetMaxLength(&length));

    *pLength = length;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetScrollBars(_Out_ DWORD *pScrollBars)
{
    // Disable all scrolling.  The ScrollViewer containing the view handles that, not RichEdit.
    *pScrollBars = 0;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetPasswordChar(_Out_ TCHAR *pch)
{
    return m_pTextBox->TxGetPasswordChar(pch);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetAcceleratorPos(LONG *pcp)
{
    ASSERT(FALSE); // We don't set TXTBIT_SHOWACCELERATOR and therefore should never be called here.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetExtent(LPSIZEL lpExtent)
{
    // TODO: this needs to be implemented for zoom to work.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::OnTxCharFormatChange (const CHARFORMATW * pCF)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::OnTxParaFormatChange (const PARAFORMAT * pPF)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetPropertyBits(
    _In_ DWORD mask,
    _Out_ DWORD *pFlags
    )
{
    XUINT32 flags;

    IFC_RETURN(m_pTextBox->TxGetPropertyBits(mask, &flags));

    *pFlags = flags;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxNotify(DWORD iNotify, void *pv)
{
    IFC_RETURN(m_pTextBox->TxNotify(iNotify, pv));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
HIMC TextServicesHost::TxImmGetContext()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    HWND inputHwnd;
    HIMC context = 0;

    IFC(TxGetWindow(&inputHwnd));

    context = ImmGetContext(inputHwnd);

Cleanup:
    return context;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxImmReleaseContext(_In_ HIMC himc)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    HWND inputHwnd;

    IFC(TxGetWindow(&inputHwnd));

    ImmReleaseContext(inputHwnd, himc);

Cleanup:
    ;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost override.
//
//  Notes:
//      "Selection bar" refers to the right margin, double or triple clicking
//      in this region will select text.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetSelectionBarWidth(_Out_ LONG *pSelectionBarWidth)
{
    XINT32 selectionBarWidth;

    IFC_RETURN(m_pTextBox->TxGetSelectionBarWidth(&selectionBarWidth));

    *pSelectionBarWidth = selectionBarWidth;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
BOOL TextServicesHost::TxIsDoubleClickPending()
{
    ASSERT(FALSE);
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetWindow(_Out_ HWND* inputHwnd)
{
    *inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_pTextBox->GetElementIslandInputSite().Get());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxSetForegroundWindow()
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HPALETTE    TextServicesHost::TxGetPalette()
{
    ASSERT(FALSE);
    return 0;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetEastAsianFlags(LONG *pFlags)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HCURSOR TextServicesHost::TxSetCursor2(
    _In_ HCURSOR cursor,
    _In_ BOOL usedForText
    )
{
    // Temporary workaround for bug 393209.
    // It will not hide the cursor when typing text, however it is better than not
    // restoring it later when mouse is moved within the application.
    if (cursor != NULL)
    {
        // Workaround for bug 34063662.
        // System input is now processed on a dedicated input queue, so the below call
        // to SetCursor will no longer affect the cursor seen by the user.
        // In DetectiveEmoji and FamilyEmoji, we show a wait cursor while loading an rtf file.
        ASSERT(cursor == NULL || cursor == LoadCursor(NULL, IDC_WAIT));

        //return SetCursor(cursor);
    }
    return NULL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
void TextServicesHost::TxFreeTextServicesNotification()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetEditStyle(DWORD dwItem, DWORD *pdwData)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetWindowStyles(
    _Out_ DWORD *pStyle,
    _Out_ DWORD *pExStyle
    )
{
    *pStyle = 0;
    *pExStyle = 0;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxShowDropCaret(BOOL fShow, HDC hdc, LPCRECT prc)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxDestroyCaret()
{
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::TxGetHorzExtent(LONG *plHorzExtent)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

_Check_return_ HRESULT TextServicesHost::EnsureGrippers()
{
    HRESULT hr = S_OK;

    if (m_grippersCreated)
    {
        goto Cleanup;
    }

    IFC(CreateGripper(RichEditGripperCommon::GripperType::Start));
    IFC(CreateGripper(RichEditGripperCommon::GripperType::End));

    m_pStartGripper->m_pPopupChild->SetPeer(m_pEndGripper);
    m_pEndGripper->m_pPopupChild->SetPeer(m_pStartGripper);

Cleanup:
    m_grippersCreated = TRUE;
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override to create a gripper to be controled by RichEdit.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextServicesHost::CreateGripper(RichEditGripperCommon::GripperType type)
{
    xref_ptr<CRichEditGripper> spGripper;
    xref_ptr<CGripperPopup> spPopup;

    ASSERT(m_pTextBox != nullptr);

    CREATEPARAMETERS      createParameters(m_pTextBox->GetContext());

    IFC_RETURN(CRichEditGripper::Create(spGripper.ReleaseAndGetAddressOf(), &createParameters));

    IFC_RETURN(spGripper->InitializeGripper(m_pTextBox->GetTextServices(), type, this, m_pTextBox));

    IFC_RETURN(CGripperPopup::Create(reinterpret_cast<CDependencyObject **>(spPopup.ReleaseAndGetAddressOf()), &createParameters));

    // For island only case, we need to associate just created popup to island visual tree
    VisualTree* visualTree = VisualTree::GetForElementNoRef(m_pTextBox);
    spPopup->SetAssociatedVisualTree(visualTree);

    // The popup will protect the peer for the gripper after the following call.  And the popup is protected
    // because it's a root.  (It indicates that it's a root by setting ControlsManagedPeerLifetime.)
    spGripper->SetPopup(spPopup.detach());

    switch (type)
    {
        case RichEditGripperCommon::GripperType::Start:
            m_pStartGripper = spGripper.detach();
            break;

        case RichEditGripperCommon::GripperType::End:
            m_pEndGripper = spGripper.detach();
            break;
    }

    return S_OK;
}


CRichEditGripper* TextServicesHost::GetGripper(RichEditGripperCommon::GripperType type)
{
    HRESULT             hr       = S_OK; // WARNING_IGNORES_FAILURES
    CRichEditGripper   *pGripper = nullptr;

    IFC(EnsureGrippers());

    switch (type)
    {
    case RichEditGripperCommon::GripperType::Start:
        pGripper = m_pStartGripper;
        break;

    case RichEditGripperCommon::GripperType::End:
        pGripper = m_pEndGripper;
        break;

    default:
        break;
    }

Cleanup:
    return pGripper;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override to show a gripper.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextServicesHost::ShowGrippers(_In_ POINT *pBeginPoint, float beginLineHeight, _In_ POINT *pEndPoint, float endLineHeight)
{
    // Grippers have not been created yet, don't need to do anything to hide them
    if ((pBeginPoint == nullptr) && (pEndPoint == nullptr) && !m_grippersCreated)
    {
        return S_OK;
    }

    CRichEditGripper *pBeginGripper = GetGripper(RichEditGripperCommon::GripperType::Start);
    CRichEditGripper *pEndGripper = GetGripper(RichEditGripperCommon::GripperType::End);
    if (nullptr == pBeginGripper || nullptr == pEndGripper)
    {
        return S_OK;
    }

    ctl::ComPtr<ITextSelection2> selection;
    IFC_RETURN(m_pTextBox->GetSelection(&selection));

    long cchSel = 0;
    IFC_RETURN(selection->GetCch(&cchSel));

    bool bForSelection = (0 != cchSel);

    XRECT_WH rectStartGripper;
    IFC_RETURN(ShowGripper(pBeginGripper, pBeginPoint, beginLineHeight, &rectStartGripper, bForSelection));

    XRECT_WH rectEndGripper;
    IFC_RETURN(ShowGripper(pEndGripper, pEndPoint, endLineHeight, &rectEndGripper, bForSelection));

    IFC_RETURN(m_pTextBox->SetGripperRects(rectStartGripper, rectEndGripper));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::GetVisibleRect(_Out_ RECT *pRect)
{
    return m_pTextBox->GetVisibleRect(reinterpret_cast<XRECT_RB*>(pRect));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::EnsureRectVisible(_In_ const  RECT &rect)
{
    IFC_RETURN(m_pTextBox->EnsureRectVisible(rect));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override: called by RichEdit to  Bring a rect(padding required)
//      such as caret into view
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::EnsureRectVisibleWithPadding(_In_ const RECT &rect)
{
    IFC_RETURN(m_pTextBox->EnsureRectVisibleWithPadding(rect));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IGripperHost2 override.
//
//---------------------------------------------------------------------------
HRESULT TextServicesHost::GetGripperMetrics(_Out_ int *pnWidth, _Out_ int *pnDescent)
{
    *pnWidth = CRichEditGripperChild::c_nGripperWidth;
    *pnDescent = CRichEditGripperChild::c_nGripperDescent;

    return S_OK;
}

_Check_return_ HRESULT TextServicesHost::ShowGripper(_In_ CRichEditGripper *pGripper, _In_ POINT *pPoint, float lineHeight, XRECT_WH *pRectGripper, bool bForSelection)
{
    CValue  val;

    if (pPoint != nullptr)
    {
        XPOINTF centerWorldCoordinate;
        POINT point = *pPoint;
        bool applyRasterizationScale = !XamlOneCoreTransforms::IsEnabled();
        IFC_RETURN(m_pTextBox->TextBoxToClient(reinterpret_cast<XPOINT*>(&point), applyRasterizationScale));

        centerWorldCoordinate.x = static_cast<XFLOAT>(point.x);
        centerWorldCoordinate.y = static_cast<XFLOAT>(point.y);

        IFC_RETURN(pGripper->UpdateCenterWorldCoordinate(centerWorldCoordinate, lineHeight));
        IFC_RETURN(pGripper->Show(bForSelection));

        val.SetBool(TRUE);
        IFC_RETURN(pGripper->GetPopup()->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, val));

        *pRectGripper = pGripper->GetGripperScreenRect();
    }
    else
    {
        IFC_RETURN(pGripper->Hide());

        val.SetBool(FALSE);
        IFC_RETURN(pGripper->GetPopup()->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, val));

        pRectGripper->X = -1;
        pRectGripper->Width = -1;
        pRectGripper->Y = -1;
        pRectGripper->Height = -1;
    }

    return S_OK;
}

_Check_return_ HRESULT TextServicesHost::UpdateGripperColors()
{
    if (m_pStartGripper != NULL)
    {
        IFC_RETURN(m_pStartGripper->UpdateThemeColor());
    }

    if (m_pEndGripper != NULL)
    {
        IFC_RETURN(m_pEndGripper->UpdateThemeColor());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo override,  Used to ask the host for the name of
//      the default font used for the Text Box control.
//
//---------------------------------------------------------------------------
BSTR TextServicesHost::GetDefaultFont()
{
    BSTR fontNameBstr = NULL;
    if (m_pTextBox != NULL && m_pTextBox->GetView() != NULL)
    {
        fontNameBstr = m_pTextBox->GetView()->GetDefaultFont();
    }
    return fontNameBstr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo override,  Used to ask the host for the name of
//      a given font face (while streaming out).
//
//---------------------------------------------------------------------------
BSTR TextServicesHost::GetSerializableFontName(
    _In_ DWORD fontFaceId
)
{
    BSTR fontNameBstr = NULL;
    if (m_pTextBox != NULL && m_pTextBox->GetView() != NULL)
    {
        fontNameBstr = m_pTextBox->GetView()->GetSerializableFontName(fontFaceId);
    }
    return fontNameBstr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo override, Used to ask the host which font face
//      should be used for new characters.
//
//---------------------------------------------------------------------------
DWORD TextServicesHost::GetRunFontFaceId(
    _In_z_ const wchar_t *pCurrentFontName,
    _In_ DWRITE_FONT_WEIGHT weight,
    _In_ DWRITE_FONT_STRETCH stretch,
    _In_ DWRITE_FONT_STYLE style,
    _In_ LCID lcid,
    _In_reads_opt_(charCount) const wchar_t *pText,
    _In_ unsigned int charCount,
    _In_ DWORD fontFaceIdCurrent,
    _Out_ unsigned int& runCount
    )
{
    XUINT32 fontId = 0;
    if (m_pTextBox != NULL && m_pTextBox->GetView() != NULL)
    {
        fontId = m_pTextBox->GetView()->GetRunFontFaceId(pCurrentFontName, weight, stretch, style, lcid, pText, charCount, fontFaceIdCurrent, &runCount);
    }
    return fontId;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo override, converts a font face handle from a
//      previous GetRunFontFaceId call into an IDWriteFontFace.
//
//---------------------------------------------------------------------------
IDWriteFontFace* TextServicesHost::GetFontFace(
    _In_ DWORD font_id
    )
{
    IDWriteFontFace *pDWriteFontFace = NULL;
    if (m_pTextBox != NULL && m_pTextBox->GetView() != NULL)
    {
        pDWriteFontFace = m_pTextBox->GetView()->GetFontFace(font_id);
    }
    return pDWriteFontFace;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates a new message only timer window.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextServicesHost::CreateTimerWindow()
{
    ASSERT(0 == m_timerWindow);

    IFC_RETURN(EnsureTimerWindowClass());

    m_timerWindow = CreateWindow(
        MAKEINTATOM(s_timerWindowClass),
        NULL,
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetDirectUIInstance(), NULL);

    if (0 == m_timerWindow)
    {
        IFC_RETURN(::HResultFromKnownLastError());
    }

    SetLastError(0);
    if (SetWindowLongPtr(m_timerWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this)) == 0 &&
        GetLastError() != 0)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      If not already registered, registers the timer window class.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextServicesHost::EnsureTimerWindowClass()
{
    HRESULT hr = S_OK;
    HANDLE mutex = 0;

    // Skip the expensive path in the common case that the window class is already registered.
    if (0 == s_timerWindowClass)
    {
        // Always create a mutex, if we are the first then that will be the shared mutex.
        HANDLE localMutex = CreateMutex(NULL, FALSE /* initialOwner */, NULL);
        IFCOOMFAILFAST(localMutex);

        // Try to set the mutex in the static variable. If we don't have one we'll get 0 as the
        // original value and the mutex we created will be the singleton. Otherwise, the
        // return value is the one we'll use.
        mutex = reinterpret_cast<HANDLE>(InterlockedCompareExchangePointer(&s_mutex, localMutex, 0));

        if (0 == mutex)
        {
            // If the static mutex was 0 then we've just set it.
            mutex = localMutex;
        }
        else
        {
            // s_mutex was already set, we don't need the new mutex.
            CloseHandle(localMutex);
        }
        localMutex = NULL;

        if (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0)
        {
            mutex = NULL; // We don't have the mutex, don't release it below.
            IFC(E_FAIL);
        }

        // Now that we hold the mutex, check again that the window class is unregistered.
        if (0 == s_timerWindowClass)
        {
            WNDCLASS windowClass = { 0 };
            windowClass.lpfnWndProc = TimerWindowProc;
            windowClass.hInstance = GetDirectUIInstance();
            windowClass.lpszClassName = L"WinUI_TextServicesTimerClass";

            s_timerWindowClass = RegisterClass(&windowClass);

            if (0 == s_timerWindowClass)
            {
                IFC(E_FAIL);
            }
        }
    }

Cleanup:
    if (mutex != 0)
    {
        ReleaseMutex(mutex);
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback for the message only timer window.
//
//---------------------------------------------------------------------------
LRESULT CALLBACK TextServicesHost::TimerWindowProc(
    _In_  HWND window,
    _In_  UINT message,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    TextServicesHost *pHost = reinterpret_cast<TextServicesHost *>(GetWindowLongPtr(window, GWLP_USERDATA));
    LRESULT result = 0;

    switch (message)
    {
        case WM_TIMER:
            if (pHost->m_pTextBox->IsActive())
            {
                IFC(pHost->m_pTextBox->GetTextServices()->TxSendMessage(WM_TIMER, wParam, lParam, &result));
            }
            break;

        default:
            result = DefWindowProc(window, message, wParam, lParam);
            break;
    }

Cleanup:
    return result;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the HINSTANCE of Microsoft.UI.Xaml.dll.
//
//---------------------------------------------------------------------------
HINSTANCE TextServicesHost::GetDirectUIInstance()
{
    return GetModuleHandle(L"Microsoft.UI.Xaml.dll");
}
