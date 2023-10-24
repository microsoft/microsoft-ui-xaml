// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <CControl.h>
#include <optional>

struct FocusRectangleOptions;
class CCalendarView;
class CCoreServices;
class IContentRenderer;
class CUIElement;
class CBrush;
class TextBlockAlignments;
class TextBlockFontProperties;

// the sequence that CCalendarViewBaseItemChrome needs to render
// (in order of lowest layer to highest layer).
enum class CalendarViewBaseItemChromeLayerPosition
{
    Pre,
    TemplateChild_Post, // after TemplateChild, before Chrome TextBlocks - for the density colors layer
    Post,
};

class CCalendarViewBaseItemChrome : public CControl
{

protected:
    CCalendarViewBaseItemChrome(
        _In_ CCoreServices *pCore);

    _Check_return_ HRESULT MeasureOverride(
        _In_ XSIZEF availableSize,
        _Out_ XSIZEF& desiredSize
        ) override;

    _Check_return_ HRESULT ArrangeOverride(
        _In_ XSIZEF finalSize,
        _Out_ XSIZEF& newFinalSize
        ) override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

public:

    _Check_return_ HRESULT RenderChrome(
        _In_ IContentRenderer* pContentRenderer,
        _In_ CalendarViewBaseItemChromeLayerPosition layer
        );

    _Check_return_ HRESULT RenderDensityBars(
        _In_ IContentRenderer* pContentRenderer
        );

    void SetOwner(_In_opt_ CCalendarView* pOwner);
    xref_ptr<CCalendarView> GetOwner() const;

    _Check_return_ HRESULT  SetDensityColors(_In_opt_ wfc::IIterable<wu::Color>* pColors);

    _Check_return_ HRESULT SetIsToday(_In_ bool state);
    _Check_return_ HRESULT SetIsKeyboardFocused(_In_ bool state);
    _Check_return_ HRESULT SetIsSelected(_In_ bool state);
    _Check_return_ HRESULT SetIsBlackout(_In_ bool state);
    _Check_return_ HRESULT SetIsHovered(_In_ bool state);
    _Check_return_ HRESULT SetIsPressed(_In_ bool state);
    _Check_return_ HRESULT SetIsOutOfScope(_In_ bool state);

    _Check_return_ HRESULT UpdateTextBlocksForeground();
    _Check_return_ HRESULT UpdateTextBlocksFontProperties();
    _Check_return_ HRESULT UpdateTextBlocksAlignments();
    _Check_return_ HRESULT UpdateTextBlocksForegroundOpacity();
    _Check_return_ HRESULT UpdateTextBlocksMargin();
    _Check_return_ HRESULT UpdateBackgroundAndBorderBrushes();
    _Check_return_ HRESULT UpdateBlackoutStrikethroughSize();
    _Check_return_ HRESULT UpdateCornerRadius();

    void InvalidateRender(bool invalidateChildren = false);

    _Check_return_ HRESULT UpdateMainText(_In_ HSTRING mainText);
    _Check_return_ HRESULT UpdateLabelText(_In_ HSTRING labelText);
    _Check_return_ HRESULT ShowLabelText(_In_ bool showLabel);

    xref_ptr<CUIElement> GetOuterBorder() const;

    _Check_return_ HRESULT GetMainText(_Out_ HSTRING* pMainText) const;

    bool IsHovered() const;
    bool IsPressed() const;

#pragma region framework overrides
    // Returns true if this FE already has an expanded template in its tree.
    bool HasTemplateChild() final;

    // Adds the given CUIElement to the children collection as the designated templated child.
    // Should only be used if !HasTemplateChild.
    _Check_return_ HRESULT AddTemplateChild(_In_ CUIElement* pUI) override;

    // Removes the templated child from the children collection of this FE. Only use if
    // HasTemplateChild is TRUE.
    _Check_return_ HRESULT RemoveTemplateChild() override;
#pragma endregion

#pragma region uielement overrides
    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    CUIElement* GetFirstChild() override;
#pragma endregion

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT CustomizeFocusRectangle(_Inout_ FocusRectangleOptions& options, _Out_ bool* shouldDrawFocusRect);

    _Check_return_ HRESULT ShouldDrawDottedLinesFocusVisual(_Out_ bool* shouldDrawDottedLines);

    // Sets the outer border CornerRadius.
    _Check_return_ HRESULT SetOuterBorderCornerRadius();

    // Sets the inner border CornerRadius.
    _Check_return_ HRESULT SetInnerBorderCornerRadius();

    XCORNERRADIUS GetCornerRadius() const override final;

    // Sets the outer CornerRadius.
    _Check_return_ HRESULT SetCornerRadius();

    static void ClearIsRoundedCalendarViewBaseItemChromeEnabledCache();
    static bool IsRoundedCalendarViewBaseItemChromeEnabled(_In_ CCoreServices* core);
    static bool IsRoundedCalendarViewBaseItemChromeDenied();
    static bool IsRoundedCalendarViewBaseItemChromeForced();

    bool IsRoundedCalendarViewBaseItemChromeEnabled() const;

private:
    static bool IsTransparentSolidColorBrush(_In_ CBrush* brush);

    _Check_return_ HRESULT EnsureTextBlock(_Inout_ xref_ptr<CTextBlock>& spTextBlock);
    _Check_return_ HRESULT CreateTextBlock(_Inout_ xref_ptr<CTextBlock>& spTextBlock);

    _Check_return_ HRESULT UpdateTextBlockForegroundOpacity(_In_ CTextBlock* pTextBlock);
    _Check_return_ HRESULT UpdateTextBlockForeground(_In_ CTextBlock* pTextBlock);
    _Check_return_ HRESULT UpdateTextBlockFontProperties(_In_ CTextBlock* pTextBlock);
    _Check_return_ HRESULT UpdateTextBlockAlignments(_In_ CTextBlock* pTextBlock);
    _Check_return_ HRESULT UpdateTextBlockMargin(_In_ CTextBlock* textBlock);

    _Check_return_ HRESULT DrawDensityBar(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    _Check_return_ HRESULT DrawBorder(
        _In_ IContentRenderer* pContentRenderer,
        _In_ CBrush* pBrush,
        _In_ const XRECTF& bounds,
        _In_ const XTHICKNESS& pNinegrid,
        _In_ bool isHollow
    );

    _Check_return_ HRESULT DrawBorder(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    // for Selected+Today inner border.
    _Check_return_ HRESULT DrawInnerBorder(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    _Check_return_ HRESULT DrawFocusBorder(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    // draw the chromed background layer (those background properties on CalendarView,
    // which apply to all calendarviewbaseitems).
    // e.g. CalendarViewTodayBackground, CalendarViewOutOfScopeBackground...
    _Check_return_ HRESULT DrawBackground(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    // draw the Control Background on CalendarViewBaseItem, which applys to this item only.
    // this is drawn on top of above Background so developer could customize a "SelectedBackground"
    // (which we don't support yet)
    _Check_return_ HRESULT DrawControlBackground(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const XRECTF& bounds
        );

    // Sets up and adds the outer border to the tree.
    _Check_return_ HRESULT EnsureOuterBorder();

    // Sets up and adds the inner border to the tree.
    _Check_return_ HRESULT EnsureInnerBorder();

    // Sets up and adds the blackout strikethrough line to the tree.
    _Check_return_ HRESULT EnsureStrikethroughLine();

    // Sets the outer border Background brush based on current visual state.
    _Check_return_ HRESULT SetOuterBorderBackground();

    // Sets the outer border BorderBrush.
    _Check_return_ HRESULT SetOuterBorderBrush();

    // Sets the outer border BorderThickness.
    _Check_return_ HRESULT SetOuterBorderThickness();

    // Sets the inner border Background brush.
    _Check_return_ HRESULT SetInnerBorderBackground();

    // Sets the inner border BorderBrush.
    _Check_return_ HRESULT SetInnerBorderBrush();

    // Sets the inner border BorderThickness.
    _Check_return_ HRESULT SetInnerBorderThickness();

    // Sets the brush for the blackout strikethrough line.
    _Check_return_ HRESULT SetBlackoutStrikethroughBrush();

    // Sets the size and position for the blackout strikethrough line.
    _Check_return_ HRESULT SetBlackoutStrikethroughSize();

    // Returns the CalendarView's CalendarItemCornerRadius when set or half the size.
    XCORNERRADIUS GetOuterBorderCornerRadius();

    XTHICKNESS GetItemBorderThickness();
    CBrush* GetTextBlockForeground() const;
    CBrush* GetItemBorderBrush(_In_ bool forFocus) const;
    CBrush* GetItemFocusAltBorderBrush() const;
    // for Selected+Today inner border.
    CBrush* GetItemInnerBorderBrush() const;
    CBrush* GetItemBackgroundBrush() const;
    float GetTextBlockForegroundOpacity() const;

protected:

    struct TextBlockFontProperties
    {
        XFLOAT fontSize;
        DirectUI::FontStyle fontStyle;
        DirectUI::CoreFontWeight fontWeight;
        CFontFamily* pFontFamilyNoRef;
    };

    virtual bool GetTextBlockFontProperties(
        _In_ bool isLabel,
        _Out_ TextBlockFontProperties *pProperties) = 0;

    struct TextBlockAlignments
    {
        DirectUI::HorizontalAlignment horizontalAlignment;
        DirectUI::VerticalAlignment verticalAlignment;
    };

    virtual bool GetTextBlockAlignments(
        _In_ bool isLabel,
        _Out_ TextBlockAlignments *pAlignments) = 0;

    virtual XTHICKNESS GetTextBlockMargin(
        bool isLabel) const = 0;

    _Check_return_ bool IsLabel(_In_ CTextBlock* pTextBlock);

    bool ShouldUseLayoutRounding();

private:
    xref_ptr<CTextBlock> m_pMainTextBlock;
    xref_ptr<CTextBlock> m_pLabelTextBlock;
    xref_ptr<CBorder>    m_outerBorder;
    xref_ptr<CBorder>    m_innerBorder;
    xref_ptr<CLine>      m_strikethroughLine;

    xref::weakref_ptr<CCalendarView> m_wrOwner;

    static const XTHICKNESS s_innerBorderThickness;
    static const float s_strikethroughThickness;
    static const float s_strikethroughFontSizeMultiplier;
    static const int s_maxNumberOfDensityBars = 10;
    static std::optional<bool> s_isRoundedCalendarViewBaseItemChromeEnabled;

    std::array<wu::Color, s_maxNumberOfDensityBars> m_densityBarColors;
    UINT m_numberOfDensityBar;

protected:
    bool m_isToday : 1;
    bool m_isKeyboardFocused : 1;
    bool m_isSelected : 1;
    bool m_isBlackout : 1;
    bool m_isHovered : 1;
    bool m_isPressed : 1;
    bool m_isOutOfScope : 1;
    bool m_hasLabel : 1;
};


class CCalendarViewItemChrome : public CCalendarViewBaseItemChrome
{
protected:
    CCalendarViewItemChrome(
        _In_ CCoreServices *pCore) : CCalendarViewBaseItemChrome(pCore) {}

    ~CCalendarViewItemChrome() override{}

public:
    DECLARE_CREATE(CCalendarViewItemChrome);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCalendarViewItemChrome>::Index;
    }

protected:
    bool GetTextBlockFontProperties(
        _In_ bool isLabel,
        _Out_ TextBlockFontProperties *pProperties) override;

    bool GetTextBlockAlignments(
        _In_ bool isLabel,
        _Out_ TextBlockAlignments *pAlignments) override;

    XTHICKNESS GetTextBlockMargin(
        bool isLabel) const override;
};

class CCalendarViewDayItemChrome : public CCalendarViewBaseItemChrome
{
protected:
    CCalendarViewDayItemChrome(
        _In_ CCoreServices *pCore) : CCalendarViewBaseItemChrome(pCore) {}

    ~CCalendarViewDayItemChrome() override{}

public:
    DECLARE_CREATE(CCalendarViewDayItemChrome);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCalendarViewDayItemChrome>::Index;
    }

protected:
    bool GetTextBlockFontProperties(
        _In_ bool isLabel,
        _Out_ TextBlockFontProperties *pProperties) override;

    bool GetTextBlockAlignments(
        _In_ bool isLabel,
        _Out_ TextBlockAlignments *pAlignments) override;

    XTHICKNESS GetTextBlockMargin(
        bool isLabel) const override;
};
