// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkInputViewHandler.h"
#include "InputServices.h"
#include "RectUtil.h"
#include <DXamlServices.h>

CFrameworkInputViewHandler::~CFrameworkInputViewHandler()
{
    if (m_spCoreInputView)
    {
        VERIFYHR(FrameworkInputView_UnregisterListener(m_spCoreInputView.Get(), this));
    }
}

// Internal CoreInputView factory interface (os.2020: ICoreInputViewInternal.h). Lets us create a
// CoreInputView keyed to a real HWND, so CCoreInputView::m_windowId is non-null and
// DeserializeAndTransformOcclusions can transform the IME occlusion instead of discarding it.
MIDL_INTERFACE("CC129710-F5EC-424C-9D39-B35EEB173F26")
ICoreInputViewFactoryInternal : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE GetCoreFrameworkInputViewForWindow(HWND appWindow, REFIID riid, void** ppView) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindInputView(HWND windowId, wuv::Core::ICoreInputView** inputView) = 0;
};

_Check_return_ HRESULT CFrameworkInputViewHandler::Initialize()
{
    // Always resolve the per-view instance first. Besides giving us a guaranteed fallback, this
    // primes CCoreInputViewFactory's InitOnce (which creates the factory's critical section).
    // Downlevel - on an OS without the GetCoreFrameworkInputViewForWindow InitOnce fix - calling
    // GetForWindow on a cold factory dereferences an uninitialized CRITICAL_SECTION and takes the
    // whole app down with an access violation. Priming here makes the window-keyed path safe on
    // every OS version: it simply does nothing useful downlevel, instead of crashing.
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewStatics> spInputViewStatics;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
            &spInputViewStatics));

        IFC_RETURN(spInputViewStatics->GetForCurrentView(&m_spCoreInputView));
    }

    HWND targetHwnd = nullptr;
    GUITHREADINFO gti = {};
    gti.cbSize = sizeof(gti);
    if (::GetGUIThreadInfo(0, &gti))
    {
        targetHwnd = (gti.hwndFocus != nullptr) ? gti.hwndFocus : gti.hwndActive;

        // Use the top-level window: occlusion rects arrive in screen coordinates and are
        // transformed against this window's client area. Keying off the focused child HWND
        // yields a client rect too small to intersect the IME candidate window, producing an
        // empty occlusion set.
        if (targetHwnd != nullptr)
        {
            HWND rootHwnd = ::GetAncestor(targetHwnd, GA_ROOT);
            if (rootHwnd != nullptr)
            {
                targetHwnd = rootHwnd;
            }
        }
    }

    if (targetHwnd != nullptr)
    {
        // Prefer a window-keyed CoreInputView. The view from GetForCurrentView() carries no HWND
        // (m_windowId == nullptr), so the OS cannot transform the candidate-window occlusion into
        // client space and silently hands us an empty occlusion set.
        wrl::ComPtr<ICoreInputViewFactoryInternal> spFactoryInternal;
        HRESULT hrFactory = wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView).Get(),
            &spFactoryInternal);

        if (SUCCEEDED(hrFactory) && spFactoryInternal)
        {
            // Creates and registers the view for this HWND (sets m_windowId)...
            wrl::ComPtr<IInspectable> spFrameworkView;
            IGNOREHR(spFactoryInternal->GetCoreFrameworkInputViewForWindow(
                targetHwnd, __uuidof(IInspectable), &spFrameworkView));

            // ...then hand us back that same window-keyed ICoreInputView. Keep the
            // GetForCurrentView instance if either step fails, so we degrade to the previous
            // behaviour rather than losing the subscription entirely.
            wrl::ComPtr<wuv::Core::ICoreInputView> spWindowView;
            if (SUCCEEDED(spFactoryInternal->FindInputView(targetHwnd, &spWindowView)) && spWindowView)
            {
                m_spCoreInputView = spWindowView;
            }
        }
    }

    // listen to FrameworkOcclusionsChanged
    IFC_RETURN(FrameworkInputView_RegisterListener(m_spCoreInputView.Get(), this));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFrameworkInputViewHandler::OnFrameworkInputViewOcclusionsChanged(
    _In_ boolean handled,
    _In_ wfc::IVectorView<wuv::Core::CoreInputViewOcclusion*> *pOcclusions)
{

    // do nothing if App has already handled OccludingInputViewChanged event
    if (handled)
    {
        return S_OK;
    }

    CDependencyObject* pFocusedElement = m_focusManager->GetFocusedElementNoRef();
    if (!CInputServices::IsTextEditableControl(pFocusedElement))
    {
        return S_OK;
    }

    UINT uiOcclusionCount = 0;
    wf::Rect finalRect = DirectUI::RectUtil::CreateEmptyRect();

    IFC_RETURN(pOcclusions->get_Size(&uiOcclusionCount));

    for (UINT32 i = 0; i < uiOcclusionCount; i++)
    {
        wrl::ComPtr<wuv::Core::ICoreInputViewOcclusion> currentOcclusion;
        IFC_RETURN(pOcclusions->GetAt(i, &currentOcclusion));
        wuv::Core::CoreInputViewOcclusionKind currentOcclusionKind;
        IFC_RETURN(currentOcclusion->get_OcclusionKind(&currentOcclusionKind));
        if (currentOcclusionKind == wuv::Core::CoreInputViewOcclusionKind::CoreInputViewOcclusionKind_Overlay)
        {
            wf::Rect currentOccludingRect = DirectUI::RectUtil::CreateEmptyRect();
            IFC_RETURN(currentOcclusion->get_OccludingRect(&currentOccludingRect));
            IFC_RETURN(DirectUI::RectUtil::Union(finalRect, currentOccludingRect));
        }
    }

    // we may get empty rect at this point, but that is expected for scenario such as IME window closing
    BOOLEAN isEmpty = FALSE;
    IFC_RETURN(DirectUI::RectUtil::GetIsEmpty(finalRect, &isEmpty));

    CTextBoxBase *pTextControl = do_pointer_cast<CTextBoxBase>(pFocusedElement);


    if (isEmpty)
    {
        XRECTF dummy {};
        IFC_RETURN(pTextControl->RaiseCandidateWindowBoundsChangedEvent(dummy));
    }
    else
    {
        IFC_RETURN(pTextControl->RaiseCandidateWindowBoundsChangedEventForDIPs(finalRect));
    }

    return S_OK;
}
