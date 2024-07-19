// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "brush.h"
#include <ContentRoot.h>

class CXamlCompositionBrush final : public CBrush
{
private:
    CXamlCompositionBrush(_In_ CCoreServices *pCore);
    ~CXamlCompositionBrush() override {}

    _Check_return_ HRESULT CreateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush);

    bool CheckUIElementParent();
    _Check_return_ HRESULT FireOnConnected();
    _Check_return_ HRESULT FireOnDisconnected();

protected:
// CNoParentShareableDependencyObject overrides

    CXamlCompositionBrush(_In_ const CXamlCompositionBrush& original, _Out_ HRESULT& hr);

    // CNoParentShareableDependencyObject skips calling Enter/Leave unless it's the first enter/last leave. In those cases, we
    // still need to fire OnConnected/OnDisconnected.
    // Note: If the brush is parented to a CNoParentShareableDO then it's still a problem.
    _Check_return_ HRESULT OnSkippedLiveEnter() override;
    _Check_return_ HRESULT OnSkippedLiveLeave() override;

public:
// Creation method

    DECLARE_CREATE(CXamlCompositionBrush);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CXamlCompositionBrush>::Index;
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *namescopeOwner, EnterParams params) override;

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *namescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

// CNoParentShareableDependencyObject overrides

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CXamlCompositionBrush);

// CBrush overrides

    _Check_return_ HRESULT GetCompositionBrush(_In_opt_ const CUIElement* element, _In_opt_ WUComp::ICompositor* compositor, _Outptr_result_maybenull_ WUComp::ICompositionBrush** compBrush);
    void SetCompositionBrush(_In_opt_ WUComp::ICompositionBrush* compBrush);

    _Out_ WUComp::ICompositionBrush* GetBrushForContentRootNoRef(_In_ CContentRoot* contentRoot);
    void SetBrushForContentRoot(_In_ CContentRoot* contentRoot, _In_ WUComp::ICompositionBrush* brush);
    void ClearCompositionBrushMap();
    void ClearBrushForContentRoot(_In_ CContentRoot* contentRoot, _In_ WUComp::ICompositionBrush* brush);

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//     For XamlCompositionBrush, these behave like a SolidColorBrush using m_fallbackColor
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT UpdateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams
    ) final;

    _Check_return_ HRESULT GetPrintBrush(
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
    ) final;

    bool HasCompositionBrush();

public:
    XUINT32 m_fallbackColor = 0x00FFFFFF;   // Default fallback color is transparent white

    // There are two ways to store CompositionBrush(S) in this brush: the singleton, and the newer map.
    // It is up to the user to ensure the correct one is being referenced. When the render walk queries
    // for the brush, it will first check the map, and if there is no brush for that XamlRoot, it will
    // use the singleton brush.
    wrl::ComPtr<WUComp::ICompositionBrush> m_compositionBrush;
    containers::vector_map< xref::weakref_ptr<CContentRoot>, wrl::ComPtr<WUComp::ICompositionBrush> > m_brushMap;

private:
    // Only call OnConnected when first attached to a UIElement.
    bool m_isParentedToUIElement = false;

    // Stored fallback color brush in case of reuse.
    wrl::ComPtr<WUComp::ICompositionColorBrush> m_fallbackColorBrush;

    containers::vector_set<xref::weakref_ptr<CDependencyObject>> m_liveParents;
};
