// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "layoutelement.h"
#include "FloatUtil.h"
#include "RootScale.h"

CLayoutElement::CLayoutElement(_In_ CCoreServices *core)
    : CUIElement(core)
{
}

_Check_return_ HRESULT
CLayoutElement::ActualWidth(
    _In_ CDependencyObject *object,
    _In_ unsigned int numArgs,
    _In_reads_(numArgs) CValue *args,
    _In_opt_ IInspectable* /*valueOuter*/,
    _Out_ CValue *result)
{
#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (numArgs == 0 && args == nullptr && result != nullptr)
    {
        // Getter code path.
        auto element = static_cast<CLayoutElement*>(object);
        result->SetFloat(element->GetActualWidth());
    }
    else
    {
        // Setter code path - which is disallowed.
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

_Check_return_ HRESULT
CLayoutElement::ActualHeight(
    _In_ CDependencyObject *object,
    _In_ unsigned int numArgs,
    _In_reads_(numArgs) CValue *args,
    _In_opt_ IInspectable* /*valueOuter*/,
    _Out_ CValue *result)
{
#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (numArgs == 0 && args == nullptr && result != nullptr)
    {
        // Getter code path.
        auto element = static_cast<CLayoutElement*>(object);
        result->SetFloat(element->GetActualHeight());
    }
    else
    {
        // Setter code path - which is disallowed.
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
_Check_return_ HRESULT
CLayoutElement::MeasureCore(XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    // Subtract the margins from the available size
    float roundedMarginWidth = m_margin.left + m_margin.right;
    float roundedMarginHeight = m_margin.top + m_margin.bottom;

    // Layout round the margins to match ArrangeCore
    if (GetUseLayoutRounding())
    {
        roundedMarginWidth = LayoutRound(roundedMarginWidth);
        roundedMarginHeight = LayoutRound(roundedMarginHeight);
    }

    // We check to see if availableSize.width and availableSize.height are finite since that will
    // also protect against NaN getting in.
    XSIZEF frameworkAvailableSize = {
        IsFiniteF(availableSize.width) ? std::max(availableSize.width - roundedMarginWidth, 0.0f) : XFLOAT_INF,
        IsFiniteF(availableSize.height) ? std::max(availableSize.height - roundedMarginHeight, 0.0f) : XFLOAT_INF
    };

    // Adjust available size by Min/Max Width/Height
    float minWidth = 0.f, maxWidth = 0.f, minHeight = 0.f, maxHeight = 0.0f;
    GetMinMaxSize(minWidth, maxWidth, minHeight, maxHeight);

    frameworkAvailableSize.width = std::max(minWidth, std::min(frameworkAvailableSize.width, maxWidth));
    frameworkAvailableSize.height = std::max(minHeight, std::min(frameworkAvailableSize.height, maxHeight));

    if (GetUseLayoutRounding())
    {
        // If we subtracted off margins, layout round with Floor the availableSize to
        // ensure we don't end up measuring and rounding up beyond the real available space.
        if (roundedMarginWidth > 0 && IsFiniteF(frameworkAvailableSize.width))
        {
            frameworkAvailableSize.width = LayoutRoundFloor(frameworkAvailableSize.width);
        }
        if (roundedMarginHeight > 0 && IsFiniteF(frameworkAvailableSize.height))
        {
            frameworkAvailableSize.height = LayoutRoundFloor(frameworkAvailableSize.height);
        }
    }

    if (EventEnabledMeasureOverrideBegin())
    {
        TraceMeasureOverrideBegin(reinterpret_cast<XUINT64>(this));
    }

    IFC_RETURN(MeasureWorker(frameworkAvailableSize, desiredSize));

    if (EventEnabledMeasureOverrideEnd())
    {
        TraceMeasureOverrideEnd(reinterpret_cast<XUINT64>(this));
    }

    // Maximize desired size with user provided min size. It's also possible that MeasureOverride returned NaN for either
    // width or height, in which case we should use the min size as well.
    desiredSize.width = std::max(desiredSize.width, minWidth);
    if (DirectUI::FloatUtil::IsNaN(desiredSize.width))
    {
        desiredSize.width = minWidth;
    }

    desiredSize.height = std::max(desiredSize.height, minHeight);
    if (DirectUI::FloatUtil::IsNaN(desiredSize.height))
    {
        desiredSize.height = minHeight;
    }

    // We need to round now since we save the values off, and use them to determine
    // if a layout clip will be applied.
    if (GetUseLayoutRounding())
    {
        desiredSize.width = LayoutRound(desiredSize.width);
        desiredSize.height = LayoutRound(desiredSize.height);
    }

    // Here is the "true minimum" desired size - the one that is
    // for sure enough for the control to render its content.
    IFC_RETURN(EnsureLayoutStorage());
    UnclippedDesiredSize = desiredSize;

    // Clamp desired size to our max size.
    if (desiredSize.width > maxWidth)
    {
        desiredSize.width = maxWidth;
    }

    if (desiredSize.height > maxHeight)
    {
        desiredSize.height = maxHeight;
    }

    // Because of negative margins, clipped desired size may be negative.
    // Need to keep it as XFLOATS for that reason and maximize with 0 at the
    // very last point - before returning desired size to the parent.

    float clippedDesiredWidth = desiredSize.width + roundedMarginWidth;
    float clippedDesiredHeight = desiredSize.height + roundedMarginHeight;

    // In overconstrained scenario, parent wins and measured size of the child,
    // including any sizes set or computed, can not be larger then
    // available size. We will clip the guy later.
    if (clippedDesiredWidth > availableSize.width)
    {
        clippedDesiredWidth = availableSize.width;
    }

    if (clippedDesiredHeight > availableSize.height)
    {
        clippedDesiredHeight = availableSize.height;
    }

    //  Note: unclippedDesiredSize is needed in ArrangeCore,
    //  because due to the layout protocol, arrange should be called
    //  with constraints greater or equal to child's desired size
    //  returned from MeasureOverride. But in most circumstances
    //  it is possible to reconstruct original unclipped desired size.

    desiredSize.width = std::max(0.f, clippedDesiredWidth);
    desiredSize.height = std::max(0.f, clippedDesiredHeight);

    // We need to round again in case the desired size has been modified since we originally
    // rounded it.
    if (GetUseLayoutRounding())
    {
        desiredSize.width = LayoutRound(desiredSize.width);
        desiredSize.height = LayoutRound(desiredSize.height);
    }

    return S_OK;
}

_Check_return_ HRESULT CLayoutElement::ArrangeCore(XRECTF finalRect)
{
    bool needsClipBounds = false;

    XSIZEF arrangeSize = finalRect.Size();

    float marginWidth = m_margin.left + m_margin.right;
    float marginHeight = m_margin.top + m_margin.bottom;

    XPOINTF oldOffset = {};

    IFC_RETURN(EnsureLayoutStorage());

    auto unclippedDesiredSize = UnclippedDesiredSize;
    auto oldRenderSize = RenderSize;

    XSIZEF arrangeSizeWithoutMargin = {
        std::max(arrangeSize.width - marginWidth, 0.0f),
        std::max(arrangeSize.height - marginHeight, 0.0f)
    };

    float roundedMarginWidth = marginWidth;
    float roundedMarginHeight = marginHeight;

    if (GetUseLayoutRounding())
    {
        roundedMarginWidth = LayoutRound(marginWidth);
        roundedMarginHeight = LayoutRound(marginHeight);
    }

    float arrangeWidthWithoutRoundedMargin = std::max(arrangeSize.width - roundedMarginWidth, 0.0f);
    marginWidth = roundedMarginWidth;
    arrangeSize.width = arrangeWidthWithoutRoundedMargin;

    float arrangeHeightWithoutRoundedMargin = std::max(arrangeSize.height - roundedMarginHeight, 0.0f);
    marginHeight = roundedMarginHeight;
    arrangeSize.height = arrangeHeightWithoutRoundedMargin;

    if (IsLessThanReal(arrangeSize.width, unclippedDesiredSize.width))
    {
        needsClipBounds = true;
        arrangeSize.width = unclippedDesiredSize.width;
    }

    if (IsLessThanReal(arrangeSize.height, unclippedDesiredSize.height))
    {
        needsClipBounds = true;
        arrangeSize.height = unclippedDesiredSize.height;
    }

    // Alignment==Stretch --> arrange at the slot size minus margins
    // Alignment!=Stretch --> arrange at the unclippedDesiredSize
    if (m_horizontalAlignment != DirectUI::HorizontalAlignment::Stretch)
    {
        arrangeSize.width = unclippedDesiredSize.width;
    }

    if (m_verticalAlignment != DirectUI::VerticalAlignment::Stretch)
    {
        arrangeSize.height = unclippedDesiredSize.height;
    }

    float minWidth = 0.f, maxWidth = 0.f, minHeight = 0.f, maxHeight = 0.0f;
    GetMinMaxSize(minWidth, maxWidth, minHeight, maxHeight);

    // We have to choose max between UnclippedDesiredSize and Max here, because
    // otherwise setting of max property could cause arrange at less then unclippedDS.
    // Clipping by Max is needed to limit stretch here

    float effectiveMaxWidth = std::max(unclippedDesiredSize.width, maxWidth);
    if (IsLessThanReal(effectiveMaxWidth, arrangeSize.width))
    {
        needsClipBounds = true;
        arrangeSize.width = effectiveMaxWidth;
    }

    float effectiveMaxHeight = std::max(unclippedDesiredSize.height, maxHeight);
    if (IsLessThanReal(effectiveMaxHeight, arrangeSize.height))
    {
        needsClipBounds = true;
        arrangeSize.height = effectiveMaxHeight;
    }

    if (EventEnabledArrangeOverrideBegin())
    {
        TraceArrangeOverrideBegin(reinterpret_cast<XUINT64>(this));
    }

    XSIZEF innerInkSize = {};
    IFC_RETURN(ArrangeWorker(arrangeSize, innerInkSize));

    if (EventEnabledArrangeOverrideEnd())
    {
        TraceArrangeOverrideEnd(reinterpret_cast<XUINT64>(this));
    }

    // Here we use un-clipped InkSize because element does not know that it is
    // clipped by layout system and it shoudl have as much space to render as
    // it returned from its own ArrangeOverride
    // Inner ink size is not guaranteed to be rounded, but should be.
    // TODO: inner ink size currently only rounded if plateau > 1 to minimize impact in RC,
    // but should be consistently rounded in all plateaus.
    const auto scale = RootScale::GetRasterizationScaleForElement(this);
    if ((scale != 1.0f) && GetUseLayoutRounding())
    {
        innerInkSize.width = LayoutRound(innerInkSize.width);
        innerInkSize.height = LayoutRound(innerInkSize.height);
    }

    RenderSize = innerInkSize;

    if (!IsSameSize(oldRenderSize, innerInkSize))
    {
        (VisualTree::GetLayoutManagerForElement(this))->EnqueueForSizeChanged(this, oldRenderSize);
    }

    // ClippedInkSize differs from InkSize only what MaxWidth/Height explicitly clip the
    // otherwise good arrangement. For ex, DS<clientSize but DS>MaxWidth - in this
    // case we should initiate clip at MaxWidth and only show Top-Left portion
    // of the element limited by Max properties. It is Top-left because in case when we
    // are clipped by container we also degrade to Top-Left, so we are consistent.
    XSIZEF clippedInkSize = {
        std::min(innerInkSize.width, maxWidth),
        std::min(innerInkSize.height, maxHeight)
    };

    // remember we have to clip if Max properties limit the inkSize
    needsClipBounds |=
        IsLessThanReal(clippedInkSize.width, innerInkSize.width)
        || IsLessThanReal(clippedInkSize.height, innerInkSize.height);

    // Note that inkSize now can be bigger then layoutSlotSize-margin (because of layout
    // squeeze by the parent or LayoutConstrained=true, which clips desired size in Measure).

    // The client size is the size of layout slot decreased by margins.
    // This is the "window" through which we see the content of the child.
    // Alignments position ink of the child in this "window".
    // Max with 0 is necessary because layout slot may be smaller then unclipped desired size.

    XSIZEF clientSize = {
        std::max(0.0f, finalRect.Width - marginWidth),
        std::max(0.0f, finalRect.Height - marginHeight)
    };

    // Remember we have to clip if clientSize limits the inkSize
    needsClipBounds |=
        IsLessThanReal(clientSize.width, clippedInkSize.width)
        || IsLessThanReal(clientSize.height, clippedInkSize.height);

    VisualOffset.x = finalRect.X;
    VisualOffset.y = finalRect.Y;

    // If the VisualOffset or RenderSize has changed, this element should be redrawn.
    {
        // Note: a similar check occurs in CUIElement::ArrangeInternal which calls this method, but the check there is between
        // the size passed in and the final size. The comparison here is between the previous size and the final size.

        if (oldRenderSize.width != RenderSize.width || oldRenderSize.height != RenderSize.height)
        {
            // Mark this element's rendering content as dirty.
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        }

        if (VisualOffset.x != oldOffset.x || VisualOffset.y != oldOffset.y
            || ((oldRenderSize.width != RenderSize.width) && this->IsRightToLeft()))
        {
            // Mark this element's transform as dirty.
            CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        }
    }

    SetRequiresClip(static_cast<XUINT32>(needsClipBounds));

    return S_OK;
}

_Check_return_ HRESULT CLayoutElement::MeasureWorker(XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    desiredSize.width = desiredSize.height = 0;

    auto children = static_cast<CUIElementCollection*>(GetChildren());
    if (children)
    {
        for (auto& childDO : (*children))
        {
            auto child = static_cast<CUIElement*>(childDO);
            ASSERT(child);

            IFC_RETURN(child->Measure(availableSize));

            IFC_RETURN(child->EnsureLayoutStorage());
            const auto& childDesiredSize = child->GetLayoutStorage()->m_desiredSize;

            desiredSize.width = std::max(desiredSize.width, childDesiredSize.width);
            desiredSize.height = std::max(desiredSize.height, childDesiredSize.height);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CLayoutElement::ArrangeWorker(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    newFinalSize = finalSize;

    auto children = static_cast<CUIElementCollection*>(GetChildren());
    if (children)
    {
        for (auto& childDO : (*children))
        {
            auto child = static_cast<CUIElement*>(childDO);
            ASSERT(child);

            IFC_RETURN(child->EnsureLayoutStorage());
            const auto& childDesiredSize = child->GetLayoutStorage()->m_desiredSize;

            XRECTF innerRect = {
                0, 0,
                std::max(finalSize.width, childDesiredSize.width),
                std::max(finalSize.height, childDesiredSize.height)
            };

            IFC_RETURN(child->Arrange(innerRect));
        }
    }

    return S_OK;
}

float CLayoutElement::GetActualWidth()
{
    float width = m_width;

    if (_isnan(width))
    {
        // If the Width is not set, but MinWidth is set
        // we set width to MinWidth, This way the next line would return
        // the proper value
        width = std::min(m_minWidth, XFLOAT_INF);
    }

    // If the element is measure dirty and at the same time, doesn't
    // have a layout storage, this means that it has not been measured
    // due to its parent invisibility.
    // In such case, we should return 0.0f as the actual width.
    // Note that this condition can hold only if parent has never been
    // visible (Initially, Its Visibility is set to Collapsed).
    if (GetIsMeasureDirty() && !HasLayoutStorage())
    {
        width = 0.0f;
    }
    else if (HasLayoutStorage())
    {
        width = RenderSize.width;
    }
    else
    {
        width = std::min(std::max(width, m_minWidth), m_maxWidth);
    }

    return width;
}

float CLayoutElement::GetActualHeight()
{
    float height = m_height;

    if (_isnan(height))
    {
        // If the Width is not set, but MinHeight is set
        // we set width to MinHeight, This way the next line would return
        // the proper value
        height = std::min(m_minHeight, XFLOAT_INF);
    }

    // If the element is measure dirty and at the same time, doesn't
    // have a layout storage, this means that it has not been measured
    // due to its parent invisibility.
    // In such case, we should return 0.0f as the actual height.
    // Note that this condition can hold only if parent has never been
    // visible (Initially, Its Visibility is set to Collapsed).
    if (GetIsMeasureDirty() && !HasLayoutStorage())
    {
        height = 0.0f;
    }
    else if (HasLayoutStorage())
    {
        height = RenderSize.height;
    }
    else
    {
        height = std::min(std::max(height, m_minHeight), m_maxHeight);
    }

    return height;
}

void CLayoutElement::GetMinMaxSize(float& minWidth, float& maxWidth, float& minHeight, float& maxHeight)
{
    bool isDefaultWidth = (_isnan(m_width) || IsPropertyDefaultByIndex(GetWidthProperty()));
    bool isDefaultHeight = (_isnan(m_height) || IsPropertyDefaultByIndex(GetHeightProperty()));

    // Calculate min/max width.
    minWidth = std::max(std::min((isDefaultWidth ? 0 : m_width), m_maxWidth), m_minWidth);
    maxWidth = std::max(std::min((isDefaultWidth ? XFLOAT_INF : m_width), m_maxWidth), m_minWidth);

    // Calculate min/max height
    minHeight = std::max(std::min((isDefaultHeight ? 0 : m_height), m_maxHeight), m_minHeight);
    maxHeight = std::max(std::min((isDefaultHeight ? XFLOAT_INF : m_height), m_maxHeight), m_minHeight);

    if (GetUseLayoutRounding())
    {
        minWidth = LayoutRound(minWidth);

        if (IsFiniteF(maxWidth))
        {
            maxWidth = LayoutRound(maxWidth);
        }

        minHeight = LayoutRound(minHeight);

        if (IsFiniteF(maxHeight))
        {
            maxHeight = LayoutRound(maxHeight);
        }
    }
}
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
