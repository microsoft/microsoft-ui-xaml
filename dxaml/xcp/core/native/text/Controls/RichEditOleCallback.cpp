// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditOleCallback.h"
#include "TextBoxBase.h"
#include "TextContextMenu.h"
#include "TextBoxCommandHandler.h"

// *pContainsText will be true if if clipboard contains data which can be represented in text format
_Check_return_ HRESULT XAML_ClipboardContainsText(_Out_ bool *pContainsText)
{
    Microsoft::WRL::ComPtr<wadt::IClipboardStatics> spClipboardStatics;
    if (SUCCEEDED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_Clipboard).Get(),
        &spClipboardStatics)))
    {
        Microsoft::WRL::ComPtr<wadt::IDataPackageView> spDataPackage;
        IFC_RETURN(spClipboardStatics->GetContent(&spDataPackage));
        Microsoft::WRL::ComPtr<wadt::IStandardDataFormatsStatics> standardDataFormats;
        IFC_RETURN(wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_StandardDataFormats).Get(),
                    &standardDataFormats));
        boolean hasData;
        wrl_wrappers::HString stringText;
        IFC_RETURN(standardDataFormats->get_Text(stringText.GetAddressOf()));
        IFC_RETURN(spDataPackage->Contains(stringText.Get(), &hasData));

        *pContainsText = !!hasData;
    }
    else  // use OLE clipboard API instead
    {
        *pContainsText = !!IsClipboardFormatAvailable(CF_UNICODETEXT);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Instantiates a new instance.
//
//---------------------------------------------------------------------------
RichEditOleCallback::RichEditOleCallback(_In_ CTextBoxBase *pTextBox) :
    m_pTextBox(pTextBox),
    m_referenceCount(1)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases allocated resources.
//
//---------------------------------------------------------------------------
RichEditOleCallback::~RichEditOleCallback()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void **ppObject
    )
{
    HRESULT hr = E_NOINTERFACE;
    *ppObject = NULL;

    if (riid == IID_IUnknown ||
        riid == IID_IRichEditOleCallback)
    {
        *ppObject = static_cast<IRichEditOleCallback *>(this);
    }

    if (*ppObject != NULL)
    {
        AddRef();
        hr = S_OK;
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE RichEditOleCallback::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE RichEditOleCallback::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Not expected to be called.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::ContextSensitiveHelp(BOOL fEnterMode)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Notification that an image is about to be deleted. Nothing for Jupiter to do here.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::DeleteObject(LPOLEOBJECT lpoleobj)
{
    RRETURN(S_OK);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by RichEdit to ask for an IDataObject representing a run of
//      content.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::GetClipboardData(CHARRANGE *pchrg, DWORD reco, LPDATAOBJECT *plpdataobj)
{
    // E_NOTIMPL instructs RichEdit to build its own default IDataObject.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by RichEdit whenever a context menu should be displayed.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::GetContextMenu(
    _In_ WORD seltype,
    _In_ LPOLEOBJECT lpoleobj,
    _In_ CHARRANGE *pchrg,
    _In_ HMENU *phmenu
    )
{
    // TODO: remove these declarations once latest RichEdit.h is published.
    struct GetContextMenuEx
    {
        CHARRANGE chrg;
        DWORD dwFlags;
        POINT pt;
    };
    const XUINT32 Gcm_TouchMenu = 0x4000;
    const XUINT32 Gcm_MouseMenu = 0x2000;

    XPOINTF pointerPosition = {0,0};
    bool isSelectionEmpty = (seltype & 0xFF) == SEL_EMPTY;
    EventHandle hEvent;
    LONG canUndo = 0;
    LONG canRedo = 0;
    CTextBoxView* pView = m_pTextBox->GetView();

    if ((seltype & (Gcm_TouchMenu | Gcm_MouseMenu)) != 0)
    {
        GetContextMenuEx *pMenuEx = reinterpret_cast<GetContextMenuEx *>(pchrg);

        XPOINTF menuPt = {static_cast<XFLOAT>(pMenuEx->pt.x), static_cast<XFLOAT>(pMenuEx->pt.y)};

        pointerPosition = menuPt;
    }
    else
    {
        // Keyboard activated menu.  No pointer position, use active selection edge.
        XRECTF caretRect;

        caretRect = pView->GetCaretRect();

        if (pView->IsCaretInViewport())
        {
            pointerPosition.x = caretRect.X;
            pointerPosition.y = caretRect.Y;
        }
        else
        {
            XRECTF viewportRect = pView->GetViewportContentRect();

            pointerPosition.x = viewportRect.X;
            pointerPosition.y = viewportRect.Y;
        }
    }

    // Transform the invocation point to root
    wrl::ComPtr<ITransformer> pTransformToRoot;
    IFC_RETURN(pView->TransformToRoot(&pTransformToRoot));
    IFC_RETURN(pTransformToRoot->Transform(&pointerPosition, &pointerPosition, 1));

    bool showPasteForBitmap = false;

    switch (m_pTextBox->GetTypeIndex())
    {
    case KnownTypeIndex::RichEditBox:
        hEvent.index = KnownEventIndex::RichEditBox_ContextMenuOpening;
        showPasteForBitmap = ClipboardContainsBitmap();
        break;
    case KnownTypeIndex::TextBox:
        hEvent.index = KnownEventIndex::TextBox_ContextMenuOpening;
        break;
    case KnownTypeIndex::PasswordBox:
        hEvent.index = KnownEventIndex::PasswordBox_ContextMenuOpening;
        break;
    default:
        ASSERT(FALSE); // Unknown type.
        break;
    }

    bool clipboardContainsText = false;
    IFC_RETURN(XAML_ClipboardContainsText(&clipboardContainsText));
    wrl::ComPtr<ITextDocument2> pTextDocument;
    IFC_RETURN(m_pTextBox->GetDocument(&pTextDocument));
    IFC_RETURN(pTextDocument->GetProperty(tomCanUndo, &canUndo));
    IFC_RETURN(pTextDocument->GetProperty(tomCanRedo, &canRedo));

    bool showCut       = !m_pTextBox->IsReadOnly() && !isSelectionEmpty && !m_pTextBox->IsPassword();
    bool showCopy      = !isSelectionEmpty && !m_pTextBox->IsPassword();
    bool showPaste     = !m_pTextBox->IsReadOnly() && (clipboardContainsText || showPasteForBitmap);
    bool showUndo      = !m_pTextBox->IsReadOnly() && !m_pTextBox->IsPassword() && (canUndo == tomTrue);
    bool showRedo      = !m_pTextBox->IsReadOnly() && !m_pTextBox->IsPassword() && (canRedo == tomTrue);
    bool showSelectAll = !m_pTextBox->IsReadOnly() && !m_pTextBox->IsEmpty();

    m_pTextBox->SetHandleRightTappedEvent(showCut || showCopy || showPaste || showUndo || showRedo || showSelectAll);

    IFC_RETURN(pView->OnContextMenuOpen((seltype & (Gcm_TouchMenu)) != 0));
    IFC_RETURN(TextContextMenu::RaiseContextMenuOpeningEvent(
        hEvent,
        m_pTextBox,
        pointerPosition,
        showCut,
        showCopy,
        showPaste,
        showUndo,
        showRedo,
        showSelectAll));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Not expected to be called.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::GetDragDropEffect(BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Not expected to be called.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::GetInPlaceContext(LPOLEINPLACEFRAME *plpFrame, LPOLEINPLACEUIWINDOW *plpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      This method is called by RichEdit to get storage for any OLE objects
//      being pasted into the RichEdit control.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::GetNewStorage(LPSTORAGE *plpstg)
{
    // Return E_NOTIMPL for now till RichEdit implements D2D rendering
    // callbacks for OLE objects. Currently, any objects other than images
    // in native image format are not D2D aware.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by RichEdit on a paste operation or a drag event to determine
//      if the data that is pasted or dragged should be accepted.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::QueryAcceptData(LPDATAOBJECT lpdataobj, CLIPFORMAT *pcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict)
{
    // Tell RichEdit to fall back to its default behavior by returning E_NOTIMPL.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Called by RichEdit to check if the object identified by CLSID
//      lpclsid is allowed to be inserted into the RichEditBox.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::QueryInsertObject(LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp)
{
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IRichEditOleCallback override.
//
//  Notes:
//      Not expected to be called.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE RichEditOleCallback::ShowContainerUI(BOOL fShow)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

bool RichEditOleCallback::ClipboardContainsBitmap()
{
    Microsoft::WRL::ComPtr<wadt::IClipboardStatics> spClipboardStatics;

    if (FAILED(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_Clipboard).Get(),
            &spClipboardStatics)))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<wadt::IDataPackageView> spDataPackage;

    if (FAILED(spClipboardStatics->GetContent(&spDataPackage)))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<wadt::IStandardDataFormatsStatics> standardDataFormats;

    if (FAILED(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_StandardDataFormats).Get(),
            &standardDataFormats)))
    {
        return false;
    }

    wrl_wrappers::HString bitmapText;

    if (FAILED(standardDataFormats->get_Bitmap(bitmapText.GetAddressOf())))
    {
        return false;
    }

    boolean hasData;

    if (SUCCEEDED(spDataPackage->Contains(bitmapText.Get(), &hasData)))
    {
        return !!hasData;
    }

    return false;
}
