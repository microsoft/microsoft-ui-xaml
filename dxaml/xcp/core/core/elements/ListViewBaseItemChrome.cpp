// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "string.h"
#include "MetadataAPI.h"
#include <ContentRenderer.h>
#include <FocusRectOptions.h>
#include <RuntimeEnabledFeatures.h>
#include <ColorUtil.h>
#include "XamlCompositionBrush.h"
#include "DependencyObject.h"
#include "BrushTransition.g.h"
#include "CListViewBase.g.h"
#include "RootScale.h"

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get ListViewBaseItemChrome debug outputs, and 0 otherwise
#define LVBIC_DBG 0
//#define LVBIC_DEBUG

std::optional<bool> CListViewBaseItemChrome::s_isRoundedListViewBaseItemChromeEnabled;

// Size of check mark visual (old style).
const XSIZEF        CListViewBaseItemChrome::s_selectionCheckMarkVisualSize            = { 40.0f, 40.0f };

// Focus rectangle thickness.
const XTHICKNESS    CListViewBaseItemChrome::s_focusBorderThickness                    = { 2.0f, 2.0f, 2.0f, 2.0f };

// Offset from top-right corner of control border bounds.
const XPOINTF       CListViewBaseItemChrome::s_checkmarkOffset                         = { -20.0f, 6.0f };

// Offsets for swipe hint animations (horizontal and vertical).
const XPOINTF       CListViewBaseItemChrome::s_swipeHintOffset                         = { -23.0f, 15.0f };

// Opacity value for swiping hint checkmark.
const XFLOAT        CListViewBaseItemChrome::s_swipingCheckSteadyStateOpacity          = 0.5f;

// Opacity value for swiping hint checkmark when item is selecting.
const XFLOAT        CListViewBaseItemChrome::s_selectingSwipingCheckSteadyStateOpacity = 1.0f;

// Special value for opacity not being set.
const XFLOAT        CListViewBaseItemChrome::s_cOpacityUnset                           = -1.0f;

// Default corner radius of the outer selection border or backplate visual used when
// GetCornerRadius() returns 0 and IsRoundedListViewBaseItemChromeForced() returns True.
const XFLOAT        CListViewBaseItemChrome::s_generalCornerRadius                     = 4.0f;

// Minimum corner radius of inner border visual.
const XFLOAT        CListViewBaseItemChrome::s_innerBorderCornerRadius                 = 3.0f;

// Size of selection indicator rectangular visual.
const XSIZEF        CListViewBaseItemChrome::s_selectionIndicatorSize                  = { 3.0f, 16.0f };

// Selection indicator height decrease when pressed.
const XFLOAT        CListViewBaseItemChrome::s_selectionIndicatorHeightShrinkage       = 6.0f;

// Margin for the selection indicator rectangular visual.
const XTHICKNESS    CListViewBaseItemChrome::s_selectionIndicatorMargin                = { 4.0f, 20.0f, 0.0f, 20.0f };

// Size of multiselect square visual.
const XSIZEF        CListViewBaseItemChrome::s_multiSelectSquareSize                   = { 20.0f, 20.0f };

// Border thickness for the multiselect square.
const XTHICKNESS    CListViewBaseItemChrome::s_multiSelectSquareThickness              = { 2.0f, 2.0f, 2.0f, 2.0f };

// Border thickness for the multiselect square when rounded corners are applied.
const XTHICKNESS    CListViewBaseItemChrome::s_multiSelectRoundedSquareThickness       = { 1.0f, 1.0f, 1.0f, 1.0f };

// Margin for the multiselect square for inline (ListView)
const XTHICKNESS    CListViewBaseItemChrome::s_multiSelectSquareInlineMargin           = { 12.0f, 0.0f, 0.0f, 0.0f };

// Margin for the multiselect square for inline (ListView)
const XTHICKNESS    CListViewBaseItemChrome::s_multiSelectRoundedSquareInlineMargin    = { 14.0f, 0.0f, 0.0f, 0.0f };

// Margin for the multiselect square for overlay (GridView)
const XTHICKNESS    CListViewBaseItemChrome::s_multiSelectSquareOverlayMargin          = { 0.0f, 2.0f, 2.0f, 0.0f };

// Backplate margin.
const XTHICKNESS    CListViewBaseItemChrome::s_backplateMargin                         = { 4.0f, 2.0f, 4.0f, 2.0f };

// Unselected outer border thickness.
const XTHICKNESS    CListViewBaseItemChrome::s_borderThickness                         = { 1.0f, 1.0f, 1.0f, 1.0f };

// Extra inner selection border thickness compared to outer border.
const XTHICKNESS    CListViewBaseItemChrome::s_innerSelectionBorderThickness           = { 1.0f, 1.0f, 1.0f, 1.0f };

// Offset of ListViewItem multiselect content.
const XFLOAT        CListViewBaseItemChrome::s_listViewItemMultiSelectContentOffset    = 32.0f;

// Offset of ListViewItem multiselect content when rounded corners are applied.
const XFLOAT        CListViewBaseItemChrome::s_multiSelectRoundedContentOffset         = 28.0f;

// Points forming the CheckMark path
const XPOINTF       CListViewBaseItemChrome::s_checkMarkPoints[6]                      = { { 0.0f, 7.0f }, { 2.2f, 4.3f }, { 6.1f, 7.9f }, { 12.4f, 0.0f }, { 15.0f, 2.4f }, { 6.6f, 13.0f } };

// ListViewItem focus border thickness
const XFLOAT        CListViewBaseItemChrome::s_listViewItemFocusBorderThickness        = 1.0f;

// ListViewItem focus border thickness
const XFLOAT        CListViewBaseItemChrome::s_gridViewItemFocusBorderThickness        = 2.0f;

// FontSize of the CheckMark glyph
const XFLOAT        CListViewBaseItemChrome::s_checkMarkGlyphFontSize                  = 16.0f;

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strCheckMarkGlyphStorage, L"\uE73E");

// Template specializations for string to enum mappings.
template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::CommonStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::CommonStates>::s_map[] =
{
    { L"Normal",             CommonStates_Normal },
    { L"PointerOver",        CommonStates_PointerOver },
    { L"Pressed",            CommonStates_Pressed },
    { L"PointerOverPressed", CommonStates_PointerOverPressed },
    { L"Disabled",           CommonStates_Disabled }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::FocusStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::FocusStates>::s_map[] =
{
    { L"Focused",        FocusStates_Focused },
    { L"Unfocused",      FocusStates_Unfocused },
    { L"PointerFocused", FocusStates_PointerFocused }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionHintStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionHintStates>::s_map[] =
{
    { L"HorizontalSelectionHint", SelectionHintStates_HorizontalSelectionHint },
    { L"VerticalSelectionHint",   SelectionHintStates_VerticalSelectionHint },
    { L"NoSelectionHint",         SelectionHintStates_NoSelectionHint }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionStates>::s_map[] =
{
    { L"Unselecting",           SelectionStates_Unselecting },
    { L"Unselected",            SelectionStates_Unselected },
    { L"UnselectedPointerOver", SelectionStates_UnselectedPointerOver },
    { L"UnselectedSwiping",     SelectionStates_UnselectedSwiping },
    { L"Selecting",             SelectionStates_Selecting },
    { L"Selected",              SelectionStates_Selected },
    { L"SelectedSwiping",       SelectionStates_SelectedSwiping },
    { L"SelectedUnfocused",     SelectionStates_SelectedUnfocused }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DragStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DragStates>::s_map[] =
{
    { L"NotDragging",                 DragStates_NotDragging },
    { L"Dragging",                    DragStates_Dragging },
    { L"DraggingTarget",              DragStates_DraggingTarget },
    { L"MultipleDraggingPrimary",     DragStates_MultipleDraggingPrimary },
    { L"MultipleDraggingSecondary",   DragStates_MultipleDraggingSecondary },
    { L"DraggedPlaceholder",          DragStates_DraggedPlaceholder },
    { L"Reordering",                  DragStates_Reordering },
    { L"ReorderingTarget",            DragStates_ReorderingTarget },
    { L"MultipleReorderingPrimary",   DragStates_MultipleReorderingPrimary },
    { L"ReorderedPlaceholder",        DragStates_ReorderedPlaceholder },
    { L"DragOver",                    DragStates_DragOver }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::ReorderHintStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::ReorderHintStates>::s_map[] =
{
    { L"NoReorderHint",     ReorderHintStates_NoReorderHint },
    { L"BottomReorderHint", ReorderHintStates_BottomReorderHint },
    { L"TopReorderHint",    ReorderHintStates_TopReorderHint },
    { L"RightReorderHint",  ReorderHintStates_RightReorderHint },
    { L"LeftReorderHint",   ReorderHintStates_LeftReorderHint }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DataVirtualizationStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DataVirtualizationStates>::s_map[] =
{
    { L"DataAvailable",   DataVirtualizationStates_DataAvailable },
    { L"DataPlaceholder", DataVirtualizationStates_DataPlaceholder }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::CommonStates2>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::CommonStates2>::s_map[] =
{
    { L"Normal", CommonStates2_Normal },
    { L"PointerOver", CommonStates2_PointerOver },
    { L"Pressed", CommonStates2_Pressed },
    { L"Selected", CommonStates2_Selected },
    { L"PointerOverSelected", CommonStates2_PointerOverSelected },
    { L"PressedSelected", CommonStates2_PressedSelected }
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DisabledStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::DisabledStates>::s_map[] =
{
    { L"Enabled", DisabledStates_Enabled },
    { L"Disabled", DisabledStates_Disabled },
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::MultiSelectStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::MultiSelectStates>::s_map[] =
{
    { L"MultiSelectDisabled", MultiSelectStates_MultiSelectDisabled },
    { L"MultiSelectEnabled", MultiSelectStates_MultiSelectEnabled },
};

template<>
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionIndicatorStates>::MapItemType
CListViewBaseItemChrome::Mapping<CListViewBaseItemChrome::SelectionIndicatorStates>::s_map[] =
{
    { L"SelectionIndicatorDisabled", SelectionIndicatorStates_SelectionIndicatorDisabled },
    { L"SelectionIndicatorEnabled", SelectionIndicatorStates_SelectionIndicatorEnabled },
};
// Helpers

void CListViewBaseItemChrome::LayoutRoundHelper(
    _Inout_ XSIZEF* pSize)
{
    pSize->width  = LayoutRound(pSize->width);
    pSize->height = LayoutRound(pSize->height);
}

void CListViewBaseItemChrome::LayoutRoundHelper(
    _Inout_ XTHICKNESS* pThickness)
{
    pThickness->left    = LayoutRound(pThickness->left);
    pThickness->right   = LayoutRound(pThickness->right);
    pThickness->top     = LayoutRound(pThickness->top);
    pThickness->bottom  = LayoutRound(pThickness->bottom);
}

XRECTF CListViewBaseItemChrome::LayoutRoundHelper(
    _In_ const XRECTF& bounds)
{
    XRECTF newBounds;

    newBounds.X      = LayoutRound(bounds.X);
    newBounds.Y      = LayoutRound(bounds.Y);
    newBounds.Width  = LayoutRound(bounds.Width + bounds.X);
    newBounds.Height = LayoutRound(bounds.Height + bounds.Y);
    newBounds.Width  -= newBounds.X;
    newBounds.Height -= newBounds.Y;

    return newBounds;
}

bool CListViewBaseItemChrome::ShouldUseLayoutRounding()
{
    // Similar to what Borders do, but we don't care about corner radius (ours is always 0).
    const auto scale = RootScale::GetRasterizationScaleForElement(this);
    return (scale != 1.0f) && GetUseLayoutRounding();
}

inline _Check_return_ HRESULT CListViewBaseItemChrome::AddRectangle(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds,
    _In_ CBrush* pBrush,
    _In_opt_ CUIElement *pUIElement
    )
{
    if (!IsNullCompositionBrush(pBrush))
    {
        BrushParams brushParams;
        brushParams.m_element = pUIElement;
        brushParams.m_brushProperty = ElementBrushProperty::Fill;
        IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
            bounds,
            bounds,
            pBrush,
            brushParams,
            pUIElement,
            nullptr,    // pNinegrid
            nullptr,    // pShapeHwTexture
            false));     // fIsHollow
    }

    return S_OK;
}

inline _Check_return_ HRESULT CListViewBaseItemChrome::AddBorder(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds,
    _In_ const XTHICKNESS& thickness,
    _In_ CBrush* pBrush,
    _In_opt_ CListViewBaseItemChrome* listViewItemChrome
    )
{
    CMILMatrix            brushToElement(TRUE);
    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams        localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);
    TransformAndClipStack transformsAndClips;
    XRECTF                localBounds = { 0.0f, 0.0f, bounds.Width, bounds.Height };

    if (!IsNullCompositionBrush(pBrush))
    {
        if (bounds.X != 0.0f ||
            bounds.Y != 0.0f)
        {
            // Borders are ninegrids which need to have (X, Y) = (0, 0).
            // Apply transform in case if it is not aligned at the origin.
            IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
            brushToElement.AppendTranslation(bounds.X, bounds.Y);
            transformsAndClips.PrependTransform(brushToElement);
            localRP.pTransformsAndClipsToCompNode = &transformsAndClips;
        }

        BrushParams emptyBrushParams;
        IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
            localBounds,
            localBounds,
            pBrush,
            emptyBrushParams,
            listViewItemChrome,
            &thickness,     // pNinegrid
            nullptr,        // pShapeHwTexture
            true));         // fIsHollow
    }

    return S_OK;
}

inline _Check_return_ HRESULT CListViewBaseItemChrome::AddChromeAssociatedPath(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds,
    _In_ const CMILMatrix& transform,
    _In_ CBrush* pBrush,
    _In_ XFLOAT opacityMultiplier,
    _In_ bool primaryChrome
    )
{
    HRESULT hr = S_OK;

    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams        localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);
    TransformAndClipStack transformsAndClips;
    ChromedPathFillPart*  pFillMaskPart = NULL;
    CUIElement*           pCacheUIElement = NULL;

    IFC(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(transform);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;
    localRP.opacityToCompNode *= opacityMultiplier;

    if (primaryChrome)
    {
        pCacheUIElement = this;
        pFillMaskPart = new ChromedPathFillPart(this, pBrush);
    }
    else
    {
        ASSERT(m_pSecondaryChrome != NULL);
        pCacheUIElement = m_pSecondaryChrome;
        pFillMaskPart = new ChromedPathFillPart(m_pSecondaryChrome, pBrush);
    }

    IFC(pContentRenderer->MaskCombinedRenderHelper(
        pCacheUIElement,
        nullptr, /*pStrokeMaskMart*/
        pFillMaskPart,
        false    /*forceMaskDirty*/
        ));

Cleanup:
    delete pFillMaskPart;
    return hr;
}

// Commands

ListViewBaseItemAnimationCommand::ListViewBaseItemAnimationCommand(
    _In_ bool isStarting,
    _In_ bool steadyStateOnly)
    : m_isStarting(isStarting)
    , m_steadyStateOnly(steadyStateOnly)
{}

ListViewBaseItemAnimationCommand::~ListViewBaseItemAnimationCommand()
{}

ListViewBaseItemAnimationCommand_Pressed::ListViewBaseItemAnimationCommand_Pressed(
    _In_ bool pressed,
    _In_ xref::weakref_ptr<CUIElement> pAnimationTarget,
    _In_ bool isStarting,
    _In_ bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_pressed(pressed)
    , m_pAnimationTarget(std::move(pAnimationTarget))
{
}

ListViewBaseItemAnimationCommand_Pressed::~ListViewBaseItemAnimationCommand_Pressed()
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_Pressed::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    RRETURN(pVisitor->VisitAnimationCommand(this));
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_Pressed::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_Pressed(m_pressed, m_pAnimationTarget, m_isStarting, m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_Pressed::GetPriority() const
{
    return 3;
}

ListViewBaseItemAnimationCommand_ReorderHint::ListViewBaseItemAnimationCommand_ReorderHint(
    _In_ XFLOAT offsetX,
    _In_ XFLOAT offsetY,
    _In_ xref::weakref_ptr<CListViewBaseItemChrome> pAnimationTarget,
    _In_ bool isStarting,
    _In_ bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_pAnimationTarget(std::move(pAnimationTarget))
    , m_offsetX(offsetX)
    , m_offsetY(offsetY)
{
}

ListViewBaseItemAnimationCommand_ReorderHint::~ListViewBaseItemAnimationCommand_ReorderHint()
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_ReorderHint::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    RRETURN(pVisitor->VisitAnimationCommand(this));
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_ReorderHint::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_ReorderHint(m_offsetX, m_offsetY, m_pAnimationTarget, m_isStarting, m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_ReorderHint::GetPriority() const
{
    return 1;
}

ListViewBaseItemAnimationCommand_DragDrop::ListViewBaseItemAnimationCommand_DragDrop(
    _In_ DragDropState state,
    _In_ xref::weakref_ptr<CListViewBaseItemChrome> pBaseAnimationTarget,
    _In_ xref::weakref_ptr<CFrameworkElement> pFadeOutAnimationTarget,
    _In_ bool isStarting,
    _In_ bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_pBaseAnimationTarget(std::move(pBaseAnimationTarget))
    , m_pFadeOutAnimationTarget(std::move(pFadeOutAnimationTarget))
    , m_state(state)
{
}

ListViewBaseItemAnimationCommand_DragDrop::~ListViewBaseItemAnimationCommand_DragDrop()
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_DragDrop::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    RRETURN(pVisitor->VisitAnimationCommand(this));
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_DragDrop::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_DragDrop(m_state, m_pBaseAnimationTarget, m_pFadeOutAnimationTarget, m_isStarting, m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_DragDrop::GetPriority() const
{
    return (m_state == ListViewBaseItemAnimationCommand_DragDrop::DragDropState_Target) ? 1 : 0;
}

ListViewBaseItemAnimationCommand_MultiSelect::ListViewBaseItemAnimationCommand_MultiSelect(
    bool isRoundedListViewBaseItemChromeEnabled,
    bool entering,
    double checkBoxTranslationX,
    double contentTranslationX,
    DirectUI::ListViewItemPresenterCheckMode checkMode,
    xref::weakref_ptr<CUIElement> multiSelectCheckBox,
    xref::weakref_ptr<CUIElement> contentPresenter,
    bool isStarting,
    bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_isRoundedListViewBaseItemChromeEnabled(isRoundedListViewBaseItemChromeEnabled)
    , m_entering(entering)
    , m_checkBoxTranslationX(checkBoxTranslationX)
    , m_contentTranslationX(contentTranslationX)
    , m_checkMode(checkMode)
    , m_multiSelectCheckBox(std::move(multiSelectCheckBox))
    , m_contentPresenter(std::move(contentPresenter))
{
}

ListViewBaseItemAnimationCommand_MultiSelect::~ListViewBaseItemAnimationCommand_MultiSelect()
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_MultiSelect::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    RRETURN(pVisitor->VisitAnimationCommand(this));
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_MultiSelect::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_MultiSelect(
        m_isRoundedListViewBaseItemChromeEnabled,
        m_entering,
        m_checkBoxTranslationX,
        m_contentTranslationX,
        m_checkMode,
        m_multiSelectCheckBox,
        m_contentPresenter,
        m_isStarting,
        m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_MultiSelect::GetPriority() const
{
    return 3;
}

ListViewBaseItemAnimationCommand_IndicatorSelect::ListViewBaseItemAnimationCommand_IndicatorSelect(
    bool entering,
    double translationX,
    DirectUI::ListViewItemPresenterSelectionIndicatorMode selectionIndicatorMode,
    xref::weakref_ptr<CUIElement> selectionIndicator,
    xref::weakref_ptr<CUIElement> contentPresenter,
    bool isStarting,
    bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_entering(entering)
    , m_translationX(translationX)
    , m_selectionIndicatorMode(selectionIndicatorMode)
    , m_selectionIndicator(std::move(selectionIndicator))
    , m_contentPresenter(std::move(contentPresenter))
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_IndicatorSelect::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    IFC_RETURN(pVisitor->VisitAnimationCommand(this));

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_IndicatorSelect::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_IndicatorSelect(
        m_entering,
        m_translationX,
        m_selectionIndicatorMode,
        m_selectionIndicator,
        m_contentPresenter,
        m_isStarting,
        m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_IndicatorSelect::GetPriority() const
{
    return 3;
}

ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility::ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility(
    bool selected,
    double fromScale,
    xref::weakref_ptr<CUIElement> selectionIndicator,
    bool isStarting,
    bool steadyStateOnly)
    : ListViewBaseItemAnimationCommand(isStarting, steadyStateOnly)
    , m_selected(selected)
    , m_fromScale(fromScale)
    , m_selectionIndicator(std::move(selectionIndicator))
{
}

ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility::~ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility()
{
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility::Accept(
    _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor)
{
    IFC_RETURN(pVisitor->VisitAnimationCommand(this));

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility::Clone(
    _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone)
{
    clone.reset(new ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility(
        m_selected,
        m_fromScale,
        m_selectionIndicator,
        m_isStarting,
        m_steadyStateOnly));

    return S_OK;
}

XINT32 ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility::GetPriority() const
{
    return 3;
}

// CListViewBaseItemChrome

CListViewBaseItemChrome::CListViewBaseItemChrome(_In_ CCoreServices *pCore)
    : CContentPresenter(pCore)
    , m_isFocusVisualDrawnByFocusManager(false)
    , m_fFillBrushDirty(false)
    , m_shouldRenderChrome(true)
    , m_isInIndicatorSelect(false)
    , m_isInMultiSelect(false)
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: CListViewBaseItemChrome - entry.", this));
    }
#endif // LVBIC_DEBUG

    m_visualStates.commonState              = CommonStates_Normal;
    m_visualStates.focusState               = FocusStates_Unfocused;
    m_visualStates.selectionHintState       = SelectionHintStates_NoSelectionHint;
    m_visualStates.selectionState           = SelectionStates_Unselected;
    m_visualStates.dragState                = DragStates_NotDragging;
    m_visualStates.reorderHintState         = ReorderHintStates_NoReorderHint;
    m_visualStates.dataVirtualizationState  = DataVirtualizationStates_DataAvailable;

    m_visualStates.commonState2             = CommonStates2_Normal;
    m_visualStates.disabledState            = DisabledStates_Enabled;
    m_visualStates.multiSelectState         = MultiSelectStates_MultiSelectDisabled;
    m_visualStates.selectionIndicatorState  = SelectionIndicatorStates_SelectionIndicatorDisabled;

    EmptyRectF(&m_checkGeometryBounds);
    EmptyRectF(&m_contentMargin);
}

CListViewBaseItemChrome::~CListViewBaseItemChrome()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: ~CListViewBaseItemChrome - entry.", this));
    }
#endif // LVBIC_DEBUG

    // Removes and disposes all commands from the animation command queue.
    for (auto commandsIterator = m_animationCommands.begin(); commandsIterator != m_animationCommands.end(); ++commandsIterator)
    {
        delete *commandsIterator;
    }

    ReleaseInterface(m_pCheckHintBrush);
    ReleaseInterface(m_pCheckSelectingBrush);
    ReleaseInterface(m_pCheckBrush);
    ReleaseInterface(m_pCheckPressedBrush);
    ReleaseInterface(m_pCheckDisabledBrush);
    ReleaseInterface(m_pDragBackground);
    ReleaseInterface(m_pDragForeground);
    ReleaseInterface(m_pFocusBorderBrush);
    ReleaseInterface(m_pPlaceholderBackground);
    ReleaseInterface(m_pPointerOverBorderBrush);
    ReleaseInterface(m_pPointerOverBackground);
    ReleaseInterface(m_pPointerOverForeground);
    ReleaseInterface(m_pSelectedBackground);
    ReleaseInterface(m_pSelectedForeground);
    ReleaseInterface(m_pSelectedPointerOverBackground);
    ReleaseInterface(m_pSelectedPointerOverBorderBrush);
    ReleaseInterface(m_pSelectedBorderBrush);
    ReleaseInterface(m_pSelectedPressedBorderBrush);
    ReleaseInterface(m_pSelectedDisabledBorderBrush);
    ReleaseInterface(m_pSelectedInnerBorderBrush);
    ReleaseInterface(m_pSelectedPressedBackground);
    ReleaseInterface(m_pSelectedDisabledBackground);
    ReleaseInterface(m_pPressedBackground);
    ReleaseInterface(m_pCheckBoxBrush);
    ReleaseInterface(m_pCheckBoxPointerOverBrush);
    ReleaseInterface(m_pCheckBoxPressedBrush);
    ReleaseInterface(m_pCheckBoxDisabledBrush);
    ReleaseInterface(m_pCheckBoxSelectedBrush);
    ReleaseInterface(m_pCheckBoxSelectedPointerOverBrush);
    ReleaseInterface(m_pCheckBoxSelectedPressedBrush);
    ReleaseInterface(m_pCheckBoxSelectedDisabledBrush);
    ReleaseInterface(m_pCheckBoxBorderBrush);
    ReleaseInterface(m_pCheckBoxPointerOverBorderBrush);
    ReleaseInterface(m_pCheckBoxPressedBorderBrush);
    ReleaseInterface(m_pCheckBoxDisabledBorderBrush);
    ReleaseInterface(m_pSelectionIndicatorBrush);
    ReleaseInterface(m_pSelectionIndicatorPointerOverBrush);
    ReleaseInterface(m_pSelectionIndicatorPressedBrush);
    ReleaseInterface(m_pSelectionIndicatorDisabledBrush);
    ReleaseInterface(m_pFocusSecondaryBorderBrush);
    ReleaseInterface(m_pDragItemsCountTextBlock);
    ReleaseInterface(m_pCurrentEarmarkBrush);
    ReleaseInterface(m_pCheckGeometryData);
    ReleaseInterface(m_pSecondaryChrome);
}

template <typename VisualStateType>
bool CListViewBaseItemChrome::UpdateVisualStateGroup(
    _In_z_ const WCHAR* pName,
    _Inout_ VisualStateType* pState,
    _Out_ bool* pStateFound)
{
    typedef Mapping<VisualStateType> MappingType;
    typedef typename MappingType::MapItemType MapItemType;

    XINT32 mapSize = sizeof(MappingType::s_map) / sizeof(MapItemType);
    *pStateFound = FALSE;

    for (XINT32 i = 0; i < mapSize; ++i)
    {
        MapItemType& item = MappingType::s_map[i];

        if (wcscmp(item.m_pName, pName) == 0)
        {
            *pStateFound = TRUE;

            if (item.m_pEnumValue != *pState)
            {
                *pState = item.m_pEnumValue;
                return true;
            }

            return false;
        }
    }
    return false;
}

// Given chrome bounds, updates transformation matrix for checkmark taking into consideration layout rounding and RTL.
void
CListViewBaseItemChrome::AppendCheckmarkTransform(
    _In_ XRECTF chromeBounds,
    _Inout_ CMILMatrix* pMatrix)
{
    XPOINTF checkmarkTranslate = { };
    XFLOAT rtlCorrection = 0.0f;

    if (m_isRightToLeft)
    {
        // Unflip checkmark for RTL.
        pMatrix->Scale(-1.0, 1.0);
        rtlCorrection = m_checkGeometryBounds.right - m_checkGeometryBounds.left;
    }

    checkmarkTranslate.x = chromeBounds.Width - m_contentMargin.right + s_checkmarkOffset.x + rtlCorrection;
    checkmarkTranslate.y = m_contentMargin.top + s_checkmarkOffset.y;

    if (ShouldUseLayoutRounding())
    {
        checkmarkTranslate.x = LayoutRound(checkmarkTranslate.x);
        checkmarkTranslate.y = LayoutRound(checkmarkTranslate.y);
    }

    checkmarkTranslate.x = MAX(0.0f, checkmarkTranslate.x);
    checkmarkTranslate.y = MAX(0.0f, checkmarkTranslate.y);

    pMatrix->AppendTranslation(checkmarkTranslate.x, checkmarkTranslate.y);
}

// Given chrome bounds, updates transformation matrix for earmark.
void
CListViewBaseItemChrome::AppendEarmarkTransform(
    _In_ XRECTF chromeBounds,
    _Inout_ CMILMatrix* pMatrix)
{
    XPOINTF earmarkTranslate = { };

    earmarkTranslate.x = chromeBounds.Width - m_contentMargin.right - s_selectionCheckMarkVisualSize.width;
    earmarkTranslate.y = m_contentMargin.top;

    if (ShouldUseLayoutRounding())
    {
        earmarkTranslate.x = LayoutRound(earmarkTranslate.x);
        earmarkTranslate.y = LayoutRound(earmarkTranslate.y);
    }

    earmarkTranslate.x = MAX(0.0f, earmarkTranslate.x);
    earmarkTranslate.y = MAX(0.0f, earmarkTranslate.y);

    pMatrix->AppendTranslation(earmarkTranslate.x, earmarkTranslate.y);
}

_Check_return_ HRESULT
CListViewBaseItemChrome::MeasureOverride(
    _In_ XSIZEF availableSize,
    _Out_ XSIZEF& desiredSize)
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: MeasureOverride - entry.", this));
    }
#endif // LVBIC_DEBUG

    IFC_RETURN(MeasureNewStyle(availableSize, desiredSize));

    return S_OK;
}

_Check_return_
HRESULT
CListViewBaseItemChrome::ArrangeOverride(
    _In_ XSIZEF finalSize,
    _Out_ XSIZEF& newFinalSize)
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: ArrangeOverride - entry.", this));
    }
#endif // LVBIC_DEBUG

    IFC_RETURN(ArrangeNewStyle(finalSize, newFinalSize));

    return S_OK;
}

bool CListViewBaseItemChrome::HasTemplateChild()
{
    return GetTemplateChildIfExists() != nullptr;
}

_Check_return_ HRESULT CListViewBaseItemChrome::AddTemplateChild(_In_ CUIElement* pUI)
{
    ASSERT(GetTemplateChildIfExists() == NULL);

    IFC_RETURN(InsertChild(m_backplateRectangle ? 1 : 0, pUI));

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::RemoveTemplateChild()
{
    CUIElement* pTemplateChild = GetTemplateChildIfExists();

    if (pTemplateChild != NULL)
    {
        IFC_RETURN(RemoveChild(pTemplateChild));
    }

    return S_OK;
}

CUIElement* CListViewBaseItemChrome::GetTemplateChildNoRef()
{
    return GetTemplateChildIfExists();
}

CUIElement* CListViewBaseItemChrome::GetTemplateChildIfExists()
{
    CUIElement* templateChild = GetFirstChildNoAddRef();

    if (m_backplateRectangle && templateChild == m_backplateRectangle)
    {
        CUIElementCollection* childrenNoRef = GetChildren();

        if (childrenNoRef->GetCount() > 1)
        {
            templateChild = static_cast<CUIElement*>(childrenNoRef->GetItemWithAddRef(1));
            ReleaseInterfaceNoNULL(templateChild);
        }
    }

    if (templateChild != m_pDragItemsCountTextBlock &&
        templateChild != m_pSecondaryChrome &&
        templateChild != m_multiSelectCheckBoxRectangle &&
        templateChild != m_selectionIndicatorRectangle &&
        templateChild != m_innerSelectionBorder &&
        templateChild != m_outerBorder &&
        templateChild != m_backplateRectangle)
    {
        return templateChild;
    }
    else
    {
        return nullptr;
    }
}

// Raise exception if no appropriate parent is set.
_Check_return_ HRESULT
CListViewBaseItemChrome::SetWrongParentError()
{
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strGridViewItem, L"GridViewItem");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strListViewItem, L"ListViewItem");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strGridViewItemPresenter, L"GridViewItemPresenter");
    DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strListViewItemPresenter, L"ListViewItemPresenter");

    xephemeral_string_ptr parameters[2];
    bool isOfExpectedType = false;

    if (OfTypeByIndex<KnownTypeIndex::GridViewItemPresenter>())
    {
        c_strGridViewItemPresenter.Demote(&parameters[0]);
        c_strGridViewItem.Demote(&parameters[1]);
        isOfExpectedType = TRUE;
    }
    else if (OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        c_strListViewItemPresenter.Demote(&parameters[0]);
        c_strListViewItem.Demote(&parameters[1]);
        isOfExpectedType = TRUE;
    }

    HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
    if (isOfExpectedType)
    {
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_CHROME_WRONG_PARENT, 2, parameters));
    }

    IFC_RETURN(hrToOriginate);

    return S_OK;
}

// Retrieve parent item or raise exception if no parent is set.
_Check_return_ HRESULT
CListViewBaseItemChrome::GetParentListViewBaseItemNoRef(
    _Outptr_ CContentControl** ppParentListViewBaseItem)
{
    *ppParentListViewBaseItem = nullptr;

    if (m_pParentListViewBaseItemNoRef)
    {
        *ppParentListViewBaseItem = m_pParentListViewBaseItemNoRef;
    }
    else
    {
        IFC_RETURN(SetWrongParentError());
    }

    return S_OK;
}

CListViewBase* CListViewBaseItemChrome::GetParentListViewBase(_In_opt_ CContentControl* listViewBaseItem) const
{
    CDependencyObject* parent = listViewBaseItem;
    while (parent != nullptr && !parent->OfTypeByIndex<KnownTypeIndex::ListViewBase>())
    {
        parent = parent->GetParentFollowPopups();
    }
    return (parent != nullptr) ? static_cast<CListViewBase*>(parent) : nullptr;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::GoToChromedState(
    _In_z_ const WCHAR *pStateName,
    _In_ bool useTransitions,
    _Out_ bool* pWentToState)
{
    IFC_RETURN(GoToChromedStateNewStyle(pStateName, useTransitions, pWentToState));

    return S_OK;
}

// Draws the base layer (swipe hint).
_Check_return_ HRESULT
CListViewBaseItemChrome::DrawBaseLayer(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds)
{
    // Draw selection hint check mark
    if (GetSelectionCheckMarkVisualEnabled())
    {
        CBrush* pHintCheckBrushNoRef = NULL;

        if (m_visualStates.HasState(SelectionHintStates_NoSelectionHint))
        {
            // No selection hint. Use the selecting brush.
            pHintCheckBrushNoRef = m_pCheckSelectingBrush;
        }
        else
        {
            // Selection hint. Use the hint brush.
            pHintCheckBrushNoRef = m_pCheckHintBrush;
        }

        if (pHintCheckBrushNoRef &&
            (m_visualStates.HasState(SelectionStates_Selecting) ||
             m_visualStates.HasState(SelectionStates_UnselectedSwiping) ||
             (!m_visualStates.HasState(SelectionHintStates_NoSelectionHint) &&
              !m_visualStates.HasState(SelectionStates_Selected) &&
              !m_visualStates.HasState(SelectionStates_SelectedSwiping) &&
              !m_visualStates.HasState(SelectionStates_SelectedUnfocused))))
        {
            CMILMatrix brushToElement(TRUE);
            XFLOAT opacityMultiplier = 0.0f;

            // Store the brush away so we can access it when we're generating our edges.
            if (m_currentCheckBrush.get() != pHintCheckBrushNoRef)
            {
                m_currentCheckBrush = pHintCheckBrushNoRef;
            }

            AppendCheckmarkTransform(
                bounds,
                &brushToElement);

            if (m_swipeHintCheckOpacity != s_cOpacityUnset)
            {
                // This is an external override for opacity set during animation.
                opacityMultiplier = m_swipeHintCheckOpacity;
            }
            else if (m_visualStates.HasState(SelectionStates_UnselectedSwiping))
            {
                opacityMultiplier = s_swipingCheckSteadyStateOpacity;
            }
            else if (m_visualStates.HasState(SelectionStates_Selecting))
            {
                opacityMultiplier = s_selectingSwipingCheckSteadyStateOpacity;
            }

            IFC_RETURN(AddChromeAssociatedPath(
                pContentRenderer,
                bounds,
                brushToElement,
                pHintCheckBrushNoRef,
                opacityMultiplier,
                TRUE // primaryChrome
                ));
        }
    }

    return S_OK;
}

// Draws the below-content layer (selection visuals, etc).
_Check_return_ HRESULT
CListViewBaseItemChrome::DrawUnderContentLayer(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds)
{
    bool hasPointerOver = m_visualStates.HasState(CommonStates_PointerOver) ||
        m_visualStates.HasState(CommonStates_PointerOverPressed);

    // Local, layout rounded values
    XRECTF controlBorderBounds = { };
    XTHICKNESS contentMargin   = m_contentMargin;

    CContentControl* pParentListViewBaseItemNoRef = NULL;

    IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&contentMargin);
    }

    controlBorderBounds = bounds;
    CSizeUtil::Deflate(&controlBorderBounds, contentMargin);

    // Pointer over background
    if (hasPointerOver &&
        m_pPointerOverBackground)
    {
        XRECTF pointerOverBounds               = { };
        XTHICKNESS pointerOverBackgroundMargin = GetPointerOverBackgroundMargin();

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&pointerOverBackgroundMargin);
        }

        pointerOverBounds = bounds;
        CSizeUtil::Deflate(&pointerOverBounds, pointerOverBackgroundMargin);

        IFC_RETURN(AddRectangle(
            pContentRenderer,
            pointerOverBounds,
            m_pPointerOverBackground,
            this
            ));
    }

    // Focus border
    if (m_visualStates.HasState(FocusStates_Focused) &&
        m_pFocusBorderBrush)
    {
        XTHICKNESS focusBorderThickness = s_focusBorderThickness;

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&focusBorderThickness);
        }

        IFC_RETURN(AddBorder(
            pContentRenderer,
            bounds,
            focusBorderThickness,
            m_pFocusBorderBrush,
            this
            ));
    }

    // Selection background
    if (m_visualStates.HasState(SelectionStates_Selected) ||
        m_visualStates.HasState(SelectionStates_SelectedSwiping) ||
        m_visualStates.HasState(SelectionStates_Selecting) ||
        m_visualStates.HasState(SelectionStates_SelectedUnfocused))
    {
        CBrush* pSelectedBackgroundBrushNoRef = (hasPointerOver) ? m_pSelectedPointerOverBackground : m_pSelectedBackground;

        if (pSelectedBackgroundBrushNoRef)
        {
            IFC_RETURN(AddRectangle(
                pContentRenderer,
                controlBorderBounds,
                pSelectedBackgroundBrushNoRef,
                this
                ));
        }
    }

    // Control background
    if (pParentListViewBaseItemNoRef->m_pBackground)
    {
        IFC_RETURN(AddRectangle(
            pContentRenderer,
            controlBorderBounds,
            pParentListViewBaseItemNoRef->m_pBackground,
            this
            ));
    }

    return S_OK;
}

// Draws the above-content layer (swipe hint check, etc).
_Check_return_ HRESULT
CListViewBaseItemChrome::DrawOverContentLayer(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds
    )
{
    bool isSelected = m_visualStates.HasState(SelectionStates_Selected) ||
        m_visualStates.HasState(SelectionStates_SelectedSwiping) ||
        m_visualStates.HasState(SelectionStates_SelectedUnfocused);

    bool hasPointerOver = m_visualStates.HasState(CommonStates_PointerOver) ||
        m_visualStates.HasState(CommonStates_PointerOverPressed);

    CBrush* pSelectedBorderBrushNoRef = (hasPointerOver) ? m_pSelectedPointerOverBorderBrush : m_pSelectedBackground;

    XRECTF controlBorderBounds        = { };
    XTHICKNESS contentMargin          = m_contentMargin;
    XTHICKNESS controlBorderThickness = { };

    CContentControl* pParentListViewBaseItemNoRef = NULL;

    IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

    controlBorderThickness = pParentListViewBaseItemNoRef->m_borderThickness;

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&contentMargin);
        LayoutRoundHelper(&controlBorderThickness);
    }

    controlBorderBounds = bounds;
    CSizeUtil::Deflate(&controlBorderBounds, contentMargin);

    // Placeholder overlay.
    if (m_visualStates.HasState(DataVirtualizationStates_DataPlaceholder) &&
        m_pPlaceholderBackground)
    {
        XRECTF placeholderBounds = { };

        placeholderBounds = controlBorderBounds;
        CSizeUtil::Deflate(&placeholderBounds, controlBorderThickness);

        IFC_RETURN(AddRectangle(
            pContentRenderer,
            placeholderBounds,
            m_pPlaceholderBackground,
            this
            ));
    }

    // Control border
    if (pParentListViewBaseItemNoRef->m_pBorderBrush)
    {
        IFC_RETURN(AddBorder(
            pContentRenderer,
            controlBorderBounds,
            controlBorderThickness,
            pParentListViewBaseItemNoRef->m_pBorderBrush,
            this
            ));
    }

    // Selected border.  Paint it before earmark and checkmark, so it does not obscure it.
    if ((isSelected || m_visualStates.HasState(SelectionStates_Selecting)) &&
        pSelectedBorderBrushNoRef)
    {
        XTHICKNESS selectedBorderThickness = GetSelectedBorderThickness();

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&selectedBorderThickness);
        }

        IFC_RETURN(AddBorder(
            pContentRenderer,
            controlBorderBounds,
            selectedBorderThickness,
            pSelectedBorderBrushNoRef,
            this
            ));
    }

    // Earmark
    if (isSelected &&
        GetSelectionCheckMarkVisualEnabled() &&
        pSelectedBorderBrushNoRef)
    {
        CMILMatrix brushToElement(TRUE);

        // Store the brush away so the secondary chrome can access it when it is
        // generating its edges.
        if (m_pCurrentEarmarkBrush != pSelectedBorderBrushNoRef)
        {
            ReleaseInterface(m_pCurrentEarmarkBrush);
            m_pCurrentEarmarkBrush = pSelectedBorderBrushNoRef;
            AddRefInterface(m_pCurrentEarmarkBrush);
        }

        AppendEarmarkTransform(
            bounds,
            &brushToElement);

        IFC_RETURN(AddChromeAssociatedPath(
            pContentRenderer,
            bounds,
            brushToElement,
            pSelectedBorderBrushNoRef,
            1.0f,  // opacityMultiplier
            FALSE  // primaryChrome
            ));
    }

    // Checkmark
    if (isSelected &&
        GetSelectionCheckMarkVisualEnabled() &&
        m_pCheckBrush)
    {
        CMILMatrix brushToElement(TRUE);

        // Store the brush away so we can access it when we're generating our edges.
        if (m_currentCheckBrush.get() != m_pCheckBrush)
        {
            m_currentCheckBrush = m_pCheckBrush;
        }

        AppendCheckmarkTransform(
            bounds,
            &brushToElement);

        IFC_RETURN(AddChromeAssociatedPath(
            pContentRenderer,
            bounds,
            brushToElement,
            m_pCheckBrush,
            1.0f, // opacityMultiplier
            TRUE  // primaryChrome
            ));
    }

    return S_OK;
}

// Draws the above-content drag overlay.
_Check_return_ HRESULT
CListViewBaseItemChrome::DrawDragOverlayLayer(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds)
{
    // Drag overlay.
    if (m_visualStates.HasState(DragStates_MultipleDraggingPrimary) &&
        m_pDragBackground)
    {
        XRECTF controlBorderBounds = { };
        XTHICKNESS contentMargin   = m_contentMargin;

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&contentMargin);
        }

        controlBorderBounds = bounds;
        CSizeUtil::Deflate(&controlBorderBounds, contentMargin);

        IFC_RETURN(AddRectangle(
            pContentRenderer,
            controlBorderBounds,
            m_pDragBackground,
            this
            ));
    }

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::RenderLayer(
    _In_ IContentRenderer* pContentRenderer,
    _In_ ListViewBaseItemChromeLayerPosition layer
    )
{
    if (m_shouldRenderChrome && GetActualWidth() > 0 && GetActualHeight() > 0)
    {
        XRECTF bounds = { 0.0f, 0.0f, GetActualWidth(), GetActualHeight() };

        if (ShouldUseLayoutRounding())
        {
            bounds = LayoutRoundHelper(bounds);
        }

        if (ShouldDrawUnderContentLayerHere(layer))
        {
            IFC_RETURN(DrawUnderContentLayerNewStyle(pContentRenderer, bounds));
        }

        if (ShouldDrawOverContentLayerHere(layer))
        {
            IFC_RETURN(DrawOverContentLayerNewStyle(pContentRenderer, bounds));
        }
    }

    return S_OK;
}

// Returns TRUE of the base layer graphics should be drawn at the given layer.
bool
CListViewBaseItemChrome::ShouldDrawBaseLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer)
{
    return layer == ListViewBaseItemChromeLayerPosition_Base_Pre;
}

// Returns TRUE if the under-content layer graphics should be drawn at the given layer.
bool
CListViewBaseItemChrome::ShouldDrawUnderContentLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer)
{
    return layer == ListViewBaseItemChromeLayerPosition_PrimaryChrome_Pre;
}

// Returns TRUE if the over-content layer graphics should be drawn at the given layer.
bool
CListViewBaseItemChrome::ShouldDrawOverContentLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer)
{
    // Order of precedence matters!
    // Default to Primary Chrome Post.
    ListViewBaseItemChromeLayerPosition targetLayer = ListViewBaseItemChromeLayerPosition_PrimaryChrome_Post;

    // Some states require us to draw the over-content layer on the secondary chrome instead, so we can animate it.
    if (m_visualStates.HasState(DragStates_MultipleDraggingPrimary) || m_visualStates.HasState(DragStates_Dragging) ||
        m_visualStates.HasState(DragStates_MultipleReorderingPrimary) || m_visualStates.HasState(DragStates_Reordering))
    {
        targetLayer = ListViewBaseItemChromeLayerPosition_SecondaryChrome_Post;
    }

    return layer == targetLayer;
}

// Obtains the next animation command to execute, or NULL if none exists. Note that the ref
// to the returned item is still controlled by the chrome - see UnlockLayersForAnimationAndDisposeCommand.
_Check_return_ HRESULT
CListViewBaseItemChrome::GetNextPendingAnimation(_Outptr_ ListViewBaseItemAnimationCommand** ppAnimationCommand)
{
    std::unique_ptr<ListViewBaseItemAnimationCommand> command;

    IFC_RETURN(DequeueAnimationCommand(command));

    *ppAnimationCommand = command.release();

    return S_OK;
}

// Tells us to assign the layer positions of the various chrome visuals to match the requirements of the given
// command. Returns TRUE if the layers could be rearranged (meaning the animation can proceed), FALSE otherwise.
_Check_return_ HRESULT
CListViewBaseItemChrome::LockLayersForAnimation(
    _In_ ListViewBaseItemAnimationCommand* pCommand,
    _Out_ bool* pShouldProceed)
{
    XINT32 commandPriority = pCommand->GetPriority();

    if (m_currentHighestCommandPriority >= commandPriority)
    {
        *pShouldProceed = TRUE;

        // Generate a stop command for the current animation if we're supplanting it.
        if (m_pCurrentHighestPriorityCommand &&
            m_currentHighestCommandPriority != commandPriority)
        {
            std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

            IFC_RETURN(m_pCurrentHighestPriorityCommand->Clone(animationCommand));

            animationCommand->m_isStarting = FALSE;

            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }

        m_pCurrentHighestPriorityCommand = pCommand;
        m_currentHighestCommandPriority = commandPriority;
    }
    else
    {
        *pShouldProceed = FALSE;
    }

    return S_OK;
}

// Unlocks the layers for the given command, and releases the command.
void
CListViewBaseItemChrome::UnlockLayersForAnimationAndDisposeCommand(_In_opt_ ListViewBaseItemAnimationCommand* command)
{
    if (m_pCurrentHighestPriorityCommand == command)
    {
        m_pCurrentHighestPriorityCommand = nullptr; // Will delete below.
        m_currentHighestCommandPriority = XINT32_MAX;
    }

    delete command;
}

// Reparents the inner selection border if needed and updates its affected properties.
// According to the visual design, the inner and outer borders are meant to be overlapping each other. In order to avoid bleed through at the borders' edges though,
// we can afford to host a bloated inner border inside the outer border when the latter is opaque. That trick is not applicable when the outer border is semi-transparent,
// and that is OK since the bleed through is not obvious in those cases.
_Check_return_ HRESULT
CListViewBaseItemChrome::UpdateBordersParenting()
{
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_outerBorder);

    if (!m_innerSelectionBorder)
    {
        return S_OK;
    }

    const bool isOuterBorderBrushOpaque = IsOuterBorderBrushOpaque();
    const bool areBordersNested = m_outerBorder == m_innerSelectionBorder->GetParentInternal();

#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: UpdateBordersParenting - entry. isOuterBorderBrushOpaque=%d, areBordersNested=%d", this, isOuterBorderBrushOpaque, areBordersNested));
    }
#endif // LVBIC_DEBUG

    if (isOuterBorderBrushOpaque != areBordersNested)
    {
        if (areBordersNested)
        {
            IFC_RETURN(m_outerBorder->RemoveChild(m_innerSelectionBorder));
        }
        else
        {
            IFC_RETURN(RemoveChild(m_innerSelectionBorder));
        }

        if (isOuterBorderBrushOpaque)
        {
            IFC_RETURN(m_outerBorder->AddChild(m_innerSelectionBorder.get()));
        }
        else
        {
            IFC_RETURN(AddChild(m_innerSelectionBorder.get()));
        }

        IFC_RETURN(SetInnerSelectionBorderProperties());
    }

    return S_OK;
}

// If necessary, creates all the TransitionTargets necessary for our animation targets.
_Check_return_ HRESULT
CListViewBaseItemChrome::EnsureTransitionTarget()
{
    HRESULT hr = S_OK;
    CDependencyObject* pTransitionTarget = NULL;
    CDependencyObject* pTransitionTarget2 = NULL;
    CDependencyObject* pTransitionTarget3 = NULL;
    CDependencyObject* pTransitionTarget4 = NULL;
    CUIElement* pTemplateChild = GetTemplateChildIfExists();
    CREATEPARAMETERS cp(GetContext());

    if (!HasTransitionTarget())
    {
        IFC(CTransitionTarget::Create(&pTransitionTarget, &cp));
        IFC(SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, pTransitionTarget));
    }

    if (m_pSecondaryChrome &&
        !m_pSecondaryChrome->HasTransitionTarget())
    {
        IFC(CTransitionTarget::Create(&pTransitionTarget2, &cp));
        IFC(m_pSecondaryChrome->SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, pTransitionTarget2));
    }

    if (m_pParentListViewBaseItemNoRef &&
        !m_pParentListViewBaseItemNoRef->HasTransitionTarget())
    {
        IFC(CTransitionTarget::Create(&pTransitionTarget3, &cp));
        IFC(m_pParentListViewBaseItemNoRef->SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, pTransitionTarget3));
    }

    if (pTemplateChild &&
        !pTemplateChild->HasTransitionTarget())
    {
        IFC(CTransitionTarget::Create(&pTransitionTarget4, &cp));
        IFC(pTemplateChild->SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, pTransitionTarget4));
    }

Cleanup:
    ReleaseInterface(pTransitionTarget);
    ReleaseInterface(pTransitionTarget2);
    ReleaseInterface(pTransitionTarget3);
    ReleaseInterface(pTransitionTarget4);
    return hr;
}

// The "ListViewBaseItemRoundedChromeEnabled" theme resource value is used to turn on/off the rendering with rounded corners.
// For performance reasons, the resource is only evaluated once. TAEF tests can invalidate the cache by calling TestServices::Utilities::DeleteResourceDictionaryCaches().
bool
CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeEnabled() const
{
    if (!s_isRoundedListViewBaseItemChromeEnabled.has_value())
    {
        s_isRoundedListViewBaseItemChromeEnabled = IsRoundedListViewBaseItemChromeEnabled(GetContext());
    }

    return s_isRoundedListViewBaseItemChromeEnabled.value();
}

bool
CListViewBaseItemChrome::IsChromeForGridViewItem() const
{
    return m_pParentListViewBaseItemNoRef && m_pParentListViewBaseItemNoRef->OfTypeByIndex<KnownTypeIndex::GridViewItem>();
}

bool
CListViewBaseItemChrome::IsChromeForListViewItem() const
{
    return m_pParentListViewBaseItemNoRef && m_pParentListViewBaseItemNoRef->OfTypeByIndex<KnownTypeIndex::ListViewItem>();
}

// Uses the ListViewItemPresenter.SelectionIndicatorVisualEnabled property as well as RuntimeEnabledFeature's DenySelectionIndicatorVisualEnabled/ForceSelectionIndicatorVisualEnabled
// overwrites to determine whether a ListViewItem should render the selection indicator or not.
bool
CListViewBaseItemChrome::IsSelectionIndicatorVisualEnabled() const
{
    if (!IsChromeForListViewItem() ||
        !OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        return false;
    }

    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DenySelectionIndicatorVisualEnabled))
    {
        return false;
    }

    CValue value;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorVisualEnabled, &value));

    return (IsRoundedListViewBaseItemChromeEnabled() && value.AsBool()) || runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorVisualEnabled);
}

// Indicates whether the selection indicator is allowed to be rendered and the SelectionMode is either Single or Extended.
bool
CListViewBaseItemChrome::IsInSelectionIndicatorMode() const
{
    if (!IsSelectionIndicatorVisualEnabled())
    {
        return false;
    }

    CListViewBase* listViewBaseNoRef = GetParentListViewBase(m_pParentListViewBaseItemNoRef);

    if (!listViewBaseNoRef)
    {
        return false;
    }

    CValue valSelectionMode;

    IFCFAILFAST(listViewBaseNoRef->GetValueByIndex(KnownPropertyIndex::ListViewBase_SelectionMode, &valSelectionMode));

    DirectUI::ListViewSelectionMode selectionMode = static_cast<DirectUI::ListViewSelectionMode>(valSelectionMode.AsEnum());

    return selectionMode == DirectUI::ListViewSelectionMode::Single || selectionMode == DirectUI::ListViewSelectionMode::Extended;
}

// Returns True when the GridViewItem's outer border is present and uses an opaque brush.
bool
CListViewBaseItemChrome::IsOuterBorderBrushOpaque() const
{
    ASSERT(IsChromeForGridViewItem());

    if (!m_outerBorder)
    {
        return false;
    }

    CValue valBorderBrush;

    IFCFAILFAST(m_outerBorder->GetValueByIndex(KnownPropertyIndex::Border_BorderBrush, &valBorderBrush));

    CBrush* outerBorderBrush = static_cast<CBrush*>(valBorderBrush.AsObject());

    return outerBorderBrush && CListViewBaseItemChrome::IsOpaqueBrush(outerBorderBrush);
}

// Sets the border CornerRadius based on value returned by GetGeneralCornerRadius.
_Check_return_ HRESULT CListViewBaseItemChrome::SetGeneralCornerRadius(xref_ptr<CBorder> border)
{
    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(border);

    CValue value;
    XCORNERRADIUS cornerRadius = GetGeneralCornerRadius();

    value.WrapCornerRadius(&cornerRadius);
    IFC_RETURN(border->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));

    return S_OK;
}

// Returns this ListViewBaseItemPresenter's CornerRadius when set,
// or the default 4.0 value when rounded corner rendering is forced.
XCORNERRADIUS CListViewBaseItemChrome::GetGeneralCornerRadius() const
{
    XCORNERRADIUS cornerRadius = GetCornerRadius();

    if (CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced() &&
        cornerRadius.bottomLeft == 0.0f &&
        cornerRadius.bottomRight == 0.0f &&
        cornerRadius.topLeft == 0.0f &&
        cornerRadius.topRight == 0.0f)
    {
        cornerRadius = CornerRadius(s_generalCornerRadius);
    }

    return cornerRadius;
}

// Returns this ListViewBaseItemPresenter's CheckBoxCornerRadius when set,
// or the default 3.0 value when rounded corner rendering is forced.
XCORNERRADIUS CListViewBaseItemChrome::GetCheckBoxCornerRadius() const
{
    XCORNERRADIUS cornerRadius{};

    if (OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        CValue valCheckBoxCornerRadius;

        IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_CheckBoxCornerRadius, &valCheckBoxCornerRadius));

        cornerRadius = *(valCheckBoxCornerRadius.AsCornerRadius());

        if (CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced() &&
            cornerRadius.bottomLeft == 0.0f &&
            cornerRadius.bottomRight == 0.0f &&
            cornerRadius.topLeft == 0.0f &&
            cornerRadius.topRight == 0.0f)
        {
            cornerRadius = CornerRadius(GetDefaultCheckBoxCornerRadius());
        }
    }

    return cornerRadius;
}

// Returns this ListViewBaseItemPresenter's SelectionIndicatorCornerRadius when set,
// or the default 1.5 value when rounded corner rendering is forced.
XCORNERRADIUS CListViewBaseItemChrome::GetSelectionIndicatorCornerRadius() const
{
    XCORNERRADIUS cornerRadius{};

    if (OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        CValue valSelectionIndicatorCornerRadius;

        IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorCornerRadius, &valSelectionIndicatorCornerRadius));

        cornerRadius = *(valSelectionIndicatorCornerRadius.AsCornerRadius());

        if (CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced() &&
            cornerRadius.bottomLeft == 0.0f &&
            cornerRadius.bottomRight == 0.0f &&
            cornerRadius.topLeft == 0.0f &&
            cornerRadius.topRight == 0.0f)
        {
            cornerRadius = CornerRadius(GetDefaultSelectionIndicatorCornerRadius());
        }
    }

    return cornerRadius;
}

// Returns the selection indicator's desired height given the ListViewItem's available height.
XFLOAT CListViewBaseItemChrome::GetSelectionIndicatorHeightFromAvailableHeight(XFLOAT availableHeight) const
{
    if (availableHeight <= s_selectionIndicatorSize.height)
    {
        return availableHeight;
    }

    return MAX(s_selectionIndicatorSize.height, availableHeight - s_selectionIndicatorMargin.top - s_selectionIndicatorMargin.bottom);
}

// Uses the ListViewItemPresenter.SelectionIndicatorMode property as well as RuntimeEnabledFeature's ForceSelectionIndicatorModeInline/ForceSelectionIndicatorModeOverlay
// overwrites to determine whether the selection indicator must rendered inline or overlayed.
DirectUI::ListViewItemPresenterSelectionIndicatorMode CListViewBaseItemChrome::GetSelectionIndicatorMode() const
{
    ASSERT(OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>());

    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorModeInline))
    {
        return DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline;
    }

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorModeOverlay))
    {
        return DirectUI::ListViewItemPresenterSelectionIndicatorMode::Overlay;
    }

    CValue value;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorMode, &value));

    return static_cast<DirectUI::ListViewItemPresenterSelectionIndicatorMode>(value.AsEnum());
}

CBrush* CListViewBaseItemChrome::GetRevealBackgroundBrushNoRef() const
{
    if (!OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        return nullptr;
    }

    CValue valRevealBackgroundBrush;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_RevealBackground, &valRevealBackgroundBrush));

    return static_cast<CBrush*>(valRevealBackgroundBrush.AsObject());
}

CBrush* CListViewBaseItemChrome::GetRevealBorderBrushNoRef() const
{
    if (!OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        return nullptr;
    }

    CValue valRevealBorderBrush;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_RevealBorderBrush, &valRevealBorderBrush));

    return static_cast<CBrush*>(valRevealBorderBrush.AsObject());
}

bool CListViewBaseItemChrome::GetRevealBackgroundShowsAboveContent() const
{
    if (!OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>())
    {
        return false;
    }

    CValue valRevealBackgroundShowsAboveContent;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_RevealBackgroundShowsAboveContent, &valRevealBackgroundShowsAboveContent));

    return valRevealBackgroundShowsAboveContent.AsBool();
}

bool CListViewBaseItemChrome::GetSelectionCheckMarkVisualEnabled() const
{
    CValue valSelectionCheckMarkVisualEnabled;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_SelectionCheckMarkVisualEnabled : KnownPropertyIndex::GridViewItemPresenter_SelectionCheckMarkVisualEnabled,
        &valSelectionCheckMarkVisualEnabled));

    return valSelectionCheckMarkVisualEnabled.AsBool();
}

XTHICKNESS CListViewBaseItemChrome::GetRevealBorderThickness() const
{
    ASSERT(OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>());

    CValue valRevealBorderThickness;

    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::ListViewItemPresenter_RevealBorderThickness, &valRevealBorderThickness));

    return *(valRevealBorderThickness.AsThickness());
}

XTHICKNESS CListViewBaseItemChrome::GetSelectedBorderThickness() const
{
    CValue valSelectedBorderThickness;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_SelectedBorderThickness : KnownPropertyIndex::GridViewItemPresenter_SelectedBorderThickness,
        &valSelectedBorderThickness));

    return *(valSelectedBorderThickness.AsThickness());
}

XTHICKNESS CListViewBaseItemChrome::GetPointerOverBackgroundMargin() const
{
    CValue valPointerOverBackgroundMargin;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_PointerOverBackgroundMargin : KnownPropertyIndex::GridViewItemPresenter_PointerOverBackgroundMargin,
        &valPointerOverBackgroundMargin));

    return *(valPointerOverBackgroundMargin.AsThickness());
}

XFLOAT CListViewBaseItemChrome::GetDisabledOpacity() const
{
    CValue valDisabledOpacity;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_DisabledOpacity : KnownPropertyIndex::GridViewItemPresenter_DisabledOpacity,
        &valDisabledOpacity));

    return valDisabledOpacity.AsFloat();
}

XFLOAT CListViewBaseItemChrome::GetDragOpacity() const
{
    CValue valDragOpacity;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_DragOpacity : KnownPropertyIndex::GridViewItemPresenter_DragOpacity,
        &valDragOpacity));

    return valDragOpacity.AsFloat();
}

XFLOAT CListViewBaseItemChrome::GetReorderHintOffset() const
{
    CValue valReorderHintOffset;

    IFCFAILFAST(GetValueByIndex(
        OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() ? KnownPropertyIndex::ListViewItemPresenter_ReorderHintOffset : KnownPropertyIndex::GridViewItemPresenter_ReorderHintOffset,
        &valReorderHintOffset));

    return valReorderHintOffset.AsFloat();
}

void CListViewBaseItemChrome::NWSetContentDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::ListViewItemPresenter>() || pTarget->OfTypeByIndex<KnownTypeIndex::GridViewItemPresenter>());

    CListViewBaseItemChrome *pChrome = static_cast<CListViewBaseItemChrome *>(pTarget);

    CFrameworkElement::NWSetContentDirty(pTarget, flags);

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pChrome->m_fFillBrushDirty = true;
    }
}

// Builds the checkmark path.
_Check_return_ HRESULT
CListViewBaseItemChrome::PrepareCheckPath()
{
    HRESULT hr = S_OK;

    auto core = GetContext();
    CREATEPARAMETERS cp(core);
    CValue cVal;

    CPathGeometry *pGeo = NULL;
    CPathFigure *pFigure = NULL;
    CPathSegmentCollection *pSegmentsNoRef = NULL;
    CPathFigureCollection *pFiguresNoRef = NULL;

    const XPOINTF* checkMarkPoints = s_checkMarkPoints;

    if (m_pCheckGeometryData)
    {
        goto Cleanup;
    }

    IFC(CPathGeometry::Create(reinterpret_cast<CDependencyObject **>(&pGeo), &cp));
    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject **>(&pFigure), &cp));

    // Set start point.
    cVal.WrapPoint(&checkMarkPoints[0]);
    IFC(pFigure->SetValueByIndex(KnownPropertyIndex::PathFigure_StartPoint, cVal));

    // Get and populate the line segments.
    IFC(pFigure->GetValueByIndex(KnownPropertyIndex::PathFigure_Segments, &cVal));
    pSegmentsNoRef = do_pointer_cast<CPathSegmentCollection>(cVal.AsObject());
    IFCEXPECT(pSegmentsNoRef);

    for (int i = 1; i < 6; ++i)
    {
        IFC(AddLineSegmentToSegmentCollection(core, pSegmentsNoRef, checkMarkPoints[i]));
    }

    // Populate the figure collection.
    IFC(pGeo->GetValueByIndex(KnownPropertyIndex::PathGeometry_Figures, &cVal));
    pFiguresNoRef = do_pointer_cast<CPathFigureCollection>(cVal.AsObject());

    cVal.WrapObjectNoRef(pFigure);
    IFC(CCollection::Add(pFiguresNoRef, 1, &cVal, NULL));

    // Get the bounds of the geometry.
    IFC(pGeo->GetBounds(&m_checkGeometryBounds));

    // Save the geometry for use in rendering.
    m_pCheckGeometryData = pGeo;
    pGeo = NULL;

Cleanup:
    ReleaseInterface(pGeo);
    ReleaseInterface(pFigure);

    return hr;
}

// SegmentCollection helper.
_Check_return_ HRESULT
CListViewBaseItemChrome::AddLineSegmentToSegmentCollection(_In_ CCoreServices *pCore, _In_ CPathSegmentCollection *pSegments, _In_ XPOINTF point)
{
    HRESULT hr = S_OK;

    CLineSegment *pSegment = NULL;
    CREATEPARAMETERS cp(pCore);
    CValue cVal;

    IFC(CLineSegment::Create(reinterpret_cast<CDependencyObject **>(&pSegment), &cp));

    cVal.WrapPoint(&point);
    IFC(pSegment->SetValueByIndex(KnownPropertyIndex::LineSegment_Point, cVal));

    cVal.WrapObjectNoRef(pSegment);
    IFC(CCollection::Add(pSegments, 1, &cVal, NULL));

Cleanup:
    ReleaseInterface(pSegment);
    return hr;
}

// Provides the geometry of the CheckMark, so the renderer knows how to size the brush.
_Check_return_ HRESULT
CListViewBaseItemChrome::GetCheckMarkBounds(_Out_ XRECTF_RB* pBounds)
{
    *pBounds = m_checkGeometryBounds;
    return S_OK;
}

// Sets whether or not the drag overlay text block is shown.
_Check_return_ HRESULT
CListViewBaseItemChrome::SetDragOverlayTextBlockVisible(_In_ bool isVisible)
{
    if (isVisible)
    {
        IFC_RETURN(EnsureDragOverlayTextBlock());

        auto core = GetContext();

        IFC_RETURN(EnsureMultiSelectCheckBox());

        // For RS1, drag items count text will show inside a border, and a background is also applied to the text.
        // So, we re-use the multi-select checkbox to have the drag count textblock as its child. This will produce the effect of a border around the text.
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Clip, nullptr));
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetChild(m_pDragItemsCountTextBlock));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_borderBrush, L"SystemControlBackgroundChromeWhiteBrush");
        xref_ptr<CDependencyObject> borderBrush = nullptr;
        IFC_RETURN(core->LookupThemeResource(c_borderBrush, borderBrush.ReleaseAndGetAddressOf()));
        if (borderBrush != nullptr)
        {
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, borderBrush.get()));
        }

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_background, L"SystemControlBackgroundAccentBrush");
        xref_ptr<CDependencyObject> backgroundBrush = nullptr;
        IFC_RETURN(core->LookupThemeResource(c_background, backgroundBrush.ReleaseAndGetAddressOf()));
        if (backgroundBrush != nullptr)
        {
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, backgroundBrush.get()));
        }

        CValue value;
        value.Set(DirectUI::Visibility::Visible);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByIndex(KnownPropertyIndex::UIElement_Visibility, value));

        if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay)
        {
            // For GridView, the item count should show in the center just like in File Explorer
            XTHICKNESS thickness = { 2.0f, 2.0f, 2.0f, 2.0f };
            value.WrapThickness(const_cast<XTHICKNESS*>(&thickness));
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

            value.Set(DirectUI::VerticalAlignment::Center);
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

            value.Set(DirectUI::HorizontalAlignment::Center);
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));
        }

        InvalidateMeasure();
    }

    if (m_pDragItemsCountTextBlock)
    {
        CValue valueVisible;
        valueVisible.Set(isVisible ? DirectUI::Visibility::Visible : DirectUI::Visibility::Collapsed);
        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByIndex(KnownPropertyIndex::UIElement_Visibility, valueVisible));
    }

    return S_OK;
}

// Sets the drag count display.
_Check_return_ HRESULT
CListViewBaseItemChrome::SetDragItemsCount(
    _In_ XUINT32 dragItemsCount)
{
    xstring_ptr strString;
    CValue stringVal;

    IFC_RETURN(EnsureDragOverlayTextBlock());

    IFC_RETURN(xstring_ptr::CreateFromUInt32(dragItemsCount, &strString));

    stringVal.SetString(strString);
    IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Text, stringVal));

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::SetSwipeHintCheckOpacity(_In_ XFLOAT opacity)
{
    if (m_swipeHintCheckOpacity != opacity)
    {
        m_swipeHintCheckOpacity = opacity;

        if (m_pParentListViewBaseItemNoRef)
        {
            CContentControl::NWSetContentDirty(m_pParentListViewBaseItemNoRef, DirtyFlags::Bounds);
        }
    }

    return S_OK;
}

// Sets up and adds the drag overlay text block to the tree.
_Check_return_ HRESULT
CListViewBaseItemChrome::EnsureDragOverlayTextBlock()
{
    if (!m_pDragItemsCountTextBlock)
    {
        CREATEPARAMETERS cp(GetContext());
        IFC_RETURN(CTextBlock::Create(reinterpret_cast<CDependencyObject **>(&m_pDragItemsCountTextBlock), &cp));
        IFC_RETURN(SetDragOverlayTextBlockProperties());
    }

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::SetDragOverlayTextBlockProperties()
{
    if (m_pDragItemsCountTextBlock)
    {
        CValue value;

        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        value.SetEnum(UIAXcp::AccessibilityView_Raw);
        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, value));

        // Keep the drag items count textblock collapsed by default. It is made visible on demand when we change visual states.
        value.Set(DirectUI::Visibility::Collapsed);
        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByIndex(KnownPropertyIndex::UIElement_Visibility, value));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_style, L"CaptionTextBlockStyle");
        xref_ptr<CDependencyObject> style = nullptr;
        IFC_RETURN(GetContext()->LookupThemeResource(c_style, style.ReleaseAndGetAddressOf()));
        if (style != nullptr)
        {
            IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Style, do_pointer_cast<CStyle>(style.get())));
        }

        value.Set(DirectUI::HorizontalAlignment::Center);
        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        value.Set(DirectUI::VerticalAlignment::Center);
        IFC_RETURN(m_pDragItemsCountTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));
    }

    return S_OK;
}

// Sets up the secondary chrome and adds it to the tree.
_Check_return_ HRESULT
CListViewBaseItemChrome::AddSecondaryChrome()
{
    CUIElementCollection* pChildrenCollectionNoRef = NULL;

    if (!m_pSecondaryChrome)
    {
        CREATEPARAMETERS cp(GetContext());
        CValue cVal;

        IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_ChildrenInternal, &cVal));

        IFC_RETURN(DoPointerCast(pChildrenCollectionNoRef, cVal.AsObject()));

        IFC_RETURN(CListViewBaseItemSecondaryChrome::Create(reinterpret_cast<CDependencyObject **>(&m_pSecondaryChrome), &cp));

        m_pSecondaryChrome->m_pPrimaryChromeNoRef = this;
        IFC_RETURN(pChildrenCollectionNoRef->Append(m_pSecondaryChrome));
    }

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::SetChromedListViewBaseItem(_In_ CUIElement* pParent)
{
    m_pParentListViewBaseItemNoRef = NULL;

    if (pParent)
    {
        IFCEXPECT_RETURN(pParent->OfTypeByIndex<KnownTypeIndex::ContentControl>());
        m_pParentListViewBaseItemNoRef = static_cast<CContentControl*>(pParent);
    }

    return S_OK;
}

// Enqueue the given command. It will be presented in FIFO order by DequeueAnimationCommand.
// This function takes control of the command's lifetime. To enforce this, it will clear the
// given pointer when called (no exceptions).
_Check_return_ HRESULT
CListViewBaseItemChrome::EnqueueAnimationCommand(_In_ std::unique_ptr<ListViewBaseItemAnimationCommand>& command)
{
    IFC_RETURN(EnsureTransitionTarget());

    IFC_RETURN(m_animationCommands.push_back(command.release()));

    return S_OK;
}

// Dequeues an animation command. You now own the ref (note that GetNextPendingAnimation, a caller
// to this function, takes logical ownership itself).
_Check_return_ HRESULT
CListViewBaseItemChrome::DequeueAnimationCommand(_Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& command)
{
    if (!m_animationCommands.empty())
    {
        command.reset(m_animationCommands[0]);
        m_animationCommands.erase(0);
    }
    else
    {
        command = nullptr;
    }

    return S_OK; // RRETURN_REMOVAL
}

// Given a reorder hint state, figure out where the hint animation should animate to.
_Check_return_ HRESULT
CListViewBaseItemChrome::ComputeReorderHintOffset(_In_ ReorderHintStates state, _Out_ XPOINTF* pOffset)
{
    XFLOAT reorderHintOffset = GetReorderHintOffset();

    pOffset->x = 0.0f;
    pOffset->y = 0.0f;

    switch(state)
    {
        case ReorderHintStates_BottomReorderHint:
            pOffset->y = reorderHintOffset;
            break;

        case ReorderHintStates_TopReorderHint:
            pOffset->y = -reorderHintOffset;
            break;

        case ReorderHintStates_LeftReorderHint:
            pOffset->x = -reorderHintOffset;
            break;

        case ReorderHintStates_RightReorderHint:
            pOffset->x = reorderHintOffset;
            break;
    }
    return S_OK;
}

// Given a swipe hint state, figure out where the hint animation should animate to.
_Check_return_ HRESULT
CListViewBaseItemChrome::ComputeSwipeHintOffset(_In_ SelectionHintStates state, _Out_ XPOINTF* pOffset)
{
    pOffset->x = 0.0f;
    pOffset->y = 0.0f;

    switch(state)
    {
        case SelectionHintStates_VerticalSelectionHint:
            pOffset->y = s_swipeHintOffset.y;
            break;

        case SelectionHintStates_HorizontalSelectionHint:
            pOffset->x = s_swipeHintOffset.x;
            break;
    }
    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = GetActualWidth();
    pBounds->bottom = GetActualHeight();

    return hr;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit)
{
    if (GetContext()->InvisibleHitTestMode())
    {
        // Normally we only get here if we passed all bounds checks, but this is not the case if InvisibleHitTestMode is
        // enabled. Here we're hit testing invisible elements, which skips all bounds checks. We don't want to blindly
        // return true here, because that would mean that this element will always be hit under InvisibleHitTestMode,
        // regardless of the incoming point. Instead, make an explicit check against this element's layout size.
        //
        // InvisibleHitTestMode is used by FindElementsInHostCoordinates with includeAllElements set to true, which the
        // VS designer uses to hit test elements that are collapsed or have no background.
        XRECTF layoutSize = { 0, 0, GetActualWidth(), GetActualHeight() };
        *pHit = DoesRectContainPoint(layoutSize, target);
    }
    else
    {
        // All hits within the hit test bounds count as a hit.
        *pHit = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit)
{
    if (GetContext()->InvisibleHitTestMode())
    {
        // Normally we only get here if we passed all bounds checks, but this is not the case if InvisibleHitTestMode is
        // enabled. Here we're hit testing invisible elements, which skips all bounds checks. We don't want to blindly
        // return true here, because that would mean that this element will always be hit under InvisibleHitTestMode,
        // regardless of the incoming point. Instead, make an explicit check against this element's layout size.
        //
        // InvisibleHitTestMode is used by FindElementsInHostCoordinates with includeAllElements set to true, which the
        // VS designer uses to hit test elements that are collapsed or have no background.
        XRECTF layoutSize = { 0, 0, GetActualWidth(), GetActualHeight() };
        *pHit = target.IntersectsRect(layoutSize);
    }
    else
    {
        // All hits within the hit test bounds count as a hit.
        *pHit = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::HitTestLocalInternalPostChildren(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit)
{
    *pHit = m_pPlaceholderBackground
         && m_visualStates.HasState(DataVirtualizationStates_DataPlaceholder);

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::HitTestLocalInternalPostChildren(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit)
{
    *pHit = m_pPlaceholderBackground
         && m_visualStates.HasState(DataVirtualizationStates_DataPlaceholder);

    return S_OK;
}

void CListViewBaseItemChrome::InvalidateRender()
{
    NWSetContentDirty(this, DirtyFlags::Bounds);

    if (m_pParentListViewBaseItemNoRef)
    {
        CContentControl::NWSetContentDirty(m_pParentListViewBaseItemNoRef, DirtyFlags::Bounds);
    }

    if (m_pSecondaryChrome)
    {
        CFrameworkElement::NWSetContentDirty(m_pSecondaryChrome, DirtyFlags::Bounds);
    }
}

_Check_return_ HRESULT CListViewBaseItemChrome::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CContentPresenter::OnPropertyChanged(args));
    IFC_RETURN(OnPropertyChangedNewStyle(args));

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::MeasureNewStyle(
    _In_ XSIZEF availableSize,
    _Out_ XSIZEF& desiredSize)
{
    XTHICKNESS contentMargin = m_contentMargin;
    XTHICKNESS controlBorderThickness = {};
    XSIZEF totalSize = {};

    if (!m_backplateRectangle && IsRoundedListViewBaseItemChromeEnabled())
    {
        IFC_RETURN(EnsureBackplate());
    }

    CUIElement* pTemplateChild = GetTemplateChildIfExists();
    CContentControl* pParentListViewBaseItemNoRef = nullptr;

    // We can't be used without a parent LVB.
    IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

    controlBorderThickness = pParentListViewBaseItemNoRef->m_borderThickness;

    desiredSize = availableSize;

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&contentMargin);
        LayoutRoundHelper(&controlBorderThickness);
    }

    if (pTemplateChild)
    {
        XSIZEF contentAvailableSize = availableSize;
        float contentPrefixWidth = 0.0f;

        // Size available for content is availableSize - content margin - control border
        CSizeUtil::Deflate(&contentAvailableSize, contentMargin);
        CSizeUtil::Deflate(&contentAvailableSize, controlBorderThickness);

        // Check to see if we need to offset the content due to the potential CheckBox in Inline MultiSelect state.
        if (GetSelectionCheckMarkVisualEnabled() &&
            m_visualStates.HasState(MultiSelectStates_MultiSelectEnabled) &&
            m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
        {
            contentPrefixWidth = IsRoundedListViewBaseItemChromeEnabled() ? s_multiSelectRoundedContentOffset : s_listViewItemMultiSelectContentOffset;
        }

        if (IsInSelectionIndicatorMode() &&
            GetSelectionIndicatorMode() == DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline)
        {
            contentPrefixWidth = std::max(contentPrefixWidth, s_selectionIndicatorMargin.left + s_selectionIndicatorSize.width + s_selectionIndicatorMargin.right);
        }

        // subtract the offset to have the child arrange using the new width
        contentAvailableSize.width -= contentPrefixWidth;

        IFC_RETURN(pTemplateChild->Measure(contentAvailableSize));

        // Use child's desired size for our size.
        totalSize = pTemplateChild->DesiredSize;

        // If SelectionMode is Multiple and a CheckBox is potentially visible (Inline mode), we add the buffer of the CheckBox back.
        // If SelectionMode is Single or Extended and a SelectionIndicator is potentially visible (Inline mode), we add the buffer of the SelectionIndicator back.
        totalSize.width += contentPrefixWidth;
    }

    // border should be accounted for regardless if there is content.
    CSizeUtil::Inflate(&totalSize, contentMargin);
    CSizeUtil::Inflate(&totalSize, controlBorderThickness);

    if (GetSelectionCheckMarkVisualEnabled() && m_multiSelectCheckBoxRectangle)
    {
        IFC_RETURN(m_multiSelectCheckBoxRectangle->Measure(totalSize));
    }

    if (IsSelectionIndicatorVisualEnabled() && m_selectionIndicatorRectangle)
    {
        IFC_RETURN(m_selectionIndicatorRectangle->Measure(totalSize));
    }

    if (m_backplateRectangle)
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        IFC_RETURN(m_backplateRectangle->Measure(totalSize));
    }

    if (m_innerSelectionBorder && m_outerBorder != m_innerSelectionBorder->GetParentInternal())
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        ASSERT(IsChromeForGridViewItem());
        IFC_RETURN(m_innerSelectionBorder->Measure(totalSize));
    }

    if (m_outerBorder)
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        ASSERT(IsChromeForGridViewItem());
        IFC_RETURN(m_outerBorder->Measure(totalSize));
    }

    // Minimum size
    {
        XSIZEF minimumSize = { pParentListViewBaseItemNoRef->m_pLayoutProperties->m_eMinWidth, pParentListViewBaseItemNoRef->m_pLayoutProperties->m_eMinHeight };

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&minimumSize);
        }

        totalSize.width = MAX(totalSize.width, minimumSize.width);
        totalSize.height = MAX(totalSize.height, minimumSize.height);
    }

    desiredSize = totalSize;

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::ArrangeNewStyle(
    _In_ XSIZEF finalSize,
    _Out_ XSIZEF& newFinalSize)
{
    XTHICKNESS contentMargin = m_contentMargin;
    XRECTF finalBounds = { 0.0f, 0.0f, finalSize.width, finalSize.height };
    XRECTF controlBorderBounds = {};

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&contentMargin);
    }


    // Peel off content margin whitespace.
    controlBorderBounds = finalBounds;
    CSizeUtil::Deflate(&controlBorderBounds, contentMargin);

    float contentPrefixWidth = 0.0f;

    if (GetSelectionCheckMarkVisualEnabled() && m_visualStates.HasState(MultiSelectStates_MultiSelectEnabled))
    {
        XSIZEF multiSelectSquareSize = s_multiSelectSquareSize;

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&multiSelectSquareSize);
        }

        // If checkmark is visible, make sure there's at least enough space to show it.
        finalBounds.Width = MAX(finalBounds.Width, multiSelectSquareSize.width);
        finalBounds.Height = MAX(finalBounds.Height, multiSelectSquareSize.height);

        // Check to see if we need to offset the content due to the CheckBox in MultiSelect state.
        if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
        {
            contentPrefixWidth = IsRoundedListViewBaseItemChromeEnabled() ? s_multiSelectRoundedContentOffset : s_listViewItemMultiSelectContentOffset;
        }
    }

    if (IsInSelectionIndicatorMode())
    {
        XSIZEF selectionIndicatorSize = { s_selectionIndicatorMargin.left + s_selectionIndicatorSize.width + s_selectionIndicatorMargin.right, s_selectionIndicatorHeightShrinkage + 1.0f };

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&selectionIndicatorSize);
        }

        // If a selection indicator is potentially visible, make sure there's at least enough space to show it.
        finalBounds.Width = MAX(finalBounds.Width, selectionIndicatorSize.width);
        finalBounds.Height = MAX(finalBounds.Height, selectionIndicatorSize.height);

        // Check to see if we need to offset the content due to the Inline selection indicator.
        if (GetSelectionIndicatorMode() == DirectUI::ListViewItemPresenterSelectionIndicatorMode::Inline)
        {
            contentPrefixWidth = MAX(contentPrefixWidth, selectionIndicatorSize.width);
        }
    }

    if (contentPrefixWidth)
    {
        // will be used by ArrangeTemplateChild
        controlBorderBounds.X += contentPrefixWidth;

        // subtract the offset to have the child arrange using the new width
        controlBorderBounds.Width -= contentPrefixWidth;
    }

    if (m_multiSelectCheckBoxRectangle)
    {
        IFC_RETURN(m_multiSelectCheckBoxRectangle->Arrange(finalBounds));
    }

    if (m_selectionIndicatorRectangle)
    {
        XRECTF selectionIndicatorBounds = finalBounds;
        XFLOAT selectionIndicatorHeight = GetSelectionIndicatorHeightFromAvailableHeight(finalBounds.Height);

        if (m_visualStates.HasState(CommonStates2_Pressed) ||
            m_visualStates.HasState(CommonStates2_PressedSelected))
        {
            ASSERT(selectionIndicatorHeight > s_selectionIndicatorHeightShrinkage);
            selectionIndicatorHeight -= s_selectionIndicatorHeightShrinkage;
        }

        XFLOAT excessAvailableHeight = finalBounds.Height - selectionIndicatorHeight;

        if (excessAvailableHeight > 0)
        {
            selectionIndicatorBounds.Y += excessAvailableHeight / 2.0f;
            selectionIndicatorBounds.Height -= excessAvailableHeight;
        }

        IFC_RETURN(m_selectionIndicatorRectangle->Arrange(selectionIndicatorBounds));
    }

    if (m_backplateRectangle)
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        IFC_RETURN(m_backplateRectangle->Arrange(finalBounds));
    }

    if (m_innerSelectionBorder && m_outerBorder != m_innerSelectionBorder->GetParentInternal())
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        ASSERT(IsChromeForGridViewItem());
        IFC_RETURN(m_innerSelectionBorder->Arrange(finalBounds));
    }

    if (m_outerBorder)
    {
        ASSERT(IsRoundedListViewBaseItemChromeEnabled());
        ASSERT(IsChromeForGridViewItem());
        IFC_RETURN(m_outerBorder->Arrange(finalBounds));
    }

    newFinalSize.width = finalBounds.Width;
    newFinalSize.height = finalBounds.Height;

    IFC_RETURN(ArrangeTemplateChild(controlBorderBounds));

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::ArrangeTemplateChild(
    _In_ XRECTF& controlBorderBounds)
{
    CUIElement* pTemplateChild = GetTemplateChildIfExists();

    if (pTemplateChild)
    {
        XSIZEF contentAvailableSize = controlBorderBounds.Size();
        XSIZEF contentSize = {};
        XRECTF contentArrangedBounds = {};
        XTHICKNESS controlBorderThickness = {};

        CContentControl* pParentListViewBaseItemNoRef = nullptr;
        DirectUI::HorizontalAlignment horizontalContentAlignment = GetHorizontalContentAlignment();
        DirectUI::VerticalAlignment verticalContentAlignment = GetVerticalContentAlignment();

        // We can't be used without a parent LVB.
        IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

        controlBorderThickness = pParentListViewBaseItemNoRef->m_borderThickness;

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&controlBorderThickness);
        }

        // control border is not going to be available for content.
        CSizeUtil::Deflate(&contentAvailableSize, controlBorderThickness);

        // If alignment is Stretch, use entire available size, otherwise control's desired size.
        contentSize.width = (horizontalContentAlignment == DirectUI::HorizontalAlignment::Stretch) ? contentAvailableSize.width : pTemplateChild->DesiredSize.width;
        contentSize.height = (verticalContentAlignment == DirectUI::VerticalAlignment::Stretch) ? contentAvailableSize.height : pTemplateChild->DesiredSize.height;

        CFrameworkElement::ComputeAlignmentOffset(
            horizontalContentAlignment,
            verticalContentAlignment,
            contentAvailableSize,
            contentSize,
            contentArrangedBounds.X,
            contentArrangedBounds.Y);

        // making sure we are still within the boundaries of the control
        if (contentSize.width > contentAvailableSize.width)
        {
            contentSize.width = contentAvailableSize.width;
        }

        if (contentSize.height > contentAvailableSize.height)
        {
            contentSize.height = contentAvailableSize.height;
        }

        // Adjust top/left coordinate to account for space used for chrome.
        contentArrangedBounds.X += controlBorderThickness.left + controlBorderBounds.X;
        contentArrangedBounds.Y += controlBorderThickness.top + controlBorderBounds.Y;
        contentArrangedBounds.Width = contentSize.width;
        contentArrangedBounds.Height = contentSize.height;

        IFC_RETURN(pTemplateChild->Arrange(contentArrangedBounds));
    }

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::GoToChromedStateNewStyle(
    _In_z_ const WCHAR *pStateName,
    _In_ bool useTransitions,
    _Out_ bool* pWentToState)
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: GoToChromedStateNewStyle - entry. stateName=%s, useTransitions=%d", this, pStateName, useTransitions));
    }
#endif // LVBIC_DEBUG

    bool dirty = false;
    bool needsMeasure = false;
    bool needsArrange = false;
    bool disabledChanged = false;
    CUIElement* templateChild = GetTemplateChildIfExists();

    const bool isRoundedListViewBaseItemChromeEnabled = IsRoundedListViewBaseItemChromeEnabled();
    const CommonStates2 oldCommonState2 = m_visualStates.commonState2;
    const double listViewItemSelectionIndicatorContentOffset = s_selectionIndicatorMargin.left + s_selectionIndicatorSize.width + s_selectionIndicatorMargin.right;

    if (UpdateVisualStateGroup(pStateName, &m_visualStates.commonState2, pWentToState))
    {
        std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

        const bool selected =
            m_visualStates.HasState(CommonStates2_Selected) ||
            m_visualStates.HasState(CommonStates2_PressedSelected) ||
            m_visualStates.HasState(CommonStates2_PointerOverSelected);

        const bool roundedGridViewItem =
            isRoundedListViewBaseItemChromeEnabled && IsChromeForGridViewItem();

        if (selected)
        {
            if (m_multiSelectCheckGlyph)
            {
                CValue value;

                value.SetDouble(1.0);
                IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, value));
            }

            if (roundedGridViewItem)
            {
                if (!m_outerBorder)
                {
                    IFC_RETURN(EnsureOuterBorder());
                }
                else
                {
                    IFC_RETURN(SetOuterBorderBrush());
                    IFC_RETURN(SetOuterBorderThickness());
                }

                if (!m_innerSelectionBorder)
                {
                    IFC_RETURN(EnsureInnerSelectionBorder());
                }
                else
                {
                    IFC_RETURN(SetInnerSelectionBorderBrush());
                }
            }
        }
        else
        {
            if (m_multiSelectCheckGlyph)
            {
                CValue value;

                value.SetDouble(0.0);
                IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, value));
            }

            if (roundedGridViewItem)
            {
                if (m_innerSelectionBorder)
                {
                    IFC_RETURN(RemoveInnerSelectionBorder());
                }

                const bool renderOuterBorder = !m_visualStates.HasState(DisabledStates_Disabled) && m_visualStates.HasState(CommonStates2_PointerOver);

                if (!m_outerBorder)
                {
                    if (renderOuterBorder)
                    {
                        IFC_RETURN(EnsureOuterBorder());
                    }
                }
                else
                {
                    if (!renderOuterBorder)
                    {
                        IFC_RETURN(RemoveOuterBorder());
                    }
                    else
                    {
                        IFC_RETURN(SetOuterBorderBrush());
                        IFC_RETURN(SetOuterBorderThickness());
                    }
                }
            }
        }

        if (m_multiSelectCheckBoxRectangle)
        {
            IFC_RETURN(SetMultiSelectCheckBoxBackground());
            if (isRoundedListViewBaseItemChromeEnabled)
            {
                IFC_RETURN(SetMultiSelectCheckBoxBorder());
            }
        }

        const bool isInSelectionIndicatorMode = IsInSelectionIndicatorMode();

        if (isInSelectionIndicatorMode || m_selectionIndicatorRectangle)
        {
            const bool oldPressed =
                oldCommonState2 == CommonStates2_Pressed ||
                oldCommonState2 == CommonStates2_PressedSelected;
            const bool pressed =
                m_visualStates.HasState(CommonStates2_Pressed) ||
                m_visualStates.HasState(CommonStates2_PressedSelected);

            bool updateSelectionIndicatorVisibility = true;
            bool showSelectionIndicator = true;
            float fromScale = 0.0f;

            if (isInSelectionIndicatorMode)
            {
                const bool oldSelected = oldCommonState2 == CommonStates2_Selected || oldCommonState2 == CommonStates2_PressedSelected || oldCommonState2 == CommonStates2_PointerOverSelected;

                updateSelectionIndicatorVisibility = oldSelected != selected;
                showSelectionIndicator = selected;
            }
            else
            {
                showSelectionIndicator = false;
            }

            if (updateSelectionIndicatorVisibility)
            {
                if (showSelectionIndicator)
                {
                    IFC_RETURN(EnsureSelectionIndicator());
                }

                // Trigger animation to show/hide the selection indicator.
                animationCommand.reset(new ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility(
                    showSelectionIndicator /*selected*/,
                    0.0 /*fromScale*/,
                    xref::get_weakref(m_selectionIndicatorRectangle.get()),
                    true /*isStarting*/,
                    !useTransitions /*steadyStateOnly*/));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }
            else if (m_selectionIndicatorRectangle && showSelectionIndicator && oldPressed != pressed)
            {
                float currentHeight = m_selectionIndicatorRectangle->GetActualHeight();

                if (pressed && currentHeight <= s_selectionIndicatorHeightShrinkage)
                {
                    // s_selectionIndicatorHeightShrinkage equals 6.  currentHeight may be the rounded down value of s_selectionIndicatorHeightShrinkage + 1,
                    // but it is not expected to be smaller than or equal to s_selectionIndicatorHeightShrinkage.
                    ASSERT(false);
                    fromScale = 1.0f;
                }
                else
                {
                    fromScale = pressed ? currentHeight / (currentHeight - s_selectionIndicatorHeightShrinkage)
                                        : currentHeight / (currentHeight + s_selectionIndicatorHeightShrinkage);
                }

                // Trigger animation to shrink/expand the selection indicator.
                animationCommand.reset(new ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility(
                    true /*selected*/,
                    fromScale,
                    xref::get_weakref(m_selectionIndicatorRectangle.get()),
                    true /*isStarting*/,
                    !useTransitions /*steadyStateOnly*/));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }

            if (showSelectionIndicator)
            {
                const bool pointerOver = m_visualStates.HasState(CommonStates2_PointerOverSelected);
                bool updateSelectionIndicatorBackground = updateSelectionIndicatorVisibility;

                if (!updateSelectionIndicatorVisibility)
                {
                    updateSelectionIndicatorBackground = ((oldCommonState2 == CommonStates2_PointerOverSelected) != pointerOver) || ((oldCommonState2 == CommonStates2_PressedSelected) != pressed);
                }

                if (updateSelectionIndicatorBackground)
                {
                    IFC_RETURN(SetSelectionIndicatorBackground());
                }
            }

            if (oldPressed != pressed)
            {
                needsArrange = true;
            }
        }

        IFC_RETURN(SetForegroundBrush());

        if (!isRoundedListViewBaseItemChromeEnabled)
        {
            if (m_visualStates.HasState(CommonStates2_Pressed) ||
                m_visualStates.HasState(CommonStates2_PressedSelected))
            {
                animationCommand.reset(new ListViewBaseItemAnimationCommand_Pressed(
                    true /* pressed */,
                    xref::get_weakref(templateChild),
                    TRUE /*isStarting*/,
                    !useTransitions));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }
            else if (m_visualStates.HasState(CommonStates2_Normal) ||
                m_visualStates.HasState(CommonStates2_PointerOver) ||
                m_visualStates.HasState(CommonStates2_Selected) ||
                m_visualStates.HasState(CommonStates2_PointerOverSelected))
            {
                if (oldCommonState2 == CommonStates2_Pressed ||
                    oldCommonState2 == CommonStates2_PressedSelected)
                {
                    animationCommand.reset(new ListViewBaseItemAnimationCommand_Pressed(
                        false /* pressed */,
                        xref::get_weakref(templateChild),
                        TRUE /*isStarting*/,
                        !useTransitions));
                    IFC_RETURN(EnqueueAnimationCommand(animationCommand));
                }
            }
            else
            {
                animationCommand.reset(new ListViewBaseItemAnimationCommand_Pressed(
                    false /* pressed */,
                    xref::get_weakref(templateChild),
                    FALSE /*isStarting*/,
                    !useTransitions));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }
        }

        if (m_backplateRectangle)
        {
            ASSERT(isRoundedListViewBaseItemChromeEnabled);
            IFC_RETURN(SetBackplateBackground());
        }

        dirty = true;
    }

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.disabledState, pWentToState))
    {
        const bool roundedGridViewItem = isRoundedListViewBaseItemChromeEnabled && IsChromeForGridViewItem();

        if (roundedGridViewItem)
        {
            const bool disabled =
                m_visualStates.HasState(DisabledStates_Disabled);
            const bool pointerOver =
                m_visualStates.HasState(CommonStates2_PointerOver);
            const bool selected =
                m_visualStates.HasState(CommonStates2_Selected) ||
                m_visualStates.HasState(CommonStates2_PressedSelected) ||
                m_visualStates.HasState(CommonStates2_PointerOverSelected);

            if (!m_outerBorder)
            {
                if (selected || (!disabled && pointerOver))
                {
                    IFC_RETURN(EnsureOuterBorder());
                }
            }
            else
            {
                if (selected || (!disabled && pointerOver))
                {
                    IFC_RETURN(SetOuterBorderBrush());
                }
                else
                {
                    IFC_RETURN(RemoveOuterBorder());
                }
            }

            if (!m_innerSelectionBorder)
            {
                if (selected)
                {
                    IFC_RETURN(EnsureInnerSelectionBorder());
                }
            }
            else
            {
                if (selected)
                {
                    IFC_RETURN(SetInnerSelectionBorderBrush());
                }
                else
                {
                    IFC_RETURN(RemoveInnerSelectionBorder());
                }
            }
        }

        if (isRoundedListViewBaseItemChromeEnabled)
        {
            if (m_backplateRectangle)
            {
                IFC_RETURN(SetBackplateBackground());
            }

            if (m_multiSelectCheckGlyph)
            {
                IFC_RETURN(SetMultiSelectCheckBoxForeground());
            }

            if (m_multiSelectCheckBoxRectangle)
            {
                IFC_RETURN(SetMultiSelectCheckBoxBackground());
                IFC_RETURN(SetMultiSelectCheckBoxBorder());
            }

            if (m_selectionIndicatorRectangle)
            {
                IFC_RETURN(SetSelectionIndicatorBackground());
            }
        }

        disabledChanged = true;
        dirty = true;
    }

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.focusState, pWentToState))
    {
        dirty = true;
    }

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.multiSelectState, pWentToState))
    {
        const bool entering = m_visualStates.HasState(MultiSelectStates_MultiSelectEnabled);
        double contentTranslationX = isRoundedListViewBaseItemChromeEnabled ? s_multiSelectRoundedContentOffset : s_listViewItemMultiSelectContentOffset;
        std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

        if (entering)
        {
            if (m_isInIndicatorSelect)
            {
                contentTranslationX -= listViewItemSelectionIndicatorContentOffset;
            }
            m_isInIndicatorSelect = false;
            m_isInMultiSelect = true;
            IFC_RETURN(EnsureMultiSelectCheckBox());
        }
        else if (IsInSelectionIndicatorMode())
        {
            contentTranslationX -= listViewItemSelectionIndicatorContentOffset;
        }

        animationCommand.reset(new ListViewBaseItemAnimationCommand_MultiSelect(
            isRoundedListViewBaseItemChromeEnabled,
            entering,
            s_multiSelectSquareSize.width /*checkBoxTranslationX*/,
            contentTranslationX,
            m_checkMode,
            xref::get_weakref(m_multiSelectCheckBoxRectangle.get()),
            xref::get_weakref(templateChild),
            true /*isStarting*/,
            !useTransitions));
        IFC_RETURN(EnqueueAnimationCommand(animationCommand));

        needsMeasure = true;
        needsArrange = true;
        dirty = true;
    }

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.selectionIndicatorState, pWentToState))
    {
        const bool entering = m_visualStates.HasState(SelectionIndicatorStates_SelectionIndicatorEnabled);
        const bool enqueueAnimationCommand = !m_isInMultiSelect;

        if (entering)
        {
            m_isInIndicatorSelect = true;
            m_isInMultiSelect = false;
        }
        else if (m_visualStates.HasState(MultiSelectStates_MultiSelectDisabled))
        {
            m_isInIndicatorSelect = false;
            m_isInMultiSelect = false;
        }

        if (enqueueAnimationCommand)
        {
            std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

            animationCommand.reset(new ListViewBaseItemAnimationCommand_IndicatorSelect(
                entering,
                listViewItemSelectionIndicatorContentOffset,
                GetSelectionIndicatorMode(),
                xref::get_weakref(m_selectionIndicatorRectangle.get()),
                xref::get_weakref(templateChild),
                TRUE /*isStarting*/,
                !useTransitions));
            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }

        needsMeasure = true;
        needsArrange = true;
        dirty = true;
    }

    ReorderHintStates oldReorderHintState = m_visualStates.reorderHintState;

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.reorderHintState, pWentToState))
    {
        std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

        if (m_visualStates.HasState(ReorderHintStates_NoReorderHint))
        {
            XPOINTF offset = { 0.0f, 0.0f };

            IFC_RETURN(ComputeReorderHintOffset(oldReorderHintState, &offset));
            animationCommand.reset(new ListViewBaseItemAnimationCommand_ReorderHint(
                offset.x,
                offset.y,
                xref::get_weakref(this),
                FALSE /*isStarting*/,
                !useTransitions));
            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }
        else
        {
            XPOINTF offset = { 0.0f, 0.0f };

            if (oldReorderHintState != ReorderHintStates_NoReorderHint)
            {
                // Gotta clear out the old hint first.
                IFC_RETURN(ComputeReorderHintOffset(oldReorderHintState, &offset));
                animationCommand.reset(new ListViewBaseItemAnimationCommand_ReorderHint(
                    offset.x,
                    offset.y,
                    xref::get_weakref(this),
                    FALSE /*isStarting*/,
                    TRUE /*steadyStateOnly*/));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }

            IFC_RETURN(ComputeReorderHintOffset(m_visualStates.reorderHintState, &offset));
            animationCommand.reset(new ListViewBaseItemAnimationCommand_ReorderHint(
                offset.x,
                offset.y,
                xref::get_weakref(this),
                TRUE /*isStarting*/,
                !useTransitions));
            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }
        dirty = true;
    }

    DragStates oldDragDropState = m_visualStates.dragState;

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.dragState, pWentToState))
    {
        std::unique_ptr<ListViewBaseItemAnimationCommand> animationCommand;

        bool dragCountTextBlockVisible = false;
        if (m_visualStates.HasState(DragStates_NotDragging))
        {
            animationCommand.reset(new ListViewBaseItemAnimationCommand_DragDrop(
                ListViewBaseItemAnimationCommand_DragDrop::DragDropState_Target,
                xref::get_weakref(this),
                xref::get_weakref(m_pSecondaryChrome),
                FALSE /*isStarting*/,
                !useTransitions));
            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }
        else
        {
            ListViewBaseItemAnimationCommand_DragDrop::DragDropState state;
            xref::weakref_ptr<CListViewBaseItemChrome> pBaseAnimationTarget = xref::get_weakref(this);
            xref::weakref_ptr<CFrameworkElement> pFadeOutAnimationTarget;

            // We need to have the secondary chrome _now_, since we need it as a target!
            IFC_RETURN(AddSecondaryChrome());
            pFadeOutAnimationTarget = xref::get_weakref(m_pSecondaryChrome);

            if (m_visualStates.HasState(DragStates_Dragging))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_SinglePrimary;
            }
            else if (m_visualStates.HasState(DragStates_MultipleDraggingPrimary))
            {
                dragCountTextBlockVisible = TRUE;
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_MultiPrimary;
            }
            else if (m_visualStates.HasState(DragStates_MultipleDraggingSecondary))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_MultiSecondary;
                pFadeOutAnimationTarget = xref::get_weakref(this);
            }
            else if (m_visualStates.HasState(DragStates_DraggingTarget))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_Target;
            }
            else if (m_visualStates.HasState(DragStates_DraggedPlaceholder))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_DraggedPlaceholder;
                pFadeOutAnimationTarget = xref::get_weakref(this);

                if (m_visualStates.HasState(MultiSelectStates_MultiSelectEnabled))
                {
                    IFC_RETURN(EnsureMultiSelectCheckBox());
                }
                else if (m_multiSelectCheckBoxRectangle)
                {
                    // Extended selection mode, we suppress item count border for the placeholder
                    IFC_RETURN(RemoveMultiSelectCheckBox());
                }
            }
            else if (m_visualStates.HasState(DragStates_Reordering))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingSinglePrimary;
            }
            else if (m_visualStates.HasState(DragStates_MultipleReorderingPrimary))
            {
                dragCountTextBlockVisible = TRUE;
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingMultiPrimary;
            }
            else if (m_visualStates.HasState(DragStates_ReorderingTarget))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderingTarget;
            }
            else if (m_visualStates.HasState(DragStates_ReorderedPlaceholder))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_ReorderedPlaceholder;
                pFadeOutAnimationTarget = xref::get_weakref(this);

                if (m_visualStates.HasState(MultiSelectStates_MultiSelectEnabled))
                {
                    IFC_RETURN(EnsureMultiSelectCheckBox());
                }
                else if (m_multiSelectCheckBoxRectangle)
                {
                    // Extended selection mode, we suppress item count border for the placeholder
                    IFC_RETURN(RemoveMultiSelectCheckBox());
                }
            }
            else if (m_visualStates.HasState(DragStates_DragOver))
            {
                state = ListViewBaseItemAnimationCommand_DragDrop::DragDropState_DragOver;
            }

            if (oldDragDropState != DragStates_NotDragging)
            {
                // Gotta clear out the old state first.
                animationCommand.reset(new ListViewBaseItemAnimationCommand_DragDrop(
                    state,
                    pBaseAnimationTarget,
                    pFadeOutAnimationTarget,
                    FALSE /*isStarting*/,
                    TRUE /*steadyStateOnly*/));
                IFC_RETURN(EnqueueAnimationCommand(animationCommand));
            }

            animationCommand.reset(new ListViewBaseItemAnimationCommand_DragDrop(
                state,
                pBaseAnimationTarget,
                pFadeOutAnimationTarget,
                TRUE /*isStarting*/,
                !useTransitions));

            IFC_RETURN(EnqueueAnimationCommand(animationCommand));
        }

        IFC_RETURN(SetDragOverlayTextBlockVisible(dragCountTextBlockVisible));
        dirty = true;
    }

    if (!*pWentToState &&
        UpdateVisualStateGroup(pStateName, &m_visualStates.dataVirtualizationState, pWentToState))
    {
        dirty = true;
    }

    if (disabledChanged)
    {
        CValue opacityValue;

        if (m_visualStates.HasState(DisabledStates_Disabled))
        {
            opacityValue.SetFloat(GetDisabledOpacity());
        }
        else
        {
            ASSERT(m_visualStates.HasState(DisabledStates_Enabled));
            opacityValue.SetFloat(1.0f);
        }

        if (isRoundedListViewBaseItemChromeEnabled)
        {
            if (templateChild)
            {
                IFC_RETURN(templateChild->SetValueByIndex(KnownPropertyIndex::UIElement_Opacity, opacityValue));
            }
        }
        else
        {
            CContentControl* parentListViewBaseItemNoRef = nullptr;

            IFC_RETURN(GetParentListViewBaseItemNoRef(&parentListViewBaseItemNoRef));
            IFC_RETURN(parentListViewBaseItemNoRef->SetValueByIndex(KnownPropertyIndex::UIElement_Opacity, opacityValue));
        }
    }

    if (needsMeasure)
    {
        InvalidateMeasure();
    }

    if (needsArrange)
    {
        InvalidateArrange();
    }

    if (dirty)
    {
        InvalidateRender();
    }

    return S_OK;
}

// Draws the below-content layer
_Check_return_ HRESULT CListViewBaseItemChrome::DrawUnderContentLayerNewStyle(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds
    )
{
    XRECTF controlBorderBounds = bounds;

    if (!IsRoundedListViewBaseItemChromeEnabled())
    {
        CContentControl* pParentListViewBaseItemNoRef = nullptr;

        IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

        // Control background
        if (pParentListViewBaseItemNoRef->m_pBackground)
        {
            IFC_RETURN(AddRectangle(
                pContentRenderer,
                controlBorderBounds,
                pParentListViewBaseItemNoRef->m_pBackground,
                this
                ));
        }

        // Border background
        // Responsible for the different visual states in CommonStates2
        // For ListViewItem, we render a filled rectangle under the content
        // For GridViewItem, we render a hollow rectangle on top of the content
        if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
        {
            CBrush* backgroundBrush = nullptr;

            if (m_visualStates.HasState(CommonStates2_PointerOver))
            {
                backgroundBrush = m_pPointerOverBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_Pressed))
            {
                backgroundBrush = m_pPressedBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_Selected))
            {
                backgroundBrush = m_pSelectedBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_PointerOverSelected))
            {
                backgroundBrush = m_pSelectedPointerOverBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_PressedSelected))
            {
                backgroundBrush = m_pSelectedPressedBackground;
            }

            if (backgroundBrush)
            {
                if (m_previousBackgroundBrush.get() != backgroundBrush)
                {
                    bool isAnimationEnabled = true;
                    IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

                    DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

                    if (dcompTreeHost != nullptr && isAnimationEnabled)
                    {
                        CBrushTransition* const backgroundTransition = CUIElement::GetBrushTransitionNoRef(*this, KnownPropertyIndex::ContentPresenter_BackgroundTransition);

                        if (backgroundTransition != nullptr)
                        {
                            dcompTreeHost->GetWUCBrushManager()->SetUpBrushTransitionIfAllowed(
                                m_previousBackgroundBrush.get() /* from */,
                                backgroundBrush /* to */,
                                *this,
                                ElementBrushProperty::Fill,
                                *backgroundTransition);
                        }
                        else
                        {
                            dcompTreeHost->GetWUCBrushManager()->CleanUpBrushTransition(*this, ElementBrushProperty::Fill);
                        }
                    }

                    m_previousBackgroundBrush = backgroundBrush;
                }

                IFC_RETURN(AddRectangle(
                    pContentRenderer,
                    controlBorderBounds,
                    backgroundBrush,
                    this
                    ));
            }
            else
            {
                m_previousBackgroundBrush.reset();
            }
        }
    }

    // Draw Reveal Background brush below content
    if (GetRevealBackgroundBrushNoRef() && !GetRevealBackgroundShowsAboveContent())
    {
        IFC_RETURN(DrawRevealBackground(pContentRenderer, controlBorderBounds));
    }

    return S_OK;
}

// Draws the above-content layer
_Check_return_ HRESULT CListViewBaseItemChrome::DrawOverContentLayerNewStyle(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds
    )
{
    XRECTF controlBorderBounds = {};
    XTHICKNESS controlBorderThickness = {};

    CContentControl* pParentListViewBaseItemNoRef = nullptr;

    IFC_RETURN(GetParentListViewBaseItemNoRef(&pParentListViewBaseItemNoRef));

    controlBorderThickness = pParentListViewBaseItemNoRef->m_borderThickness;

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&controlBorderThickness);
    }

    controlBorderBounds = bounds;

    // Placeholder
    if (m_visualStates.HasState(DataVirtualizationStates_DataPlaceholder) && m_pPlaceholderBackground)
    {
        IFC_RETURN(AddRectangle(
            pContentRenderer,
            controlBorderBounds,
            m_pPlaceholderBackground,
            this
            ));
    }

    // Control border
    if (pParentListViewBaseItemNoRef->m_pBorderBrush)
    {
        IFC_RETURN(AddBorder(
            pContentRenderer,
            controlBorderBounds,
            controlBorderThickness,
            pParentListViewBaseItemNoRef->m_pBorderBrush,
            this
            ));
    }

    // Border background
    // Responsible for the different visual states in CommonStates2
    // For ListViewItem, we render a filled rectangle under the content
    // For GridViewItem, we render a hollow rectangle (of thickness 2) on top of the content
    if (!IsRoundedListViewBaseItemChromeEnabled() && m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay)
    {
        if (m_visualStates.HasState(FocusStates_Unfocused))
        {
            // if item is not focused currently, we want chrome to draw border. so, clearing the boolean for m_isFocusVisualDrawnByFocusManager
            m_isFocusVisualDrawnByFocusManager = false;
        }

        if (m_isFocusVisualDrawnByFocusManager)
        {
            // if we get here, it means the item has focus and the focus manager has drawn the inner border. chrome should not draw any border.
            m_isFocusVisualDrawnByFocusManager = false;
        }
        else
        {
            // We want the chrome to draw selection/hover/press visual border only if focus manager did not draw the border
            CBrush* borderBrush = nullptr;

            if (m_visualStates.HasState(CommonStates2_PointerOver))
            {
                borderBrush = m_pPointerOverBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_Pressed))
            {
                borderBrush = m_pPressedBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_Selected))
            {
                borderBrush = m_pSelectedBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_PointerOverSelected))
            {
                borderBrush = m_pSelectedPointerOverBackground;
            }
            else if (m_visualStates.HasState(CommonStates2_PressedSelected))
            {
                borderBrush = m_pSelectedPressedBackground;
            }

            if (borderBrush)
            {
                FocusRectangleOptions focusOptions;

                focusOptions.drawFirst = true;
                focusOptions.firstThickness = Thickness(s_gridViewItemFocusBorderThickness);
                focusOptions.firstBrush = static_cast<CSolidColorBrush*>(borderBrush);
                focusOptions.isContinuous = true;

                IFC_RETURN(pContentRenderer->RenderFocusRectangle(
                    this,
                    focusOptions));
            }
        }
    }

    // TH2 Focus Rectangle (dotted lines) - In this case, the chrome draws focus rectangles, and overrides the FocusRectManager.
    if (!m_visualStates.HasState(DisabledStates_Disabled) && m_visualStates.HasState(FocusStates_Focused))
    {
        bool shouldDrawDottedLines = false;
        IFC_RETURN(ShouldDrawDottedLinesFocusVisual(&shouldDrawDottedLines));
        if (shouldDrawDottedLines)
        {
            XFLOAT thickness = 0.0f;
            FocusRectangleOptions focusOptions;

            if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
            {
                thickness = s_listViewItemFocusBorderThickness;
            }
            else
            {
                ASSERT(m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay);
                thickness = s_gridViewItemFocusBorderThickness;
            }

            focusOptions.firstThickness = Thickness(thickness);

            if (m_pFocusSecondaryBorderBrush)
            {
                focusOptions.firstBrush = static_cast<CSolidColorBrush*>(m_pFocusSecondaryBorderBrush);
                focusOptions.drawFirst = true;
            }

            if (m_pFocusBorderBrush)
            {
                focusOptions.secondBrush = static_cast<CSolidColorBrush*>(m_pFocusBorderBrush);
                focusOptions.drawSecond = true;
            }

            if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay &&
                (m_visualStates.HasState(CommonStates2_PointerOver) ||
                    m_visualStates.HasState(CommonStates2_Selected) ||
                    m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
                    m_visualStates.HasState(CommonStates2_Pressed) ||
                    m_visualStates.HasState(CommonStates2_PressedSelected)))
            {
                focusOptions.drawFirst = false;
            }

            IFC_RETURN(pContentRenderer->RenderFocusRectangle(
                this,
                focusOptions));
        }
    }

    {
        // disable hittest for reveal brushes which are above content
        HWRenderParams rp = *(pContentRenderer->GetRenderParams());
        HWRenderParamsOverride hwrpOverride(pContentRenderer, &rp);
        rp.m_isHitTestVisibleSubtree = false;

        // Draw Reveal Background brush over content
        if (GetRevealBackgroundBrushNoRef() && GetRevealBackgroundShowsAboveContent())
        {
            IFC_RETURN(DrawRevealBackground(pContentRenderer, controlBorderBounds));
        }

        // Reveal border is above everything
        CBrush* revealBorderBrush = GetRevealBorderBrushNoRef();

        if (revealBorderBrush)
        {
            IFC_RETURN(AddBorder(
                pContentRenderer,
                controlBorderBounds,
                GetRevealBorderThickness(),
                revealBorderBrush,
                this
            ));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::CustomizeFocusRectangle(
    _Inout_ FocusRectangleOptions& options,
    _Out_ bool* shouldDrawFocusRect)
{
    // Callback from CFocusRectManager at beginning of render walk.  It's an opportunity to tell
    // CFocusRectManager how to render the focus rectangle, and whether or not it should draw it
    // at all.
    bool shouldDrawDottedLines = false;
    IFC_RETURN(ShouldDrawDottedLinesFocusVisual(&shouldDrawDottedLines));
    if (shouldDrawDottedLines)
    {
        *shouldDrawFocusRect = false;
    }
    else
    {
        if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay)
        {
            if (m_visualStates.HasState(CommonStates2_PointerOver))
            {
                options.secondThickness = Thickness(s_gridViewItemFocusBorderThickness);
                options.secondBrush = static_cast<CSolidColorBrush*>(m_pPointerOverBackground);
            }
            else if (m_visualStates.HasState(CommonStates2_Pressed))
            {
                options.secondThickness = Thickness(s_gridViewItemFocusBorderThickness);
                options.secondBrush = static_cast<CSolidColorBrush*>(m_pPressedBackground);
            }
            else if (m_visualStates.HasState(CommonStates2_Selected))
            {
                options.secondThickness = Thickness(s_gridViewItemFocusBorderThickness);
                options.secondBrush = static_cast<CSolidColorBrush*>(m_pSelectedBackground);
            }
            else if (m_visualStates.HasState(CommonStates2_PointerOverSelected))
            {
                options.secondThickness = Thickness(s_gridViewItemFocusBorderThickness);
                options.secondBrush = static_cast<CSolidColorBrush*>(m_pSelectedPointerOverBackground);
            }
            else if (m_visualStates.HasState(CommonStates2_PressedSelected))
            {
                options.secondThickness = Thickness(s_gridViewItemFocusBorderThickness);
                options.secondBrush = static_cast<CSolidColorBrush*>(m_pSelectedPressedBackground);
            }
        }

        *shouldDrawFocusRect = true;
        m_isFocusVisualDrawnByFocusManager = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
CListViewBaseItemChrome::ShouldDrawDottedLinesFocusVisual(_Out_ bool* shouldDrawDottedLines)
{
    *shouldDrawDottedLines = false;

    CContentControl* parentListViewBaseItem = nullptr;
    IFC_RETURN(GetParentListViewBaseItemNoRef(&parentListViewBaseItem));

    CValue useSystemFocusVisuals;
    IFC_RETURN(parentListViewBaseItem->GetValue(
        parentListViewBaseItem->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_UseSystemFocusVisuals),
        &useSystemFocusVisuals));

    auto focusVisualKind = CApplication::GetFocusVisualKind();

    if (useSystemFocusVisuals.AsBool() && focusVisualKind == DirectUI::FocusVisualKind::DottedLine)
    {
        *shouldDrawDottedLines = true;
    }

    return S_OK;
}

// Removes the multi-select checkbox from the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::RemoveMultiSelectCheckBox()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: RemoveMultiSelectCheckBox - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(m_multiSelectCheckBoxRectangle);

    IFC_RETURN(RemoveChild(m_multiSelectCheckBoxRectangle));

    m_multiSelectCheckGlyph.reset();
    m_multiSelectCheckBoxRectangle.reset();
    m_multiSelectCheckBoxClip.reset();

    return S_OK;
}

// Removes the selection indicator from the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::RemoveSelectionIndicator()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: RemoveSelectionIndicator - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(m_selectionIndicatorRectangle);

    IFC_RETURN(RemoveChild(m_selectionIndicatorRectangle));

    m_selectionIndicatorRectangle.reset();

    return S_OK;
}

// Removes the inner selection border from the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::RemoveInnerSelectionBorder()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: RemoveInnerSelectionBorder - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_innerSelectionBorder);

    const bool areBordersNested = m_outerBorder == m_innerSelectionBorder->GetParentInternal();

    if (areBordersNested)
    {
        IFC_RETURN(m_outerBorder->RemoveChild(m_innerSelectionBorder));
    }
    else
    {
        IFC_RETURN(RemoveChild(m_innerSelectionBorder));
    }

    m_innerSelectionBorder.reset();

    return S_OK;
}

// Removes the outer border from the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::RemoveOuterBorder()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: RemoveOuterBorder - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_outerBorder);

    IFC_RETURN(RemoveChild(m_outerBorder));

    m_outerBorder.reset();

    if (m_backplateRectangle)
    {
        IFC_RETURN(SetBackplateMargin());
    }

    return S_OK;
}

// Sets up and adds the multi-select checkbox to the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::EnsureMultiSelectCheckBox()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: EnsureMultiSelectCheckBox - entry.", this));
    }
#endif // LVBIC_DEBUG

    CValue value;
    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);

    if (!m_multiSelectCheckBoxRectangle)
    {
        CREATEPARAMETERS cp(GetContext());
        xref_ptr<CDependencyObject> transitionTarget;
        XSIZEF multiSelectSquareSize = s_multiSelectSquareSize;

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&multiSelectSquareSize);
        }

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject **>(m_multiSelectCheckBoxRectangle.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        value.SetDouble(multiSelectSquareSize.width);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_MinWidth, value));

        value.SetDouble(multiSelectSquareSize.height);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, value));

        IFC_RETURN(CTransitionTarget::Create(transitionTarget.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, transitionTarget.get()));

        IFC_RETURN(AddChild(m_multiSelectCheckBoxRectangle.get()));
    }

    // create checkmark glyph
    if (!m_multiSelectCheckGlyph)
    {
        CREATEPARAMETERS cp(GetContext());
        const xstring_ptr strCheckMarkGlyph = XSTRING_PTR_FROM_STORAGE(c_strCheckMarkGlyphStorage);

        IFC_RETURN(CFontIcon::Create(reinterpret_cast<CDependencyObject **>(m_multiSelectCheckGlyph.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        if (selected)
        {
            value.SetDouble(1.0);
        }
        else
        {
            value.SetDouble(0.0);
        }
        IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, value));

        value.SetDouble(s_checkMarkGlyphFontSize);
        IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::FontIcon_FontSize, value));

        // Setting the glyph for the check mark
        value.SetString(strCheckMarkGlyph);
        IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::FontIcon_Glyph, value));
    }

    // add the glyph to the check box children
    IFC_RETURN(m_multiSelectCheckBoxRectangle->SetChild(m_multiSelectCheckGlyph.get()));

    IFC_RETURN(SetMultiSelectCheckBoxProperties());

    return S_OK;
}

// Sets up and adds the selection indicator to the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::EnsureSelectionIndicator()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: EnsureSelectionIndicator - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsInSelectionIndicatorMode());

    if (!m_selectionIndicatorRectangle)
    {
        CValue value;
        CREATEPARAMETERS cp(GetContext());
        xref_ptr<CDependencyObject> transitionTarget;
        XSIZEF selectionIndicatorSize = s_selectionIndicatorSize;
        const XTHICKNESS zeroThickness = {};
        XTHICKNESS selectionIndicatorMargin = { s_selectionIndicatorMargin.left, 0.0f, s_selectionIndicatorMargin.right, 0.0f };

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&selectionIndicatorSize);
            LayoutRoundHelper(&selectionIndicatorMargin);
        }

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_selectionIndicatorRectangle.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        value.WrapThickness(const_cast<XTHICKNESS*>(&selectionIndicatorMargin));
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));

        value.SetDouble(selectionIndicatorSize.width);
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, value));

        IFC_RETURN(CTransitionTarget::Create(transitionTarget.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_TransitionTarget, transitionTarget.get()));

        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, nullptr));

        value.WrapThickness(const_cast<XTHICKNESS*>(&zeroThickness));
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

        value.Set(DirectUI::VerticalAlignment::Stretch);
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Left);
        IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        IFC_RETURN(AddChild(m_selectionIndicatorRectangle.get()));
    }

    IFC_RETURN(SetSelectionIndicatorBackground());
    IFC_RETURN(SetSelectionIndicatorCornerRadius());

    return S_OK;
}

// Sets up and adds the backplate to the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::EnsureBackplate()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: EnsureBackplate - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());

    if (!m_backplateRectangle)
    {
        CValue value;
        CREATEPARAMETERS cp(GetContext());
        const XTHICKNESS zeroThickness = {};

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_backplateRectangle.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, nullptr));

        if (IsChromeForListViewItem())
        {
            XTHICKNESS backplateMargin = s_backplateMargin;

            if (ShouldUseLayoutRounding())
            {
                LayoutRoundHelper(&backplateMargin);
            }

            value.WrapThickness(const_cast<XTHICKNESS*>(&backplateMargin));
            IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));
        }

        value.WrapThickness(const_cast<XTHICKNESS*>(&zeroThickness));
        IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

        value.Set(DirectUI::VerticalAlignment::Stretch);
        IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        // Inserting the backplate into first position so it is rendered underneath the content
        IFC_RETURN(InsertChild(0, m_backplateRectangle.get()));
    }

    IFC_RETURN(SetBackplateCornerRadius());
    IFC_RETURN(SetBackplateBackground());

    if (IsChromeForGridViewItem())
    {
        IFC_RETURN(SetBackplateMargin());
    }

    return S_OK;
}

// Sets up and adds the inner selection border to the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::EnsureInnerSelectionBorder()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: EnsureInnerSelectionBorder - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());

    if (!m_innerSelectionBorder)
    {
        CValue value;
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_innerSelectionBorder.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, nullptr));

        value.Set(DirectUI::VerticalAlignment::Stretch);
        IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        if (IsOuterBorderBrushOpaque())
        {
            // When the outer border is opaque, it hosts the inner border to avoid any bleed through at the edges.
            IFC_RETURN(m_outerBorder->AddChild(m_innerSelectionBorder.get()));
        }
        else
        {
            // Appending the border into last position so it is rendered over the content
            IFC_RETURN(AddChild(m_innerSelectionBorder.get()));
        }
    }

    IFC_RETURN(SetInnerSelectionBorderProperties());

    return S_OK;
}

// Sets up and adds the outer border to the tree.
_Check_return_ HRESULT CListViewBaseItemChrome::EnsureOuterBorder()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: EnsureOuterBorder - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());

    if (!m_outerBorder)
    {
        CValue value;
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_outerBorder.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, nullptr));

        value.Set(DirectUI::VerticalAlignment::Stretch);
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        // Appending the border into last position so it is rendered over the content
        IFC_RETURN(AddChild(m_outerBorder.get()));
    }

    IFC_RETURN(SetOuterBorderProperties());

    return S_OK;
}

// Sets up the multi-select checkbox to the tree. (colors, alignment, clip)
_Check_return_ HRESULT CListViewBaseItemChrome::SetMultiSelectCheckBoxProperties()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetMultiSelectCheckBoxProperties - entry.", this));
    }
#endif // LVBIC_DEBUG

    const bool isRoundedListViewBaseItemChromeEnabled = IsRoundedListViewBaseItemChromeEnabled();

    CValue value;
    XTHICKNESS multiSelectSquareMargin = {};

    if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
    {
        // ListViewItemBase case
        if (!m_multiSelectCheckBoxClip)
        {
            CREATEPARAMETERS cp(GetContext());
            const XRECTF multiSelectSquareBounds = { 0.0f, 0.0f, s_multiSelectSquareSize.width, s_multiSelectSquareSize.height };

            IFC_RETURN(CRectangleGeometry::Create(reinterpret_cast<CDependencyObject **>(m_multiSelectCheckBoxClip.ReleaseAndGetAddressOf()), &cp));

            value.WrapRect(&multiSelectSquareBounds);
            IFC_RETURN(m_multiSelectCheckBoxClip->SetValueByKnownIndex(KnownPropertyIndex::RectangleGeometry_Rect, value));
        }

        multiSelectSquareMargin = isRoundedListViewBaseItemChromeEnabled ? s_multiSelectRoundedSquareInlineMargin : s_multiSelectSquareInlineMargin;

        value.Set(DirectUI::VerticalAlignment::Center);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Left);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Clip, m_multiSelectCheckBoxClip.get()));
    }
    else
    {
        ASSERT(m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay);

        // GridViewItemBase case
        if (isRoundedListViewBaseItemChromeEnabled)
        {
            XTHICKNESS selectedBorderThickness = GetSelectedBorderThickness();

            multiSelectSquareMargin.top = s_innerSelectionBorderThickness.top + selectedBorderThickness.top + 1.0f;
            multiSelectSquareMargin.right = s_innerSelectionBorderThickness.right + selectedBorderThickness.right + 1.0f;
        }
        else
        {
            multiSelectSquareMargin = s_multiSelectSquareOverlayMargin;
        }

        value.Set(DirectUI::VerticalAlignment::Top);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.Set(DirectUI::HorizontalAlignment::Right);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));
    }

    if (isRoundedListViewBaseItemChromeEnabled)
    {
        XCORNERRADIUS cornerRadius = GetCheckBoxCornerRadius();

        value.WrapCornerRadius(&cornerRadius);
        IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));
    }

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&multiSelectSquareMargin);
    }
    value.WrapThickness(const_cast<XTHICKNESS*>(&multiSelectSquareMargin));
    IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));

    // set the check box background brush
    IFC_RETURN(SetMultiSelectCheckBoxBackground());

    // set the check box border brush and thickness
    IFC_RETURN(SetMultiSelectCheckBoxBorder());

    // set the glyph's foreground brush
    IFC_RETURN(SetMultiSelectCheckBoxForeground());

    if (!isRoundedListViewBaseItemChromeEnabled)
    {
        IFC_RETURN(SetForegroundBrush());
    }

    return S_OK;
}

// Sets up the inner selection border visual variable properties.
_Check_return_ HRESULT CListViewBaseItemChrome::SetInnerSelectionBorderProperties()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetInnerSelectionBorderProperties - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_innerSelectionBorder);

    IFC_RETURN(SetInnerSelectionBorderBrush());
    IFC_RETURN(SetInnerSelectionBorderCornerRadius());
    IFC_RETURN(SetInnerSelectionBorderThickness());

    return S_OK;
}

// Sets up the outer border visual variable properties.
_Check_return_ HRESULT CListViewBaseItemChrome::SetOuterBorderProperties()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetOuterBorderProperties - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_outerBorder);

    IFC_RETURN(SetOuterBorderBrush());
    IFC_RETURN(SetOuterBorderCornerRadius());
    IFC_RETURN(SetOuterBorderThickness());

    return S_OK;
}

// Sets the backplate Background brush based on current visual state.
_Check_return_ HRESULT CListViewBaseItemChrome::SetBackplateBackground()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetBackplateBackground - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(m_backplateRectangle);

    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);
    const bool pointerOver =
        m_visualStates.HasState(CommonStates2_PointerOver) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected);
    const bool pressed =
        m_visualStates.HasState(CommonStates2_Pressed) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);

    CBrush* backplateRectangleBackground = nullptr;

    if (m_visualStates.HasState(DisabledStates_Disabled))
    {
        if (selected)
        {
            backplateRectangleBackground = m_pSelectedDisabledBackground;
        }
    }
    else if (pressed)
    {
        if (selected)
        {
            backplateRectangleBackground = m_pSelectedPressedBackground;
        }
        else
        {
            backplateRectangleBackground = m_pPressedBackground;
        }
    }
    else if (pointerOver)
    {
        if (selected)
        {
            backplateRectangleBackground = m_pSelectedPointerOverBackground;
        }
        else
        {
            backplateRectangleBackground = m_pPointerOverBackground;
        }
    }
    else if (selected)
    {
        backplateRectangleBackground = m_pSelectedBackground;
    }
    else
    {
        CContentControl* parentListViewBaseItemNoRef = nullptr;

        IFC_RETURN(GetParentListViewBaseItemNoRef(&parentListViewBaseItemNoRef));

        if (parentListViewBaseItemNoRef)
        {
            backplateRectangleBackground = parentListViewBaseItemNoRef->m_pBackground;
        }
    }

    IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, backplateRectangleBackground));

    return S_OK;
}

// Sets up the backplate CornerRadius.
_Check_return_ HRESULT CListViewBaseItemChrome::SetBackplateCornerRadius()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetBackplateCornerRadius - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(m_backplateRectangle);

    IFC_RETURN(SetGeneralCornerRadius(m_backplateRectangle));

    return S_OK;
}

// Sets up the backplate Margin for GridViewItem.
_Check_return_ HRESULT CListViewBaseItemChrome::SetBackplateMargin()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetBackplateMargin - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_backplateRectangle);

    // When the outer border is present and opaque, the backplate gets a 1px margin to avoid any bleed through at the edges.
    const XTHICKNESS zeroThickness = {};
    const XTHICKNESS oneThickness = { 1.0f, 1.0f, 1.0f, 1.0f };
    XTHICKNESS backplateMargin = IsOuterBorderBrushOpaque() ? oneThickness : zeroThickness;
    CValue value;

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&backplateMargin);
    }

    value.WrapThickness(const_cast<XTHICKNESS*>(&backplateMargin));
    IFC_RETURN(m_backplateRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));

    return S_OK;
}

// Sets the inner selection border's BorderBrush based on current SelectionBorderBrush property.
_Check_return_ HRESULT CListViewBaseItemChrome::SetInnerSelectionBorderBrush()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetInnerSelectionBorderBrush - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_innerSelectionBorder);

#ifdef DBG
    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);
    ASSERT(selected);
#endif // DBG

    IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, m_pSelectedInnerBorderBrush));

    return S_OK;
}

// Sets the inner selection border's CornerRadius.
_Check_return_ HRESULT CListViewBaseItemChrome::SetInnerSelectionBorderCornerRadius()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetInnerSelectionBorderCornerRadius - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_innerSelectionBorder);

    CValue value;
    XCORNERRADIUS cornerRadius = GetGeneralCornerRadius();
    const bool areBordersNested = m_outerBorder == m_innerSelectionBorder->GetParentInternal();

    if (areBordersNested)
    {
        XTHICKNESS selectedBorderThickness = GetSelectedBorderThickness();

        // Decrease inner border corner radius to account for outer border thickness.
        cornerRadius.bottomLeft = MAX(s_innerBorderCornerRadius, cornerRadius.bottomLeft - selectedBorderThickness.left);
        cornerRadius.bottomRight = MAX(s_innerBorderCornerRadius, cornerRadius.bottomRight - selectedBorderThickness.right);
        cornerRadius.topLeft = MAX(s_innerBorderCornerRadius, cornerRadius.topLeft - selectedBorderThickness.left);
        cornerRadius.topRight = MAX(s_innerBorderCornerRadius, cornerRadius.topRight - selectedBorderThickness.right);
    }

    value.WrapCornerRadius(&cornerRadius);
    IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));

    return S_OK;
}

// Sets the outer selection border's BorderBrush based on current visual state.
_Check_return_ HRESULT CListViewBaseItemChrome::SetOuterBorderBrush()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetOuterBorderBrush - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_outerBorder);

    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);

    ASSERT(selected || (!m_visualStates.HasState(DisabledStates_Disabled) && m_visualStates.HasState(CommonStates2_PointerOver)));

    CBrush* borderBrush = nullptr;

    if (selected)
    {
        if (m_visualStates.HasState(DisabledStates_Disabled))
        {
            borderBrush = m_pSelectedDisabledBorderBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PressedSelected))
        {
            borderBrush = m_pSelectedPressedBorderBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PointerOverSelected))
        {
            borderBrush = m_pSelectedPointerOverBorderBrush;
        }
        else
        {
            borderBrush = m_pSelectedBorderBrush;
        }
    }
    else
    {
        borderBrush = m_pPointerOverBorderBrush;
    }

    IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, borderBrush));

    IFC_RETURN(UpdateBordersParenting());

    if (m_backplateRectangle)
    {
        IFC_RETURN(SetBackplateMargin());
    }

    return S_OK;
}

// Sets the outer border's CornerRadius.
_Check_return_ HRESULT CListViewBaseItemChrome::SetOuterBorderCornerRadius()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetOuterBorderCornerRadius - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(m_outerBorder);

    IFC_RETURN(SetGeneralCornerRadius(m_outerBorder));

    return S_OK;
}

// Sets the inner selection border's BorderThickness based on current SelectedBorderThickness property.
_Check_return_ HRESULT CListViewBaseItemChrome::SetInnerSelectionBorderThickness()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetInnerSelectionBorderThickness - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_innerSelectionBorder);

#ifdef DBG
    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);
    ASSERT(selected);
#endif // DBG

    const bool areBordersNested = m_outerBorder == m_innerSelectionBorder->GetParentInternal();
    XTHICKNESS innerSelectionBorderThickness = s_innerSelectionBorderThickness;
    CValue value;

    if (areBordersNested)
    {
        // When the outer border is opaque, it hosts the inner border which is expanded all around by a pixel
        // to avoid any bleed through at the edges and in-between the borders.
        innerSelectionBorderThickness.left += 1.0f;
        innerSelectionBorderThickness.top += 1.0f;
        innerSelectionBorderThickness.right += 1.0f;
        innerSelectionBorderThickness.bottom += 1.0f;

        XTHICKNESS innerSelectionBorderMargin = { -1.0f, -1.0f, -1.0f, -1.0f };

        if (ShouldUseLayoutRounding())
        {
            LayoutRoundHelper(&innerSelectionBorderMargin);
        }

        value.WrapThickness(const_cast<XTHICKNESS*>(&innerSelectionBorderMargin));
        IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));
    }
    else
    {
        XTHICKNESS selectedBorderThickness = GetSelectedBorderThickness();

        innerSelectionBorderThickness.left += selectedBorderThickness.left;
        innerSelectionBorderThickness.top += selectedBorderThickness.top;
        innerSelectionBorderThickness.right += selectedBorderThickness.right;
        innerSelectionBorderThickness.bottom += selectedBorderThickness.bottom;
    }

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&innerSelectionBorderThickness);
    }

    value.WrapThickness(const_cast<XTHICKNESS*>(&innerSelectionBorderThickness));
    IFC_RETURN(m_innerSelectionBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

    return S_OK;
}

// Sets the outer selection border's BorderThickness based on current visual state.
_Check_return_ HRESULT CListViewBaseItemChrome::SetOuterBorderThickness()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetOuterBorderThickness - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsRoundedListViewBaseItemChromeEnabled());
    ASSERT(IsChromeForGridViewItem());
    ASSERT(m_outerBorder);

    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);

    ASSERT(selected || (!m_visualStates.HasState(DisabledStates_Disabled) && m_visualStates.HasState(CommonStates2_PointerOver)));

    XTHICKNESS outerBorderThickness = selected ? GetSelectedBorderThickness() : s_borderThickness;

    if (ShouldUseLayoutRounding())
    {
        LayoutRoundHelper(&outerBorderThickness);
    }

    CValue value;

    value.WrapThickness(const_cast<XTHICKNESS*>(&outerBorderThickness));
    IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

    return S_OK;
}

// Sets the multi-select checkbox background brush.
_Check_return_ HRESULT CListViewBaseItemChrome::SetMultiSelectCheckBoxBackground()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetMultiSelectCheckBoxBackground - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(m_multiSelectCheckBoxRectangle);
    ASSERT(m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline || m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay);

    const bool selected =
        m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected);

    CBrush* checkBoxBrush = nullptr;

    if (IsRoundedListViewBaseItemChromeEnabled())
    {
        if (m_visualStates.HasState(DisabledStates_Disabled))
        {
            checkBoxBrush = selected ? m_pCheckBoxSelectedDisabledBrush : m_pCheckBoxDisabledBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PressedSelected))
        {
            checkBoxBrush = m_pCheckBoxSelectedPressedBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_Pressed))
        {
            checkBoxBrush = m_pCheckBoxPressedBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PointerOverSelected))
        {
            checkBoxBrush = m_pCheckBoxSelectedPointerOverBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PointerOver))
        {
            checkBoxBrush = m_pCheckBoxPointerOverBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_Selected))
        {
            checkBoxBrush = m_pCheckBoxSelectedBrush;
        }
        else
        {
            checkBoxBrush = m_pCheckBoxBrush;
        }
    }
    else if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay)
    {
        checkBoxBrush = selected ? m_pSelectedBackground : m_pCheckBoxBrush;
    }

    IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, checkBoxBrush));

    return S_OK;
}

// Sets the multi-select checkbox border brush and thickness.
_Check_return_ HRESULT CListViewBaseItemChrome::SetMultiSelectCheckBoxBorder()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetMultiSelectCheckBoxBorder - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(m_multiSelectCheckBoxRectangle);
    ASSERT(m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline || m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay);

    const bool isRoundedListViewBaseItemChromeEnabled = IsRoundedListViewBaseItemChromeEnabled();
    const XTHICKNESS zeroThickness = {};
    CValue value;
    CBrush* checkBoxBorderBrush = nullptr;

    if (isRoundedListViewBaseItemChromeEnabled)
    {
        const bool selected =
            m_visualStates.HasState(CommonStates2_Selected) ||
            m_visualStates.HasState(CommonStates2_PointerOverSelected) ||
            m_visualStates.HasState(CommonStates2_PressedSelected);

        if (selected)
        {
            value.WrapThickness(const_cast<XTHICKNESS*>(&zeroThickness));
        }
        else
        {
            XTHICKNESS multiSelectSquareThickness = s_multiSelectRoundedSquareThickness;

            if (ShouldUseLayoutRounding())
            {
                LayoutRoundHelper(&multiSelectSquareThickness);
            }

            value.WrapThickness(const_cast<XTHICKNESS*>(&multiSelectSquareThickness));

            if (m_visualStates.HasState(DisabledStates_Disabled))
            {
                checkBoxBorderBrush = m_pCheckBoxDisabledBorderBrush;
            }
            else if (m_visualStates.HasState(CommonStates2_Pressed))
            {
                checkBoxBorderBrush = m_pCheckBoxPressedBorderBrush;
            }
            else if (m_visualStates.HasState(CommonStates2_PointerOver))
            {
                checkBoxBorderBrush = m_pCheckBoxPointerOverBorderBrush;
            }
            else
            {
                checkBoxBorderBrush = m_pCheckBoxBorderBrush;
            }
        }
    }
    else
    {
        if (m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline)
        {
            // ListViewItemBase case
            value.WrapThickness(&s_multiSelectSquareThickness);

            checkBoxBorderBrush = m_pCheckBoxBrush;
        }
        else
        {
            // GridViewItemBase case, m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Overlay
            value.WrapThickness(const_cast<XTHICKNESS*>(&zeroThickness));
        }
    }

    IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));
    IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, checkBoxBorderBrush));

    return S_OK;
}

// Sets the multi-select checkbox glyph's foreground brush.
_Check_return_ HRESULT CListViewBaseItemChrome::SetMultiSelectCheckBoxForeground()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetMultiSelectCheckBoxForeground - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(m_multiSelectCheckGlyph);

    // set the Glyph's brush to m_pCheckBrush, m_pCheckPressedBrush or m_pCheckDisabledBrush
    CBrush* checkBrush = m_pCheckBrush;

    if (IsRoundedListViewBaseItemChromeEnabled())
    {
        if (m_visualStates.HasState(DisabledStates_Disabled))
        {
            checkBrush = m_pCheckDisabledBrush;
        }
        else if (m_visualStates.HasState(CommonStates2_PressedSelected))
        {
            checkBrush = m_pCheckPressedBrush;
        }
    }

    IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::IconElement_Foreground, checkBrush));

    return S_OK;
}

// Sets the selection indicator background brush.
_Check_return_ HRESULT CListViewBaseItemChrome::SetSelectionIndicatorBackground()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetSelectionIndicatorBackground - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsSelectionIndicatorVisualEnabled());
    ASSERT(m_selectionIndicatorRectangle);

    const bool disabled = m_visualStates.HasState(DisabledStates_Disabled);
    const bool pressed = m_visualStates.HasState(CommonStates2_PressedSelected);
    const bool pointerOver = m_visualStates.HasState(CommonStates2_PointerOverSelected);

    ASSERT(m_visualStates.HasState(CommonStates2_Selected) || pointerOver || pressed);

    CBrush* selectionIndicatorRectangleBackground = nullptr;

    if (disabled)
    {
        selectionIndicatorRectangleBackground = m_pSelectionIndicatorDisabledBrush;
    }
    else if (pressed)
    {
        selectionIndicatorRectangleBackground = m_pSelectionIndicatorPressedBrush;
    }
    else if (pointerOver)
    {
        selectionIndicatorRectangleBackground = m_pSelectionIndicatorPointerOverBrush;
    }
    else
    {
        selectionIndicatorRectangleBackground = m_pSelectionIndicatorBrush;
    }

    IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, selectionIndicatorRectangleBackground));

    return S_OK;
}

// Sets the selection indicator corner radius.
_Check_return_ HRESULT CListViewBaseItemChrome::SetSelectionIndicatorCornerRadius()
{
#ifdef LVBIC_DEBUG
    if (LVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_LISTVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_LISTVIEWBASEITEMCHROME | LVBIC_DBG) /*traceType*/,
            L"LVBIC[0x%p]: SetSelectionIndicatorCornerRadius - entry.", this));
    }
#endif // LVBIC_DEBUG

    ASSERT(IsSelectionIndicatorVisualEnabled());
    ASSERT(m_selectionIndicatorRectangle);

    CValue value;
    XCORNERRADIUS cornerRadius = GetSelectionIndicatorCornerRadius();

    value.WrapCornerRadius(&cornerRadius);
    IFC_RETURN(m_selectionIndicatorRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));

    return S_OK;
}

// Sets / clears content's foreground brush in Common visual states.
// This function is only called for new styles
_Check_return_ HRESULT CListViewBaseItemChrome::SetForegroundBrush()
{
    CValue foregroundValue;

    if (m_visualStates.HasState(CommonStates2_Selected) ||
        m_visualStates.HasState(CommonStates2_PressedSelected) ||
        m_visualStates.HasState(CommonStates2_PointerOverSelected))
    {
        if (m_pSelectedForeground)
        {
            foregroundValue.WrapObjectNoRef(m_pSelectedForeground);
        }
    }
    else if (m_visualStates.HasState(CommonStates2_PointerOver) ||
             m_visualStates.HasState(CommonStates2_Pressed))
    {
        // PointerOverForeground is a Threshold property added to ListViewItemPresenter
        // GridViewItemPresenter does not have the PointerOverForeground property
        // Calling IsPropertyDefault on a property that does not exist will cause a crash
        if (!OfTypeByIndex<KnownTypeIndex::GridViewItemPresenter>())
        {
            // We want to explicitly set the brush in 1 of 2 cases
            // 1- Brush not NULL meaning it is set to some color
            // 2- Developer explicitly sets it to NULL (By default, if the developer does not set the property, it is NULL)
            if (m_pPointerOverForeground || !IsPropertyDefaultByIndex(KnownPropertyIndex::ListViewItemPresenter_PointerOverForeground))
            {
                foregroundValue.WrapObjectNoRef(m_pPointerOverForeground);
            }
        }
    }

    // if the value is not set, we clear the Brush value
    if (foregroundValue.IsUnset())
    {
        IFC_RETURN(ClearAnimatedValue(GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_Foreground), CValue::Empty()));

        if (!IsRoundedListViewBaseItemChromeEnabled() &&
            m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline &&
            m_multiSelectCheckBoxRectangle)
        {
            // set the CheckBox's brush
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, m_pCheckBoxBrush));

            // set the CheckMark Glyph's brush
            IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::IconElement_Foreground, m_pCheckBrush));
        }
    }
    else
    {
        IFC_RETURN(SetAnimatedValue(GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_Foreground), foregroundValue));

        // in the case of Selection or PointerOver, we want the CheckBox and the glyph to have the same color as the item's Foreground
        if (!IsRoundedListViewBaseItemChromeEnabled() &&
            m_checkMode == DirectUI::ListViewItemPresenterCheckMode::Inline &&
            m_multiSelectCheckBoxRectangle)
        {
            // set the CheckBox's brush
            IFC_RETURN(m_multiSelectCheckBoxRectangle->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, foregroundValue));

            // set the CheckMark Glyph's brush
            IFC_RETURN(m_multiSelectCheckGlyph->SetValueByKnownIndex(KnownPropertyIndex::IconElement_Foreground, foregroundValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue
)
{
    // Forward properties.
    switch (pdp->GetIndex())
    {
        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterHorizontalContentAlignment:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterHorizontalContentAlignment:
        {
            return GetValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment), pValue);
        }
        break;

        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterVerticalContentAlignment:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterVerticalContentAlignment:
        {
            return GetValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment), pValue);
        }
        break;

        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterPadding:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterPadding:
        {
            return GetValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_Padding), pValue);
        }
        break;
    }

    return(__super::GetValue(pdp, pValue));
}


_Check_return_ HRESULT CListViewBaseItemChrome::SetValue(_In_ const SetValueParams& args)
{
    // Forward properties
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterHorizontalContentAlignment:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterHorizontalContentAlignment:
        {
            return SetValue(
                SetValueParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment),
                args.m_value,
                args.m_baseValueSource,
                args.m_pValueOuterNoRef));

        }
        break;

        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterVerticalContentAlignment:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterVerticalContentAlignment:
        {
            return SetValue(
                SetValueParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment),
                args.m_value,
                args.m_baseValueSource,
                args.m_pValueOuterNoRef));
        }
        break;

        case KnownPropertyIndex::ListViewItemPresenter_ListViewItemPresenterPadding:
        case KnownPropertyIndex::GridViewItemPresenter_GridViewItemPresenterPadding:
        {
            return SetValue(
                SetValueParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_Padding),
                args.m_value,
                args.m_baseValueSource,
                args.m_pValueOuterNoRef));
        }
        break;
    }

    return(__super::SetValue(args));
}

// Handles the property changed for the new ListViewBaseItem style for Threshold
_Check_return_ HRESULT CListViewBaseItemChrome::OnPropertyChangedNewStyle(
    _In_ const PropertyChangedParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
            // Brushes used for both Threshold and Blue
        case KnownPropertyIndex::ListViewItemPresenter_SelectedForeground:
        case KnownPropertyIndex::GridViewItemPresenter_SelectedForeground:
        case KnownPropertyIndex::ListViewItemPresenter_PointerOverForeground:
            IFC_RETURN(SetForegroundBrush());
            break;

        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorVisualEnabled:
        case KnownPropertyIndex::ListViewItemPresenter_SelectionCheckMarkVisualEnabled:
        case KnownPropertyIndex::GridViewItemPresenter_SelectionCheckMarkVisualEnabled:
        case KnownPropertyIndex::ListViewItemPresenter_CheckHintBrush:
        case KnownPropertyIndex::GridViewItemPresenter_CheckHintBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckSelectingBrush:
        case KnownPropertyIndex::GridViewItemPresenter_CheckSelectingBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorMode:
            InvalidateRender();
            break;

        case KnownPropertyIndex::ListViewItemPresenter_CheckMode:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckPressedBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckDisabledBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxPressedBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxDisabledBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxCornerRadius:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedBackground:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedPointerOverBackground:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedPressedBackground:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedDisabledBackground:
        {
            // only update changes if the checkbox already exists
            if (m_multiSelectCheckBoxRectangle)
            {
                IFC_RETURN(SetMultiSelectCheckBoxProperties());
                InvalidateRender();
            }

            if (m_backplateRectangle &&
                (args.m_pDP->GetIndex() == KnownPropertyIndex::ListViewItemPresenter_SelectedBackground ||
                 args.m_pDP->GetIndex() == KnownPropertyIndex::ListViewItemPresenter_SelectedPointerOverBackground ||
                 args.m_pDP->GetIndex() == KnownPropertyIndex::ListViewItemPresenter_SelectedPressedBackground ||
                 args.m_pDP->GetIndex() == KnownPropertyIndex::ListViewItemPresenter_SelectedDisabledBackground))
            {
                IFC_RETURN(SetBackplateBackground());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_SelectedInnerBorderBrush:
        {
            if (m_innerSelectionBorder)
            {
                IFC_RETURN(SetInnerSelectionBorderBrush());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxPointerOverBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxPressedBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxDisabledBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedPointerOverBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedPressedBrush:
        case KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedDisabledBrush:
        {
            if (m_multiSelectCheckBoxRectangle)
            {
                IFC_RETURN(SetMultiSelectCheckBoxBackground());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_PointerOverBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedPointerOverBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedPressedBorderBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectedDisabledBorderBrush:
        {
            if (m_outerBorder)
            {
                IFC_RETURN(SetOuterBorderBrush());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_SelectedBorderThickness:
        {
            if (m_outerBorder)
            {
                IFC_RETURN(SetOuterBorderThickness());
                InvalidateRender();
            }
            if (m_innerSelectionBorder)
            {
                IFC_RETURN(SetInnerSelectionBorderThickness());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorPointerOverBrush:
        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorPressedBrush:
        {
            if (m_selectionIndicatorRectangle)
            {
                IFC_RETURN(SetSelectionIndicatorBackground());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorCornerRadius:
        {
            if (m_selectionIndicatorRectangle)
            {
                IFC_RETURN(SetSelectionIndicatorCornerRadius());
                InvalidateRender();
            }
            break;
        }

        case KnownPropertyIndex::ContentPresenter_CornerRadius:
        {
            if (IsRoundedListViewBaseItemChromeEnabled())
            {
                if (m_backplateRectangle)
                {
                    IFC_RETURN(SetBackplateCornerRadius());
                    InvalidateRender();
                }


                if (m_outerBorder)
                {
                    IFC_RETURN(SetOuterBorderCornerRadius());
                    InvalidateRender();
                }

                if (m_innerSelectionBorder)
                {
                    IFC_RETURN(SetInnerSelectionBorderCornerRadius());
                    InvalidateRender();
                }
            }
            break;
        }

        default:
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CListViewBaseItemChrome::DrawRevealBackground(
    _In_ IContentRenderer* pContentRenderer,
    _In_ XRECTF bounds)
{
    XRECTF placeholderBounds = bounds;

    // exclude the reveal border area
    CSizeUtil::Deflate(&placeholderBounds, GetRevealBorderThickness());

    IFC_RETURN(AddRectangle(
        pContentRenderer,
        placeholderBounds,
        GetRevealBackgroundBrushNoRef(),
        this
    ));

    return S_OK;
}


// all XCBBs are backed by a comp brush
// For performance reason, if it's reveal and in fallback mode, comp brush is set to null
// LVIP would detect this and not render it.
/*static*/
bool CListViewBaseItemChrome::IsNullCompositionBrush(_In_ CBrush* pBrush)
{
    bool nullCompositionBrush = false;
    if (pBrush != nullptr && KnownTypeIndex::XamlCompositionBrushBase == pBrush->GetTypeIndex())
    {
        CXamlCompositionBrush *pXamlCompositionBrush= static_cast<CXamlCompositionBrush*>(pBrush);
        nullCompositionBrush = !(pXamlCompositionBrush->HasCompositionBrush());
    }
    return nullCompositionBrush;
}

/*static*/
bool CListViewBaseItemChrome::IsOpaqueBrush(_In_ CBrush* brush)
{
    if (brush->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        CSolidColorBrush* solidColorBrush = static_cast<CSolidColorBrush*>(brush);

        return ColorUtils::IsOpaqueColor(solidColorBrush->m_rgb);
    }
    else if (brush->OfTypeByIndex<KnownTypeIndex::GradientBrush>())
    {
        CGradientBrush* gradientBrush = static_cast<CGradientBrush*>(brush);

        return gradientBrush->IsOpaque();
    }
    return false;
}

// Invoked when a TAEF test calls TestServices::Utilities::DeleteResourceDictionaryCaches().
/*static*/
void CListViewBaseItemChrome::ClearIsRoundedListViewBaseItemChromeEnabledCache()
{
    s_isRoundedListViewBaseItemChromeEnabled.reset();
}


/*static*/
bool CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeEnabled(_In_ CCoreServices* core)
{
    if (CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced())
    {
        return true;
    }

    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_listViewBaseItemRoundedChromeEnabled, L"ListViewBaseItemRoundedChromeEnabled");
    bool isRoundedListViewBaseItemChromeEnabled = false;

    IFCFAILFAST(CDependencyProperty::GetBooleanThemeResourceValue(core, c_listViewBaseItemRoundedChromeEnabled, &isRoundedListViewBaseItemChromeEnabled));

    return isRoundedListViewBaseItemChromeEnabled;
}

/*static*/
bool CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced()
{
    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DenyRoundedListViewBaseItemChrome))
    {
        return false;
    }

    return runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceRoundedListViewBaseItemChrome);
}

#ifdef DBG
// Used by ListViewItemPresenter::SetRoundedListViewBaseItemChromeFallbackColors.
/*static*/
bool CListViewBaseItemChrome::IsSelectionIndicatorVisualForced()
{
    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DenySelectionIndicatorVisualEnabled))
    {
        return false;
    }

    return runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceSelectionIndicatorVisualEnabled);
}
#endif // DBG
