// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "IsEnabledChangedEventArgs.h"
#include "VisualStateChangedEventArgs.h"
#include "RenderingEventArgs.h"
#include "RenderedEventArgs.h"
#include "ContextMenuEventArgs.h"
#include "ExceptionRoutedEventArgs.h"
#include "HyperlinkClickEventArgs.h"
#include "HoldingEventArgs.h"
#include "CandidateWindowBoundsChangedEventArgs.h"
#include "TextChangedEventArgs.h"
#include "TextControlPasteEventArgs.h"
#include "TextControlCopyingToClipboardEventArgs.h"
#include "TextControlCuttingToClipboardEventArgs.h"
#include "TextCompositionEventArgs.h"
#include "DownloadProgressEventArgs.h"
#include "LoadedImageSourceLoadCompletedEventArgs.h"
#include "palthread.h" // uses PAL_Interlocked*
#include "SvgImageSourceOpenedEventArgs.h"
#include "SvgImageSourceFailedEventArgs.h"

CRoutedEventArgs::CRoutedEventArgs()
    : CEventArgs()
#ifdef DBG
    , m_threadId(::GetCurrentThreadId())
#endif
{
}

CRoutedEventArgs::~CRoutedEventArgs()
{
#ifdef DBG
    ASSERT(m_threadId == ::GetCurrentThreadId());
#endif
    if (m_pSource)
    {
        m_pSource->UnpegManagedPeer();
        ReleaseInterface(m_pSource);
    }
    ReleaseInterface(m_pPeggedDXamlPeer);
}

_Check_return_ HRESULT CRoutedEventArgs::PegManagedPeerForRoutedEventArgs()
{
    if (m_pPeggedDXamlPeer == nullptr)
    {
        IFC_RETURN(CreateFrameworkPeer(&m_pPeggedDXamlPeer));
    }
    else
    {
        // We only expect to be pegged once.
        ASSERT(FALSE);
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

void CRoutedEventArgs::UnpegManagedPeerForRoutedEventArgs()
{
    ReleaseInterface(m_pPeggedDXamlPeer);
}

_Check_return_ HRESULT CRoutedEventArgs::GetFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    if (m_pPeggedDXamlPeer != nullptr)
    {
        SetInterface(*ppPeer, m_pPeggedDXamlPeer);
        RRETURN(S_OK);
    }
    else
    {
        RRETURN(CreateFrameworkPeer(ppPeer));
    }
}

_Check_return_ HRESULT CRoutedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CRoutedEventArgs::get_Source(_Outptr_ CDependencyObject** ppSource)
{
    SetInterface(*ppSource, m_pSource);
    RRETURN(S_OK);
}

_Check_return_ HRESULT CRoutedEventArgs::put_Source(_In_ CDependencyObject* pSource)
{
    if (m_pSource)
    {
        m_pSource->UnpegManagedPeer();
        ReleaseInterface(m_pSource);
    }

    if (pSource)
    {
        IFC_RETURN(pSource->PegManagedPeer());
        SetInterface(m_pSource, pSource);
    }

    return S_OK;
}

_Check_return_ HRESULT CIsEnabledChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateIsEnabledChangedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CVisualStateChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateVisualStateChangedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CRenderingEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateRenderingEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CRenderedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateRenderedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CContextMenuEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateContextMenuEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CExceptionRoutedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateExceptionRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CHyperlinkClickEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateHyperlinkClickEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CHoldingEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateHoldingRoutedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CCandidateWindowBoundsChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateCandidateWindowBoundsChangedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateTextChangedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextControlPasteEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateTextControlPasteEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextControlCopyingToClipboardEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateTextControlCopyingToClipboardEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextControlCuttingToClipboardEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateTextControlCuttingToClipboardEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CSvgImageSourceOpenedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateSvgImageSourceOpenedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CSvgImageSourceFailedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateSvgImageSourceFailedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextCompositionStartedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return (DirectUI::OnFrameworkCreateTextCompositionStartedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextCompositionChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return(DirectUI::OnFrameworkCreateTextCompositionChangedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CTextCompositionEndedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    return(DirectUI::OnFrameworkCreateTextCompositionEndedEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CDownloadProgressEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateDownloadProgressEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateEventArgs(this, ppPeer));
}

_Check_return_ HRESULT CEventArgs::GetFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(CreateFrameworkPeer(ppPeer));
}

ULONG STDMETHODCALLTYPE CEventArgs::AddRef()
{
    XUINT32 cRef = PAL_InterlockedIncrement((XINT32 *)&m_cRef);
    return cRef;
}

ULONG STDMETHODCALLTYPE CEventArgs::Release()
{
    XUINT32 cRef = PAL_InterlockedDecrement((XINT32 *)&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

_Check_return_ HRESULT CLoadedImageSourceLoadCompletedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateLoadedImageSourceLoadCompletedEventArgs(this, ppPeer));
}
