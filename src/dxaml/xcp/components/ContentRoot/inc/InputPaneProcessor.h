// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"
#include "ScrollContentControl.h"

struct IXcpInputPaneHandler;
struct IPALInputPaneInteraction;
class CFrameworkInputViewHandler;

class CInputManager;

namespace ContentRootInput
{
    class InputPaneProcessor
    {
    public:
        InputPaneProcessor(_In_ CInputManager& inputManager);
        void Init();

        bool IsSipOpen() const;
        bool IsInputPaneShowing() const;
        bool IsInputPaneShowingBringIntoViewNotHandled() const;

        void CreateInputPaneHandler();
        _Check_return_ HRESULT RegisterInputPaneHandler(_In_opt_ XHANDLE hCoreWindow);
        void UnregisterInputPaneHandler();
        void DestroyInputPaneHandler();

        _Check_return_ HRESULT RegisterFrameworkInputView();

        IXcpInputPaneHandler* GetInputPaneHandler() const;
        IPALInputPaneInteraction* GetInputPaneInteraction() const;
        DirectUI::InputPaneState GetInputPaneState() const;

        _Check_return_ HRESULT GetInputPaneBounds(_Out_ XRECTF* pInputPaneBounds) const;
        void AdjustBringIntoViewRecHeight(_In_ float topGlobal, _In_ float bottomGlobal, _Inout_ float &height);

        _Check_return_ HRESULT NotifyFocusChanged(_In_opt_ CDependencyObject* pFocusedElement, _In_ bool bringIntoView, _In_ bool animateIfBringIntoView);

    private:
        static bool CanUseInputPane(_In_ CDependencyObject *pObject);

        CInputManager& m_inputManager;

        std::unique_ptr<IXcpInputPaneHandler> m_pInputPaneHandler;
        xref_ptr<CScrollContentControl> m_pRegisteredRootScrollViewer;
        xref_ptr<IPALInputPaneInteraction> m_pInputPaneInteraction;
        xref_ptr<CFrameworkInputViewHandler> m_spFrameworkInputViewHandler;
    };
};