// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <optional>

struct HWRenderParams;
struct HWElementRenderParams;
struct FocusRectangleOptions;
class CListViewBaseItemSecondaryChrome;
class IContentRenderer;

// CListViewBaseItemChrome knows how to render all layers of the chrome, including
// the layers present in the Grid/ListViewItem and in the secondary chrome.
// So we can let the chrome know which layer to render, we have this enum
// (in order of lowest layer to highest layer).
enum ListViewBaseItemChromeLayerPosition
{
    ListViewBaseItemChromeLayerPosition_Base_Pre,
    ListViewBaseItemChromeLayerPosition_PrimaryChrome_Pre,
    ListViewBaseItemChromeLayerPosition_SecondaryChrome_Pre,
    ListViewBaseItemChromeLayerPosition_SecondaryChrome_Post,
    ListViewBaseItemChromeLayerPosition_PrimaryChrome_Post,
    ListViewBaseItemChromeLayerPosition_Base_Post,
};

// Animation command visitor pattern. Each command knows how to tell a visitor
// how to process it. Each concrete command class represents one particular animation
// (not necessarily specific to one VisualStateGroup).
class ListViewBaseItemAnimationCommand_Pressed;
class ListViewBaseItemAnimationCommand_ReorderHint;
class ListViewBaseItemAnimationCommand_DragDrop;
class ListViewBaseItemAnimationCommand_MultiSelect;
class ListViewBaseItemAnimationCommand_IndicatorSelect;
class ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility;

class ListViewBaseItemAnimationCommandVisitor
{
public:
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_Pressed* pCommand) = 0;
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_ReorderHint* pCommand) = 0;
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_DragDrop* pCommand) = 0;
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_MultiSelect* pCommand) = 0;
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_IndicatorSelect* pCommand) = 0;
    virtual _Check_return_ HRESULT VisitAnimationCommand(ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility* pCommand) = 0;
};

class ListViewBaseItemAnimationCommand
{
protected:
    ListViewBaseItemAnimationCommand(
        _In_ bool isStarting,
        _In_ bool steadyStateOnly);

public:
    virtual ~ListViewBaseItemAnimationCommand();
    virtual _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) = 0;
    virtual _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) = 0;
    virtual XINT32 GetPriority() const = 0;

    bool m_isStarting;
    bool m_steadyStateOnly;
};

class CListViewBaseItemChrome : public CContentPresenter
{
private:
    // Enums and helpers specifying the visual state groups. Each group has an appropriately-typed
    // FromString overload that parses a string state name.

    enum CommonStates : uint8_t
    {
        CommonStates_Normal,
        CommonStates_PointerOver,
        CommonStates_Pressed,
        CommonStates_PointerOverPressed,
        CommonStates_Disabled
    };

    enum FocusStates : uint8_t
    {
        FocusStates_Focused,
        FocusStates_Unfocused,
        FocusStates_PointerFocused
    };

    enum SelectionHintStates : uint8_t
    {
        SelectionHintStates_HorizontalSelectionHint,
        SelectionHintStates_VerticalSelectionHint,
        SelectionHintStates_NoSelectionHint
    };

    enum SelectionStates : uint8_t
    {
        SelectionStates_Unselecting,
        SelectionStates_Unselected,
        SelectionStates_UnselectedPointerOver,
        SelectionStates_UnselectedSwiping,
        SelectionStates_Selecting,
        SelectionStates_Selected,
        SelectionStates_SelectedSwiping,
        SelectionStates_SelectedUnfocused
    };

    enum DragStates : uint8_t
    {
        DragStates_NotDragging,
        DragStates_Dragging,
        DragStates_DraggingTarget,
        DragStates_MultipleDraggingPrimary,
        DragStates_MultipleDraggingSecondary,
        DragStates_DraggedPlaceholder,
        DragStates_Reordering,
        DragStates_ReorderingTarget,
        DragStates_MultipleReorderingPrimary,
        DragStates_ReorderedPlaceholder,
        DragStates_DragOver
    };

    enum ReorderHintStates : uint8_t
    {
        ReorderHintStates_NoReorderHint,
        ReorderHintStates_BottomReorderHint,
        ReorderHintStates_TopReorderHint,
        ReorderHintStates_RightReorderHint,
        ReorderHintStates_LeftReorderHint
    };

    enum DataVirtualizationStates : uint8_t
    {
        DataVirtualizationStates_DataAvailable,
        DataVirtualizationStates_DataPlaceholder
    };

    // Visual states for new ListViewBaseItem styles
    enum CommonStates2 : uint8_t
    {
        CommonStates2_Normal,
        CommonStates2_PointerOver,
        CommonStates2_Pressed,
        CommonStates2_Selected,
        CommonStates2_PointerOverSelected,
        CommonStates2_PressedSelected
    };

    enum DisabledStates : uint8_t
    {
        DisabledStates_Enabled,
        DisabledStates_Disabled
    };

    // enum FocusStates (using the same one as above)

    enum MultiSelectStates : uint8_t
    {
        MultiSelectStates_MultiSelectDisabled,
        MultiSelectStates_MultiSelectEnabled,
    };

    enum SelectionIndicatorStates : uint8_t
    {
        SelectionIndicatorStates_SelectionIndicatorDisabled,
        SelectionIndicatorStates_SelectionIndicatorEnabled,
    };

    // A struct holding and analyzing the current set of VisualStates active in the chrome.
    struct VisualStates
    {
        CommonStates                commonState;
        FocusStates                 focusState;
        SelectionHintStates         selectionHintState;
        SelectionStates             selectionState;
        DragStates                  dragState;
        ReorderHintStates           reorderHintState;
        DataVirtualizationStates    dataVirtualizationState;

        CommonStates2               commonState2;
        DisabledStates              disabledState;
        MultiSelectStates           multiSelectState;
        SelectionIndicatorStates    selectionIndicatorState;

        // These methods return TRUE if the set of active VisualStates contains
        // the given state.
        bool HasState(CommonStates state)              { return commonState == state; }
        bool HasState(FocusStates state)               { return focusState == state; }
        bool HasState(SelectionHintStates state)       { return selectionHintState == state; }
        bool HasState(SelectionStates state)           { return selectionState == state; }
        bool HasState(DragStates state)                { return dragState == state; }
        bool HasState(ReorderHintStates state)         { return reorderHintState == state; }
        bool HasState(DataVirtualizationStates state)  { return dataVirtualizationState == state; }

        bool HasState(CommonStates2 state)             { return commonState2 == state; }
        bool HasState(DisabledStates state)            { return disabledState == state; }
        bool HasState(MultiSelectStates state)         { return multiSelectState == state; }
        bool HasState(SelectionIndicatorStates state)  { return selectionIndicatorState == state; }
    };

    template<typename EnumType>
    struct MapItem
    {
        const WCHAR* m_pName;
        EnumType m_pEnumValue;
    };

    template<typename EnumType>
    struct Mapping
    {
        typedef const MapItem<EnumType> MapItemType;
        static MapItemType s_map[];
    };

    // Given a string state name and a pointer to a visual state:
    // - tries to parse the string as the appropriate VisualState type.
    // - If parsing successful, updates the value at the pointer.
    // - If the value at the pointer was updated, return TRUE. FALSE otherwise.
    template <typename VisualStateType>
    static bool UpdateVisualStateGroup (
        _In_z_ const WCHAR* pName,
        _Inout_ VisualStateType* pState,
        _Out_ bool* pStateFound);

    static const XSIZEF      s_selectionCheckMarkVisualSize;
    static const XTHICKNESS  s_focusBorderThickness;
    static const XPOINTF     s_checkmarkOffset;
    static const XPOINTF     s_swipeHintOffset;
    static const XFLOAT      s_swipingCheckSteadyStateOpacity;
    static const XFLOAT      s_selectingSwipingCheckSteadyStateOpacity;
    static const XFLOAT      s_generalCornerRadius;
    static const XFLOAT      s_innerBorderCornerRadius;
    static const XSIZEF      s_selectionIndicatorSize;
    static const XFLOAT      s_selectionIndicatorHeightShrinkage;
    static const XTHICKNESS  s_selectionIndicatorMargin;
    static const XSIZEF      s_multiSelectSquareSize;
    static const XTHICKNESS  s_multiSelectSquareThickness;
    static const XTHICKNESS  s_multiSelectRoundedSquareThickness;
    static const XTHICKNESS  s_multiSelectSquareInlineMargin;
    static const XTHICKNESS  s_multiSelectRoundedSquareInlineMargin;
    static const XTHICKNESS  s_multiSelectSquareOverlayMargin;
    static const XTHICKNESS  s_backplateMargin;
    static const XTHICKNESS  s_borderThickness;
    static const XTHICKNESS  s_innerSelectionBorderThickness;
    static const XFLOAT      s_listViewItemMultiSelectContentOffset;
    static const XFLOAT      s_multiSelectRoundedContentOffset;
    static const XPOINTF     s_checkMarkPoints[6];
    static const XFLOAT      s_listViewItemFocusBorderThickness;
    static const XFLOAT      s_gridViewItemFocusBorderThickness;
    static const XFLOAT      s_checkMarkGlyphFontSize;

public:
    // Used by CDependencyProperty::GetDefaultValue
    static constexpr XCORNERRADIUS s_defaultSelectionIndicatorCornerRadius = { 1.5f, 1.5f, 1.5f, 1.5f };
    static constexpr XCORNERRADIUS s_defaultCheckBoxCornerRadius = { 3.0f, 3.0f, 3.0f, 3.0f };
    static constexpr XTHICKNESS s_selectedBorderThicknessRounded = { 2.0f, 2.0f, 2.0f, 2.0f };
    static constexpr XTHICKNESS s_selectedBorderThickness = { 0.0f, 0.0f, 0.0f, 0.0f };

private:
    // Raise exception if no appropriate parent is set.
    _Check_return_ HRESULT SetWrongParentError();

    // Retrieve parent item or raise exception if no parent is set.
    _Check_return_ HRESULT GetParentListViewBaseItemNoRef(
        _Outptr_ CContentControl** ppParentListViewBaseItem);

    CListViewBase* GetParentListViewBase(_In_opt_ CContentControl* listViewBaseItem) const;

private:
    CContentControl*                    m_pParentListViewBaseItemNoRef{ nullptr };
    CListViewBaseItemSecondaryChrome*   m_pSecondaryChrome{ nullptr };

    // Checkmark geometry and bounds.
    // TODO make this shared b/w chromes. Might only be able to
    // share the actual path data inside this, as brushes may change, etc.
    // The check is "owned" by the Chrome (meaning the chrome holds dirtyness and render cache).
    CGeometry*                          m_pCheckGeometryData{ nullptr };

    // TextBlock we create on-the-fly to display our DragItemsCount.
    // TODO: Integrate text rendering into the chrome.
    // This isn't a trivial task.
    CTextBlock*                         m_pDragItemsCountTextBlock{ nullptr };

    xref_ptr<CBorder>                   m_multiSelectCheckBoxRectangle;
    xref_ptr<CRectangleGeometry>        m_multiSelectCheckBoxClip;
    xref_ptr<CFontIcon>                 m_multiSelectCheckGlyph;

    xref_ptr<CBorder>                   m_backplateRectangle;
    xref_ptr<CBorder>                   m_selectionIndicatorRectangle;
    xref_ptr<CBorder>                   m_innerSelectionBorder;
    xref_ptr<CBorder>                   m_outerBorder;

    // This brush is analogous to m_pCurrentEarmarkBrush, except we use it for the checkmark rendered by us.
    // It holds a ref.
    xref_ptr<CBrush>                    m_currentCheckBrush;

    // The list of our animation commands, waiting for processing in dxaml.
    xvector<ListViewBaseItemAnimationCommand*> m_animationCommands;

    ListViewBaseItemAnimationCommand*   m_pCurrentHighestPriorityCommand{ nullptr };

    // Opacity of our swipe hint check mark.
    XFLOAT                              m_swipeHintCheckOpacity{ s_cOpacityUnset };

    // Tracking for the highest priority command. Only animations with >= priority
    // will be processed (note that this does not mean low-pri visual states aren't
    // processed or displayed - it just means they won't be animated).
    // We don't own m_pCurrentHighestPriorityCommand but we get notified when the
    // reference is no longer valid.
    XINT32                              m_currentHighestCommandPriority{ XINT32_MAX };
    XRECTF_RB                           m_checkGeometryBounds;

    // Used for brush transitions. Saves the last brush that we used to render the background. If this brush changes, then
    // we kick off a brush transition (if the API enabled it).
    // Note: wrl::ComPtr expects Release to return an unsigned long, but CDependencyObject uses void. Use xref_ptr instead.
    xref_ptr<CBrush>                    m_previousBackgroundBrush;

    VisualStates                        m_visualStates;

    bool                                m_isFocusVisualDrawnByFocusManager : 1;

    // Path rendering fields.
    bool                                m_fFillBrushDirty : 1;
    bool                                m_shouldRenderChrome : 1;
    bool                                m_isInIndicatorSelect : 1;  // true when states are in MultiSelectStates_MultiSelectDisabled + SelectionIndicatorStates_SelectionIndicatorEnabled
    bool                                m_isInMultiSelect : 1;      // true when states are in MultiSelectStates_MultiSelectEnabled  + SelectionIndicatorStates_SelectionIndicatorDisabled

    static std::optional<bool>          s_isRoundedListViewBaseItemChromeEnabled;

public:
    // This brush is exposed so the secondary chrome can use it while rendering its earmark geometry.
    // It holds a ref.
    CBrush* m_pCurrentEarmarkBrush{ nullptr };
    CBrush* m_pCheckHintBrush{ nullptr };
    CBrush* m_pCheckSelectingBrush{ nullptr };
    CBrush* m_pCheckBrush{ nullptr };
    CBrush* m_pCheckPressedBrush{ nullptr };
    CBrush* m_pCheckDisabledBrush{ nullptr };
    CBrush* m_pDragBackground{ nullptr };
    CBrush* m_pDragForeground{ nullptr };
    CBrush* m_pFocusBorderBrush{ nullptr };
    CBrush* m_pPlaceholderBackground{ nullptr };
    CBrush* m_pPointerOverBorderBrush{ nullptr };
    CBrush* m_pPointerOverBackground{ nullptr };
    CBrush* m_pPointerOverForeground{ nullptr };
    CBrush* m_pSelectedBackground{ nullptr };
    CBrush* m_pSelectedForeground{ nullptr };
    CBrush* m_pSelectedPointerOverBackground{ nullptr };
    CBrush* m_pSelectedPointerOverBorderBrush{ nullptr };
    CBrush* m_pSelectedBorderBrush{ nullptr };
    CBrush* m_pSelectedPressedBorderBrush{ nullptr };
    CBrush* m_pSelectedDisabledBorderBrush{ nullptr };
    CBrush* m_pSelectedInnerBorderBrush{ nullptr };
    CBrush* m_pSelectedPressedBackground{ nullptr };
    CBrush* m_pSelectedDisabledBackground{ nullptr };
    CBrush* m_pPressedBackground{ nullptr };
    CBrush* m_pCheckBoxBrush{ nullptr };
    CBrush* m_pCheckBoxPointerOverBrush{ nullptr };
    CBrush* m_pCheckBoxPressedBrush{ nullptr };
    CBrush* m_pCheckBoxDisabledBrush{ nullptr };
    CBrush* m_pCheckBoxSelectedBrush{ nullptr };
    CBrush* m_pCheckBoxSelectedPointerOverBrush{ nullptr };
    CBrush* m_pCheckBoxSelectedPressedBrush{ nullptr };
    CBrush* m_pCheckBoxSelectedDisabledBrush{ nullptr };
    CBrush* m_pCheckBoxBorderBrush{ nullptr };
    CBrush* m_pCheckBoxPointerOverBorderBrush{ nullptr };
    CBrush* m_pCheckBoxPressedBorderBrush{ nullptr };
    CBrush* m_pCheckBoxDisabledBorderBrush{ nullptr };
    CBrush* m_pSelectionIndicatorBrush{ nullptr };
    CBrush* m_pSelectionIndicatorPointerOverBrush{ nullptr };
    CBrush* m_pSelectionIndicatorPressedBrush{ nullptr };
    CBrush* m_pSelectionIndicatorDisabledBrush{ nullptr };
    CBrush* m_pFocusSecondaryBorderBrush{ nullptr };

    XTHICKNESS m_contentMargin{};
    DirectUI::ListViewItemPresenterCheckMode m_checkMode{ DirectUI::ListViewItemPresenterCheckMode::Inline };

    static const XFLOAT s_cOpacityUnset;

protected:
    CListViewBaseItemChrome(
        _In_ CCoreServices *pCore);

    ~CListViewBaseItemChrome() override;

    _Check_return_ HRESULT MeasureOverride(
        _In_ XSIZEF availableSize,
        _Out_ XSIZEF& desiredSize
        ) override;

    _Check_return_ HRESULT ArrangeOverride(
        _In_ XSIZEF finalSize,
        _Out_ XSIZEF& newFinalSize
        ) override;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

public:
    _Check_return_ HRESULT HitTestLocalInternalPostChildren(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        );

    _Check_return_ HRESULT HitTestLocalInternalPostChildren(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        );

protected:
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

public:
    DECLARE_CREATE(CListViewBaseItemChrome);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CListViewBaseItemChrome>::Index;
    }

    // Lets us know we have a parent ListViewBaseItem. We don't take a ref.
    _Check_return_ HRESULT SetChromedListViewBaseItem(
        _In_ CUIElement* pParent);

    // Main entry point to change visual state.
    _Check_return_ HRESULT GoToChromedState(
        _In_z_ const WCHAR *pStateName,
        _In_ bool useTransitions,
        _Out_ bool* pWentToState);

    // Sets the drag count display.
    _Check_return_ HRESULT SetDragItemsCount(
        _In_ XUINT32 dragItemsCount);

    _Check_return_ HRESULT SetSwipeHintCheckOpacity(_In_ XFLOAT opacity);

    // Removes the multi-select checkbox from the tree.
    _Check_return_ HRESULT RemoveMultiSelectCheckBox();

    // Removes the selection indicator from the tree.
    _Check_return_ HRESULT RemoveSelectionIndicator();

    // Removes the inner selection border from the tree.
    _Check_return_ HRESULT RemoveInnerSelectionBorder();

    // Removes the outer border from the tree.
    _Check_return_ HRESULT RemoveOuterBorder();

    // Main rendering code: render the given layer.
    _Check_return_ HRESULT RenderLayer(
        _In_ IContentRenderer* pContentRenderer,
        _In_ ListViewBaseItemChromeLayerPosition layer
        );

    // Interface to ListViewBaseItem

    // Obtains the next animation command to execute, or NULL if none exists. Note that the ref
    // to the returned item is still controlled by the chrome - see UnlockLayersForAnimationAndDisposeCommand.
    _Check_return_ HRESULT GetNextPendingAnimation(
        _Outptr_ ListViewBaseItemAnimationCommand** ppAnimationCommand);

    // Tells us to assign the layer positions of the various chrome visuals to match the requirements of the given
    // command. Returns TRUE if the layers could be rearranged (meaning the animation can proceed), FALSE otherwise.
    _Check_return_ HRESULT LockLayersForAnimation(
        _In_ ListViewBaseItemAnimationCommand* pCommand,
        _Out_ bool* pShouldProceed);

    // Unlocks the layers for the given command, and releases the command (sets it to NULL).
    void UnlockLayersForAnimationAndDisposeCommand(
        _In_opt_ ListViewBaseItemAnimationCommand* command);

    // Interface to renderer.

    static void NWSetContentDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    void NWCleanDirtyFlags() override
    {
        m_fFillBrushDirty = false;
        __super::NWCleanDirtyFlags();
    }

    // Overrides to default templating behavior. In normal operation here, we can have non-templated
    // children in us (the secondary chrome and the reorder count text block, namely).
    bool HasTemplateChild() final;
    CUIElement* GetTemplateChildNoRef() override;
    _Check_return_ HRESULT AddTemplateChild(_In_ CUIElement* pUI) override;
    _Check_return_ HRESULT RemoveTemplateChild() override;

    CUIElement* GetTemplateChildIfExists();

    // Rect helpers
    bool ShouldUseLayoutRounding();

    XRECTF LayoutRoundHelper(
        _In_ const XRECTF& bounds);

    void LayoutRoundHelper(
        _Inout_ XSIZEF* pSize);

    void LayoutRoundHelper(
        _Inout_ XTHICKNESS* pThickness);

    // SegmentCollection helper.
    static _Check_return_ HRESULT AddLineSegmentToSegmentCollection(_In_ CCoreServices *pCore, _In_ CPathSegmentCollection *pSegments, _In_ XPOINTF point);

    // Provides the geometry of the CheckMark, so the renderer knows how to size the brush.
    _Check_return_ HRESULT GetCheckMarkBounds(_Out_ XRECTF_RB* pBounds);

    // Flag which determines if chrome gets rendered.
    void SetShouldRenderChrome(
        _In_ bool shouldRenderChrome)
    {
        if (m_shouldRenderChrome != shouldRenderChrome)
        {
            m_shouldRenderChrome = shouldRenderChrome;
            InvalidateRender();
        }
    }
    void InvalidateRender();

    static _Check_return_ HRESULT AddRectangle(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds,
        _In_ CBrush* pBrush,
        _In_opt_ CUIElement *pUIElement);

    static _Check_return_ HRESULT AddBorder(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds,
        _In_ const XTHICKNESS& thickness,
        _In_ CBrush* pBrush,
        _In_opt_ CListViewBaseItemChrome* listViewItemChrome);

    _Check_return_ HRESULT AddChromeAssociatedPath(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds,
        _In_ const CMILMatrix& transform,
        _In_ CBrush* pBrush,
        _In_ XFLOAT opacityMultiplier,
        _In_ bool primaryChrome);

    _Check_return_ HRESULT GetValue(
        _In_  const CDependencyProperty *pdp,
        _Out_ CValue              *pValue
    ) final;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    _Check_return_ HRESULT CustomizeFocusRectangle(_Inout_ FocusRectangleOptions& options, _Out_ bool* shouldDrawFocusRect);

    _Check_return_ HRESULT  ShouldDrawDottedLinesFocusVisual(_Out_ bool* shouldDrawDottedLines);

    XFLOAT GetDragOpacity() const;

    static XFLOAT GetDefaultSelectionIndicatorCornerRadius()
    {
        // Default corner radius of the selection indicator visual.
        // Used when the SelectionIndicatorCornerRadius property returns 0 and IsRoundedListViewBaseItemChromeForced() returns True.
        return 1.5f;
    }

    static XFLOAT GetDefaultCheckBoxCornerRadius()
    {
        // Default corner radius of the checkbox visual.
        // Used when the CheckBoxCornerRadius property returns 0 and IsRoundedListViewBaseItemChromeForced() returns True.
        return 3.0f;
    }

    static XFLOAT GetDefaultDisabledOpacity(bool forRoundedListViewBaseItemChrome)
    {
        return forRoundedListViewBaseItemChrome ? 0.3f : 0.55f;
    }

    static XFLOAT GetDefaultDragOpacity()
    {
        return 0.8f;
    }

    static XFLOAT GetDefaultListViewItemReorderHintOffset()
    {
        return 10.0f;
    }

    static XFLOAT GetDefaultGridViewItemReorderHintOffset()
    {
        return 16.0f;
    }

    static XFLOAT GetSelectedBorderThickness(bool forRoundedListViewBaseItemChrome)
    {
        return forRoundedListViewBaseItemChrome ? 2.0f : 0.0f;
    }

    static bool GetDefaultSelectionCheckMarkVisualEnabled()
    {
        return true;
    }

    static void ClearIsRoundedListViewBaseItemChromeEnabledCache();
    static bool IsRoundedListViewBaseItemChromeEnabled(_In_ CCoreServices* core);
    static bool IsRoundedListViewBaseItemChromeForced();

    static const XTHICKNESS* GetSelectedBorderXThickness(bool forRoundedListViewBaseItemChrome)
    {
        return forRoundedListViewBaseItemChrome ? &CListViewBaseItemChrome::s_selectedBorderThicknessRounded : &CListViewBaseItemChrome::s_selectedBorderThickness;
    }

#ifdef DBG
    static bool IsSelectionIndicatorVisualForced();
#endif // DBG

private:
    // Returns TRUE if the base layer graphics should be drawn at the given layer.
    bool ShouldDrawBaseLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer);

    // Returns TRUE if the under-content layer graphics should be drawn at the given layer.
    bool ShouldDrawUnderContentLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer);

    // Returns TRUE if the over-content layer graphics should be drawn at the given layer.
    bool ShouldDrawOverContentLayerHere(_In_ ListViewBaseItemChromeLayerPosition layer);

    // Draws the base layer (swipe hint).
    _Check_return_ HRESULT DrawBaseLayer(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds);

    // Draws the below-content layer (selection visuals, etc).
    _Check_return_ HRESULT DrawUnderContentLayer(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds);

    // Draws the above-content layer (swipe hint check, etc).
    _Check_return_ HRESULT DrawOverContentLayer(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds);

    // Draws the above-content drag overlay.
    _Check_return_ HRESULT DrawDragOverlayLayer(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds);

    // Sets up the secondary chrome and adds it to the tree.
    _Check_return_ HRESULT AddSecondaryChrome();

    // Sets whether or not the drag overlay text block is shown.
    _Check_return_ HRESULT SetDragOverlayTextBlockVisible(_In_ bool isVisible);

    // Sets up and adds the drag overlay text block to the tree.
    _Check_return_ HRESULT EnsureDragOverlayTextBlock();

    // Sets up properties of drag count TextBlock depending on whether the app is TH2-, or, RS2+
    _Check_return_ HRESULT SetDragOverlayTextBlockProperties();

    // Sets up and adds the multi-select checkbox to the tree.
    _Check_return_ HRESULT EnsureMultiSelectCheckBox();

    // Sets up and adds the selection indicator to the tree.
    _Check_return_ HRESULT EnsureSelectionIndicator();

    // Sets up and adds the backplate to the tree.
    _Check_return_ HRESULT EnsureBackplate();

    // Sets up and adds the inner selection border to the tree.
    _Check_return_ HRESULT EnsureInnerSelectionBorder();

    // Sets up and adds the outer border to the tree.
    _Check_return_ HRESULT EnsureOuterBorder();

    // Sets up the multi-select checkbox to the tree. (colors, alignment, clip)
    _Check_return_ HRESULT SetMultiSelectCheckBoxProperties();

    // Sets up the inner selection border visuals variable properties.
    _Check_return_ HRESULT SetInnerSelectionBorderProperties();

    // Sets up the outer border visuals variable properties.
    _Check_return_ HRESULT SetOuterBorderProperties();

    // Sets the backplate Background brush based on current visual state.
    _Check_return_ HRESULT SetBackplateBackground();

    // Sets up the backplate CornerRadius.
    _Check_return_ HRESULT SetBackplateCornerRadius();

    // Sets up the backplate Margin for GridViewItem.
    _Check_return_ HRESULT SetBackplateMargin();

    // Sets the inner selection border's BorderBrush based on current SelectionBorderBrush property.
    _Check_return_ HRESULT SetInnerSelectionBorderBrush();

    // Sets the inner selection border's CornerRadius
    _Check_return_ HRESULT SetInnerSelectionBorderCornerRadius();

    // Sets the inner selection border's BorderThickness based on current SelectedBorderThickness property.
    _Check_return_ HRESULT SetInnerSelectionBorderThickness();

    // Sets the outer border's BorderBrush based on current visual state.
    _Check_return_ HRESULT SetOuterBorderBrush();

    // Sets the outer border's CornerRadius.
    _Check_return_ HRESULT SetOuterBorderCornerRadius();

    // Sets the outer border's BorderThickness based on current visual state.
    _Check_return_ HRESULT SetOuterBorderThickness();

    // Sets the multi-select checkbox background brush.
    _Check_return_ HRESULT SetMultiSelectCheckBoxBackground();

    // Sets the multi-select checkbox border brush and thickness.
    _Check_return_ HRESULT SetMultiSelectCheckBoxBorder();

    // Sets the multi-select checkbox glyph's foreground brush.
    _Check_return_ HRESULT SetMultiSelectCheckBoxForeground();

    // Sets the selection indicator background brush.
    _Check_return_ HRESULT SetSelectionIndicatorBackground();

    // Sets the selection indicator corner radius.
    _Check_return_ HRESULT SetSelectionIndicatorCornerRadius();

    // Builds the checkmark path.
    _Check_return_ HRESULT PrepareCheckPath();

    // Reparents the inner selection border if needed and updates its affected properties.
    _Check_return_ HRESULT UpdateBordersParenting();

    // If necessary, creates all the TransitionTargets necessary for our animation targets.
    _Check_return_ HRESULT EnsureTransitionTarget();

    bool IsRoundedListViewBaseItemChromeEnabled() const;
    bool IsChromeForGridViewItem() const;
    bool IsChromeForListViewItem() const;
    bool IsSelectionIndicatorVisualEnabled() const;
    bool IsInSelectionIndicatorMode() const;
    bool IsOuterBorderBrushOpaque() const;

    _Check_return_ HRESULT SetGeneralCornerRadius(xref_ptr<CBorder> border); // Sets the border CornerRadius based on value returned by GetGeneralCornerRadius.
    XCORNERRADIUS GetGeneralCornerRadius() const;
    XCORNERRADIUS GetCheckBoxCornerRadius() const;
    XCORNERRADIUS GetSelectionIndicatorCornerRadius() const;
    XFLOAT GetSelectionIndicatorHeightFromAvailableHeight(XFLOAT availableHeight) const;
    DirectUI::ListViewItemPresenterSelectionIndicatorMode GetSelectionIndicatorMode() const;
    CBrush* GetRevealBackgroundBrushNoRef() const;
    CBrush* GetRevealBorderBrushNoRef() const;
    bool GetRevealBackgroundShowsAboveContent() const;
    bool GetSelectionCheckMarkVisualEnabled() const;
    XTHICKNESS GetRevealBorderThickness() const;
    XTHICKNESS GetSelectedBorderThickness() const;
    XTHICKNESS GetPointerOverBackgroundMargin() const;
    XFLOAT GetDisabledOpacity() const;
    XFLOAT GetReorderHintOffset() const;

    // Given chrome bounds, updates transformation matrix for checkmark taking into consideration layout rounding and RTL.
    void AppendCheckmarkTransform(
        _In_ XRECTF chromeBounds,
        _Inout_ CMILMatrix* pMatrix);

    // Given chrome bounds, updates transformation matrix for earmark.
    void AppendEarmarkTransform(
        _In_ XRECTF chromeBounds,
        _Inout_ CMILMatrix* pMatrix);

    // Animation command helpers

    // Enqueue the given command. It will be presented in FIFO order by DequeueAnimationCommand.
    // This function takes control of the command's lifetime. To enforce this, it will clear the
    // given pointer when called (no exceptions).
    _Check_return_ HRESULT EnqueueAnimationCommand(_In_ std::unique_ptr<ListViewBaseItemAnimationCommand>& command);

    // Dequeues an animation command. You now own the ref (note that GetNextPendingAnimation, a caller
    // to this function, takes logical ownership itself).
    _Check_return_ HRESULT DequeueAnimationCommand(_Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& command);

    // Given a reorder hint state, figure out where the hint animation should animate to.
    _Check_return_ HRESULT ComputeReorderHintOffset(_In_ ReorderHintStates state, _Out_ XPOINTF* pOffset);

    // Given a swipe hint state, figure out where the hint animation should animate to.
    _Check_return_ HRESULT ComputeSwipeHintOffset(_In_ SelectionHintStates state, _Out_ XPOINTF* pOffset);

    _Check_return_ HRESULT MeasureNewStyle(
        _In_ XSIZEF availableSize,
        _Out_ XSIZEF& desiredSize);

    _Check_return_ HRESULT ArrangeNewStyle(
        _In_ XSIZEF finalSize,
        _Out_ XSIZEF& newFinalSize);

    _Check_return_ HRESULT ArrangeTemplateChild(
        _In_ XRECTF& controlBorderBounds);

    _Check_return_ HRESULT GoToChromedStateNewStyle(
        _In_z_ const WCHAR *pStateName,
        _In_ bool useTransitions,
        _Out_ bool* pWentToState);

    // Draws the below-content layer
    _Check_return_ HRESULT DrawUnderContentLayerNewStyle(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds
        );

    // Draws the above-content layer
    _Check_return_ HRESULT DrawOverContentLayerNewStyle(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds
        );

    // Sets/clears content's foreground brush in Common visual states.
    _Check_return_ HRESULT SetForegroundBrush();

    // Handles the property changed for the new ListViewBaseItem style for Threshold
    _Check_return_ HRESULT OnPropertyChangedNewStyle(
        _In_ const PropertyChangedParams& args);

    _Check_return_ HRESULT DrawRevealBackground(
        _In_ IContentRenderer* pContentRenderer,
        _In_ XRECTF bounds);

    static bool IsNullCompositionBrush(_In_ CBrush* pBrush);
    static bool IsOpaqueBrush(_In_ CBrush* brush);
};

class ListViewBaseItemAnimationCommand_Pressed : public ListViewBaseItemAnimationCommand
{
public:
    ListViewBaseItemAnimationCommand_Pressed(
        _In_ bool pressed,
        _In_ xref::weakref_ptr<CUIElement> pAnimationTarget,
        _In_ bool isStarting,
        _In_ bool steadyStateOnly);
    ~ListViewBaseItemAnimationCommand_Pressed() override;
    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    bool m_pressed;
    xref::weakref_ptr<CUIElement> m_pAnimationTarget;
};

class ListViewBaseItemAnimationCommand_ReorderHint : public ListViewBaseItemAnimationCommand
{
public:
    ListViewBaseItemAnimationCommand_ReorderHint(
        _In_ XFLOAT offsetX,
        _In_ XFLOAT offsetY,
        _In_ xref::weakref_ptr<CListViewBaseItemChrome> pAnimationTarget,
        _In_ bool isStarting,
        _In_ bool steadyStateOnly);
    ~ListViewBaseItemAnimationCommand_ReorderHint() override;
    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    XFLOAT m_offsetX;
    XFLOAT m_offsetY;
    xref::weakref_ptr<CListViewBaseItemChrome> m_pAnimationTarget;
};

class ListViewBaseItemAnimationCommand_DragDrop : public ListViewBaseItemAnimationCommand
{
public:
    enum DragDropState
    {
        DragDropState_SinglePrimary,
        DragDropState_MultiPrimary,
        DragDropState_MultiSecondary,
        DragDropState_DraggedPlaceholder,
        DragDropState_Target,
        DragDropState_ReorderingSinglePrimary,
        DragDropState_ReorderingMultiPrimary,
        DragDropState_ReorderedPlaceholder,
        DragDropState_ReorderingTarget,
        DragDropState_DragOver
    };

    ListViewBaseItemAnimationCommand_DragDrop(
        _In_ DragDropState state,
        _In_ xref::weakref_ptr<CListViewBaseItemChrome> pBaseAnimationTarget,
        _In_ xref::weakref_ptr<CFrameworkElement> pFadeOutAnimationTarget,
        _In_ bool isStarting,
        _In_ bool steadyStateOnly);
    ~ListViewBaseItemAnimationCommand_DragDrop() override;
    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    DragDropState m_state;
    xref::weakref_ptr<CListViewBaseItemChrome> m_pBaseAnimationTarget;
    xref::weakref_ptr<CFrameworkElement> m_pFadeOutAnimationTarget;
};

class ListViewBaseItemAnimationCommand_MultiSelect : public ListViewBaseItemAnimationCommand
{
public:
    ListViewBaseItemAnimationCommand_MultiSelect(
        bool isRoundedListViewBaseItemChromeEnabled,
        bool entering,
        double checkBoxTranslationX,
        double contentTranslationX,
        DirectUI::ListViewItemPresenterCheckMode checkMode,
        xref::weakref_ptr<CUIElement> multiSelectCheckBox,
        xref::weakref_ptr<CUIElement> contentPresenter,
        bool isStarting,
        bool steadyStateOnly);

    ~ListViewBaseItemAnimationCommand_MultiSelect() override;
    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    bool m_isRoundedListViewBaseItemChromeEnabled;
    bool m_entering;
    double m_checkBoxTranslationX;
    double m_contentTranslationX;
    DirectUI::ListViewItemPresenterCheckMode m_checkMode;
    xref::weakref_ptr<CUIElement> m_multiSelectCheckBox;
    xref::weakref_ptr<CUIElement> m_contentPresenter;
};

class ListViewBaseItemAnimationCommand_IndicatorSelect : public ListViewBaseItemAnimationCommand
{
public:
    ListViewBaseItemAnimationCommand_IndicatorSelect(
        bool entering,
        double translationX,
        DirectUI::ListViewItemPresenterSelectionIndicatorMode selectionIndicatorMode,
        xref::weakref_ptr<CUIElement> selectionIndicator,
        xref::weakref_ptr<CUIElement> contentPresenter,
        bool isStarting,
        bool steadyStateOnly);

    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    bool m_entering;
    double m_translationX;
    DirectUI::ListViewItemPresenterSelectionIndicatorMode m_selectionIndicatorMode;
    xref::weakref_ptr<CUIElement> m_selectionIndicator;
    xref::weakref_ptr<CUIElement> m_contentPresenter;
};

class ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility : public ListViewBaseItemAnimationCommand
{
public:
    ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility(
        bool selected,
        double fromScale,
        xref::weakref_ptr<CUIElement> selectionIndicator,
        bool isStarting,
        bool steadyStateOnly);

    ~ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility() override;
    _Check_return_ HRESULT Accept(
        _In_ ListViewBaseItemAnimationCommandVisitor* pVisitor) override;
    _Check_return_ HRESULT Clone(
        _Out_ std::unique_ptr<ListViewBaseItemAnimationCommand>& clone) override;
    XINT32 GetPriority() const override;

    bool m_selected;
    double m_fromScale;
    xref::weakref_ptr<CUIElement> m_selectionIndicator;
};

