// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <framework.h>
#include <brush.h>

class CContentPresenter : public CFrameworkElement
{
protected:
    CContentPresenter(_In_ CCoreServices *pCore);
    ~CContentPresenter() override;

public:
    DECLARE_CREATE(CContentPresenter);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CContentPresenter>::Index;
    }


    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    static _Check_return_ HRESULT Content(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ CTransitionCollection* GetTransitionsForChildElementNoAddRef(_In_ CUIElement* pChild) final;

     bool HasDataBoundTextBlockText();

    _Check_return_ HRESULT GetTextBlockText(_Out_ HSTRING *returnText);

    CTextBlock *GetTextBlockNoRef();

    // Inherited property support
    _Check_return_ HRESULT PullInheritedTextFormatting() final;

    virtual CUIElement* GetTemplateChildNoRef();

    _Check_return_ HRESULT RemoveTemplateChild() override;

    bool IsMaskDirty(
        _In_ HWShapeRealization *pHwShapeRealization,
        const bool renderCollapsedMask,
        bool isFillBrushAnimated,
        bool isStrokeBrushAnimated,
        _Inout_ bool* pIsFillForHitTestOnly,
        _Inout_ bool* pIsStrokeForHitTestOnly
        ) final;

    xref_ptr<CBrush> GetBorderBrush() const final;
    xref_ptr<CBrush> GetBackgroundBrush() const final;
    XTHICKNESS GetBorderThickness() const final;
    XTHICKNESS GetPadding() const final;
    XCORNERRADIUS GetCornerRadius() const final;
    DirectUI::BackgroundSizing GetBackgroundSizing() const final;
    DirectUI::HorizontalAlignment GetHorizontalContentAlignment() const;
    DirectUI::VerticalAlignment GetVerticalContentAlignment() const;

protected:
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;
    xref_ptr<CControlTemplate> GetTemplate() const final;
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    bool NWIsContentDirty() final
    {
        return m_fNWBackgroundDirty || m_fNWBorderBrushDirty || CFrameworkElement::NWIsContentDirty();
    }
    _Check_return_ HRESULT PerformEmergencyInvalidation(_In_ CDependencyObject *pToBeReparented) final;

    void NWCleanDirtyFlags() override
    {
        m_fNWBackgroundDirty = FALSE;
        m_fNWBorderBrushDirty = FALSE;
        CFrameworkElement::NWCleanDirtyFlags();
    }

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    void GenerateContentBoundsImpl(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) final;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

private:
    _Check_return_ HRESULT OnContentTemplateChanged(_In_ const PropertyChangedParams& args);
    _Check_return_ HRESULT OnContentTemplateSelectorChanged(_In_ const PropertyChangedParams& args);
    _Check_return_ HRESULT Invalidate(_In_ bool bClearChildren);
    _Check_return_ HRESULT SetDataContext();
    _Check_return_ HRESULT PropagateInheritedProperty(_In_ CUIElement *pUIElement, _In_ const CDependencyProperty *pdp);

    // Fetches the child TextBlock of the default template if we are using the default template; null otherwise.
    xref_ptr<CTextBlock> GetTextBlockChildOfDefaultTemplate(_In_ bool fAllowNullContent);

    xref_ptr<CDataTemplate> GetSelectedContentTemplate() const;

    bool ParticipatesInUnloadingContentTransition();

public:
    // Create default content by code
    _Check_return_ HRESULT CreateDefaultContent(
        _In_opt_ const xstring_ptr *pDisplayMemberPath,
        _Outptr_ CDependencyObject** ppChild);
    static void NWSetBackgroundDirty(
        _In_ CDependencyObject *pTarget,
        DirtyFlags flags
        );
    static void NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags Flags);

    static DirectUI::OpticalMarginAlignment DefaultOpticalMarginAlignment()
    {
        return DirectUI::OpticalMarginAlignment::None;
    }
    static DirectUI::TextLineBounds DefaultTextLineBounds()
    {
        return DirectUI::TextLineBounds::Full;
    }
    static DirectUI::TextWrapping DefaultTextWrapping()
    {
        return DirectUI::TextWrapping::NoWrap;
    }
    static DirectUI::LineStackingStrategy DefaultLineStackingStrategy()
    {
        return DirectUI::LineStackingStrategy::MaxHeight;
    }

    bool IsUsingDefaultTemplate() const { return m_contentPresenterUsingDefaultTemplate;}

private:
    // NOTE: these are aligned this way to provide optimal packing. Please take that into consideration if adding anything.
    xref_ptr<CDependencyObject> m_pChildFromDefaultTemplate;

public:
    CTransitionCollection* m_pContentTransitions = nullptr; // This property appears to be used too frequently to benefit from sparsifying
    CDataTemplate* m_pContentTemplate = nullptr;

private:
    CValue m_content;
    CValue m_cachedContent;

    bool m_bInOnApplyTemplate : 1;
    bool m_bDataContextInvalid : 1;
    bool m_fNWBackgroundDirty : 1;
    bool m_fNWBorderBrushDirty : 1;
    bool m_contentPresenterUsingDefaultTemplate : 1;
    // bool m_unused6-32 :1;
};
