// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CDependencyObject.h>
#include <UIElement.h>
#include <CControl.h>

#include <corep.h>

#include <GeneralTransform.h>
#include <Framework.h>
#include <EventArgs.h>
#include <Application.h>
#include <RichTextBlockView.h>
#include <LayoutTransitionElement.h>
#include <Border.h>
#include <DOCollection.h>
#include <Geometry.h>
#include <shape.h>
#include <line.h>
#include <Rectangle.h>
#include <ContentControl.h>
#include <ScrollViewer.h>
#include <Panel.h>
#include <RootVisual.h>
#include <Canvas.h>
#include <TransitionRoot.h>
#include <Transforms.h>
#include <CornerRadius.h>

#include <FocusRectangle.h>

// Text
#include <Inline.h>
#include <FocusableHelper.h>
#include <TextBlock.h>
#include <RichTextBlock.h>

#include <ListViewBaseItemChrome.h>
#include <CalendarViewBaseItemChrome.h>

#include <UIElementCollection.h>
#include <DoubleCollection.h>

#include <DOPointerCast.h>

#include <HWWalk.h>
#include <ContentRenderer.h>

#include "FocusRectManager.h"
#include "FocusRectHelper.h"
#include "FocusRectNudging.h"
#include "FocusRectOptions.h"
#include <DCompTreeHost.h>
#include <FeatureFlags.h>

#include "RevealFocusSource.h"
#include "RevealFocusAnimator.h"
#include "RevealMotion.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"
#include "ColorUtil.h"

using namespace FocusRect;

void AdjustCornerRadius(XCORNERRADIUS& cornerRadius, XTHICKNESS& adjustment)
{
    if (cornerRadius.topLeft != 0)
    {
        cornerRadius.topLeft -= (adjustment.top + adjustment.left) * 0.5f;
        if (cornerRadius.topLeft < 0) cornerRadius.topLeft = 0;
    }
    if (cornerRadius.topRight != 0)
    {
        cornerRadius.topRight -= (adjustment.top + adjustment.right) * 0.5f;
        if (cornerRadius.topRight < 0) cornerRadius.topRight = 0;
    }
    if (cornerRadius.bottomRight != 0)
    {
        cornerRadius.bottomRight -= (adjustment.bottom + adjustment.right) * 0.5f;
        if (cornerRadius.bottomRight < 0) cornerRadius.bottomRight = 0;
    }
    if (cornerRadius.bottomLeft != 0)
    {
        cornerRadius.bottomLeft -= (adjustment.bottom + adjustment.left) * 0.5f;
        if (cornerRadius.bottomLeft < 0) cornerRadius.bottomLeft = 0;
    }
}

CFocusRectManager::~CFocusRectManager()
{
    const bool isDeviceLost = false, cleanupDComp = false, clearPCData = true;
    ReleaseResources(isDeviceLost, cleanupDComp, clearPCData);
}

// Helper to get the margin for an element
XTHICKNESS
CFocusRectManager::GetFocusVisualMargin(
    _In_ CDependencyObject* element,
    _In_opt_ CDependencyObject* target)
{
    XTHICKNESS margin;

    if (auto elementAsFE = do_pointer_cast<CFrameworkElement>(element))
    {
        CValue value;
        auto pFocusTargetDescendantFE = do_pointer_cast<CFrameworkElement>(target);
        auto pValueFENoRef = elementAsFE->FixValueGetterForPropertyNoRef(pFocusTargetDescendantFE, KnownPropertyIndex::FrameworkElement_FocusVisualMargin);
        IFCFAILFAST(pValueFENoRef->GetValueByIndex(KnownPropertyIndex::FrameworkElement_FocusVisualMargin, &value));
        margin = *(value.AsThickness());
    }
    else if (CFocusableHelper::IsFocusableDO(element))
    {
        Focusable focusTarget(target);
        std::vector<XRECTF> boundsVector;

        if (!focusTarget.IsNull())
        {
            focusTarget.GetBounds(&boundsVector);
        }

        // These values for a link's focus visual margin is hardcoded, from design team.
        // There's currently no way to set them from the public API surface.
        if (boundsVector.size() > 1)
        {
            margin = { -3.0f, -1.0f, -3.0f, -1.0f };
        }
        else
        {
            margin = { -3.0f, -3.0f, -3.0f, -3.0f };
        }
    }
    else
    {
        // We should not receive an element that isn't a FrameworkElement or Hyperlink
        XAML_FAIL_FAST();
    }

    return margin;
}

// For the given element and target's properties, populate the FocusRectangleOptions struct and set finalMargin
/*static*/ void
CFocusRectManager::GetFocusOptionsForElement(
    _In_ const Focusable& focusedElement,
    _In_ const Focusable& focusTarget,
    _In_ bool hasMultipleFocusRects,
    _In_ const bool useRevealFocus,
    _Out_ FocusRectangleOptions* options,
    _Out_ XTHICKNESS* finalMargin)
{
    xref_ptr<CBrush> secondaryBrush;
    xref_ptr<CBrush> primaryBrush;
    XTHICKNESS secondaryThickness = {};
    XTHICKNESS primaryThickness = {};

    *options = FocusRectangleOptions();

    *finalMargin = GetFocusVisualMargin(focusedElement.Object, focusTarget.Object);

    if (focusedElement.IsFrameworkElement())
    {
        CUIElement* const focusTargetDescendant = focusTarget.Object == focusedElement.Object ? nullptr : focusTarget.AsFrameworkElement();
        IFCFAILFAST(focusedElement.AsFrameworkElement()->GetFocusVisualProperties(
            focusTargetDescendant,
            primaryBrush.ReleaseAndGetAddressOf(),
            secondaryBrush.ReleaseAndGetAddressOf(),
            &primaryThickness,
            &secondaryThickness));

        // The primary brush/thickness is for the outer stroke, and the secondary
        // brush/thickness is for the inner stroke
        options->drawFirst = true;
        options->firstThickness = primaryThickness;
        FAIL_FAST_ASSERT(primaryBrush->GetTypeIndex() == KnownTypeIndex::SolidColorBrush); // Only SolidColorBrush supported!
        options->firstBrush = static_sp_cast<CSolidColorBrush>(std::move(primaryBrush));

        options->drawSecond = true;
        options->secondThickness = secondaryThickness;
        FAIL_FAST_ASSERT(secondaryBrush->GetTypeIndex() == KnownTypeIndex::SolidColorBrush); // Only SolidColorBrush supported!
        options->secondBrush = static_sp_cast<CSolidColorBrush>(std::move(secondaryBrush));

        options->isContinuous = true;
        options->UseElementBounds(focusedElement.AsFrameworkElement());

        // We won't use rounded corners under reveal or dotted line.  This is still up for discussion but for now, we don't.
        // We also currently only use rounded corners for things base upon Control. 
        if (AreHighVisibilityFocusRectsEnabled() && !AreRevealFocusRectsEnabled())
        {
            KnownPropertyIndex cornerRadiusPropertyIndex =
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::Border>() ? KnownPropertyIndex::Border_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::ContentPresenter>() ? KnownPropertyIndex::ContentPresenter_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::Control>() ? KnownPropertyIndex::Control_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::Grid>() ? KnownPropertyIndex::Grid_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::RelativePanel>() ? KnownPropertyIndex::RelativePanel_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::StackPanel>() ? KnownPropertyIndex::StackPanel_CornerRadius :
                focusTarget.Object->OfTypeByIndex<KnownTypeIndex::Panel>() ? KnownPropertyIndex::Panel_CornerRadiusProtected :
                KnownPropertyIndex::UnknownType_UnknownProperty;

            if (cornerRadiusPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty) {
                CValue value;
                IFCFAILFAST(focusTarget.Object->GetValueByIndex(cornerRadiusPropertyIndex, &value));
                options->cornerRadius = *value.AsCornerRadius();

                // We need to adjust the corner radius based upon our margin which affects where the focus rect
                // will be rendered.  The objective is to keep the length of the straight edges of the focus
                // element and the two border rectangles the same length.  Applying the margin will adjust for
                // the outer border and when we render, we will adjust it again for the inner border.
                AdjustCornerRadius(options->cornerRadius, *finalMargin);
            }
        }
 
        options->drawReveal = useRevealFocus;
        if (options->drawReveal)
        {
            AddRevealFocusOptions(*options, focusTarget);
        }
    }
    else if (focusedElement.IsFocusableElement())
    {
        GetDefaultFocusOptionsForLink(focusedElement.Object->GetContext(), options);
    }
    else
    {
        // Can this actually happen?
        GetDefaultFocusOptions(focusedElement.Object->GetContext(), options);
    }
}


// Helper to call CustomizeFocusRectangle for the types that support it
/*static*/ void
CFocusRectManager::CallCustomizationFunction(
    _In_ CDependencyObject* focusTarget,
    _Inout_ FocusRectangleOptions& options,
    _Out_ bool* shouldDrawFocusRect)
{
    CDependencyObject* focusTargetOrChrome = focusTarget;

    // Special case: for items with chrome, the chrome is responsible for drawing the focus rect
    auto typeIndex = focusTarget->GetTypeIndex();
    if (typeIndex == KnownTypeIndex::ListViewItem || typeIndex == KnownTypeIndex::GridViewItem)
    {
        auto elementAsContentControl = static_cast<CContentControl*>(focusTarget);
        auto chrome = elementAsContentControl->m_pListViewBaseItemChrome;
        if (chrome)
        {
            focusTargetOrChrome = chrome;
        }
    }

    // Call into customization functions to let chrome tweak the values, or opt out of drawing the focus rect at all
    CallCustomizationFunctionIfExists<CListViewBaseItemChrome>(focusTargetOrChrome, options, shouldDrawFocusRect);
    CallCustomizationFunctionIfExists<CCalendarViewBaseItemChrome>(focusTargetOrChrome, options, shouldDrawFocusRect);
}


// Fill in options and boundsVector given the focused element, focus target, and focus host.  Here's where we really
// decide where we're going to draw the focus rect(s).  Results are in the options struct and boundsVector.
/*static*/ void
CFocusRectManager::DetermineRenderOptions(
    _In_ const Focusable& focusedElement,
    _In_ const Focusable& focusTarget,
    _In_ const FocusRectHost& focusHost,
    _In_ const Focusable& previousFocusTarget,
    _In_ bool useRevealFocus,
    _Inout_ FocusRectangleOptions& options,
    _Inout_ std::vector<XRECTF>& boundsVector)
{
    XTHICKNESS focusRectMargin;
    std::vector<XRECTF> multipleBoundsVector;

    const XRECTF elementBounds = focusTarget.GetBounds(&multipleBoundsVector);

    // Options will be reset in call to GetFocusOptionsForElement, don't give any state
    GetFocusOptionsForElement(focusedElement, focusTarget,  multipleBoundsVector.size() > 1, useRevealFocus, &options, &focusRectMargin);
    XRECTF focusRectBoundsWithNudging = {};
    if (FAILED(GetFocusRectWithNudging(
        focusTarget.GetElementResponsibleForDrawingThisElement(),
        elementBounds,
        focusHost,
        focusRectMargin,
        options.isRevealBorderless,
        focusRectBoundsWithNudging)))
    {
        return;
    }

    // Hyperlink can require multiple focus rectangles
    if (multipleBoundsVector.size() > 0)
    {
        for (XRECTF& rect : multipleBoundsVector)
        {
            ASSERT(!options.isRevealBorderless); // Hyperlink don't support RevealFocus
            XRECTF focusRectBoundsForHyperlinkWithNudging = {};
            if (FAILED(GetFocusRectWithNudging(
                focusTarget.GetElementResponsibleForDrawingThisElement(),
                rect,
                focusHost,
                focusRectMargin,
                options.isRevealBorderless,
                focusRectBoundsForHyperlinkWithNudging)))
            {
                return;
            }
            rect = focusRectBoundsForHyperlinkWithNudging;
        }
    }

    if (!previousFocusTarget.IsNull())
    {
        xref_ptr<CGeneralTransform> previousToCurrent;
        previousFocusTarget.GetElementResponsibleForDrawingThisElement()->TransformToVisual(
            focusTarget.GetElementResponsibleForDrawingThisElement(),
            &previousToCurrent);

        if(FAILED (previousToCurrent->TransformRect(focusRectBoundsWithNudging, &options.previousBounds)))
        {
            return;
        }
    }
    else
    {
        // No previous target, we won't be able to animate the reveal border, oh well!
        options.previousBounds = focusRectBoundsWithNudging;
    }

    options.bounds = focusRectBoundsWithNudging;

    bool shouldDrawFocusRect = true;
    CallCustomizationFunction(focusTarget.Object, options, &shouldDrawFocusRect);

    if (!shouldDrawFocusRect)
    {
        return;
    }

    boundsVector = std::move(multipleBoundsVector);

    if (boundsVector.empty())
    {
        boundsVector.push_back(focusRectBoundsWithNudging);
    }
}

// Set up FocusRectangleOptions for a focusable element
/*static*/void
CFocusRectManager::GetDefaultFocusOptionsForLink(
    _In_ CCoreServices* core,
    _Out_ FocusRectangleOptions* newOptions)
{
    CDependencyObject* baseHighBrush = nullptr;
    CDependencyObject* altHighBrush = nullptr;

    // Brushes and corner radius are treated differently.  In the case of brushes the DO is
    // stored in the options, so we transfer the reference from the value to options.  However,
    // the corner radius in the options is a structure so we transfer the value.  Hence we need
    // to make sure the refernce in the value is released.
    xref_ptr<CDependencyObject> cornerRadius;

    IFCFAILFAST(core->LookupThemeResource(
        XSTRING_PTR_EPHEMERAL(L"SystemControlForegroundBaseHighBrush"),
        &baseHighBrush));

    IFCFAILFAST(core->LookupThemeResource(
        XSTRING_PTR_EPHEMERAL(L"SystemControlForegroundAltHighBrush"),
        &altHighBrush));

    IFCFAILFAST(core->LookupThemeResource(
        XSTRING_PTR_EPHEMERAL(L"HyperlinkFocusRectCornerRadius"),
        cornerRadius.ReleaseAndGetAddressOf()));

    newOptions->firstBrush.attach(static_cast<CSolidColorBrush*>(baseHighBrush));
    newOptions->secondBrush.attach(static_cast<CSolidColorBrush*>(altHighBrush));

    if (cornerRadius)
    {
        newOptions->cornerRadius = do_pointer_cast<CCornerRadius>(cornerRadius.get())->m_cornerRadius;
    }

    newOptions->firstThickness = Thickness(2.0f);
    newOptions->secondThickness = Thickness(1.0f);
    newOptions->drawFirst = true;
    newOptions->drawSecond = true;
    newOptions->isContinuous = true;
}


// Set up FocusRectangleOptions for Win10 TH2 focus rects, "dotted-line" or "marching ants" mode
/*static*/void
CFocusRectManager::GetDefaultFocusOptions(
    _In_ CCoreServices* core,
    _Out_ FocusRectangleOptions* newOptions)
{
    CValue blackColorValue;
    blackColorValue.SetColor(0xFF000000);
    CREATEPARAMETERS cpBlackColor(core, blackColorValue);
    CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(newOptions->firstBrush.ReleaseAndGetAddressOf()), &cpBlackColor);

    CValue whiteColorValue;
    whiteColorValue.SetColor(0xFFFFFFFF);
    CREATEPARAMETERS cpWhiteColor(core, whiteColorValue);
    CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(newOptions->secondBrush.ReleaseAndGetAddressOf()), &cpWhiteColor);

    newOptions->firstThickness = Thickness(1.0f);
    newOptions->secondThickness = Thickness(1.0f);
    newOptions->drawFirst = true;
    newOptions->drawSecond = true;
}

// Set m_renderOptions and m_elementDrawingFocusRect to Win10 TH2 behavior, "dotted-line" or "marching ants" mode
/*static*/ bool
CFocusRectManager::SetLegacyRenderOptions(
    _In_ const Focusable& focusTarget,
    _Out_ FocusRectangleOptions* options)
{
    GetDefaultFocusOptions(focusTarget.Object->GetContext(), options);

    bool shouldDrawFocusRect = true;
    CallCustomizationFunction(focusTarget.Object, *options, &shouldDrawFocusRect);

    if (!shouldDrawFocusRect)
    {
        return false;
    }

    return true;
}

// Set up the given CBorder with the other given parameters. Returns a bool indicating
// if the focus element was dirtied
/*static*/ bool
CFocusRectManager::ConfigureFocusElement(
    _In_ CBorder* border,
    _In_ FocusElementType type,
    _In_ FocusRectangleOptions& options,
    _In_ XRECTF outerBounds)
{
    XTHICKNESS* thickness = (type == FocusElementType::Outer ? &options.firstThickness : &options.secondThickness);
    xref_ptr<CSolidColorBrush> brush = (type == FocusElementType::Outer ? options.firstBrush : options.secondBrush);
    bool focusRectDirty = false;

    XRECTF bounds = outerBounds;
    if (type == FocusElementType::Inner)
    {
        bounds.Width -= options.firstThickness.left + options.firstThickness.right;
        bounds.Height -= options.firstThickness.left + options.firstThickness.right;
        bounds.X += options.firstThickness.left;
        bounds.Y += options.firstThickness.top;
    }

    // Let's just not deal with negative sizes.
    bounds.Width = MAX(0.0f, bounds.Width);
    bounds.Height = MAX(0.0f, bounds.Height);

    if (SetBrush(&border->m_pBorderBrush, brush.get()))
    {
        focusRectDirty = true;
    }

    if (border->m_borderThickness != *thickness)
    {
        border->m_borderThickness = *thickness;
        focusRectDirty = true;
    }

    SetElementSize(border, bounds.Width, bounds.Height, &focusRectDirty);

    // Add the corner radius (if any) to the borders.
    {

        XCORNERRADIUS cornerRadius = {
            options.cornerRadius.topLeft,
            options.cornerRadius.topRight,
            options.cornerRadius.bottomRight,
            options.cornerRadius.bottomLeft
        };

        // If we are doing the inner border, we need to adjust our corner radius with the
        // objective being to key the lengths of of the straight edge of the focused element,
        // and both of the borders consistent.  We already adjusted the corner radius for the
        // outer when we put the corner radius into the focus rect options.
        if (type == FocusElementType::Inner)
        {
            AdjustCornerRadius(cornerRadius, options.firstThickness);
        }
        CValue cornerRadiusValue;
        XCORNERRADIUS previousCornerRadius;
        IFCFAILFAST(border->GetValueByIndex(KnownPropertyIndex::Border_CornerRadius, &cornerRadiusValue));
        previousCornerRadius = *cornerRadiusValue.AsCornerRadius();
        if (!(cornerRadius == previousCornerRadius))
        {
            focusRectDirty = true;
            cornerRadiusValue.Wrap<valueCornerRadius>(&cornerRadius);
            IFCFAILFAST(border->SetValueByIndex(KnownPropertyIndex::Border_CornerRadius, cornerRadiusValue));
        }
    }

    if (focusRectDirty)
    {
        // Try to void dirtying the border if we don't really need to
        CUIElement::NWSetContentDirty(border, DirtyFlags::Render);
    }

    SetElementPosition(border, bounds.X - options.bounds.X, bounds.Y - options.bounds.Y);

    return focusRectDirty;
}

void CFocusRectManager::ApplyRevealFocusToBorder(
    _In_ CBorder* revealBorder,
    _In_ const Focusable& focusTarget,
    _In_ FocusRectangleOptions& options,
    _In_ XRECTF bounds,
    _In_ DirectUI::FocusNavigationDirection focusNavigationDirection)
{
    // If we already have a handin visual for this border, then there isn't anything that we need to do. This happens
    // during press animations when Control.IsTemplateFocusTarget is set to the element that has a PointerPressed animation.
    // Normally when focus changes, the Border is recycled. We can also have an existing visual and get here when the border
    // color changes due to a theme update
    wrl::ComPtr<WUComp::IVisual> existingRevealVisual;
    if (revealBorder->IsUsingHandInVisual())
    {
        IFCFAILFAST(revealBorder->GetHandInVisual(&existingRevealVisual));
    }

    XRECTF revealBounds = {};
    revealBounds.X = bounds.X - options.bounds.X;
    revealBounds.Y = bounds.Y - options.bounds.Y;
    revealBounds.Width = bounds.Width;
    revealBounds.Height = bounds.Height;

    const auto previousSizeAdjustment = m_revealSource.GetSizeAdjustment();
    auto revealVisual = m_revealSource.BuildRevealFocusVisual(
        existingRevealVisual.Get(),
        focusTarget.GetElementResponsibleForDrawingThisElement(),
        options.revealColor,
        options.firstThickness,
        options.isRevealBorderless);

    XRECTF previousBounds = options.previousBounds;
    previousBounds.X -= options.bounds.X;
    previousBounds.Y -= options.bounds.Y;

    if (!m_revealAnimator)
    {
        m_revealAnimator = std::make_unique<RevealFocusAnimator>(revealBorder->GetContext()->GetDCompTreeHost()->GetEasingFunctionStatics(), revealBorder->GetContext()->GetCompositor());
    }

    // Get the composition visual and wrap it in a sprite visual and set it as a child visual
    wrl::ComPtr<IUnknown> handoffAsUnk;
    IFCFAILFAST(revealBorder->GetHandOffVisual(&handoffAsUnk));

    wrl::ComPtr<WUComp::ICompositionObject> handoffAsObj;
    IFCFAILFAST(handoffAsUnk.As(&handoffAsObj));

    // Update the size, these use ExpressionAnimations to keep the reveal visual in sync with the Handoff Visual so
    // only update if it has changed
    if (previousSizeAdjustment != m_revealSource.GetSizeAdjustment() || !existingRevealVisual)
    {
        m_revealAnimator->UpdateVisualSize(handoffAsObj.Get(), revealVisual.Get(), m_revealSource);
    }
    if (previousSizeAdjustment != m_revealSource.GetSizeAdjustment() || !existingRevealVisual)
    {
        m_revealAnimator->UpdateVisualOffset(handoffAsObj.Get(), revealVisual.Get(), m_revealSource);
    }

    // If no existing handin visual, then set it now and run the animations. If we already had a visual,
    // then it's possible we are here due to some press animation or focus color change update. Either way,
    // the animations don't need to be re-ran in this scenario (press animations occur separately)
    if (!existingRevealVisual)
    {
        wrl::ComPtr<WUComp::IVisual> revealVisualAsVisual;
        IFCFAILFAST(revealVisual.As(&revealVisualAsVisual));
        IFCFAILFAST(revealBorder->SetHandInVisual(revealVisualAsVisual.Get()));

        if (focusNavigationDirection == DirectUI::FocusNavigationDirection::None ||
            focusNavigationDirection == DirectUI::FocusNavigationDirection::Previous ||
            focusNavigationDirection == DirectUI::FocusNavigationDirection::Next)
        {
            focusNavigationDirection = RevealMotion::CalculateFrom(previousBounds);
        }

        if (m_revealSource.HasLight())
        {
            m_revealAnimator->AnimateLights(revealVisual.Get(), m_revealSource, focusNavigationDirection);
        }
    }
}

void CFocusRectManager::AddRevealFocusOptions(
    FocusRectangleOptions& options,
    const Focusable& focusTarget)
{
    ASSERT(options.drawReveal);
   // When reveal focus is enabled, we want to enable a "borderless" focus scenario. The primary user of this is Xbox.
    // In order to get this behavior in RS4, the following conditions need to be true:
    //      1. FocusVisualPrimaryBrush == Transparent
    //      2. FocusVisualSecondaryThickness == 0
    //      4. The FE is a Control,Panel, or Shape with a SolidColorBrush background/fill.
    // When these conditions are met, the color we use for the primary brush will be the same as the background
    // for the Control.

    const bool primaryIsTransparent = ColorUtils::IsTransparentColor(options.firstBrush->m_rgb);
    const bool noSecondaryThickness = (options.secondThickness.left == 0.0 && options.secondThickness.IsUniform());

    auto focusTargetAsControl = do_pointer_cast<CControl>(focusTarget.Object);
    auto focusTargetAsPanel = do_pointer_cast<CPanel>(focusTarget.Object);
    auto focusTargetAsShape = do_pointer_cast<CShape>(focusTarget.Object);
    options.revealColor = options.firstBrush->m_rgb;

    if ((focusTargetAsControl || focusTargetAsPanel || focusTargetAsShape) && primaryIsTransparent && noSecondaryThickness)
    {
        CValue value;
        if (focusTargetAsControl)
        {
            IFCFAILFAST(focusTargetAsControl->GetValueByIndex(KnownPropertyIndex::Control_Background, &value));
        }
        else if (focusTargetAsPanel)
        {
            IFCFAILFAST(focusTargetAsPanel->GetValueByIndex(KnownPropertyIndex::Panel_Background, &value));
        }
        else
        {
            IFCFAILFAST(focusTargetAsShape->GetValueByIndex(KnownPropertyIndex::Shape_Fill, &value));
        }

        if (auto background = do_pointer_cast<CSolidColorBrush>(value))
        {
            options.revealColor = background->m_rgb;
            options.isRevealBorderless = true;
        }
    }
}
// Release resources held by FocusRectManager. These
// elements are automatically created on CFrameworkManager::UpdateFocus()
// and must be released before core releases its main render target on
// shutdown. Exposed by fixing core leak RS1 bug #7300521.
void CFocusRectManager::ReleaseResources(_In_ bool isDeviceLost, _In_ bool cleanupDComp, _In_ bool clearRenderData)
{
    if (isDeviceLost)
    {
        m_revealSource.Reset(RevealFocusSource::ResetReason::DeviceLost);
    }

    if (cleanupDComp)
    {
        m_revealSource.Reset(RevealFocusSource::ResetReason::DCompCleanup);
        m_revealAnimator.reset();
    }

    // Regular focus reset
    if (!isDeviceLost && !cleanupDComp)
    {
        // Reset the reveal source first, it could have lights that are targeted visuals kept alive by elements in the tree.
        // If those UIElements are destroyed, they will close the visual and we could fail to cleanup.
        m_revealSource.Reset(RevealFocusSource::ResetReason::FocusReset);
        if (m_revealAnimator)
        {
            m_revealAnimator->StopLightAnimation(m_revealSource);
        }

        if (m_focusLte)
        {
            if(m_transitionRoot)
            {
                IFCFAILFAST(m_focusLte->DetachTransition(m_canvasForFocusRects, m_transitionRoot));
            }
            ReleaseInterface(m_focusLte);
        }

        if (m_canvasForFocusRects)
        {
            if (clearRenderData)
            {
                m_canvasForFocusRects->LeavePCSceneRecursive();
            }

            IFCFAILFAST(m_canvasForFocusRects->RemoveParent(nullptr));
            ReleaseInterface(m_canvasForFocusRects);
        }

        if (m_transitionRoot)
        {
            // m_transitionRoot could have a clip set on it still if the last focus host was the SCP parent.  Let's just
            // leave the clip there, it shouldn't do any harm and we might just use it again anyway.

            if (clearRenderData)
            {
                m_transitionRoot->LeavePCSceneRecursive();
            }

            ReleaseInterface(m_transitionRoot);
        }
    }
}

// Call this function when there may have been tree updates that affect the way
// we draw the focus rect
void CFocusRectManager::UpdateFocusRect(
    _In_ CCoreServices* core,
    _In_opt_ CDependencyObject* focusedObject,
    _In_opt_ CDependencyObject* focusTargetObject,
    _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
    _In_ bool cleanOnly)
{
    Focusable focused(focusedObject);
    Focusable focusTarget(focusTargetObject);
    FocusRectHost focusRectHost;

    // Hyperlinks don't get reveal focus, it isn't the best experience
    const bool useRevealFocus = AreRevealFocusRectsEnabled() && focused.SupportsRevealFocus();
    if (!focusTarget.IsNull())
    {
        const XTHICKNESS margin = GetFocusVisualMargin(focused.Object, focusTarget.Object);
        XRECTF focusRect = ShrinkRectByThickness(focusTarget.GetBounds(), margin);
        if (useRevealFocus)
        {
            // With reveal focus, we want the focus rect to essentially push out further and not be clipped where a normal
            // focus rect would be.
            focusRect = EnlargeRectByThickness(focusRect, RevealFocusSource::GetRevealThickness());
        }
        focusRectHost = FindFocusRectHost(focusTarget, focusRect);
    }

    CDependencyObject* const previousFocusTarget = m_canvasForFocusRects ? m_canvasForFocusRects->GetParent() : nullptr;
    CDependencyObject* const previousFocusRectHost = m_transitionRoot ? m_transitionRoot->GetParentInternal(false) : nullptr;

    const bool needsCleanup =
            (previousFocusTarget != focusTarget.GetElementResponsibleForDrawingThisElement())
        ||  (previousFocusRectHost != focusRectHost.Element);

    if (needsCleanup)
    {
        const bool isDeviceLost = false, cleanupDComp = false, clearPCData = false;
        ReleaseResources(isDeviceLost, cleanupDComp, clearPCData);
    }

    if (focusTarget.IsNull() || focused.IsNull())
    {
        return;
    }

    if (!AreHighVisibilityFocusRectsEnabled())
    {
        return;
    }

    // We just wanted to make sure the secret m_canvasForFocusRects child was cleaned up, we can exit early for better perf
    if (cleanOnly)
    {
        return;
    }

    FocusRectangleOptions options; // Beware that these options will be reset, don't give them any state even though they look like in/out param
    std::vector<XRECTF> boundsVector;
    Focusable previousTarget(m_revealSource.GetTarget());
    DetermineRenderOptions(focused, focusTarget, focusRectHost, previousTarget, useRevealFocus, options, boundsVector);
    if (boundsVector.empty())
    {
        return;
    }

    if (!m_canvasForFocusRects)
    {
        CDependencyObject* newCanvas = nullptr;
        CREATEPARAMETERS cp(core);
        IFCFAILFAST(CCanvas::Create(&newCanvas, &cp));
        m_canvasForFocusRects = static_cast<CCanvas*>(newCanvas);

        CValue hitTestVisible;
        hitTestVisible.SetBool(FALSE);
        IFCFAILFAST(m_canvasForFocusRects->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, hitTestVisible));
    }

    bool sizeChanged = false;
    SetElementSize(m_canvasForFocusRects, options.bounds.Width, options.bounds.Height, &sizeChanged);
    if (sizeChanged)
    {
        CUIElement::NWSetContentDirty(m_canvasForFocusRects, DirtyFlags::Render);
    }

    if (!m_transitionRoot)
    {
        // Set up the LTE on the host
        IFCFAILFAST(focusRectHost.Element->EnsureChildrenCollection());
        CTransitionRoot* transitionRootNoRef = focusRectHost.Element->GetLocalTransitionRoot(true);
        ReplaceInterface(m_transitionRoot, transitionRootNoRef);
    }

    unsigned int index = 0;
    for (auto it : boundsVector)
    {
        xref_ptr<CBorder> outerBorder = EnsureFocusRectBorderAtPosition(
            core,
            m_canvasForFocusRects,
            index++);

        xref_ptr<CBorder> innerBorder = EnsureFocusRectBorderAtPosition(
            core,
            m_canvasForFocusRects,
            index++);

        const bool wasDirtied = ConfigureFocusElement(outerBorder.get(), FocusElementType::Outer, options, it);

        // Apply reveal focus if the outer border was dirtied and reveal focus is enabled for this target.
        if (wasDirtied && useRevealFocus)
        {
            ApplyRevealFocusToBorder(outerBorder.get(), focusTarget, options, it, focusNavigationDirection);
        }
        ConfigureFocusElement(innerBorder.get(), FocusElementType::Inner, options, it);
    }

    // Remove any left over focus rect elements at the end of the child list
    CUIElementCollection* collection = m_canvasForFocusRects->GetChildren();
    while (collection && collection->GetCount() > boundsVector.size() * 2)
    {
        collection->RemoveAt(collection->GetCount()-1);
    }

    // Place the focus rect canvas in the tree
    if (!m_focusLte)
    {
        // Add m_canvasForFocusRects as a child of the focus target in a "secret" way.  It's not a part
        // of the focus target's child collection, so it won't be seen by the app and won't get
        // called during tree walks.  We need it here so the LTE can walk the parent nodes to understand the
        // tree structure and what transforms apply.
        IFCFAILFAST(m_canvasForFocusRects->AddParent(
            focusTarget.GetElementResponsibleForDrawingThisElement(),
            true /*fPublic*/,
            CUIElement::NWSetSubgraphDirty));

        IFCFAILFAST(CLayoutTransitionElement::Create(
            m_canvasForFocusRects,
            FALSE /* isAbsolutelyPositioned*/,
            &m_focusLte));

        IFCFAILFAST(m_focusLte->AttachTransition(m_canvasForFocusRects, m_transitionRoot));
    }

    CUIElement* targetAsUE = do_pointer_cast<CUIElement>(focusTargetObject);
    if (targetAsUE)
    {
        FLOAT lteOpacity = m_focusLte->GetOpacityLocal();
        FLOAT elementEffectiveOpacity = targetAsUE->GetEffectiveOpacity();
        if (lteOpacity != elementEffectiveOpacity)
        {
            m_focusLte->SetOpacityLocal(elementEffectiveOpacity);
            CUIElement::NWSetOpacityDirty(m_focusLte, DirtyFlags::Render);
        }
    }

    EnsureTranslateTransform(m_focusLte, options.bounds.X, options.bounds.Y);

    // We want to support a FocusRect being able to render outside the clip of a ScrollViewer, but only on the non-scrolling
    // sides.  E.g., for a ScrollViewer that scrolls vertically, we should clip/nudge on the top and bottom of the content, but on
    // the left and right let the focus rect render outside of the ScrollContentPresenter.  To do this, we have a special Grid as a
    // peer of the SCP and make that grid the focus host.  This allows the focus rect to escape the clip of the SCP, but still be
    // underneath the scrollbars in z-order.  BUT, this means we need to go add our own clip to make sure the focus rect is still
    // clipped on the top and bottom.  We'll do that here.

    // In this picture, Button is unfocused, and is in a vertically-scrollable ScrollViewer that has a scroll offset.
    // (there is other, invisible content in the ScrollViewer above the button)
    //
    //     -------------------------- << Top of ScrollViewer viewport
    //     |                   |
    //     |       Button      |
    //     |                   |
    //     ---------------------
    //     |
    //     ^ Left edge of ScrollViewer viewport
    //
    // When Button is focused, we want to let the focus rect extend outside the bounds of the ScrollViewer
    // on the left and right (non-scrollable) sides.  But we do need to still clip on the top and bottom
    // of the ScrollViewer viewport (represented by the ScrollContentPresenter).  Otherwise only parts
    // of the focus rect would be visible sometimes, as the focused element scrolls out of view (that looks
    // super weird).
    //
    //   ----------------------------- << Top of ScrollViewer viewport
    //   | |                   | |
    //   | |       Button      | |
    //   | |                   | |
    //   | --------------------- |
    //   -------------------------
    //     |
    //

    XRECTF newClipRect = GetInfiniteClip();

    // We need to apply a new clip only if focusRectHost is an ancestor of the SCP -- in this case the focus rect won't get clipped
    // by the SCP, so if the focused element scrolls out of view the focus rect would still be visible even though the focused
    // element is clipped out.  See above and FindFocusRectHost for more info on focus rects in a ScrollViewer.
    if (focusRectHost.HostedOutsideScpClip())
    {
        CUIElement* scp = focusRectHost.ScrollContentPresenter;
        auto scpClipRect = do_pointer_cast<CRectangleGeometry>(scp->GetClip());
        if (scpClipRect)
        {
            xref_ptr<CGeneralTransform> transform;
            scp->TransformToVisual(focusRectHost.Element, &transform);
            XRECTF clipInScrollViewerSpace = {};
            if (FAILED(transform->TransformRect(scpClipRect->m_rc, &clipInScrollViewerSpace)))
            {
                return;
            }
            ScrollDirection dir = GetScrollDirection(scp);

            if (dir.IsScrollable())
            {
                if (dir.Horizontal)
                {
                    newClipRect.X = clipInScrollViewerSpace.X;
                    newClipRect.Width = clipInScrollViewerSpace.Width;
                }
                else
                {
                    // Expand this side of the clipping rect to effectively remove the clip.
                    // We expand by the width of the SCP clip -- seems like a sane, reasonable amount.
                    newClipRect.X = clipInScrollViewerSpace.X - clipInScrollViewerSpace.Width;
                    newClipRect.Width = clipInScrollViewerSpace.Width * 3;
                }

                if (dir.Vertical)
                {
                    newClipRect.Y = clipInScrollViewerSpace.Y;
                    newClipRect.Height = clipInScrollViewerSpace.Height;
                }
                else
                {
                    // Expand this side of the clipping rect to effectively remove the clip.
                    // We expand by the height of the SCP clip -- seems like a sane, reasonable amount.
                    newClipRect.Y = clipInScrollViewerSpace.Y - clipInScrollViewerSpace.Height;
                    newClipRect.Height = clipInScrollViewerSpace.Height * 3;
                }
            }
        }
    }

    // Apply or remove clip to m_transitionRoot as needed.
    // It seems a little hacky to put a clip on the transition root, since this transition root coule theoretically have
    // another unrelated LTE on it, and that LTE would get clipped by this clip as well.  But this is such a specific element
    // in a specific place in the tree, it should be super unlikely that would happen.  LTEs are typically attached to
    // specific elements in a control's template.
    CRectangleGeometry* existingClipRect = static_cast<CRectangleGeometry*>(m_transitionRoot->GetClip().get());
    if (IsInfiniteRectF(newClipRect))
    {
        if (m_transitionRoot->GetClip())
        {
            CUIElement::NWSetContentDirty(m_transitionRoot, DirtyFlags::Render);
        }
        m_transitionRoot->SetClip(nullptr);
    }
    else if (!existingClipRect || existingClipRect->m_rc != newClipRect)
    {
        xref_ptr<CRectangleGeometry> svClipRect;
        CDependencyObject* obj = nullptr;
        CREATEPARAMETERS cp(core);
        CRectangleGeometry::Create(&obj, &cp);
        svClipRect.attach(static_cast<CRectangleGeometry*>(obj));

        svClipRect->m_rc = newClipRect;
        m_transitionRoot->SetClip(svClipRect);
        CUIElement::NWSetContentDirty(m_transitionRoot, DirtyFlags::Render);
    }
}

void CFocusRectManager::OnFocusedElementKeyPressed() const
{
    if (AreRevealFocusRectsEnabled() && m_revealAnimator)
    {
        m_revealAnimator->OnFocusedElementKeyPressed(m_revealSource);
    }
}

void CFocusRectManager::OnFocusedElementKeyReleased() const
{
    if (AreRevealFocusRectsEnabled() && m_revealAnimator)
    {
        m_revealAnimator->OnFocusedElementKeyReleased(m_revealSource);
    }
}

/*static*/ bool
CFocusRectManager::AreHighVisibilityFocusRectsEnabled()
{
    return CApplication::GetFocusVisualKind() != DirectUI::FocusVisualKind::DottedLine;
}

/*static*/ bool
CFocusRectManager::AreRevealFocusRectsEnabled()
{
    const auto focusVisualKind = CApplication::GetFocusVisualKind();
    return focusVisualKind == DirectUI::FocusVisualKind::Reveal;
}

/*static*/ void
CFocusRectManager::RenderFocusRectForElement(
    _In_ CUIElement* element,
    _In_ IContentRenderer* contentRenderer)
{
    // Legacy mode only
    if (AreHighVisibilityFocusRectsEnabled())
    {
        return;
    }

    Focusable focusTarget(element);
    FocusRectangleOptions options;
    if (SetLegacyRenderOptions(focusTarget, &options))
    {
        options.bounds = focusTarget.GetBounds();
        IFCFAILFAST(contentRenderer->RenderFocusRectangle(element, options));
    }
}

// Find a suitable focus host.  See header file for more info on that.
// The focus host is conceptually the element that draws the focus rect.  The focus rect
// will have the same zorder and clip as the focus host, so we make some guesses based on the
// tree about where the z-order and clips will be the best.
/*static*/ FocusRectHost
CFocusRectManager::FindFocusRectHost(
    _In_ const Focusable& target,
    _In_ const XRECTF& focusRect)
{
    if (target.IsNull())
    {
        return FocusRectHost();
    }

    CDependencyObject* candidate = target.Object;

    if (target.IsFocusableElement())
    {
        // Skip straight up to the containing framework element, since none of the children of the containing
        // element will be suitable focus rect hosts.  Since we walk the public parent chain, there's also a
        // problem because some inline collections don't have a public parent, and the walk stops there for hyperlinks.
        candidate = CFocusableHelper::GetContainingFrameworkElementIfFocusable(candidate);
    }

    CAggregateTransformer aggregateTransformer;

    // Crawl the tree upward, looking for a good candidate
    while (candidate)
    {
        CUIElement* const candidateAsElement = do_pointer_cast<CUIElement>(candidate);
        if (candidateAsElement)
        {
            // We don't know a good way to traverse an LTE yet, so don't go past one,
            // just use the focus target itself as the host.  We could consider returning
            // the candidate instead if we hit z-order problems with this.
            if (candidateAsElement->IsHiddenForLayoutTransition())
            {
                return {target.GetElementResponsibleForDrawingThisElement(), FocusRectHost::Type::LTE};
            }
        }

        CDependencyObject* parent = candidate->GetParent();
        if (parent)
        {
            // A focus rect in a popup must be obscured by a popup in a higher z-order,
            // so if we hit an element whose parent is the PopupRoot, we know we found
            // a popup -- that guy is the host.
            if (parent->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
            {
                return {candidate, FocusRectHost::Type::Popup};
            }

            // Finding the best focusRectHost inside a ScrollViewer is tricky.  Here's what we want:
            // * focusRectHost must be lower in z-order than any visible scrollbars
            // * If there's enough room for us to host the focus rect on some ancestor of the focus target
            //      that's inside the ScrollViewer (e.g., if the ScrollViewer content is a Grid with large
            //      margin), we'll host the focus rect host on that ancestor.  Then it will scroll
            //      inside the ScrollViewer just like the other content.  This is the ElementContainsFocusRect
            //      case.
            // * If we don't have enough room on an ancestor, focusRectHost should be an ancestor of the
            //      ScrollContentPresenter if possible, because we want to be able to draw outside of the
            //      ScrollContentPresenter (it clips)
            // * If the scrollViewer isn't actually scrollable there's really nothing special about it,
            //      we can ignore and keep walking up.
            //
            // There are a few different configurations of ScrollViewer's template we need to think
            // about, and we choose a focus rect host differently for each configuration:
            //
            // ScrollContentPresenterChild: the focus rect host is the child of the SCP.  Sadly, focus rect
            // will be clipped in this case.  This is the normal TH2 ScrollViewer case:
            //                    ScrollViewer
            //                         |
            //                      Border
            //                         |
            //                        Grid
            //                        /  \
            // ScrollContentPresenter     ScrollBars
            //           |
            //   (Scrolling content)
            //
            // ScrollContentPresenterPeer: In RS1 we added a peer-to-SCP Grid specifically for focus rects, just
            // after the SCP in z-order.  In this case that peer Grid serves as the focus rect host:
            //                    ScrollViewer
            //                         |
            //                      Border
            //                         |
            //                        Grid
            //                      /  |   \
            // ScrollContentPresenter Grid  ScrollBars
            //           |
            //   (Scrolling content)
            //
            // That grid gives us a node where we can attach the focus rect.  That way, it escapes the
            // ScrollContentPresenter's clip, but it's also beneath the ScrollBars in z-order.
            //
            // ScrollContentPresenterParent: Some ScrollViewers don't have ScrollBars, we can just use the
            // SCP parent as the focus rect host.  This lets us escape the SV's clip too.
            //                    ScrollViewer
            //                         |
            //                ScrollContentPresenter

            if (parent->OfTypeByIndex<KnownTypeIndex::ScrollContentPresenter>()
                && GetScrollDirection(parent).IsScrollable())
            {
                CUIElement* scp = static_cast<CUIElement*>(parent);
                CUIElementCollection* collection = do_pointer_cast<CUIElement>(scp->GetParent())->GetChildren();

                if (collection)
                {
                    const UINT collectionCount = collection->GetCount();
                    UINT scpIndex = collectionCount;
                    UINT firstScrollBarIndex = collectionCount;
                    UINT firstPeerCandidateIndex = collectionCount;
                    for (UINT i = 0; i < collectionCount; ++i)
                    {
                        CDependencyObject* peer = collection->GetCollection()[i];
                        if (peer == scp)
                        {
                            scpIndex = i;
                        }
                        else if (peer->OfTypeByIndex<KnownTypeIndex::ScrollBar>())
                        {
                            firstScrollBarIndex = MIN(i, firstScrollBarIndex);
                        }
                        else if (scpIndex < i)
                        {
                            firstPeerCandidateIndex = MIN(i, firstPeerCandidateIndex);
                        }
                    }

                    if (firstScrollBarIndex == collectionCount)
                    {
                        // No Scrollbars, we can just use the SCP parent
                        return {scp->GetParent(), FocusRectHost::Type::ScrollContentPresenterParent, scp};
                    }
                    else if (firstPeerCandidateIndex < firstScrollBarIndex)
                    {
                        // We found an element after SCP that's not a scrollbar, we can use that for a host
                        // (RS1+-style ScrollViewer template)
                        return {collection->GetCollection()[firstPeerCandidateIndex], FocusRectHost::Type::ScrollContentPresenterPeer, scp};
                    }
                }

                // Default to returning the ScrollContentPresenter's content.  Focus rect will be clipped at the SCP
                // bounds in this case.
                return {candidate, FocusRectHost::Type::ScrollContentPresenterChild, scp};
            }

            // Is the focus rect contained by "parent"?
            CUIElement* parentElement = do_pointer_cast<CUIElement>(parent);
            if (parentElement && candidateAsElement)
            {
                // If the focus rect can fit entirely within the parent element, just use the focused element as the focus rect host.
                // This helps us place the focus rect host farther down the tree, as a nearer ancestor to the focus target, and so
                // the focus rect is more likely to be occluded by all the same elements that occlude the focus target.
                //
                //          ScrollContentPresenter
                //                    |
                //                Grid (Padding="10")  <- candidate
                //                    |
                //              Button (focused)       <- focus target
                //
                // The grid's padding gives us plenty of room to draw the focus rect around the button.  All else being equal, it's
                // good to host the focus rect as low in the tree as possible to reduce the chance there's some other element in the
                // tree that draws on top of the focus target but underneath the focus rect. (this looks bad).  We don't do proper
                // occlusion testing for focus rects due to performance costs, though we may want to consider it more in the future.
                //
                // The main motivating scenario for this rule was "shy headers" in the RS2 Groove app.  In this app, we have elements that
                // are part of the content of the ScrollViewer which are placed on top of other elements via the hand-off visuals feature
                // and composition APIs.
                xref_ptr<ITransformer> transformer;
                IFCFAILFAST(candidateAsElement->GetTransformer(transformer.ReleaseAndGetAddressOf()));
                if (transformer)
                {
                    IFCFAILFAST(aggregateTransformer.Add(transformer.get()));
                }

                XRECTF focusRectInParentSpace = {};
                IFCFAILFAST(CTransformer::TransformBounds(&aggregateTransformer, &focusRect, &focusRectInParentSpace, FALSE /*isReverse*/));

                const XRECTF parentBounds = GetElementLayoutBoundsWithMargins(parentElement);

                if (focusRectInParentSpace.X >= parentBounds.X
                    && focusRectInParentSpace.Y >= parentBounds.Y
                    && focusRectInParentSpace.Right() <= parentBounds.Right()
                    && focusRectInParentSpace.Bottom() <= parentBounds.Bottom())
                {
                    return {parent, FocusRectHost::Type::ElementContainsFocusRect};
                }
            }
        }

        // We've almost walked all the way to visual root.  Choose the RootScrollViewer to avoid focus rects
        // from being drawn above popups etc.
        // Note if the RootScrollViewer can have scrollbars, the focus rect will render above the scrollbars
        if (candidate->OfTypeByIndex<KnownTypeIndex::RootScrollViewer>() || candidate->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            return {candidate, FocusRectHost::Type::Root};
        }

        // If this element has a scroll clip (i.e. we are not supposed to nudge inside the clip in one direction and/or the other),
        // then we have to chose it as the host. It doesn't benefit us to pick a parent anyway because we are not supposed to escape
        // this element's clip.
        if (!(NudgeFocusRectInsideHorizontalClip(candidateAsElement) && NudgeFocusRectInsideVerticalClip(candidateAsElement)))
        {
            return { candidate, FocusRectHost::Type::ElementContainsFocusRect };
        }

        candidate = parent;
    }

    // OK, just use the root.
    return {target.Object->GetTreeRoot(), FocusRectHost::Type::Root};
}

// set bounds to the bounds of the given element
void FocusRectangleOptions::UseElementBounds(_In_ CUIElement* element)
{
    bounds = {0.0f, 0.0f, element->GetActualWidth(), element->GetActualHeight() };
}
