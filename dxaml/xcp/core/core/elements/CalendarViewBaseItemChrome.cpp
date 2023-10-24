// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "string.h"
#include "values.h"
#include <ContentRenderer.h>
#include <RuntimeEnabledFeatures.h>
#include <FocusRectOptions.h>
#include "RootScale.h"

// Work around disruptive max/min macros
#undef max
#undef min

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get CalendarViewBaseItemChrome debug outputs, and 0 otherwise
#define CVBIC_DBG 0
//#define CVBIC_DEBUG

std::optional<bool> CCalendarViewBaseItemChrome::s_isRoundedCalendarViewBaseItemChromeEnabled;

#define InScopeDensityBarOpacity 0.35f
#define OutOfScopeDensityBarOpacity 0.10f
#define MaxDensityBarHeight 5.0f
#define TodayBlackoutOpacity 0.40f
#define FocusBorderThickness 2.0f
#define TodaySelectedInnerBorderThickness 2.0f

// Border thickness for inner border when rounded corners are applied.
const XTHICKNESS CCalendarViewBaseItemChrome::s_innerBorderThickness = { 1.0f, 1.0f, 1.0f, 1.0f };

// Stroke thickness for the blackout strikethrough lines.
const float CCalendarViewBaseItemChrome::s_strikethroughThickness = 1.0f;

// Font size multiplier used to evaluate the blackout strikethrough line size.
const float CCalendarViewBaseItemChrome::s_strikethroughFontSizeMultiplier = 1.25f;

CCalendarViewBaseItemChrome::CCalendarViewBaseItemChrome(_In_ CCoreServices *pCore)
    : CControl(pCore)
    , m_pMainTextBlock(nullptr)
    , m_pLabelTextBlock(nullptr)
    , m_isToday(false)
    , m_isKeyboardFocused(false)
    , m_isSelected(false)
    , m_isBlackout(false)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_isOutOfScope(false)
    , m_numberOfDensityBar(0)
    , m_hasLabel(false)
{
}

// Invoked when a TAEF test calls TestServices::Utilities::DeleteResourceDictionaryCaches().
/*static*/
void CCalendarViewBaseItemChrome::ClearIsRoundedCalendarViewBaseItemChromeEnabledCache()
{
    s_isRoundedCalendarViewBaseItemChromeEnabled.reset();
}

/*static*/
bool CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeEnabled(_In_ CCoreServices* core)
{
    if (CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeDenied())
    {
        return false;
    }

    if (CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeForced())
    {
        return true;
    }

    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_calendarViewBaseItemRoundedChromeEnabled, L"CalendarViewBaseItemRoundedChromeEnabled");
    bool isRoundedCalendarViewBaseItemChromeEnabled = false;

    IFCFAILFAST(CDependencyProperty::GetBooleanThemeResourceValue(core, c_calendarViewBaseItemRoundedChromeEnabled, &isRoundedCalendarViewBaseItemChromeEnabled));

    return isRoundedCalendarViewBaseItemChromeEnabled;
}

/*static*/
bool CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeDenied()
{
    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    return runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DenyRoundedCalendarViewBaseItemChrome);
}

/*static*/
bool CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeForced()
{
    if (IsRoundedCalendarViewBaseItemChromeDenied())
    {
        return false;
    }

    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    return runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceRoundedCalendarViewBaseItemChrome);
}

/*static*/
bool CCalendarViewBaseItemChrome::IsTransparentSolidColorBrush(_In_ CBrush* brush)
{
    if (brush->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        CSolidColorBrush* solidColorBrush = static_cast<CSolidColorBrush*>(brush);

        return ColorUtils::IsTransparentColor(solidColorBrush->m_rgb);
    }

    return false;
}

// The "CalendarViewBaseItemRoundedChromeEnabled" theme resource value is used to turn on/off the rendering with rounded corners.
// For performance reasons, the resource is only evaluated once. TAEF tests can invalidate the cache by calling TestServices::Utilities::DeleteResourceDictionaryCaches().
bool CCalendarViewBaseItemChrome::IsRoundedCalendarViewBaseItemChromeEnabled() const
{
    if (!s_isRoundedCalendarViewBaseItemChromeEnabled.has_value())
    {
        s_isRoundedCalendarViewBaseItemChromeEnabled = IsRoundedCalendarViewBaseItemChromeEnabled(GetContext());
    }

    return s_isRoundedCalendarViewBaseItemChromeEnabled.value();
}

bool CCalendarViewBaseItemChrome::HasTemplateChild()
{
    return GetFirstChildNoAddRef() != nullptr;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::AddTemplateChild(_In_ CUIElement* pUI)
{
    ASSERT(GetFirstChildNoAddRef() == nullptr);

    return InsertChild(0, pUI);
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::RemoveTemplateChild()
{
    CUIElement* pTemplateChild = GetFirstChildNoAddRef();

    if (pTemplateChild != nullptr)
    {
        IFC_RETURN(RemoveChild(pTemplateChild));
    }

    return S_OK;
}

CUIElement* CCalendarViewBaseItemChrome::GetFirstChild()
{
    xref_ptr<CUIElement> spFirstChild;
    // ref added in CUIElement::GetFirstChild()
    spFirstChild.attach(CControl::GetFirstChild());

    // We overrode HasTemplateChild and AddTemplateChild to make sure
    // the template child (if exists) will be always at index 0.
    // So here if the first child is our internal textblocks, borders
    // or line, it means we don't have a template child.
    if (spFirstChild.get() != m_strikethroughLine &&
        spFirstChild.get() != m_innerBorder &&
        spFirstChild.get() != m_outerBorder &&
        spFirstChild.get() != m_pMainTextBlock &&
        spFirstChild.get() != m_pLabelTextBlock)
    {
        // keep ref added, it's the caller's responsibility to release ref.
        return spFirstChild.detach();
    }
    else
    {
        return nullptr;
    }
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Control_Template)
    {
        InvalidateRender();
    }
    else if (IsRoundedCalendarViewBaseItemChromeEnabled() && args.m_pDP->GetIndex() == KnownPropertyIndex::Control_Background)
    {
        IFC_RETURN(SetInnerBorderBackground());
    }

    return CControl::OnPropertyChanged(args);
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::HitTestLocalInternal(
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

_Check_return_ HRESULT CCalendarViewBaseItemChrome::HitTestLocalInternal(
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

_Check_return_ HRESULT CCalendarViewBaseItemChrome::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds)
{
    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = GetActualWidth();
    pBounds->bottom = GetActualHeight();

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::MeasureOverride(
    _In_ XSIZEF availableSize,
    _Out_ XSIZEF& desiredSize)
{
    XTHICKNESS borderThickness = GetItemBorderThickness();
    XTHICKNESS padding = GetPadding();

    if (ShouldUseLayoutRounding())
    {
        borderThickness.left = LayoutRound(borderThickness.left);
        borderThickness.right = LayoutRound(borderThickness.right);
        borderThickness.top = LayoutRound(borderThickness.top);
        borderThickness.bottom = LayoutRound(borderThickness.bottom);

        padding.left = LayoutRound(padding.left);
        padding.right = LayoutRound(padding.right);
        padding.top = LayoutRound(padding.top);
        padding.bottom = LayoutRound(padding.bottom);
    }

    CSizeUtil::Deflate(&availableSize, borderThickness);
    CSizeUtil::Deflate(&availableSize, padding);

    if (IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        if (m_outerBorder)
        {
            IFC_RETURN(m_outerBorder->Measure(availableSize));
        }

        if (m_innerBorder)
        {
            IFC_RETURN(m_innerBorder->Measure(availableSize));
        }

        if (m_strikethroughLine)
        {
            IFC_RETURN(m_strikethroughLine->Measure(availableSize));
        }
    }

    if (m_pMainTextBlock)
    {
        IFC_RETURN(m_pMainTextBlock->Measure(availableSize));
        desiredSize = m_pMainTextBlock->DesiredSize;
    }

    if (m_pLabelTextBlock)
    {
        IFC_RETURN(m_pLabelTextBlock->Measure(availableSize));
    }

    // Get the child to measure it - if any.
    CUIElement* pChildNoRef = GetFirstChildNoAddRef();

    //If we have a child
    if (pChildNoRef)
    {
        IFC_RETURN(pChildNoRef->Measure(availableSize));
        IFC_RETURN(pChildNoRef->EnsureLayoutStorage());

        desiredSize.width = std::max(pChildNoRef->DesiredSize.width, desiredSize.width);
        desiredSize.height = std::max(pChildNoRef->DesiredSize.height, desiredSize.height);
    }

    CSizeUtil::Inflate(&desiredSize, borderThickness);
    CSizeUtil::Inflate(&desiredSize, padding);

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::ArrangeOverride(
    _In_ XSIZEF finalSize,
    _Out_ XSIZEF& newFinalSize)
{
    XRECTF finalBounds = { 0.0f, 0.0f, finalSize.width, finalSize.height };

    if (m_outerBorder)
    {
        ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

        IFC_RETURN(m_outerBorder->Arrange(finalBounds));
    }

    if (m_innerBorder)
    {
        ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

        IFC_RETURN(m_innerBorder->Arrange(finalBounds));
    }

    if (m_strikethroughLine)
    {
        ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

        IFC_RETURN(m_strikethroughLine->Arrange(finalBounds));
    }

    XTHICKNESS borderThickness = GetItemBorderThickness();
    XTHICKNESS padding = GetPadding();

    CSizeUtil::Deflate(&finalBounds, borderThickness);
    CSizeUtil::Deflate(&finalBounds, padding);

    if (ShouldUseLayoutRounding())
    {
        borderThickness.left = LayoutRound(borderThickness.left);
        borderThickness.right = LayoutRound(borderThickness.right);
        borderThickness.top = LayoutRound(borderThickness.top);
        borderThickness.bottom = LayoutRound(borderThickness.bottom);

        padding.left = LayoutRound(padding.left);
        padding.right = LayoutRound(padding.right);
        padding.top = LayoutRound(padding.top);
        padding.bottom = LayoutRound(padding.bottom);
    }

    if (m_pMainTextBlock)
    {
        IFC_RETURN(m_pMainTextBlock->Arrange(finalBounds));
    }

    if (m_pLabelTextBlock)
    {
        IFC_RETURN(m_pLabelTextBlock->Arrange(finalBounds));
    }

    CUIElement* pChildNoRef = GetFirstChildNoAddRef();

    if (pChildNoRef)
    {
        IFC_RETURN(pChildNoRef->Arrange(finalBounds));
    }

    newFinalSize = finalSize;

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::CreateTextBlock(
    _Inout_ xref_ptr<CTextBlock>& spTextBlock)
{
    CValue value;

    CREATEPARAMETERS cp(GetContext());

    IFC_RETURN(CTextBlock::Create(reinterpret_cast<CDependencyObject **>(spTextBlock.ReleaseAndGetAddressOf()), &cp));

    IFC_RETURN(spTextBlock->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

    value.Set(DirectUI::TextWrapping::NoWrap);
    IFC_RETURN(spTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextWrapping, value));

    value.SetEnum(UIAXcp::AccessibilityView_Raw);
    IFC_RETURN(spTextBlock->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, value));

    value.Set(DirectUI::Visibility::Visible);
    IFC_RETURN(spTextBlock->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, value));

    return S_OK;
}

// Sets up and adds the main and label block to the tree.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::EnsureTextBlock(_Inout_ xref_ptr<CTextBlock>& spTextBlock)
{
    CUIElementCollection* pChildrenCollectionNoRef = nullptr;

    if (!spTextBlock.get())
    {
        CValue value;

        IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_ChildrenInternal, &value));
        IFC_RETURN(DoPointerCast(pChildrenCollectionNoRef, value.AsObject()));

        IFC_RETURN(CreateTextBlock(spTextBlock));
        IFC_RETURN(pChildrenCollectionNoRef->Append((spTextBlock.get())));

        IFC_RETURN(UpdateTextBlockForeground(spTextBlock.get()));
        IFC_RETURN(UpdateTextBlockForegroundOpacity(spTextBlock.get()));
        IFC_RETURN(UpdateTextBlockFontProperties(spTextBlock.get()));
        IFC_RETURN(UpdateTextBlockAlignments(spTextBlock.get()));
        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(UpdateTextBlockMargin(spTextBlock.get()));
        }
    }

    return S_OK;
}

_Check_return_ bool CCalendarViewBaseItemChrome::IsLabel(_In_ CTextBlock* pTextBlock)
{
    ASSERT(pTextBlock && (pTextBlock == m_pLabelTextBlock || pTextBlock == m_pMainTextBlock), L"the textblock should be main textblock or label textblock.");

    return pTextBlock == m_pLabelTextBlock;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::RenderChrome(
    _In_ IContentRenderer* pContentRenderer,
    _In_ CalendarViewBaseItemChromeLayerPosition layer
    )
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: RenderChrome - entry. layer=%d", this, layer));
    }
#endif // CVBIC_DEBUG

    XRECTF bounds = { 0.0f, 0.0f, GetActualWidth(), GetActualHeight() };

    if (ShouldUseLayoutRounding())
    {
        bounds.Width = LayoutRound(bounds.Width);
        bounds.Height = LayoutRound(bounds.Height);
    }

    // we draw backgrounds in Pre phase
    if (!IsRoundedCalendarViewBaseItemChromeEnabled() &&
        layer == CalendarViewBaseItemChromeLayerPosition::Pre)
    {
        // those background properties on CalendarView, which apply to all calendarviewbaseitems.
        // e.g. CalendarViewTodayBackground, CalendarViewOutOfScopeBackground...
        IFC_RETURN(DrawBackground(pContentRenderer, bounds));

        // the Control Background on CalendarViewBaseItem, which applies to this item only.
        // this is drawn on top of above Background so developer could customize a "SelectedBackground"
        // (which we don't support yet)
        IFC_RETURN(DrawControlBackground(pContentRenderer, bounds));
    }

    // to make sure densitybars layer appears after template child (if any) and before
    // the main and label textblocks, we draw densitybars in TemplateChild_Post phase,
    // if there is no template child, then draw densitybars in Pre phase
    if (layer == CalendarViewBaseItemChromeLayerPosition::TemplateChild_Post ||
        (layer == CalendarViewBaseItemChromeLayerPosition::Pre && GetFirstChildNoAddRef() == nullptr))
    {
        IFC_RETURN(DrawDensityBar(pContentRenderer, bounds));
    }

    // we draw borders in Post phase.
    if (!IsRoundedCalendarViewBaseItemChromeEnabled() &&
        layer == CalendarViewBaseItemChromeLayerPosition::Post)
    {
        bool shouldDrawDottedLines = false;
        IFC_RETURN(ShouldDrawDottedLinesFocusVisual(&shouldDrawDottedLines));
        if (m_isKeyboardFocused && shouldDrawDottedLines)
        {
            IFC_RETURN(DrawFocusBorder(pContentRenderer, bounds));
        }
        else
        {
            IFC_RETURN(DrawBorder(pContentRenderer, bounds));
        }

        // only for Today+Selected state, we draw an additional inner border.
        if (m_isToday && m_isSelected)
        {
            auto innerBorder = bounds;
            CSizeUtil::Deflate(&innerBorder, GetItemBorderThickness());
            IFC_RETURN(DrawInnerBorder(pContentRenderer, innerBorder));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::RenderDensityBars(
    _In_ IContentRenderer* pContentRenderer
    )
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: RenderDensityBars - entry.", this));
    }
#endif // CVBIC_DEBUG

    // potential bug if we create an LTE to the template child, the density bars will
    // be rendered on the LTE as well.
    IFC_RETURN(RenderChrome(
        pContentRenderer,
        CalendarViewBaseItemChromeLayerPosition::TemplateChild_Post
        ));

    return S_OK;
}

bool CCalendarViewBaseItemChrome::ShouldUseLayoutRounding()
{
    // Similar to what Borders do, but we don't care about corner radius (ours is always 0).
    const auto scale = RootScale::GetRasterizationScaleForElement(this);
    return (scale != 1.0f) && GetUseLayoutRounding();
}

UINT32 GetIntValueOfColor(wu::Color color)
{
    return ((UINT)color.A << 24) | ((UINT)color.R << 16) | ((UINT)color.G << 8) | (UINT)color.B;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawDensityBar(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: DrawDensityBar - entry. m_numberOfDensityBar=%d", this, m_numberOfDensityBar));
    }
#endif // CVBIC_DEBUG

    auto owner = GetOwner();
    if (m_numberOfDensityBar > 0 && owner)
    {
        HWRenderParams rp = *(pContentRenderer->GetRenderParams());
        HWRenderParamsOverride hwrpOverride(pContentRenderer, &rp);
        rp.opacityToCompNode *= m_isOutOfScope ? OutOfScopeDensityBarOpacity : InScopeDensityBarOpacity;

        // density bar bounds:
        //   height: itemHeight / 10 - 1, max 5px
        //   width: itemWidth - 2, centered
        // 1 px between bars.

        XRECTF densityBarBounds = bounds;
        densityBarBounds.Height = densityBarBounds.Height / s_maxNumberOfDensityBars - 1;
        densityBarBounds.Height = std::min(densityBarBounds.Height, MaxDensityBarHeight);
        densityBarBounds.Width = bounds.Width - 2;
        densityBarBounds.X = 1;

        densityBarBounds.Y = bounds.Height - densityBarBounds.Height;

        for (UINT i = 0; i < m_numberOfDensityBar; ++i)
        {
            CBrush* pBrush = nullptr;

            IFC_RETURN(owner->GetBrushNoRef(GetIntValueOfColor(m_densityBarColors[i]), &pBrush));

            BrushParams emptyBrushParams;
            IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
                densityBarBounds,
                densityBarBounds,
                pBrush,
                emptyBrushParams,
                this,
                nullptr,    // pNinegrid
                nullptr,    // pShapeHwTexture
                false));    // fIsHollow

            densityBarBounds.Y -= densityBarBounds.Height + 1;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawBorder(
    _In_ IContentRenderer* pContentRenderer,
    _In_ CBrush* pBrush,
    _In_ const XRECTF& bounds,
    _In_ const XTHICKNESS& pNinegrid,
    _In_ bool isHollow
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    BrushParams emptyBrushParams;
    IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
        bounds,
        bounds,
        pBrush,
        emptyBrushParams,
        this,
        &pNinegrid,     // pNinegrid
        nullptr,       // pShapeHwTexture
        isHollow));    // fIsHollow

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawBorder(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    XTHICKNESS thickness = GetItemBorderThickness();
    CBrush* pBrush = GetItemBorderBrush(false /*forFocus*/);

    if (pBrush && thickness.bottom != 0 && thickness.top != 0 && thickness.left != 0 && thickness.right != 0)
    {
        IFC_RETURN(DrawBorder(pContentRenderer, pBrush, bounds, thickness /* ninegrid */, true /* isHollow */));
    }

    return S_OK;
}

// for Selected+Today inner border.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawInnerBorder(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    XTHICKNESS thickness{ TodaySelectedInnerBorderThickness, TodaySelectedInnerBorderThickness, TodaySelectedInnerBorderThickness, TodaySelectedInnerBorderThickness };
    CBrush* pBrush = GetItemInnerBorderBrush();
    if (pBrush)
    {
        CMILMatrix            brushToElement(TRUE);
        const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
        HWRenderParams        localRP = rp;
        HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);
        TransformAndClipStack transformsAndClips;
        XRECTF                localBounds = { 0.0f, 0.0f, bounds.Width, bounds.Height };

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

        IFC_RETURN(DrawBorder(pContentRenderer, pBrush, localBounds, thickness /* ninegrid */, true /* isHollow */));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawFocusBorder(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    FocusRectangleOptions focusOptions;
    CBrush* pFocusBrush = GetItemBorderBrush(true /*forFocus*/);
    CBrush* pFocusAltBrush = GetItemFocusAltBorderBrush();

    focusOptions.firstThickness = Thickness(FocusBorderThickness);

    focusOptions.drawFirst = true;
    focusOptions.firstBrush = static_cast<CSolidColorBrush*>(pFocusBrush);
    focusOptions.drawSecond = true; // always use the default CalendarItemBackground even if the item has another background (e.g. TodayBackground).
    focusOptions.secondBrush = static_cast<CSolidColorBrush*>(pFocusAltBrush);

    IFC_RETURN(pContentRenderer->RenderFocusRectangle(
        this,
        focusOptions
        ));

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawBackground(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    CBrush* pBrush = GetItemBackgroundBrush();
    if (pBrush)
    {
        BrushParams emptyBrushParams;
        return pContentRenderer->GeneralImageRenderContent(
            bounds,
            bounds,
            pBrush,
            emptyBrushParams,
            this,
            nullptr,    // pNinegrid
            nullptr,    // pShapeHwTexture
            false);     // fIsHollow
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::DrawControlBackground(
    _In_ IContentRenderer* pContentRenderer,
    _In_ const XRECTF& bounds
    )
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    CValue cVal;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Control_Background, &cVal));

    ASSERT(cVal.GetType() == ValueType::valueObject);
    auto pBrush = static_cast<CBrush*>(cVal.AsObject());

    if (pBrush)
    {
        BrushParams emptyBrushParams;
        IFC_RETURN(pContentRenderer->GeneralImageRenderContent(
            bounds,
            bounds,
            pBrush,
            emptyBrushParams,
            this,
            nullptr,    // pNinegrid
            nullptr,    // pShapeHwTexture
            false));    // fIsHollow
    }
    return S_OK;
}

void CCalendarViewBaseItemChrome::SetOwner(_In_opt_ CCalendarView* pOwner)
{
    ASSERT(!m_wrOwner.lock());
    m_wrOwner = xref::get_weakref(pOwner);
}

xref_ptr<CCalendarView> CCalendarViewBaseItemChrome::GetOwner() const
{
    return m_wrOwner.lock();
}

_Check_return_ HRESULT  CCalendarViewBaseItemChrome::SetDensityColors(
    _In_opt_ wfc::IIterable<wu::Color>* pColors)
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetDensityColors - entry.", this));
    }
#endif // CVBIC_DEBUG

    const bool hadDensityBars = m_numberOfDensityBar != 0;
    const bool isRoundedCalendarViewBaseItemChromeEnabled = IsRoundedCalendarViewBaseItemChromeEnabled();

    if (pColors)
    {
        ctl::ComPtr<wfc::IIterator<wu::Color>> spIterator;
        UINT index = 0;
        BOOLEAN hasCurrent = FALSE;

        IFC_RETURN(pColors->First(&spIterator));

        IFC_RETURN(spIterator->get_HasCurrent(&hasCurrent));

        while (hasCurrent && index < s_maxNumberOfDensityBars)
        {
            IFC_RETURN(spIterator->get_Current(&m_densityBarColors[index]));
            IFC_RETURN(spIterator->MoveNext(&hasCurrent));
            ++index;
        }
        m_numberOfDensityBar = index;

        if (isRoundedCalendarViewBaseItemChromeEnabled)
        {
            // Ensure the m_outerBorder is present so the density lines get clipped according to the corner radius.
            IFC_RETURN(EnsureOuterBorder());
        }
    }
    else
    {
        m_numberOfDensityBar = 0;
    }

    InvalidateRender(hadDensityBars && isRoundedCalendarViewBaseItemChromeEnabled /*invalidateChildren*/);

    return S_OK;
}

// Today state affects text foreground, opacity, borderbrush and FontWeight
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsToday(_In_ bool state)
{
    if (m_isToday != state)
    {
        m_isToday = state;

        IFC_RETURN(UpdateTextBlocksForeground());
        IFC_RETURN(UpdateTextBlocksForegroundOpacity());
        IFC_RETURN(UpdateTextBlocksFontProperties());

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(SetOuterBorderBrush());
            IFC_RETURN(SetOuterBorderThickness());
            IFC_RETURN(SetOuterBorderBackground());
            IFC_RETURN(SetInnerBorderBrush());
            IFC_RETURN(SetBlackoutStrikethroughBrush());
            IFC_RETURN(SetInnerBorderCornerRadius());
        }

        InvalidateRender();
    }

    return S_OK;
}

// Focus state affects background and border brush.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsKeyboardFocused(_In_ bool state)
{
    if (m_isKeyboardFocused != state)
    {
        m_isKeyboardFocused = state;

        InvalidateRender();
    }

    return S_OK;
}

// Selection state affects foreground and foreground opacity.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsSelected(_In_ bool state)
{
    if (m_isSelected != state)
    {
        m_isSelected = state;

        IFC_RETURN(UpdateTextBlocksForeground());
        IFC_RETURN(UpdateTextBlocksForegroundOpacity());

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(SetOuterBorderBrush());
            IFC_RETURN(SetOuterBorderThickness());
            IFC_RETURN(SetInnerBorderBrush());
            IFC_RETURN(SetInnerBorderCornerRadius());
        }

        InvalidateRender();
    }

    return S_OK;
}

// Blackout state affects foreground and foreground opacity.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsBlackout(_In_ bool state)
{
    if (m_isBlackout != state)
    {
        m_isBlackout = state;
        IFC_RETURN(UpdateTextBlocksForeground());
        IFC_RETURN(UpdateTextBlocksForegroundOpacity());

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(SetOuterBorderBrush());
            IFC_RETURN(SetOuterBorderThickness());
            IFC_RETURN(SetOuterBorderBackground());
            IFC_RETURN(SetInnerBorderBrush());
            IFC_RETURN(SetBlackoutStrikethroughBrush());
        }

        InvalidateRender();
    }

    return S_OK;

}

// Hover state affects border brush and background.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsHovered(_In_ bool state)
{
    if (m_isHovered != state)
    {
        m_isHovered = state;

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(UpdateTextBlocksForeground());

            IFC_RETURN(SetOuterBorderBrush());
            IFC_RETURN(SetOuterBorderThickness());
            IFC_RETURN(SetOuterBorderBackground());
        }

        InvalidateRender();
    }

    return S_OK;
}

// Pressed state affects foreground.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsPressed(_In_ bool state)
{
    if (m_isPressed != state)
    {
        m_isPressed = state;

        IFC_RETURN(UpdateTextBlocksForeground());

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(SetOuterBorderBrush());
            IFC_RETURN(SetOuterBorderThickness());
            IFC_RETURN(SetOuterBorderBackground());
            IFC_RETURN(SetInnerBorderBrush());
            IFC_RETURN(SetInnerBorderCornerRadius());
        }

        InvalidateRender();
    }

    return S_OK;
}

// Scope state affects foreground and background.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetIsOutOfScope(_In_ bool state)
{
    if (m_isOutOfScope != state)
    {
        m_isOutOfScope = state;

        IFC_RETURN(UpdateTextBlocksForeground());

        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            IFC_RETURN(SetOuterBorderBackground());
        }

        InvalidateRender();
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlocksForeground()
{
    if (m_pMainTextBlock)
    {
        IFC_RETURN(UpdateTextBlockForeground(m_pMainTextBlock));
    }
    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockForeground(m_pLabelTextBlock));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlocksForegroundOpacity()
{
    if (m_pMainTextBlock)
    {
        IFC_RETURN(UpdateTextBlockForegroundOpacity(m_pMainTextBlock));
    }

    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockForegroundOpacity(m_pLabelTextBlock));
    }

    return S_OK;
}


_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlocksFontProperties()
{
    if (m_pMainTextBlock)
    {
        IFC_RETURN(UpdateTextBlockFontProperties(m_pMainTextBlock));
    }
    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockFontProperties(m_pLabelTextBlock));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlocksAlignments()
{
    if (m_pMainTextBlock)
    {
        IFC_RETURN(UpdateTextBlockAlignments(m_pMainTextBlock));
    }
    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockAlignments(m_pLabelTextBlock));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlocksMargin()
{
    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (m_pMainTextBlock)
    {
        IFC_RETURN(UpdateTextBlockMargin(m_pMainTextBlock));
    }
    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockMargin(m_pLabelTextBlock));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlockForeground(_In_ CTextBlock* pTextBlock)
{
    CBrush *pBrush = GetTextBlockForeground();

    ASSERT(pTextBlock);

    IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Foreground, pBrush));

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlockForegroundOpacity(_In_ CTextBlock* pTextBlock)
{
    float opacity = IsRoundedCalendarViewBaseItemChromeEnabled() ? 1.0f : GetTextBlockForegroundOpacity();

    ASSERT(pTextBlock);

    if (IsLabel(pTextBlock) && !m_hasLabel)
    {
        opacity = 0.0f;
    }

    IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, opacity));

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlockFontProperties(_In_ CTextBlock* pTextBlock)
{
    TextBlockFontProperties properties;

    ASSERT(pTextBlock);

    if (GetTextBlockFontProperties(
        IsLabel(pTextBlock),
        &properties))
    {
        CValue value;

        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontSize, properties.fontSize));

        value.Set(properties.fontStyle);
        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontStyle, value));

        value.Set(properties.fontWeight);
        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontWeight, value));

        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontFamily, properties.pFontFamilyNoRef));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlockAlignments(_In_ CTextBlock* pTextBlock)
{
    TextBlockAlignments alignments;

    ASSERT(pTextBlock);

    if (GetTextBlockAlignments(
        IsLabel(pTextBlock),
        &alignments))
    {
        CValue value;

        value.Set(alignments.horizontalAlignment);
        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        value.Set(alignments.verticalAlignment);
        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));
    }

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateTextBlockMargin(_In_ CTextBlock* textBlock)
{
    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    ASSERT(textBlock);

    CValue value;
    auto textBlockMargin = GetTextBlockMargin(IsLabel(textBlock));

    if (ShouldUseLayoutRounding())
    {
        textBlockMargin.left = LayoutRound(textBlockMargin.left);
        textBlockMargin.right = LayoutRound(textBlockMargin.right);
        textBlockMargin.top = LayoutRound(textBlockMargin.top);
        textBlockMargin.bottom = LayoutRound(textBlockMargin.bottom);
    }

    value.WrapThickness(const_cast<XTHICKNESS*>(&textBlockMargin));
    IFC_RETURN(textBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));
    
    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateBackgroundAndBorderBrushes()
{
    if (IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        IFC_RETURN(SetOuterBorderBrush());
        IFC_RETURN(SetOuterBorderThickness());
        IFC_RETURN(SetOuterBorderBackground());
        IFC_RETURN(SetInnerBorderBrush());
        IFC_RETURN(SetBlackoutStrikethroughBrush());
    }
    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateBlackoutStrikethroughSize()
{
    if (IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        IFC_RETURN(SetBlackoutStrikethroughSize());
    }
    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateCornerRadius()
{
    if (IsRoundedCalendarViewBaseItemChromeEnabled())
    {
        IFC_RETURN(SetOuterBorderCornerRadius());
        IFC_RETURN(SetInnerBorderCornerRadius());
        IFC_RETURN(SetCornerRadius());
    }
    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateMainText(_In_ HSTRING mainText)
{
    CValue value;
    xstring_ptr strString;

    IFC_RETURN(EnsureTextBlock(m_pMainTextBlock));

    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(mainText, &strString));

    value.SetString(strString);
    IFC_RETURN(m_pMainTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Text, value));

    return S_OK;
}

xref_ptr<CUIElement> CCalendarViewBaseItemChrome::GetOuterBorder() const
{
    return m_outerBorder;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::GetMainText(_Out_ HSTRING* pMainText) const
{
    if (m_pMainTextBlock)
    {
        CValue value;
        wrl_wrappers::HString str;
        XUINT32 count = 0;

        IFC_RETURN(m_pMainTextBlock->GetValueByIndex(KnownPropertyIndex::TextBlock_Text, &value));

        xstring_ptr strString = value.AsString();
        const WCHAR* buff = strString.GetBufferAndCount(&count);
        IFC_RETURN(str.Set(buff, count));
        *pMainText = str.Detach();
    }

    return S_OK;
}

bool CCalendarViewBaseItemChrome::IsHovered() const
{
    return m_isHovered;
}

bool CCalendarViewBaseItemChrome::IsPressed() const
{
    return m_isPressed;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::UpdateLabelText(_In_ HSTRING labelText)
{
    CValue value;
    xstring_ptr strString;

    IFC_RETURN(EnsureTextBlock(m_pLabelTextBlock));

    IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(labelText, &strString));

    value.SetString(strString);
    IFC_RETURN(m_pLabelTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Text, value));

    return S_OK;
}

_Check_return_ HRESULT CCalendarViewBaseItemChrome::ShowLabelText(_In_ bool showLabel)
{
    m_hasLabel = showLabel;

    if (m_pLabelTextBlock)
    {
        IFC_RETURN(UpdateTextBlockForegroundOpacity(m_pLabelTextBlock.get()));
    }
    else
    {
        ASSERT(!m_hasLabel);
    }

    return S_OK;
}

void CCalendarViewBaseItemChrome::InvalidateRender(bool invalidateChildren)
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: InvalidateRender - entry. invalidateChildren=%d", this, invalidateChildren));
    }
#endif // CVBIC_DEBUG

    NWSetContentDirty(this, DirtyFlags::Bounds);
    // if we have template child and density bars, we need to invalidate render on
    // template child as well because our density bars are appending to the template
    // child's post render data. We need to make sure the template child's render
    // data gets invalidated.
    //
    // if there is no template child, the density bars are just a part of our pre
    // render data. invalidate render on this CalendarViewBaseItem is good enough.
    //
    // When density bars are present, appear or disappear, the outer border also needs
    // to be refreshed so the bars get rounded corners or are removed when
    // m_numberOfDensityBar becomes 0.
    if (invalidateChildren || m_numberOfDensityBar > 0)
    {
        auto templateChild = GetFirstChildNoAddRef();
        if (templateChild)
        {
            NWSetContentDirty(templateChild, DirtyFlags::Bounds);
        }

        if (m_outerBorder)
        {
            NWSetContentDirty(m_outerBorder, DirtyFlags::Bounds);
        }
    }
}

XTHICKNESS CCalendarViewBaseItemChrome::GetItemBorderThickness()
{
    auto pOwner = GetOwner();
    if (pOwner)
    {
        auto thickness = pOwner->m_calendarItemBorderThickness;
        if (ShouldUseLayoutRounding())
        {
            thickness.left = LayoutRound(thickness.left);
            thickness.right = LayoutRound(thickness.right);
            thickness.top = LayoutRound(thickness.top);
            thickness.bottom = LayoutRound(thickness.bottom);
        }

        return thickness;
    }
    return XTHICKNESS();
}

// when item gets keyboard focus, the first brush is same as unfocused brush, the second brush is
// same as background. This function will only return the firstbrush for focused state and it will
// return the exactly same brush for both focused and unfocused. the only exception is if all other
// states are off, for focused state we use m_pFocusBorderBrush, for unfocused state we use
// m_pCalendarItemBorderBrush.
CBrush* CCalendarViewBaseItemChrome::GetItemBorderBrush(_In_ bool forFocus) const
{
    const bool isRoundedCalendarViewBaseItemChromeEnabled = IsRoundedCalendarViewBaseItemChromeEnabled();
    CBrush* pBrush = nullptr;
    auto pOwner = GetOwner();
    if (pOwner)
    {
        // Today VS Selected, Today wins
        if (m_isToday)
        {
            if (isRoundedCalendarViewBaseItemChromeEnabled)
            {
                if (!IsEnabled())
                {
                    pBrush = pOwner->m_pSelectedDisabledBorderBrush;
                }
            }
            else
            {
                // Pressed VS Hovered, Pressed wins
                if (m_isPressed)
                {
                    pBrush = pOwner->m_pTodayPressedBorderBrush;
                }
                else if (m_isHovered)
                {
                    pBrush = pOwner->m_pTodayHoverBorderBrush;
                }
            }
        }
        else if (m_isSelected && !m_isBlackout)
        {
            if (isRoundedCalendarViewBaseItemChromeEnabled && !IsEnabled())
            {
                pBrush = pOwner->m_pSelectedDisabledBorderBrush;
            }
            else if (m_isPressed) // Pressed VS Hovered, Pressed wins
            {
                pBrush = pOwner->m_pSelectedPressedBorderBrush;
            }
            else if (m_isHovered)
            {
                pBrush = pOwner->m_pSelectedHoverBorderBrush;
            }
            else
            {
                pBrush = pOwner->m_pSelectedBorderBrush;
            }
        }
        else if (m_isPressed) // Pressed VS Hovered, Pressed wins
        {
            if (!isRoundedCalendarViewBaseItemChromeEnabled || !m_isBlackout)
            {
                pBrush = pOwner->m_pPressedBorderBrush;
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemBorderBrush;
            }
        }
        else if (m_isHovered)
        {
            if (!isRoundedCalendarViewBaseItemChromeEnabled || !m_isBlackout)
            {
                pBrush = pOwner->m_pHoverBorderBrush;
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemBorderBrush;
            }
        }
        else if (forFocus)
        {
            ASSERT(!isRoundedCalendarViewBaseItemChromeEnabled);
            ASSERT(m_isKeyboardFocused);
            pBrush = pOwner->m_pFocusBorderBrush;
        }
        else
        {
            pBrush = pOwner->m_pCalendarItemBorderBrush;
        }
    }

    return pBrush;
}

// Focus Alternative border brush is same as the item background.
CBrush* CCalendarViewBaseItemChrome::GetItemFocusAltBorderBrush() const
{
    CBrush* pBrush = nullptr;
    auto pOwner = GetOwner();
    if (pOwner)
    {
        pBrush = pOwner->m_pCalendarItemBackground;
    }
    return pBrush;
}

// for Selected+Today inner border.
CBrush* CCalendarViewBaseItemChrome::GetItemInnerBorderBrush() const
{
    ASSERT(m_isToday && m_isSelected);

    auto pOwner = GetOwner();
    if (pOwner)
    {
        return pOwner->m_pTodaySelectedInnerBorderBrush;
    }
    return nullptr;
}

CBrush* CCalendarViewBaseItemChrome::GetItemBackgroundBrush() const
{
    CBrush* pBrush = nullptr;

    auto pOwner = GetOwner();
    if (pOwner)
    {
        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            if (!IsEnabled())
            {
                if (m_isToday)
                {
                    pBrush = pOwner->m_pTodayDisabledBackground;
                }
                else
                {
                    pBrush = pOwner->m_pCalendarItemDisabledBackground;
                }
            }
            else if (m_isToday)
            {
                if (m_isBlackout)
                {
                    pBrush = pOwner->m_pTodayBlackoutBackground;
                }
                else if (m_isPressed)
                {
                    pBrush = pOwner->m_pTodayPressedBackground;
                }
                else if (m_isHovered)
                {
                    pBrush = pOwner->m_pTodayHoverBackground;
                }
                else
                {
                    pBrush = pOwner->m_pTodayBackground;
                }
            }
            else if (m_isBlackout)
            {
                pBrush = pOwner->m_pBlackoutBackground;
            }
            else if (m_isOutOfScope)
            {
                if (m_isPressed)
                {
                    pBrush = pOwner->m_pCalendarItemPressedBackground;
                }
                else if (m_isHovered)
                {
                    pBrush = pOwner->m_pCalendarItemHoverBackground;
                }
                else
                {
                    pBrush = pOwner->m_pOutOfScopeBackground;
                }
            }
            else if (m_isPressed)
            {
                pBrush = pOwner->m_pCalendarItemPressedBackground;
            }
            else if (m_isHovered)
            {
                pBrush = pOwner->m_pCalendarItemHoverBackground;
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemBackground;
            }
        }
        else
        {
            if (m_isToday)
            {
                if (m_isBlackout)
                {
                    pBrush = pOwner->m_pTodayBlackoutBackground;
                }
                else
                {
                    pBrush = pOwner->m_pTodayBackground;
                }
            }
            else if (m_isOutOfScope)
            {
                pBrush = pOwner->m_pOutOfScopeBackground;
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemBackground;
            }
        }
    }
    return pBrush;
}

CBrush* CCalendarViewBaseItemChrome::GetTextBlockForeground() const
{
    CBrush* pBrush = nullptr;
    auto pOwner = GetOwner();
    if (pOwner)
    {
        if (IsRoundedCalendarViewBaseItemChromeEnabled())
        {
            if (!IsEnabled())
            {
                if (m_isToday)
                {
                    pBrush = pOwner->m_pTodayForeground;
                }
                else if (m_isSelected)
                {
                    pBrush = pOwner->m_pSelectedDisabledForeground;
                }
                else
                {
                    pBrush = pOwner->m_pDisabledForeground;
                }
            }
            else if (m_isToday)
            {
                if (m_isBlackout)
                {
                    pBrush = pOwner->m_pTodayBlackoutForeground;
                }
                else
                {
                    pBrush = pOwner->m_pTodayForeground;
                }
            }
            else if (m_isBlackout)
            {
                pBrush = pOwner->m_pBlackoutForeground;
            }
            else if (m_isSelected)
            {
                if (m_isPressed)
                {
                    pBrush = pOwner->m_pSelectedPressedForeground;
                }
                else if (m_isHovered)
                {
                    pBrush = pOwner->m_pSelectedHoverForeground;
                }
                else
                {
                    pBrush = pOwner->m_pSelectedForeground;
                }
            }
            else if (m_isPressed)
            {
                if (m_isOutOfScope)
                {
                    pBrush = pOwner->m_pOutOfScopePressedForeground;
                }
                else
                {
                    pBrush = pOwner->m_pPressedForeground;
                }
            }
            else if (m_isOutOfScope)
            {
                if (m_isHovered)
                {
                    pBrush = pOwner->m_pOutOfScopeHoverForeground;
                }
                else
                {
                    pBrush = pOwner->m_pOutOfScopeForeground;
                }
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemForeground;
            }
        }
        else
        {
            if (!IsEnabled())
            {
                pBrush = pOwner->m_pDisabledForeground;
            }
            else if (m_isToday)
            {
                pBrush = pOwner->m_pTodayForeground;
            }
            else if (m_isBlackout)
            {
                pBrush = pOwner->m_pBlackoutForeground;
            }
            else if (m_isSelected)
            {
                pBrush = pOwner->m_pSelectedForeground;
            }
            else if (m_isPressed)
            {
                pBrush = pOwner->m_pPressedForeground;
            }
            else if (m_isOutOfScope)
            {
                pBrush = pOwner->m_pOutOfScopeForeground;
            }
            else
            {
                pBrush = pOwner->m_pCalendarItemForeground;
            }
        }
    }

    return pBrush;
}

float CCalendarViewBaseItemChrome::GetTextBlockForegroundOpacity() const
{
    ASSERT(!IsRoundedCalendarViewBaseItemChromeEnabled());

    return m_isToday && m_isBlackout ? TodayBlackoutOpacity : 1.0f;
}

_Check_return_ HRESULT
CCalendarViewBaseItemChrome::CustomizeFocusRectangle(_Inout_ FocusRectangleOptions& options, _Out_ bool* shouldDrawFocusRect)
{
    bool shouldDrawDottedLines = false;
    IFC_RETURN(ShouldDrawDottedLinesFocusVisual(&shouldDrawDottedLines));
    if (shouldDrawDottedLines)
    {
        *shouldDrawFocusRect = false;
    }
    else
    {
        if (!IsRoundedCalendarViewBaseItemChromeEnabled() && (m_isSelected || m_isHovered || m_isPressed))
        {
            options.secondBrush = static_cast<CSolidColorBrush*>(GetItemBorderBrush(false /*forFocus*/));
        }

        *shouldDrawFocusRect = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
CCalendarViewBaseItemChrome::ShouldDrawDottedLinesFocusVisual(_Out_ bool* shouldDrawDottedLines)
{
    *shouldDrawDottedLines = false;

    CValue useSystemFocusVisuals;
    if (auto owner = GetOwner())
    {
        IFC_RETURN(owner->GetValue(
            owner->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_UseSystemFocusVisuals),
            &useSystemFocusVisuals));

        auto focusVisualKind = CApplication::GetFocusVisualKind();

        if (useSystemFocusVisuals.AsBool() && focusVisualKind == DirectUI::FocusVisualKind::DottedLine)
        {
            *shouldDrawDottedLines = true;
        }
    }

    return S_OK;
}

// Sets up and adds the outer border to the tree.
// Note that as a perf optimization, m_outerBorder is only created when it is not transparent. 
_Check_return_ HRESULT CCalendarViewBaseItemChrome::EnsureOuterBorder()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: EnsureOuterBorder - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (!m_outerBorder)
    {
        CValue value;
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_outerBorder.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        // Inserting the outer border into first position so it is rendered underneath the content
        IFC_RETURN(InsertChild(HasTemplateChild() ? 1 : 0, m_outerBorder.get()));

        IFC_RETURN(SetOuterBorderBrush());
        IFC_RETURN(SetOuterBorderThickness());
        IFC_RETURN(SetOuterBorderBackground());
        IFC_RETURN(SetOuterBorderCornerRadius());
    }

    return S_OK;
}

// Sets up and adds the inner border to the tree.
// Note that as a perf optimization, m_innerBorder is only created when it is not transparent. 
_Check_return_ HRESULT CCalendarViewBaseItemChrome::EnsureInnerBorder()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: EnsureInnerBorder - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (!m_innerBorder)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CBorder::Create(reinterpret_cast<CDependencyObject**>(m_innerBorder.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        // Appending the border into second position, after the outer border, so it is rendered underneath the content
        IFC_RETURN(InsertChild(HasTemplateChild() ? (m_outerBorder ? 2 : 1) : (m_outerBorder ? 1 : 0), m_innerBorder.get()));

        IFC_RETURN(SetInnerBorderBackground());
        IFC_RETURN(SetInnerBorderCornerRadius());
        IFC_RETURN(SetInnerBorderBrush());
        IFC_RETURN(SetInnerBorderThickness());
    }

    return S_OK;
}

// Sets up and adds the blackout strikethrough line to the tree.
// Note that as a perf optimization, m_strikethroughLine is only created when it is not transparent. 
_Check_return_ HRESULT CCalendarViewBaseItemChrome::EnsureStrikethroughLine()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: EnsureStrikethroughLine - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (!m_strikethroughLine)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC_RETURN(CLine::Create(reinterpret_cast<CDependencyObject**>(m_strikethroughLine.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, FALSE));

        CValue value;

        value.SetFloat(s_strikethroughThickness);

        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, value));

        // Appending the blackout strickthrough line in last position.
        IFC_RETURN(AddChild(m_strikethroughLine.get()));

        IFC_RETURN(SetBlackoutStrikethroughBrush());
        IFC_RETURN(SetBlackoutStrikethroughSize());
    }

    return S_OK;
}

// Sets the outer border Background brush based on current visual state.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetOuterBorderBackground()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetOuterBorderBackground - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    CBrush* brush = GetItemBackgroundBrush();

    if (!m_outerBorder && brush && !IsTransparentSolidColorBrush(brush))
    {
        IFC_RETURN(EnsureOuterBorder());
    }
    else if (m_outerBorder)
    {
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, brush));
    }

    return S_OK;
}

// Sets the outer border CornerRadius.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetOuterBorderCornerRadius()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetOuterBorderCornerRadius - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (!m_outerBorder)
    {
        return S_OK;
    }

    CValue value;

    IFC_RETURN(m_outerBorder->GetValueByIndex(KnownPropertyIndex::Border_CornerRadius, &value));

    XCORNERRADIUS oldCornerRadius = *(value.AsCornerRadius());
    XCORNERRADIUS newCornerRadius = GetOuterBorderCornerRadius();

    if (oldCornerRadius.bottomLeft != newCornerRadius.bottomLeft ||
        oldCornerRadius.bottomRight != newCornerRadius.bottomRight ||
        oldCornerRadius.topLeft != newCornerRadius.topLeft ||
        oldCornerRadius.topRight != newCornerRadius.topRight)
    {
        value.WrapCornerRadius(&newCornerRadius);
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));
        InvalidateRender();
    }

    return S_OK;
}

// Sets the outer border BorderThickness.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetOuterBorderThickness()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetOuterBorderThickness - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    XTHICKNESS outerBorderThickness{};
    CBrush* borderBrush = GetItemBorderBrush(false /*forFocus*/);

    if (borderBrush)
    {
        outerBorderThickness = GetItemBorderThickness();

        if (ShouldUseLayoutRounding())
        {
            outerBorderThickness.left = LayoutRound(outerBorderThickness.left);
            outerBorderThickness.right = LayoutRound(outerBorderThickness.right);
            outerBorderThickness.top = LayoutRound(outerBorderThickness.top);
            outerBorderThickness.bottom = LayoutRound(outerBorderThickness.bottom);
        }

        if (!m_outerBorder &&
            !IsTransparentSolidColorBrush(borderBrush) &&
            (outerBorderThickness.left || outerBorderThickness.right || outerBorderThickness.top || outerBorderThickness.bottom))
        {
            IFC_RETURN(EnsureOuterBorder());

            return S_OK;
        }
    }

    if (m_outerBorder)
    {
        CValue value;

        value.WrapThickness(const_cast<XTHICKNESS*>(&outerBorderThickness));
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));
    }

    return S_OK;
}

// Sets the outer border BorderBrush.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetOuterBorderBrush()
{    
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetOuterBorderBrush - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    CBrush* borderBrush = GetItemBorderBrush(false /*forFocus*/);

    if (!m_outerBorder && borderBrush && !IsTransparentSolidColorBrush(borderBrush))
    {
        IFC_RETURN(EnsureOuterBorder());
    }
    else if (m_outerBorder)
    {
        IFC_RETURN(m_outerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, borderBrush));
    }

    return S_OK;
}

// Sets the inner border Background brush.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetInnerBorderBackground()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetInnerBorderBackground - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    CValue value;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Control_Background, &value));

    ASSERT(value.GetType() == ValueType::valueObject);
    auto backgroundBrush = static_cast<CBrush*>(value.AsObject());

    if (!m_innerBorder && backgroundBrush && !IsTransparentSolidColorBrush(backgroundBrush))
    {
        IFC_RETURN(EnsureInnerBorder());
    }
    else if (m_innerBorder)
    {
        IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_Background, backgroundBrush));
    }

    return S_OK;
}

// Sets the inner border BorderBrush.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetInnerBorderBrush()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetInnerBorderBrush - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    CBrush* innerBorderBrush = m_isToday && m_isSelected ? GetItemInnerBorderBrush() : nullptr;

    if (!m_innerBorder && innerBorderBrush && !IsTransparentSolidColorBrush(innerBorderBrush))
    {
        IFC_RETURN(EnsureInnerBorder());
    }
    else if (m_innerBorder)
    {
        IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderBrush, innerBorderBrush));
    }

    return S_OK;
}

// Sets the inner border CornerRadius.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetInnerBorderCornerRadius()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetInnerBorderCornerRadius - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (!m_innerBorder)
    {
        return S_OK;
    }

    CValue value;

    IFC_RETURN(m_innerBorder->GetValueByIndex(KnownPropertyIndex::Border_CornerRadius, &value));

    XCORNERRADIUS oldCornerRadius = *(value.AsCornerRadius());
    XCORNERRADIUS newCornerRadius = GetOuterBorderCornerRadius();
    XTHICKNESS innerBorderThickness{};

    CBrush* innerBorderBrush = m_isToday && m_isSelected ? GetItemInnerBorderBrush() : nullptr;

    if (innerBorderBrush)
    {
        // Reduce the radius by the outer border thickness.
        innerBorderThickness = GetItemBorderThickness();

        if (innerBorderThickness.left)
        {
            newCornerRadius.bottomLeft = std::max(newCornerRadius.bottomLeft - innerBorderThickness.left, 0.0f);
            newCornerRadius.topLeft = std::max(newCornerRadius.topLeft - innerBorderThickness.left, 0.0f);
        }
        if (innerBorderThickness.right)
        {
            newCornerRadius.bottomRight = std::max(newCornerRadius.bottomRight - innerBorderThickness.right, 0.0f);
            newCornerRadius.topRight = std::max(newCornerRadius.topRight - innerBorderThickness.right, 0.0f);
        }
    }

    if (oldCornerRadius.bottomLeft != newCornerRadius.bottomLeft ||
        oldCornerRadius.bottomRight != newCornerRadius.bottomRight ||
        oldCornerRadius.topLeft != newCornerRadius.topLeft ||
        oldCornerRadius.topRight != newCornerRadius.topRight)
    {
        value.WrapThickness(const_cast<XTHICKNESS*>(&innerBorderThickness));
        IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Margin, value));

        value.WrapCornerRadius(&newCornerRadius);
        IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_CornerRadius, value));
        InvalidateRender();
    }

    return S_OK;
}

// Sets the inner border BorderThickness.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetInnerBorderThickness()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetInnerBorderThickness - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());
    ASSERT(m_innerBorder);

    CValue value;

    value.WrapThickness(const_cast<XTHICKNESS*>(&s_innerBorderThickness));
    IFC_RETURN(m_innerBorder->SetValueByKnownIndex(KnownPropertyIndex::Border_BorderThickness, value));

    return S_OK;
}

// Returns the CalendarView's CalendarItemCornerRadius when set or half the size.
XCORNERRADIUS CCalendarViewBaseItemChrome::GetOuterBorderCornerRadius()
{
    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    auto owner = GetOwner();

    if (owner && owner->IsCalendarItemCornerRadiusPropertySet())
    {
        return owner->m_calendarItemCornerRadius;
    }

    float cornerRadius = std::min(GetActualWidth(), GetActualHeight()) / 2.0f;
    return {cornerRadius, cornerRadius, cornerRadius, cornerRadius};
}

XCORNERRADIUS CCalendarViewBaseItemChrome::GetCornerRadius() const
{
    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::Control_CornerRadius))
    {
        CValue value;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Control_CornerRadius, &value));
        return *(value.AsCornerRadius());
    }
    else
    {
        return CFrameworkElement::GetCornerRadius();
    }
}

// Sets the outer CornerRadius.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetCornerRadius()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetCornerRadius - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    XCORNERRADIUS cornerRadius = GetOuterBorderCornerRadius();
    CValue value;

    value.WrapCornerRadius(&cornerRadius);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Control_CornerRadius, value));
    InvalidateRender();

    return S_OK;
}

// Sets the brush for the blackout strikethrough line.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetBlackoutStrikethroughBrush()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetBlackoutStrikethroughBrush - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    CBrush* brush = nullptr;

    if (m_isBlackout)
    {
        auto owner = GetOwner();

        if (owner)
        {
            if (!IsEnabled())
            {
                if (m_isToday)
                {
                    brush = owner->m_pTodayForeground;
                }
                else
                {
                    brush = owner->m_pDisabledForeground;
                }
            }
            else if (m_isToday)
            {
                brush = owner->m_pTodayForeground;
            }
            else
            {
                brush = owner->m_pBlackoutStrikethroughBrush;
            }
        }
    }

    if (!m_strikethroughLine && brush && !IsTransparentSolidColorBrush(brush))
    {
        IFC_RETURN(EnsureStrikethroughLine());
    }
    else if (m_strikethroughLine)
    {
        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, brush));
    }

    return S_OK;
}

// Sets the size and position for the blackout strikethrough line.
_Check_return_ HRESULT CCalendarViewBaseItemChrome::SetBlackoutStrikethroughSize()
{
#ifdef CVBIC_DEBUG
    if (CVBIC_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_CALENDARVIEWBASEITEMCHROME) == S_OK)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_CALENDARVIEWBASEITEMCHROME | CVBIC_DBG) /*traceType*/,
            L"CVBIC[0x%p]: SetBlackoutStrikethroughSize - entry.", this));
    }
#endif // CVBIC_DEBUG

    ASSERT(IsRoundedCalendarViewBaseItemChromeEnabled());

    if (m_strikethroughLine)
    {
        float x1 = 0.0f, y1 = 0.0f;
        float x2 = 0.0f, y2 = 0.0f;

        TextBlockFontProperties properties;

        if (GetTextBlockFontProperties(false /*isLabel*/, &properties))
        {
            float width = GetActualWidth();
            float height = GetActualHeight();

            x1 = std::max(0.0f, std::floor((width - (s_strikethroughFontSizeMultiplier * properties.fontSize)) / 2.0f));
            x2 = width - x1;

            y1 = std::max(0.0f, std::floor((height - (s_strikethroughFontSizeMultiplier * properties.fontSize)) / 2.0f));
            y2 = height - y1;
        }

        CValue value;

        value.SetFloat(x1);
        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Line_X1, value));

        value.SetFloat(y1);
        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Line_Y1, value));

        value.SetFloat(x2);
        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Line_X2, value));

        value.SetFloat(y2);
        IFC_RETURN(m_strikethroughLine->SetValueByKnownIndex(KnownPropertyIndex::Line_Y2, value));
    }

    return S_OK;
}

// day item
bool CCalendarViewDayItemChrome::GetTextBlockFontProperties(
    _In_ bool isLabel,
    _Out_ TextBlockFontProperties *pProperties)
{
    auto pOwner = GetOwner();

    if (pOwner)
    {
        pProperties->fontSize = isLabel ? pOwner->m_firstOfMonthLabelFontSize : pOwner->m_dayItemFontSize;
        pProperties->fontStyle = isLabel ? pOwner->m_firstOfMonthLabelFontStyle : pOwner->m_dayItemFontStyle;
        if (isLabel)
        {
            pProperties->fontWeight = pOwner->m_firstOfMonthLabelFontWeight;
        }
        else if (m_isToday)
        {
            pProperties->fontWeight = pOwner->m_todayFontWeight;
        }
        else
        {
            pProperties->fontWeight = pOwner->m_dayItemFontWeight;
        }
        pProperties->pFontFamilyNoRef = isLabel ? pOwner->m_pFirstOfMonthLabelFontFamily : pOwner->m_pDayItemFontFamily;
    }

    return pOwner != nullptr;
}

bool CCalendarViewDayItemChrome::GetTextBlockAlignments(
    _In_ bool isLabel,
    _Out_ TextBlockAlignments *pProperties)
{
    auto pOwner = GetOwner();

    if (pOwner)
    {
        pProperties->horizontalAlignment = isLabel ? pOwner->m_horizontalFirstOfMonthLabelAlignment : pOwner->m_horizontalDayItemAlignment;
        pProperties->verticalAlignment = isLabel ? pOwner->m_verticalFirstOfMonthLabelAlignment : pOwner->m_verticalDayItemAlignment;
    }

    return pOwner != nullptr;
}

XTHICKNESS CCalendarViewDayItemChrome::GetTextBlockMargin(
    bool isLabel) const
{
    auto owner = GetOwner();

    if (owner)
    {
        return isLabel ? owner->m_firstOfMonthLabelMargin : owner->m_dayItemMargin;
    }

    return XTHICKNESS();
}

//month year item
bool CCalendarViewItemChrome::GetTextBlockFontProperties(
    _In_ bool isLabel,
    _Out_ TextBlockFontProperties *pProperties)
{
    auto pOwner = GetOwner();

    if (pOwner)
    {
        pProperties->fontSize = isLabel ? pOwner->m_firstOfYearDecadeLabelFontSize : pOwner->m_monthYearItemFontSize;
        pProperties->fontStyle = isLabel ? pOwner->m_firstOfYearDecadeLabelFontStyle : pOwner->m_monthYearItemFontStyle;
        if (isLabel)
        {
            pProperties->fontWeight = pOwner->m_firstOfYearDecadeLabelFontWeight;
        }
        else if (m_isToday)
        {
            pProperties->fontWeight = pOwner->m_todayFontWeight;
        }
        else
        {
            pProperties->fontWeight = pOwner->m_monthYearItemFontWeight;
        }
        pProperties->pFontFamilyNoRef = isLabel ? pOwner->m_pFirstOfYearDecadeLabelFontFamily : pOwner->m_pMonthYearItemFontFamily;
    }

    return pOwner != nullptr;
}

bool CCalendarViewItemChrome::GetTextBlockAlignments(
    _In_ bool isLabel,
    _Out_ TextBlockAlignments *pAlignments)
{
    pAlignments->horizontalAlignment = DirectUI::HorizontalAlignment::Center;
    pAlignments->verticalAlignment = isLabel ? DirectUI::VerticalAlignment::Top : DirectUI::VerticalAlignment::Center;

    return GetOwner() != nullptr;
}

XTHICKNESS CCalendarViewItemChrome::GetTextBlockMargin(
    bool isLabel) const
{
    auto owner = GetOwner();

    if (owner)
    {
        return isLabel ? owner->m_firstOfYearDecadeLabelMargin : owner->m_monthYearItemMargin;
    }

    return XTHICKNESS();
}
