// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSwapChainPanel
    : public CGrid
{
protected:
    CSwapChainPanel(_In_ CCoreServices *pCore);
    ~CSwapChainPanel() override;

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSwapChainPanel>::Index;
    }

    DECLARE_CREATE(CSwapChainPanel);

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT SetSwapChain(_In_opt_ IUnknown *pSwapChain);

    _Check_return_ HRESULT SetSwapChainHandle(_In_opt_ HANDLE swapChainHandle);

    bool HasContent() const;

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) final;

    void GetChildrenInRenderOrder(
        _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
        _Out_ XUINT32 *puiChildCount
        ) final;

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) final;

    CSwapChainElement* GetSwapChainElement() const { return m_pSwapChainElement; }

    static _Check_return_ HRESULT GetCompositionScaleX(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    static _Check_return_ HRESULT GetCompositionScaleY(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT UpdateCompositionScale(
        _In_ const CMILMatrix& compositionScale,
        bool isTransformAnimating
        );

    _Check_return_ HRESULT CreateInputPointerSource(
        _In_ ixp::InputPointerSourceDeviceKinds deviceKinds,
        _Outptr_ ixp::IInputPointerSource** ppInputPointerSource);

    wrl::ComPtr<ixp::ISpriteVisual> GetOrEnsureSwapChainVisual();

    // Lifted composition doesn't allow for transparent swap chains. Some apps use a SwapChainPanel together with a fully
    // transparent swap chain to get off-thread input over a portion of the app. As a workaround, we'll use a fully
    // transparent SpriteVisual to do that instead of hooking up a fully transparent swap chain.
    void SetUseTransparentVisualIfNeeded();

protected:
    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) final;

    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;

private:
    _Check_return_ HRESULT GetCompositionScaleImpl(
        KnownPropertyIndex propIndex,
        _Out_ CValue *pValue
        );

    _Check_return_ HRESULT ComputeCompositionScale(
        _Out_ XFLOAT *pCompositionScaleX,
        _Out_ XFLOAT *pCompositionScaleY
        );

    _Check_return_ HRESULT RaiseCompositionScaleChangedEvent();

    _Check_return_ HRESULT EnsureHWRealization(_Outptr_ HWRealization **ppHWRealization);

    _Check_return_ HRESULT EnsureSwapChainElement();

private:
    CSwapChainElement *m_pSwapChainElement;
    CUIElement** m_ppRenderChildren;
    XUINT32 m_renderChildrenCount;

    // The IInputPointerSource is created off of the UI thread, and needs the visual that contains
    // the swap chain to be created. Create that visual off-thread if it doesn't exist yet. The SwapChainElement
    // child of this SwapChainPanel will get that element and attach it to the tree.
    wil::critical_section m_swapChainVisualLock;
    wrl::ComPtr<ixp::ISpriteVisual> m_swapChainVisual;
};
