// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Panel.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      This class is the type of the invisible root of the tree.
//      This differs from Canvas in the way it arranges its children - it
//      ensures that all the children are arranged with the plugin size.
//
//------------------------------------------------------------------------

class VisualTree;
class CPopupRoot;
class CLayoutManager;
class CFocusManager;

class CRootVisual final : public CPanel
{
private:
    CRootVisual(_In_ CCoreServices *pCore)
        : CPanel(pCore)
    {
    }

    ~CRootVisual() override;
public:
    DECLARE_CREATE(CRootVisual);

    _Check_return_ HRESULT InitInstance() override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRootVisual>::Index;
    }

    bool AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const final;

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // The internal root element does not have a managed counterpart.
        return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
    }

    bool GetIsLayoutElement() const final { return false; }

    void CleanupCompositionResources();

    _Check_return_ HRESULT SetAssociatedVisualTree(_In_ VisualTree *pVisualTree);
    _Ret_maybenull_ VisualTree* GetAssociatedVisualTree() const { return m_pVisualTree; }
    CPopupRoot* GetAssociatedPopupRootNoRef() const;
    CLayoutManager* GetAssociatedLayoutManager() const;
    _Check_return_ HRESULT GetAssociatedPublicRootNoRef(_Outptr_ CUIElement** ppPublicRoot) const;

    _Check_return_ HRESULT SetBackgroundColor(XUINT32 backgroundColor);

    CUIElement* GetRootScrollViewerOrCanvas();
    CFullWindowMediaRoot* GetFullWindowMediaRoot();

    // FrameworkElement overrides
protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;
    _Check_return_ HRESULT UpdateLayoutClip(bool forceClipToRenderSize) final;

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        ) override;

private:
    VisualTree* m_pVisualTree = nullptr;
};
