// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComPtr.h>
#include <fwd/windows.ui.core.h>
#include "UIBridgeFocus\inc\IFocusController.h"

#include <Clock.h>

class CCoreServices;
class CFocusManager;
class CInputServices;
class CDependencyObject;
class CContentRoot;

namespace DirectUI
{
    enum class FocusNavigationDirection : uint8_t;
}

namespace Focus
{
    class FocusMovement;
    class FocusMovementResult;
}

// FocusObserver's main responsibility is to process focus navigation requests within XAML and 
// XAML being guest(XamlIslands in WPF or other host framework). It tags focus navigation request with unique correlationId.
// It is responsible for initiating DepartFocus to WPF or another host framework, if XAML does not have any focusable element.
//
// FocusObserver contains IFocusController to deal with focus navigation for XAML Islands. FocusController is responsible
// for setting up input and few other core functionalities on XAML islands.
//
// CoreWindowFocusObserver: This concrete derived class is specialized to deal with CoreWindow 
// specific behaviors like window activation/ deactivation. 
//
//                 +-----------------------------+         +-----------------------------+
//                 |        FocusObserver        |         |       CFocusManager         |
//                 |    IFoucsController*        |<>-----<>|                             |
//                 +-------------^---------------+         +-----------------------------+
//                               |                          
//                               |                          
//                 +-------------+---------------+
//                 |   CoreWindowFocusObserver   |
//                 | ICoreWindow*                |                               
//                 +-----------------------------+                               
//     
// <> ----| Indicates "Has a" relationship
//
// ^  ----| Indicates "Is a" relationship                       

class FocusObserver
{
public:
    FocusObserver(_In_ CCoreServices *pCoreService, _In_ CContentRoot* pContentRoot);
    virtual ~FocusObserver();

    _Check_return_ HRESULT Init(_In_opt_ xaml_hosting::IFocusController* const pFocusController);
    _Check_return_ virtual HRESULT Init(_In_ wuc::ICoreWindow* const pCoreWindow)
    {
        return S_OK;
    }    

    _Check_return_ HRESULT ProcessNavigateFocusRequest(
        _In_ xaml_hosting::IXamlSourceFocusNavigationRequest* args,
        _Out_ boolean* pHandled);

    _Check_return_ HRESULT StartInteraction(
        _In_ xaml_hosting::XamlSourceFocusNavigationReason reason,
        _In_ const wf::Rect& origin,
        _In_ GUID correlationId);


    _Check_return_ HRESULT DepartFocus(
        _In_ DirectUI::FocusNavigationDirection direction,
        _In_ GUID correlationId,
        _Inout_ bool* pHandled);

    _Check_return_ const Focus::FocusMovementResult NavigateFocusXY(
        _In_ CDependencyObject* pComponent,
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& origin);

    XRECTF_RB GetOriginFromInteraction();

    virtual bool IsActivated() const
    {
        return false;
    }
    
protected:
    virtual wuc::CoreWindowActivationMode GetActivationMode() const;
    
private:
    _Check_return_ HRESULT DeInit();

    _Check_return_ HRESULT GetOriginToComponent(
        _In_opt_ CDependencyObject* const pOldFocusedElement,
        _Out_ wf::Rect* origin);

    _Check_return_ HRESULT CalculateNewOrigin(
        _In_ DirectUI::FocusNavigationDirection direction,
        _In_ const wf::Rect& currentOrigin,
        _Out_ wf::Rect* newOrigin);

    _Check_return_ HRESULT DepartFocus(
        _In_ DirectUI::FocusNavigationDirection direction,
        _In_ const wf::Rect& origin,
        _In_ GUID correlationId,
        _Inout_ bool* pHandled);
        
    void UpdateCurrentInteraction(_In_opt_ xaml_hosting::IXamlSourceFocusNavigationRequest* pRequest)
    {
        m_spCurrentInteraction = pRequest;
    }
    void StopInteraction(boolean* pHandled);

protected:
    CContentRoot* m_contentRoot = nullptr;
    CCoreServices* m_pCoreServicesNoRef = nullptr;
    wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> m_spCurrentInteraction;

private:
    ctl::ComPtr<xaml_hosting::IFocusController> m_spFocusController;
};
