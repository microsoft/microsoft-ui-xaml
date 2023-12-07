// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowChrome.g.h"
#include "Window.g.h"
#include "CWindowChrome.h"
#include "Button.g.h"
#include "DesktopWindowImpl.h"
#include "XamlRoot.g.h"
#include "DiagnosticsInterop.h"
#include "windows.graphics.h"
#include "WindowHelpers.h"
#include "Microsoft.UI.Windowing.h"


using namespace DirectUI;
namespace RectHelpers = WindowHelpers::RectHelpers;

HWND WindowChrome::GetBridgeWindowHandle() const {return GetDesktopWindowNoRef()->GetBridgeWindowHandle();}

_Check_return_ HRESULT WindowChromeFactory::CreateInstanceImpl(
    _In_ xaml::IWindow* parent,
    _Outptr_ xaml::IWindowChrome** ppResult)
{
    ctl::ComPtr<xaml::IWindow> parentIWindow(parent);
    ctl::ComPtr<Window> parentWindow;
    parentIWindow.As(&parentWindow);
    HWND parentHwnd = nullptr;
    IFC_RETURN(parentWindow->get_WindowHandle(&parentHwnd));

    ctl::ComPtr<WindowChrome> windowChrome;
    IFC_RETURN(ctl::make(parentHwnd, &windowChrome));
    IFC_RETURN(windowChrome.CopyTo(ppResult));
    return S_OK;
}

_Check_return_ HRESULT WindowChrome::Initialize(_In_ HWND parentWindow)
{
    const auto chrome = static_cast<CWindowChrome*>(GetHandle());
    IFC_RETURN(chrome->Initialize(parentWindow));
    return S_OK;
}

_Check_return_ HRESULT WindowChrome::ApplyStyling()
{
  const auto chrome = static_cast<CWindowChrome*>(GetHandle());
  IFC_RETURN(chrome->ApplyStyling());
  return S_OK;
}


bool WindowChrome::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, _Out_ LRESULT* pResult)
{
    auto coreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    return coreWindowChrome->HandleMessage(uMsg, wParam, lParam, pResult);
}

void WindowChrome::ResizeContainer(WPARAM wParam, LPARAM lParam)
{
    auto coreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    coreWindowChrome->UpdateContainerSize(wParam, lParam);
}

void WindowChrome::MoveContainer(WPARAM wParam, LPARAM lParam)
{
    auto coreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    coreWindowChrome->UpdateBridgeWindowSizePosition();
    VERIFYHR(coreWindowChrome->OnTitleBarSizeChanged());

}
_Check_return_ HRESULT WindowChrome::SetTitleBar(_In_opt_ xaml::IUIElement* titleBar)
{
    auto pCoreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    ASSERT(pCoreWindowChrome != nullptr);
    
    if (!m_inputNonClientPtrSrc)
    {
        ctl::ComPtr<ixp::IInputNonClientPointerSourceStatics> inputNonClientPtrSrcStatics;
        IFC_RETURN(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputNonClientPointerSource).Get(),
                &inputNonClientPtrSrcStatics));
        
        mu::WindowId windowId;
        ctl::ComPtr<ixp::IAppWindow> appWindow;
        IFC_RETURN(GetDesktopWindowNoRef()->get_AppWindowImpl(&appWindow));
        IFC_RETURN(appWindow->get_Id(&windowId));
        
        IFC_RETURN(inputNonClientPtrSrcStatics->GetForWindowId(windowId, &m_inputNonClientPtrSrc));
    }
    
    //detach everything from existing titlebar
    if(m_userTitleBar.Get())
    {
        IFC_RETURN(m_titlebarSizeChangedEventHandler.DetachEventHandler(ctl::iinspectable_cast(m_userTitleBar.Get())));
        m_userTitleBar.Clear();
    }
    
    // attach everything to new titlebar
    if (titleBar)
    {
        ctl::ComPtr<xaml::IUIElement> newTitleBarAsComPtr(titleBar);
        ctl::ComPtr<FrameworkElement> newTitleBarAsFE;
        ctl::ComPtr<UIElement> newTitleBarAsUIE;
        IFC_RETURN(newTitleBarAsComPtr.As(&newTitleBarAsUIE));
        IFC_RETURN(newTitleBarAsComPtr.As(&newTitleBarAsFE));

        SetPtrValueWithQIOrNull(m_userTitleBar, newTitleBarAsUIE.Get());
        auto hr = m_titlebarSizeChangedEventHandler.AttachEventHandler(newTitleBarAsFE.Get(),
            [this](IInspectable* sender, xaml::ISizeChangedEventArgs* args)
            {
                auto pCoreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
                IFC_RETURN(pCoreWindowChrome->OnTitleBarSizeChanged());

                return S_OK;
            });
        IFC_RETURN(hr);
    }

    // Update size of the titleBar container
    IFC_RETURN(pCoreWindowChrome->OnTitleBarSizeChanged());

    return S_OK;
}


bool WindowChrome::IsChromeActive() const
{
    return static_cast<CWindowChrome*>(GetHandle())->IsChromeActive();
}

_Check_return_ HRESULT WindowChrome::SetIsChromeActive(bool value)
{
    IFC_RETURN(static_cast<CWindowChrome*>(GetHandle())->SetIsChromeActive(value));
    return S_OK;
}


// If Window.Content is changed then one should redo window.content event listener
_Check_return_ HRESULT WindowChrome::OnContentChanged(_In_ IInspectable* oldContent,
                                                    _In_ IInspectable* newContent)
{
    IFC_RETURN(__super::OnContentChanged(oldContent, newContent));


    // Fire XamlRoot.Changed
    CXamlIslandRoot* xamlIslandRoot = VisualTree::GetXamlIslandRootForElement(GetHandle());
    xamlIslandRoot->GetContentRootNoRef()->AddPendingXamlRootChangedEvent(CContentRoot::ChangeType::Content);

    return S_OK;
}

ctl::ComPtr<ixp::IAppWindow> WindowChrome::GetAppWindow() const
{
    ctl::ComPtr<ixp::IAppWindow> appWindow;
    IFCFAILFAST(m_desktopWindow->get_AppWindowImpl(&appWindow));
    return appWindow;
}

bool WindowChrome::CanDrag() const
{
    CWindowChrome* coreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    return coreWindowChrome->CanDrag();
}

void WindowChrome::UpdateCanDragStatus(bool enabled)
{
    CWindowChrome* coreWindowChrome = static_cast<CWindowChrome*>(GetHandle());
    coreWindowChrome->UpdateCanDragStatus(enabled);
}
