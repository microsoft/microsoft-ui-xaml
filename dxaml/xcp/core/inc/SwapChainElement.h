// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <UIElement.h>

class CSwapChainElement final : public CUIElement
{
protected:
    CSwapChainElement(_In_ CCoreServices *pCore);

    ~CSwapChainElement() override;

public:
    DECLARE_CREATE(CSwapChainElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSwapChainElement>::Index;
    }

    _Check_return_ HRESULT SetSwapChain(_In_opt_ IUnknown *pISwapChain);
    _Check_return_ HRESULT SetSwapChainHandle(_In_opt_ HANDLE swapChainHandle);
    void SetUseTransparentVisualIfNeeded();

    bool HasContent() const;
    IUnknown* GetSwapChain() const;
    WUComp::ICompositionSurface* GetSwapChainSurface() const;
    bool GetUseTransparentVisual() const;

private:
    _Check_return_ HRESULT UpdateRequiresComposition(_In_ bool requiresComposition, _Out_ BOOL &compositionChanged);

    // Supports both SwapChain (via ISwapChainPanelNative::SetSwapChain and ISwapChainBackgroundPanelNative::SetSwapChain)
    // and SwapChain handle (via ISwapChainPanelNative2::SetSwapChainHandle).
    Microsoft::WRL::ComPtr<IUnknown> m_pISwapChain;
    Microsoft::WRL::ComPtr<WUComp::ICompositionSurface> m_pISwapChainSurface;

    bool m_compositionSet {false};

    // Lifted composition doesn't allow for transparent swap chains. Some apps use a SwapChainPanel together with a fully
    // transparent swap chain to get off-thread input over a portion of the app. As a workaround, we'll use a fully
    // transparent SpriteVisual to do that instead of hooking up a fully transparent swap chain.
    bool m_useTransparentVisual {false};
};
