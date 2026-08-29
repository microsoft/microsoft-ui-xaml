// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "corep.h"
#include <DXamlServices.h>
#include <InitializationType.h>

#define IMetaData ShellIMetaData // Allow atlbase.h to compile
#define CString ATLCString // Allow atlbase.h to compile

#define GetClassName GetClassNameW
#include <atlbase.h> // Needed to build FrameworkInputPaneHandler.h
#include <atlhost.h>

#include <shobjidl_core.h>
#include "XcpInputPaneHandler.h"
#include "FrameworkInputPaneHandler.h"
#include "FrameworkInputPaneOneCore.h"

#include "inputpaneinterop.h"

using namespace Microsoft::WRL;

FrameworkInputPaneOneCore::FrameworkInputPaneOneCore() :
    m_inputPane(nullptr),
    m_pFrameworkHandler(NULL),
    m_bVisible(false)
{
    ZeroMemory(&m_rcInputPane, sizeof(m_rcInputPane));
}

FrameworkInputPaneOneCore::~FrameworkInputPaneOneCore()
{
    IGNOREHR(Unadvise((DWORD)0));
}

IFACEMETHODIMP FrameworkInputPaneOneCore::OnFrameworkInputPaneVisibilityChanged(
    _In_ wf::Rect rcInputPane,
    _In_ boolean isVisible,
    _In_ boolean ensureFocusedElementInView,
    _In_ boolean notifyEditFocusLostOnHiding)
{
    m_rcInputPane.left = static_cast<LONG>(rcInputPane.X);
    m_rcInputPane.right = static_cast<LONG>(rcInputPane.X + rcInputPane.Width);
    m_rcInputPane.top = static_cast<LONG>(rcInputPane.Y);
    m_rcInputPane.bottom = static_cast<LONG>(rcInputPane.Y + rcInputPane.Height);

    if (isVisible)
    {
        if (m_bVisible)
        {
            //
            // InputPane could fire multiple showing events if SIP changes its size.
            // However, continuously calling Showing() won't have any effect in Jupiter.
            // So we call Hiding() first to ensure the Showing() call will succeed
            //

            if (notifyEditFocusLostOnHiding)
            {
                IFC_RETURN(m_pFrameworkHandler->HidingWithEditFocusRemoval(FALSE));
            }
            else
            {
                IFC_RETURN(m_pFrameworkHandler->Hiding(FALSE));
            }
        }

        IFC_RETURN(m_pFrameworkHandler->Showing(&m_rcInputPane, ensureFocusedElementInView));
    }
    else
    {
        if (notifyEditFocusLostOnHiding)
        {
            IFC_RETURN(m_pFrameworkHandler->HidingWithEditFocusRemoval(ensureFocusedElementInView));
        }
        else
        {
            IFC_RETURN(m_pFrameworkHandler->Hiding(ensureFocusedElementInView));
        }
    }

    m_bVisible = isVisible;
    return S_OK;
}


IFACEMETHODIMP FrameworkInputPaneOneCore::Advise(_In_ IUnknown *pWindow, _In_ IFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie)
{
    UNREFERENCED_PARAMETER(pWindow);
    UNREFERENCED_PARAMETER(pHandler);
    UNREFERENCED_PARAMETER(pdwCookie);
    return E_NOTIMPL;
}

IFACEMETHODIMP FrameworkInputPaneOneCore::AdviseInternal(_In_ XHANDLE hWindow, _In_ CFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie)
{
    HRESULT hr = S_OK;

    auto scopeGuard = wil::scope_exit([&]()
    {
        if (FAILED(hr))
        {
            IGNOREHR(Unadvise(*pdwCookie));
        }
    });

    *pdwCookie = 1;
    m_pFrameworkHandler = pHandler;

    ComPtr<wuv::IInputPaneStatics> inputPaneStatics;
    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_InputPane).Get(), &inputPaneStatics));
    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow, which is not supported in islands. Hence, we switched to use IInputPaneInterop in Win32/ islands
    if (DirectUI::DXamlServices::GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
    {
        ComPtr<IInputPaneInterop> spInputPaneInterop;
        IFC_RETURN(inputPaneStatics.As(&spInputPaneInterop));

        IFC_RETURN(spInputPaneInterop->GetForWindow(static_cast<HWND>(hWindow), ABI::Windows::UI::ViewManagement::IID_IInputPane, &m_inputPane));
    }
    else
    {
        IFC_RETURN(inputPaneStatics->GetForCurrentView(&m_inputPane));
    }
    IFC_RETURN(InputPaneFramework_RegisterListener(m_inputPane.Get(), this));
    
    return hr;
}

IFACEMETHODIMP FrameworkInputPaneOneCore::AdviseWithHWND(_In_ HWND hwnd, _In_ IFrameworkInputPaneHandler *pHandler, _Out_ DWORD *pdwCookie)
{
    // In the windows version of this class, AdviseWithHWND is only called from within Advise. As we dont use HWND's on the phone side for this, this should never be called.
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(pHandler);
    UNREFERENCED_PARAMETER(pdwCookie);

    return E_NOTIMPL;
}

IFACEMETHODIMP FrameworkInputPaneOneCore::Unadvise(_In_ DWORD dwCookie)
{
    UNREFERENCED_PARAMETER(dwCookie);
    if (m_inputPane)
    {
        IFC_RETURN(InputPaneFramework_UnregisterListener(m_inputPane.Get(), this));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkInputPaneOneCore::Location(_Out_ RECT *prcInputPaneScreenLocation)
{
    return E_NOTIMPL;
}