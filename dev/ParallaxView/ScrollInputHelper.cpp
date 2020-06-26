// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollInputHelper.h"
#include "ScrollPresenter.h"

PCWSTR ScrollInputHelper::s_horizontalOffsetPropertyName = L"TranslationX";
PCWSTR ScrollInputHelper::s_verticalOffsetPropertyName = L"TranslationY";
PCWSTR ScrollInputHelper::s_scalePropertyName = L"Scale";

ScrollInputHelper::ScrollInputHelper(
    const ITrackerHandleManager* owner,
    std::function<void(bool, bool)> infoChangedFunction)
    :m_owner(owner)
{
    m_infoChangedFunction = infoChangedFunction;
}

ScrollInputHelper::~ScrollInputHelper()
{
    UnhookScrollPresenterPropertyChanged();
    UnhookScrollPresenterContentPropertyChanged();
    UnhookScrollViewerPropertyChanged();
    UnhookScrollViewerContentPropertyChanged();
    UnhookScrollViewerDirectManipulationStarted();
    UnhookScrollViewerDirectManipulationCompleted();
    UnhookRichEditBoxTextChanged();
    UnhookSourceControlTemplateChanged();
    UnhookSourceElementLoaded();
    UnhookTargetElementLoaded();
    UnhookCompositionTargetRendering();
}

winrt::UIElement ScrollInputHelper::TargetElement() const
{
    return m_targetElement.get();
}

winrt::CompositionPropertySet ScrollInputHelper::SourcePropertySet() const
{
    return m_sourcePropertySet;
}

bool ScrollInputHelper::IsTargetElementInSource() const
{
    return m_isTargetElementInSource;
}

winrt::hstring ScrollInputHelper::GetSourceOffsetPropertyName(winrt::Orientation orientation) const
{
    return (orientation == winrt::Orientation::Horizontal) ? s_horizontalOffsetPropertyName : s_verticalOffsetPropertyName;
}

winrt::hstring ScrollInputHelper::GetSourceScalePropertyName() const
{
    return s_scalePropertyName;
}

// Returns the offset of the scrolled element in relation to its owning source.
double ScrollInputHelper::GetOffsetFromScrollContentElement(const winrt::UIElement& element, winrt::Orientation orientation) const
{
    const winrt::UIElement scrollContentElement = GetScrollContentElement();

    if (!scrollContentElement)
    {
        return 0.0;
    }

    const winrt::GeneralTransform gt = element.TransformToVisual(scrollContentElement);
    const winrt::Windows::Foundation::Point elementOffset = gt.TransformPoint(winrt::Windows::Foundation::Point(0, 0));

    return orientation == winrt::Orientation::Horizontal ? elementOffset.X : elementOffset.Y;
}

double ScrollInputHelper::GetMaxUnderpanOffset(winrt::Orientation orientation) const
{
    return (orientation == winrt::Orientation::Horizontal) ? m_outOfBoundsPanSize.Width : m_outOfBoundsPanSize.Height;
}

double ScrollInputHelper::GetMaxOverpanOffset(winrt::Orientation orientation) const
{
    return (orientation == winrt::Orientation::Horizontal) ? m_outOfBoundsPanSize.Width : m_outOfBoundsPanSize.Height;
}

double ScrollInputHelper::GetContentSize(winrt::Orientation orientation) const
{
    return (orientation == winrt::Orientation::Horizontal) ? m_contentSize.Width : m_contentSize.Height;
}

double ScrollInputHelper::GetViewportSize(winrt::Orientation orientation) const
{
    return (orientation == winrt::Orientation::Horizontal) ? m_viewportSize.Width : m_viewportSize.Height;
}

void ScrollInputHelper::SetSourceElement(const winrt::UIElement& sourceElement)
{
    if (m_sourceElement.get() != sourceElement)
    {
        UnhookSourceControlTemplateChanged();

        m_sourceElement.set(sourceElement);
        OnSourceElementChanged(true /*allowSourceElementLoadedHookup*/);
    }
}

void ScrollInputHelper::SetTargetElement(const winrt::UIElement& targetElement)
{
    if (m_targetElement.get() != targetElement)
    {
        UnhookTargetElementLoaded();
        m_targetElement.set(targetElement);
        OnTargetElementChanged();
    }
}

void ScrollInputHelper::SetScrollViewer(const winrt::FxScrollViewer& scrollViewer)
{
    if (scrollViewer != m_scrollViewer.get())
    {
        if (m_scrollViewer)
        {
            UnhookScrollViewerPropertyChanged();
            UnhookScrollViewerDirectManipulationStarted();
            UnhookScrollViewerDirectManipulationCompleted();
            UnhookRichEditBoxTextChanged();

            m_richEditBox.set(nullptr);
            m_scrollViewerPropertySet = nullptr;
            m_isScrollViewerInDirectManipulation = false;
        }

        m_scrollViewer.set(scrollViewer);

        if (scrollViewer)
        {
            // Check if the ScrollViewer belongs to a RichEditBox so content size changes can be detected 
            // via its TextChanged event.
            m_richEditBox.set(ScrollInputHelper::GetRichEditBoxParent(scrollViewer));

            HookScrollViewerPropertyChanged();
            HookScrollViewerDirectManipulationStarted();
            HookScrollViewerDirectManipulationCompleted();
            HookRichEditBoxTextChanged();

            m_scrollViewerPropertySet = winrt::ElementCompositionPreview::GetScrollViewerManipulationPropertySet(scrollViewer);
        }

        ProcessScrollViewerContentChange();
    }
}

void ScrollInputHelper::SetScrollPresenter(const winrt::ScrollPresenter& scrollPresenter)
{
    if (scrollPresenter != m_scrollPresenter.get())
    {
        if (m_scrollPresenter)
        {
            UnhookScrollPresenterPropertyChanged();
        }

        m_scrollPresenter.set(scrollPresenter);

        if (scrollPresenter)
        {
            HookScrollPresenterPropertyChanged();
        }

        ProcessScrollPresenterContentChange();
    }
}

// Returns the parent RichEditBox if any.
winrt::RichEditBox ScrollInputHelper::GetRichEditBoxParent(const winrt::DependencyObject& childElement)
{
    if (childElement)
    {
        winrt::DependencyObject parent = winrt::VisualTreeHelper::GetParent(childElement);
        if (parent)
        {
            winrt::RichEditBox richEditBoxParent = parent.try_as<winrt::RichEditBox>();
            if (richEditBoxParent)
            {
                return richEditBoxParent;
            }
            return ScrollInputHelper::GetRichEditBoxParent(parent);
        }
    }

    return nullptr;
}

// Returns the inner ScrollViewer or ScrollPresenter if any.
void ScrollInputHelper::GetChildScrollPresenterOrScrollViewer(
    const winrt::DependencyObject& rootElement,
    _Out_ winrt::ScrollPresenter* scrollPresenter,
    _Out_ winrt::FxScrollViewer* scrollViewer)
{
    *scrollPresenter = nullptr;
    *scrollViewer = nullptr;

    if (rootElement)
    {
        const int childCount = winrt::VisualTreeHelper::GetChildrenCount(rootElement);
        for (int i = 0; i < childCount; i++)
        {
            winrt::DependencyObject current = winrt::VisualTreeHelper::GetChild(rootElement, i);
            *scrollViewer = current.try_as<winrt::FxScrollViewer>();
            if (*scrollViewer)
            {
                return;
            }
            *scrollPresenter = current.try_as<winrt::ScrollPresenter>();
            if (*scrollPresenter)
            {
                return;
            }
        }

        for (int i = 0; i < childCount; i++)
        {
            winrt::DependencyObject current = winrt::VisualTreeHelper::GetChild(rootElement, i);
            ScrollInputHelper::GetChildScrollPresenterOrScrollViewer(
                current,
                scrollPresenter,
                scrollViewer);
            if (*scrollPresenter)
            {
                return;
            }
            if (*scrollViewer)
            {
                return;
            }
        }
    }
}

// Returns the ScrollViewer content as a UIElement, if any. Or ScrollPresenter.Content, if any.
winrt::UIElement ScrollInputHelper::GetScrollContentElement() const
{
    if (m_scrollViewer)
    {
        winrt::ContentControl scrollViewerAsContentControl = m_scrollViewer.as<winrt::ContentControl>();

        winrt::IInspectable content = scrollViewerAsContentControl.Content();
        if (content)
        {
            return content.try_as<winrt::UIElement>();
        }
    }
    else if (auto scrollPresenter = m_scrollPresenter.get())
    {
        return scrollPresenter.Content();
    }
    return nullptr;
}

// Returns the effective horizontal alignment of the ScrollViewer content.
winrt::HorizontalAlignment ScrollInputHelper::GetEffectiveHorizontalAlignment() const
{
    if (m_isScrollViewerInDirectManipulation)
    {
        return m_manipulationHorizontalAlignment;
    }
    else
    {
        return ComputeHorizontalContentAlignment();
    }
}

// Returns the effective vertical alignment of the ScrollViewer content.
winrt::VerticalAlignment ScrollInputHelper::GetEffectiveVerticalAlignment() const
{
    if (m_isScrollViewerInDirectManipulation)
    {
        return m_manipulationVerticalAlignment;
    }
    else
    {
        return ComputeVerticalContentAlignment();
    }
}

// Returns the effective zoom mode of the ScrollViewer.
winrt::FxZoomMode ScrollInputHelper::GetEffectiveZoomMode() const
{
    if (m_isScrollViewerInDirectManipulation)
    {
        return m_manipulationZoomMode;
    }
    else
    {
        return ComputeZoomMode();
    }
}

// Updates the m_outOfBoundsPanSize field based on the viewport size and zoom mode.
void ScrollInputHelper::UpdateOutOfBoundsPanSize()
{
    if (m_scrollPresenter || m_scrollViewer)
    {
        const double viewportWith = GetViewportSize(winrt::Orientation::Horizontal);
        const double viewportHeight = GetViewportSize(winrt::Orientation::Vertical);

        if (m_scrollViewer && GetEffectiveZoomMode() == winrt::FxZoomMode::Disabled)
        {
            // A ScrollViewer can under/overpan up to 10% of its viewport size
            m_outOfBoundsPanSize.Width = static_cast<float>(0.1 * viewportWith);
            m_outOfBoundsPanSize.Height = static_cast<float>(0.1 * viewportHeight);
        }
        else
        {
            // When zooming is allowed for a ScrollViewer, or in general for the ScrollPresenter,
            // the content can be pushed all the way to the edge of the screen with two fingers,
            // but we limit the offset to one viewport size.
            // Note that if in the future, the ScrollPresenter's underpan & overpan limits become customizable,
            // its ExpressionAnimationSources property set will have to expose those custom limits.
            // The values would then be consumed here for a more accurate ParallaxView behavior.
            m_outOfBoundsPanSize.Width = static_cast<float>(viewportWith);
            m_outOfBoundsPanSize.Height = static_cast<float>(viewportHeight);
        }
    }
    else
    {
        m_outOfBoundsPanSize.Width = m_outOfBoundsPanSize.Height = 0.0f;
    }
}

// Updates the m_contentSize field.
void ScrollInputHelper::UpdateContentSize()
{
    m_contentSize.Width = m_contentSize.Height = 0.0f;
    auto scrollPresenter = m_scrollPresenter.get();

    if (!scrollPresenter && !m_scrollViewer)
    {
        return;
    }

    if (scrollPresenter)
    {
        winrt::CompositionPropertySet scrollPresenterPropertySet = scrollPresenter.ExpressionAnimationSources();
        winrt::float2 extent{};

        const winrt::CompositionGetValueStatus status = scrollPresenterPropertySet.TryGetVector2(ScrollPresenter::s_extentSourcePropertyName, extent);
        if (status == winrt::CompositionGetValueStatus::Succeeded)
        {
            m_contentSize.Width = extent.x;
            m_contentSize.Height = extent.y;
        }
        return;
    }
    auto scrollViewer = m_scrollViewer.get();
    const float extentWidth = static_cast<float>(scrollViewer.ExtentWidth());
    const float extentHeight = static_cast<float>(scrollViewer.ExtentHeight());

    winrt::UIElement scrollContentElement = GetScrollContentElement();
    winrt::ItemsPresenter itemsPresenter = scrollContentElement ? (scrollContentElement.try_as<winrt::ItemsPresenter>()) : nullptr;

    if (!scrollContentElement || !itemsPresenter)
    {
        m_contentSize.Width = extentWidth;
        m_contentSize.Height = extentHeight;
        return;
    }

    int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(itemsPresenter);

    if (childrenCount > 0)
    {
        winrt::DependencyObject child = winrt::VisualTreeHelper::GetChild(itemsPresenter, childrenCount == 1 ? 0 : 1);

        if (child)
        {
            winrt::VirtualizingStackPanel virtualizingStackPanel = child.try_as<winrt::VirtualizingStackPanel>();

            if (virtualizingStackPanel)
            {
                // VirtualizingStackPanel are handled specially because the ScrollViewer.ExtentWidth/ExtentHeight is unit-based instead
                // of pixel-based in the virtualized dimension. The computed size accounts for the potential margins, header and footer.
                double virtualizingSize = 0.0;
                const winrt::Thickness vspMargin = virtualizingStackPanel.Margin();
                const winrt::Thickness itMargin = itemsPresenter.Margin();

                if (virtualizingStackPanel.Orientation() == winrt::Orientation::Horizontal)
                {
                    virtualizingSize = virtualizingStackPanel.ExtentWidth() * virtualizingStackPanel.ActualWidth() / virtualizingStackPanel.ViewportWidth() +
                        vspMargin.Left + vspMargin.Right + itMargin.Left + itMargin.Right;
                }
                else
                {
                    virtualizingSize = virtualizingStackPanel.ExtentHeight() * virtualizingStackPanel.ActualHeight() / virtualizingStackPanel.ViewportHeight() +
                        vspMargin.Top + vspMargin.Bottom + itMargin.Top + itMargin.Bottom;
                }

                if (childrenCount > 1)
                {
                    child = winrt::VisualTreeHelper::GetChild(itemsPresenter, 0);

                    if (child)
                    {
                        winrt::FrameworkElement headerChild = child.try_as<winrt::FrameworkElement>();

                        if (headerChild)
                        {
                            virtualizingSize += virtualizingStackPanel.Orientation() == winrt::Orientation::Horizontal ? headerChild.ActualWidth() : headerChild.ActualHeight();
                        }
                    }
                }

                if (childrenCount > 2)
                {
                    child = winrt::VisualTreeHelper::GetChild(itemsPresenter, 2);

                    if (child)
                    {
                        winrt::FrameworkElement footerChild = child.try_as<winrt::FrameworkElement>();

                        if (footerChild)
                        {
                            virtualizingSize += virtualizingStackPanel.Orientation() == winrt::Orientation::Horizontal ? footerChild.ActualWidth() : footerChild.ActualHeight();
                        }
                    }
                }

                if (virtualizingStackPanel.Orientation() == winrt::Orientation::Horizontal)
                {
                    m_contentSize.Width = static_cast<float>(virtualizingSize);
                    m_contentSize.Height = extentHeight;
                }
                else
                {
                    m_contentSize.Width = extentWidth;
                    m_contentSize.Height = static_cast<float>(virtualizingSize);
                }

                return;
            }
        }
    }

    m_contentSize.Width = extentWidth;
    m_contentSize.Height = extentHeight;
}

// Updates the m_viewportSize field.
void ScrollInputHelper::UpdateViewportSize()
{
    auto scrollPresenter = m_scrollPresenter.get();
    if (scrollPresenter)
    {
        winrt::CompositionPropertySet scrollPresenterPropertySet = scrollPresenter.ExpressionAnimationSources();
        winrt::float2 viewport{};

        const winrt::CompositionGetValueStatus status = scrollPresenterPropertySet.TryGetVector2(ScrollPresenter::s_viewportSourcePropertyName, viewport);
        if (status == winrt::CompositionGetValueStatus::Succeeded)
        {
            m_viewportSize.Width = viewport.x;
            m_viewportSize.Height = viewport.y;
        }
        return;
    }

    if (m_scrollViewer)
    {
        auto scrollViewer = m_scrollViewer.get();
        winrt::UIElement scrollContentElement = GetScrollContentElement();

        if (scrollContentElement)
        {
            winrt::ItemsPresenter itemsPresenter = scrollContentElement.try_as<winrt::ItemsPresenter>();

            if (itemsPresenter)
            {
                int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(itemsPresenter);

                if (childrenCount > 0)
                {
                    winrt::DependencyObject child = winrt::VisualTreeHelper::GetChild(itemsPresenter, childrenCount == 1 ? 0 : 1);

                    if (child)
                    {
                        winrt::VirtualizingStackPanel virtualizingStackPanel = child.try_as<winrt::VirtualizingStackPanel>();

                        if (virtualizingStackPanel)
                        {
                            // VirtualizingStackPanel are handled specially because the ScrollViewer.ViewportWidth/ViewportHeight is unit-based instead
                            // of pixel-based in the virtualized dimension. The computed size accounts for the potential margins.
                            const winrt::Thickness itMargin = itemsPresenter.Margin();

                            if (virtualizingStackPanel.Orientation() == winrt::Orientation::Horizontal)
                            {
                                m_viewportSize.Width = static_cast<float>(itemsPresenter.ActualWidth() + itMargin.Left + itMargin.Right);
                                m_viewportSize.Height = static_cast<float>(scrollViewer.ViewportHeight());
                            }
                            else
                            {
                                m_viewportSize.Width = static_cast<float>(scrollViewer.ViewportWidth());
                                m_viewportSize.Height = static_cast<float>(itemsPresenter.ActualHeight() + itMargin.Top + itMargin.Bottom);
                            }
                            return;
                        }
                    }
                }
            }
        }
        m_viewportSize.Width = static_cast<float>(scrollViewer.ViewportWidth());
        m_viewportSize.Height = static_cast<float>(scrollViewer.ViewportHeight());
    }
    else
    {
        m_viewportSize.Width = m_viewportSize.Height = 0.0f;
    }
}

// Updates all the fields dependent on the ScrollViewer source. Stops/starts
// the internal composition animations.
void ScrollInputHelper::UpdateSource(bool allowSourceElementLoadedHookup)
{
    winrt::ScrollPresenter scrollPresenter = nullptr;
    winrt::FxScrollViewer scrollViewer = nullptr;
    auto sourceElement = m_sourceElement.get();
    if (sourceElement)
    {
        scrollPresenter = sourceElement.try_as<winrt::ScrollPresenter>();
        scrollViewer = sourceElement.try_as<winrt::FxScrollViewer>();
    }

    if (scrollPresenter || scrollViewer)
    {
        SetScrollPresenter(scrollPresenter);
        SetScrollViewer(scrollViewer);
    }
    else if (sourceElement)
    {
        ScrollInputHelper::GetChildScrollPresenterOrScrollViewer(
            sourceElement,
            &scrollPresenter,
            &scrollViewer);
        SetScrollPresenter(scrollPresenter);
        SetScrollViewer(scrollViewer);
    }
    else
    {
        SetScrollPresenter(nullptr);
        SetScrollViewer(nullptr);
    }

    if (allowSourceElementLoadedHookup &&
        !scrollPresenter &&
        !m_scrollViewer)
    {
        HookSourceElementLoaded();
    }

    if (!scrollPresenter && !m_scrollViewer)
    {
        StopInternalExpressionAnimations();
        m_sourcePropertySet = nullptr;
    }
    else if (m_targetElement)
    {
        EnsureInternalSourcePropertySetAndExpressionAnimations();
        m_sourcePropertySet = m_internalSourcePropertySet;
        StartInternalExpressionAnimations(m_scrollViewer ? m_scrollViewerPropertySet : scrollPresenter.ExpressionAnimationSources());
    }

    UpdateIsTargetElementInSource();
    UpdateContentSize();
    UpdateViewportSize();
    UpdateOutOfBoundsPanSize();
}

// Updates the m_isTargetElementInSource field.
void ScrollInputHelper::UpdateIsTargetElementInSource()
{
    auto targetElement = m_targetElement.get();

    if (targetElement)
    {
        bool sourceIsScrollViewer = m_scrollViewer != nullptr;
        bool sourceIsScrollPresenter = m_scrollPresenter != nullptr;

        if (sourceIsScrollViewer || sourceIsScrollPresenter)
        {
            winrt::DependencyObject parent = targetElement;
            do
            {
                parent = winrt::VisualTreeHelper::GetParent(parent);
                if (parent)
                {
                    if (sourceIsScrollViewer)
                    {
                        winrt::FxScrollViewer parentAsScrollViewer = parent.try_as<winrt::FxScrollViewer>();

                        if (parentAsScrollViewer == m_scrollViewer.get())
                        {
                            m_isTargetElementInSource = true;
                            return;
                        }
                    }
                    else
                    {
                        winrt::ScrollPresenter parentAsScrollPresenter = parent.try_as<winrt::ScrollPresenter>();

                        if (parentAsScrollPresenter == m_scrollPresenter.get())
                        {
                            m_isTargetElementInSource = true;
                            return;
                        }
                    }
                }
            } while (parent);
        }
    }

    m_isTargetElementInSource = false;
}

// Updates the m_manipulationZoomMode field.
void ScrollInputHelper::UpdateManipulationZoomMode()
{
    if (m_targetElement)
    {
        m_manipulationZoomMode = ComputeZoomMode();
    }
}

// Updates the m_manipulationHorizontalAlignment/m_manipulationVerticalAlignment fields.
void ScrollInputHelper::UpdateManipulationAlignments()
{
    if (m_targetElement)
    {
        m_manipulationHorizontalAlignment = ComputeHorizontalContentAlignment();
        m_manipulationVerticalAlignment = ComputeVerticalContentAlignment();
    }
}

// Updates the internal composition animations that account for the alignment portions in the ScrollViewer's manipulation property set (m_scrollViewerPropertySet).
// The offsets exposed by m_sourcePropertySet exclude those alignment portions.
void ScrollInputHelper::UpdateInternalExpressionAnimations(bool horizontalInfoChanged, bool verticalInfoChanged, bool zoomInfoChanged)
{
    bool restartAnimations = false;

    if (m_scrollViewer)
    {
        if (horizontalInfoChanged && m_internalTranslationXExpressionAnimation)
        {
            switch (GetEffectiveHorizontalAlignment())
            {
            case winrt::HorizontalAlignment::Left:
                m_internalTranslationXExpressionAnimation.Expression(L"source.Translation.X");
                break;

            case winrt::HorizontalAlignment::Stretch:
            case winrt::HorizontalAlignment::Center:
                m_internalTranslationXExpressionAnimation.Expression(
                    L"source.Translation.X + ((contentWidth * source.Scale.X - viewportWidth) < 0.0f ? (contentWidth * source.Scale.X - viewportWidth) / 2.0f : 0.0f)");
                m_internalTranslationXExpressionAnimation.SetScalarParameter(L"contentWidth", static_cast<float>(GetContentSize(winrt::Orientation::Horizontal)));
                m_internalTranslationXExpressionAnimation.SetScalarParameter(L"viewportWidth", static_cast<float>(GetViewportSize(winrt::Orientation::Horizontal)));
                break;

            case winrt::HorizontalAlignment::Right:
                m_internalTranslationXExpressionAnimation.Expression(L"source.Translation.X + ((contentWidth * source.Scale.X - viewportWidth) < 0.0f ? (contentWidth * source.Scale.X - viewportWidth) : 0.0f)");
                m_internalTranslationXExpressionAnimation.SetScalarParameter(L"contentWidth", static_cast<float>(GetContentSize(winrt::Orientation::Horizontal)));
                m_internalTranslationXExpressionAnimation.SetScalarParameter(L"viewportWidth", static_cast<float>(GetViewportSize(winrt::Orientation::Horizontal)));
                break;
            }
            restartAnimations = true;
        }

        if (verticalInfoChanged && m_internalTranslationYExpressionAnimation)
        {
            switch (GetEffectiveVerticalAlignment())
            {
            case winrt::VerticalAlignment::Top:
                m_internalTranslationYExpressionAnimation.Expression(L"source.Translation.Y");
                break;

            case winrt::VerticalAlignment::Stretch:
            case winrt::VerticalAlignment::Center:
                m_internalTranslationYExpressionAnimation.Expression(
                    L"source.Translation.Y + ((contentWidth * source.Scale.Y - viewportWidth) < 0.0f ? (contentWidth * source.Scale.Y - viewportWidth) / 2.0f : 0.0f)");
                m_internalTranslationYExpressionAnimation.SetScalarParameter(L"contentWidth", static_cast<float>(GetContentSize(winrt::Orientation::Vertical)));
                m_internalTranslationYExpressionAnimation.SetScalarParameter(L"viewportWidth", static_cast<float>(GetViewportSize(winrt::Orientation::Vertical)));
                break;

            case winrt::VerticalAlignment::Bottom:
                m_internalTranslationYExpressionAnimation.Expression(
                    L"source.Translation.Y + ((contentWidth * source.Scale.Y - viewportWidth) < 0.0f ? (contentWidth * source.Scale.Y - viewportWidth) : 0.0f)");
                m_internalTranslationYExpressionAnimation.SetScalarParameter(L"contentWidth", static_cast<float>(GetContentSize(winrt::Orientation::Vertical)));
                m_internalTranslationYExpressionAnimation.SetScalarParameter(L"viewportWidth", static_cast<float>(GetViewportSize(winrt::Orientation::Vertical)));
                break;
            }
            restartAnimations = true;
        }

        if (zoomInfoChanged && m_internalScaleExpressionAnimation)
        {
            m_internalScaleExpressionAnimation.Expression(L"source.Scale.X");
            restartAnimations = true;
        }

        if (restartAnimations && m_targetElement)
        {
            StartInternalExpressionAnimations(m_scrollViewerPropertySet);
        }
    }
    else if (auto scrollPresenter = m_scrollPresenter.get())
    {
        if (horizontalInfoChanged && m_internalTranslationXExpressionAnimation)
        {
            m_internalTranslationXExpressionAnimation.Expression(L"source.MinPosition.X - source.Position.X");
            restartAnimations = true;
        }
        if (verticalInfoChanged && m_internalTranslationYExpressionAnimation)
        {
            m_internalTranslationYExpressionAnimation.Expression(L"source.MinPosition.Y - source.Position.Y");
            restartAnimations = true;
        }

        if (zoomInfoChanged && m_internalScaleExpressionAnimation)
        {
            m_internalScaleExpressionAnimation.Expression(L"source.ZoomFactor");
            restartAnimations = true;
        }

        if (restartAnimations && m_targetElement)
        {
            StartInternalExpressionAnimations(scrollPresenter.ExpressionAnimationSources());
        }
    }
}

// Returns the ScrollViewer's content horizontal alignment.
winrt::HorizontalAlignment ScrollInputHelper::ComputeHorizontalContentAlignment() const
{
    // Panels that implement XAML's internal IScrollInfo interface: OrientedVirtualizingPanel, CarouselPanel, TextBoxView for TextBox, RichTextBox and PasswordBox.

    winrt::HorizontalAlignment horizontalAlignment = winrt::HorizontalAlignment::Stretch;

    // First access the ScrollViewer's HorizontalContentAlignment
    if (m_scrollViewer)
    {
        horizontalAlignment = m_scrollViewer.get().HorizontalContentAlignment();

        // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
        if (IsScrollContentPresenterIScrollInfoProvider())
        {
            // When the ScrollContentPresenter is the IScrollInfo implementer,
            // use the horizontal alignment of the manipulated element by default.
            winrt::UIElement scrollContentElement = GetScrollContentElement();

            if (scrollContentElement)
            {
                winrt::FrameworkElement contentAsFrameworkElement = scrollContentElement.try_as<winrt::FrameworkElement>();

                if (contentAsFrameworkElement)
                {
                    horizontalAlignment = contentAsFrameworkElement.HorizontalAlignment();
                }
            }
        }
    }

    return horizontalAlignment;
}

// Returns the ScrollViewer's content vertical alignment.
winrt::VerticalAlignment ScrollInputHelper::ComputeVerticalContentAlignment() const
{
    // Panels that implement XAML's internal IScrollInfo interface: OrientedVirtualizingPanel, CarouselPanel, TextBoxView for TextBox, RichTextBox and PasswordBox.

    winrt::VerticalAlignment verticalAlignment = winrt::VerticalAlignment::Stretch;

    // First access the ScrollViewer's VerticalContentAlignment
    if (m_scrollViewer)
    {
        verticalAlignment = m_scrollViewer.get().VerticalContentAlignment();

        // Determine whether the ScrollContentPresenter is the IScrollInfo implementer or not
        if (IsScrollContentPresenterIScrollInfoProvider())
        {
            // When the ScrollContentPresenter is the IScrollInfo implementer,
            // use the vertical alignment of the manipulated element by default.
            winrt::UIElement scrollContentElement = GetScrollContentElement();

            if (scrollContentElement)
            {
                winrt::FrameworkElement contentAsFrameworkElement = scrollContentElement.try_as<winrt::FrameworkElement>();

                if (contentAsFrameworkElement)
                {
                    verticalAlignment = contentAsFrameworkElement.VerticalAlignment();
                }
            }
        }
    }

    return verticalAlignment;
}

winrt::FxZoomMode ScrollInputHelper::ComputeZoomMode() const
{
    return m_scrollViewer ? m_scrollViewer.get().ZoomMode() : winrt::FxZoomMode::Disabled;
}

// Determines whether the ScrollViewer's ScrollContentPresenter is the IScrollInfo implementer used by the ScrollViewer.
bool ScrollInputHelper::IsScrollContentPresenterIScrollInfoProvider() const
{
    if (m_scrollViewer)
    {
        winrt::UIElement scrollContentElement = GetScrollContentElement();

        if (scrollContentElement)
        {
            winrt::ItemsPresenter itemsPresenter = scrollContentElement.try_as<winrt::ItemsPresenter>();

            if (itemsPresenter)
            {
                int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(itemsPresenter);

                if (childrenCount > 0)
                {
                    winrt::DependencyObject child = winrt::VisualTreeHelper::GetChild(itemsPresenter, childrenCount == 1 ? 0 : 1);

                    if (child)
                    {
                        winrt::OrientedVirtualizingPanel itemsPanelAsOrientedVirtualizingPanel = child.try_as<winrt::OrientedVirtualizingPanel>();
                        winrt::CarouselPanel itemsPanelAsCarouselPanel = child.try_as<winrt::CarouselPanel>();

                        if (itemsPanelAsOrientedVirtualizingPanel || itemsPanelAsCarouselPanel)
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

// Creates the internal composition property set, m_internalSourcePropertySet, that filters out the alignment portions of the ScrollViewer manipulation property set.
void ScrollInputHelper::EnsureInternalSourcePropertySetAndExpressionAnimations()
{
    auto targetElement = m_targetElement.get();
    if (!m_internalSourcePropertySet && targetElement)
    {
        winrt::Visual visual = winrt::ElementCompositionPreview::GetElementVisual(targetElement);
        winrt::Compositor compositor = visual.Compositor();

        m_internalSourcePropertySet = compositor.CreatePropertySet();
        m_internalSourcePropertySet.InsertScalar(s_horizontalOffsetPropertyName, 0.0f);
        m_internalSourcePropertySet.InsertScalar(s_verticalOffsetPropertyName, 0.0f);
        m_internalSourcePropertySet.InsertScalar(s_scalePropertyName, 1.0f);

        auto scrollViewer = m_scrollViewer.get();
        m_internalTranslationXExpressionAnimation = compositor.CreateExpressionAnimation(scrollViewer ? L"source.Translation.X" : L"source.MinPosition.X - source.Position.X");
        m_internalTranslationYExpressionAnimation = compositor.CreateExpressionAnimation(scrollViewer ? L"source.Translation.Y" : L"source.MinPosition.Y - source.Position.Y");
        m_internalScaleExpressionAnimation = compositor.CreateExpressionAnimation(scrollViewer ? L"source.Scale.X" : L"source.ZoomFactor");
    }
}

// Starts the animations targeting the properties inside m_internalSourcePropertySet.
void ScrollInputHelper::StartInternalExpressionAnimations(const winrt::CompositionPropertySet& source)
{
    if (m_internalSourcePropertySet && source)
    {
        m_internalTranslationXExpressionAnimation.SetReferenceParameter(L"source", source);
        m_internalTranslationYExpressionAnimation.SetReferenceParameter(L"source", source);
        m_internalScaleExpressionAnimation.SetReferenceParameter(L"source", source);

        m_internalSourcePropertySet.StopAnimation(s_horizontalOffsetPropertyName);
        m_internalSourcePropertySet.StopAnimation(s_verticalOffsetPropertyName);
        m_internalSourcePropertySet.StopAnimation(s_scalePropertyName);

        m_internalSourcePropertySet.StartAnimation(s_horizontalOffsetPropertyName, m_internalTranslationXExpressionAnimation);
        m_internalSourcePropertySet.StartAnimation(s_verticalOffsetPropertyName, m_internalTranslationYExpressionAnimation);
        m_internalSourcePropertySet.StartAnimation(s_scalePropertyName, m_internalScaleExpressionAnimation);
    }
}

// Stops the animations targeting the properties inside m_internalSourcePropertySet.
void ScrollInputHelper::StopInternalExpressionAnimations()
{
    if (m_internalSourcePropertySet)
    {
        m_internalSourcePropertySet.StopAnimation(s_horizontalOffsetPropertyName);
        m_internalSourcePropertySet.StopAnimation(s_verticalOffsetPropertyName);
        m_internalSourcePropertySet.StopAnimation(s_scalePropertyName);

        m_internalSourcePropertySet.InsertScalar(s_horizontalOffsetPropertyName, 0.0f);
        m_internalSourcePropertySet.InsertScalar(s_verticalOffsetPropertyName, 0.0f);
        m_internalSourcePropertySet.InsertScalar(s_scalePropertyName, 1.0f);
    }
}

void ScrollInputHelper::ProcessSourceElementChange(bool allowSourceElementLoadedHookup)
{
    winrt::CompositionPropertySet oldSourcePropertySet = m_sourcePropertySet;
    const bool oldIsTargetElementInSource = m_isTargetElementInSource;
    const double oldViewportWidth = GetViewportSize(winrt::Orientation::Horizontal);
    const double oldViewportHeight = GetViewportSize(winrt::Orientation::Vertical);
    const double oldContentWidth = GetContentSize(winrt::Orientation::Horizontal);
    const double oldContentHeight = GetContentSize(winrt::Orientation::Vertical);
    const double oldUnderpanWidth = GetMaxUnderpanOffset(winrt::Orientation::Horizontal);
    const double oldUnderpanHeight = GetMaxUnderpanOffset(winrt::Orientation::Vertical);
    const double oldOverpanWidth = GetMaxOverpanOffset(winrt::Orientation::Horizontal);
    const double oldOverpanHeight = GetMaxOverpanOffset(winrt::Orientation::Vertical);

    UnhookSourceElementLoaded();

    UpdateSource(allowSourceElementLoadedHookup);

    if (m_sourcePropertySet != oldSourcePropertySet ||
        m_isTargetElementInSource != oldIsTargetElementInSource)
    {
        OnSourceInfoChanged(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, true /*zoomInfoChanged*/);
    }
    else
    {
        const bool horizontalInfoChanged =
            oldViewportWidth != GetViewportSize(winrt::Orientation::Horizontal) ||
            oldContentWidth != GetContentSize(winrt::Orientation::Horizontal) ||
            oldUnderpanWidth != GetMaxUnderpanOffset(winrt::Orientation::Horizontal) ||
            oldOverpanWidth != GetMaxOverpanOffset(winrt::Orientation::Horizontal);

        const bool verticalInfoChanged =
            oldViewportHeight != GetViewportSize(winrt::Orientation::Vertical) ||
            oldContentHeight != GetContentSize(winrt::Orientation::Vertical) ||
            oldUnderpanHeight != GetMaxUnderpanOffset(winrt::Orientation::Vertical) ||
            oldOverpanHeight != GetMaxOverpanOffset(winrt::Orientation::Vertical);

        if (horizontalInfoChanged || verticalInfoChanged)
        {
            OnSourceInfoChanged(horizontalInfoChanged, verticalInfoChanged, true /*zoomInfoChanged*/);
        }
    }
}

void ScrollInputHelper::ProcessTargetElementChange()
{
    const bool oldIsTargetElementInSource = m_isTargetElementInSource;

    UpdateIsTargetElementInSource();

    if (m_isTargetElementInSource != oldIsTargetElementInSource)
    {
        OnSourceInfoChanged(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
    }
    else if (m_targetElement)
    {
        UpdateInternalExpressionAnimations(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
    }
}

// Invoked when the ScrollViewer.Content or ScrollPresenter.Content size changed.
void ScrollInputHelper::ProcessContentSizeChange()
{
    const double oldContentWidth = GetContentSize(winrt::Orientation::Horizontal);
    const double oldContentHeight = GetContentSize(winrt::Orientation::Vertical);

    UpdateContentSize();

    const double newContentWidth = GetContentSize(winrt::Orientation::Horizontal);
    const double newContentHeight = GetContentSize(winrt::Orientation::Vertical);

    if (oldContentWidth != newContentWidth || oldContentHeight != newContentHeight)
    {
        OnSourceInfoChanged(oldContentWidth != newContentWidth, oldContentHeight != newContentHeight, false /*zoomInfoChanged*/);
    }
}

// Invoked when the source element has changed.
void ScrollInputHelper::OnSourceElementChanged(bool allowSourceElementLoadedHookup)
{
    auto sourceElement = m_sourceElement.get();
    if (sourceElement)
    {
        winrt::Control sourceAsControl = sourceElement.try_as<winrt::Control>();
        winrt::FxScrollViewer sourceAsScrollViewer = sourceElement.try_as<winrt::FxScrollViewer>();

        if (sourceAsControl && !sourceAsScrollViewer)
        {
            HookSourceControlTemplateChanged();
        }
    }
    else
    {
        // No need to find the inner ScrollViewer at the next UI thread tick.
        UnhookCompositionTargetRendering();
    }

    ProcessSourceElementChange(allowSourceElementLoadedHookup);
}

// Invoked when the target element has changed.
void ScrollInputHelper::OnTargetElementChanged()
{
    auto scrollViewer = m_scrollViewer.get();
    auto targetElement = m_targetElement.get();
    auto scrollPresenter = m_scrollPresenter.get();
    if (targetElement)
    {
        winrt::DependencyObject parent = winrt::VisualTreeHelper::GetParent(targetElement);
        if (!parent)
        {
            HookTargetElementLoaded();
        }
    }

    if (scrollPresenter || scrollViewer)
    {
        EnsureInternalSourcePropertySetAndExpressionAnimations();
        m_sourcePropertySet = m_internalSourcePropertySet;
        if (targetElement)
        {
            StartInternalExpressionAnimations(scrollViewer ? m_scrollViewerPropertySet : scrollPresenter.ExpressionAnimationSources());
            if (m_isScrollViewerInDirectManipulation)
            {
                UpdateManipulationAlignments();
                UpdateManipulationZoomMode();
            }
        }

        ProcessTargetElementChange();
    }
}

// Invoked when the source is a Control other than a ScrollViewer, and its Template property changed.
void ScrollInputHelper::ProcessSourceControlTemplateChange()
{
    // Wait for one UI thread tick so the new control template gets applied and the potential inner ScrollViewer can be set.
    HookCompositionTargetRendering();
}

// Invoked when a source characteristic influencing the composition animations changed.
void ScrollInputHelper::OnSourceInfoChanged(bool horizontalInfoChanged, bool verticalInfoChanged, bool zoomInfoChanged)
{
    MUX_ASSERT(horizontalInfoChanged || verticalInfoChanged);

    if ((m_scrollPresenter || m_scrollViewer) && m_targetElement)
    {
        UpdateInternalExpressionAnimations(horizontalInfoChanged, verticalInfoChanged, zoomInfoChanged);
    }

    if (m_infoChangedFunction)
    {
        // Let the ScrollInputHelper consumer know about the characteristic change too.
        m_infoChangedFunction(horizontalInfoChanged, verticalInfoChanged);
    }
}

void ScrollInputHelper::OnTargetElementLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    UnhookTargetElementLoaded();
    ProcessTargetElementChange();
}

void ScrollInputHelper::OnSourceElementLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    UnhookSourceElementLoaded();
    ProcessSourceElementChange(false /*allowSourceElementLoadedHookup*/);
}

void ScrollInputHelper::OnSourceElementPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::Control::TemplateProperty())
    {
        ProcessSourceControlTemplateChange();
    }
}

void ScrollInputHelper::ProcessScrollViewerContentChange()
{
    UnhookScrollViewerContentPropertyChanged();

    m_sourceContent.set(nullptr);

    auto scrollViewer = m_scrollViewer.get();
    if (scrollViewer)
    {
        winrt::IInspectable newContent = scrollViewer.Content();

        if (newContent)
        {
            m_sourceContent.set(newContent.try_as<winrt::FrameworkElement>());

            HookScrollViewerContentPropertyChanged();
        }
    }

    OnSourceInfoChanged(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, true /*zoomInfoChanged*/);
}

void ScrollInputHelper::ProcessScrollPresenterContentChange()
{
    auto scrollPresenter = m_scrollPresenter.get();
    UnhookScrollPresenterContentPropertyChanged();

    m_sourceContent.set(nullptr);

    if (scrollPresenter)
    {
        winrt::UIElement newContent = scrollPresenter.Content();

        if (newContent)
        {
            m_sourceContent.set(newContent.try_as<winrt::FrameworkElement>());

            HookScrollPresenterContentPropertyChanged();
        }
    }

    OnSourceInfoChanged(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, true /*zoomInfoChanged*/);
}

void ScrollInputHelper::ProcessScrollViewerZoomModeChange()
{
    UpdateOutOfBoundsPanSize();
    OnSourceInfoChanged(true /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
}

void ScrollInputHelper::OnSourceSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    const double oldViewportWidth = GetViewportSize(winrt::Orientation::Horizontal);
    const double oldViewportHeight = GetViewportSize(winrt::Orientation::Vertical);
    const double oldUnderpanWidth = GetMaxUnderpanOffset(winrt::Orientation::Horizontal);
    const double oldUnderpanHeight = GetMaxUnderpanOffset(winrt::Orientation::Vertical);
    const double oldOverpanWidth = GetMaxOverpanOffset(winrt::Orientation::Horizontal);
    const double oldOverpanHeight = GetMaxOverpanOffset(winrt::Orientation::Vertical);

    UpdateViewportSize();
    UpdateOutOfBoundsPanSize();

    const bool horizontalInfoChanged =
        oldViewportWidth != GetViewportSize(winrt::Orientation::Horizontal) ||
        oldUnderpanWidth != GetMaxUnderpanOffset(winrt::Orientation::Horizontal) ||
        oldOverpanWidth != GetMaxOverpanOffset(winrt::Orientation::Horizontal);

    const bool verticalInfoChanged =
        oldViewportHeight != GetViewportSize(winrt::Orientation::Vertical) ||
        oldUnderpanHeight != GetMaxUnderpanOffset(winrt::Orientation::Vertical) ||
        oldOverpanHeight != GetMaxOverpanOffset(winrt::Orientation::Vertical);

    if (horizontalInfoChanged || verticalInfoChanged)
    {
        OnSourceInfoChanged(horizontalInfoChanged, verticalInfoChanged, false /*zoomInfoChanged*/);
    }
}

void ScrollInputHelper::OnScrollViewerDirectManipulationStarted(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    // Alignment and zoom mode changes during a manipulation are ignored until the end of that manipulation.

    m_isScrollViewerInDirectManipulation = true;

    UpdateManipulationAlignments();
    UpdateManipulationZoomMode();
}

void ScrollInputHelper::OnScrollViewerDirectManipulationCompleted(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    // Alignment and zoom mode changes that occurred during this completed manipulation are now taken into account.

    winrt::HorizontalAlignment oldEffectiveHorizontalAlignment = winrt::HorizontalAlignment::Left;
    winrt::VerticalAlignment oldEffectiveVerticalAlignment = winrt::VerticalAlignment::Top;
    winrt::FxZoomMode oldZoomMode = winrt::FxZoomMode::Disabled;

    if (m_targetElement)
    {
        oldEffectiveHorizontalAlignment = GetEffectiveHorizontalAlignment();
        oldEffectiveVerticalAlignment = GetEffectiveVerticalAlignment();
        oldZoomMode = GetEffectiveZoomMode();
    }

    m_isScrollViewerInDirectManipulation = false;

    if (m_targetElement)
    {
        const winrt::FxZoomMode newZoomMode = GetEffectiveZoomMode();

        if (oldZoomMode != newZoomMode)
        {
            ProcessScrollViewerZoomModeChange();
        }

        const winrt::HorizontalAlignment newEffectiveHorizontalAlignment = GetEffectiveHorizontalAlignment();
        const winrt::VerticalAlignment newEffectiveVerticalAlignment = GetEffectiveVerticalAlignment();

        if (oldEffectiveHorizontalAlignment != newEffectiveHorizontalAlignment || oldEffectiveVerticalAlignment != newEffectiveVerticalAlignment)
        {
            UpdateInternalExpressionAnimations(
                oldEffectiveHorizontalAlignment != newEffectiveHorizontalAlignment,
                oldEffectiveVerticalAlignment != newEffectiveVerticalAlignment,
                false /*zoomInfoChanged*/);
        }
    }
}

void ScrollInputHelper::OnRichEditBoxTextChanged(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    ProcessContentSizeChange();
}

void ScrollInputHelper::OnCompositionTargetRendering(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    // Unhook the Rendering event handler and attempt to find the potential new inner ScrollViewer.
    UnhookCompositionTargetRendering();
    ProcessSourceElementChange(false /*allowSourceElementLoadedHookup*/);
}

void ScrollInputHelper::OnSourceContentSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    ProcessContentSizeChange();
}

// Invoked when a tracked dependency property changes for the ScrollViewer dependency object.
void ScrollInputHelper::OnScrollViewerPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::ContentControl::ContentProperty())
    {
        ProcessScrollViewerContentChange();
    }
    else if (args == winrt::FxScrollViewer::ZoomModeProperty())
    {
        if (!m_isScrollViewerInDirectManipulation)
        {
            ProcessScrollViewerZoomModeChange();
        }
    }
    else if (args == winrt::Control::HorizontalContentAlignmentProperty())
    {
        if (!m_isScrollViewerInDirectManipulation)
        {
            UpdateInternalExpressionAnimations(true /*horizontalInfoChanged*/, false /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
        }
    }
    else if (args == winrt::Control::VerticalContentAlignmentProperty())
    {
        if (!m_isScrollViewerInDirectManipulation)
        {
            UpdateInternalExpressionAnimations(false /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
        }
    }
}

// Invoked when a tracked dependency property changes for the ScrollPresenter dependency object.
void ScrollInputHelper::OnScrollPresenterPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::ScrollPresenter::ContentProperty())
    {
        ProcessScrollPresenterContentChange();
    }
}

// Invoked when a tracked dependency property changes for the ScrollViewer.Content dependency object.
void ScrollInputHelper::OnScrollViewerContentPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& args)
{
    if (args == winrt::FrameworkElement::HorizontalAlignmentProperty())
    {
        if (!m_isScrollViewerInDirectManipulation)
        {
            UpdateInternalExpressionAnimations(true /*horizontalInfoChanged*/, false /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
        }
    }
    else if (args == winrt::FrameworkElement::VerticalAlignmentProperty())
    {
        if (!m_isScrollViewerInDirectManipulation)
        {
            UpdateInternalExpressionAnimations(false /*horizontalInfoChanged*/, true /*verticalInfoChanged*/, false /*zoomInfoChanged*/);
        }
    }
}

void ScrollInputHelper::HookSourceElementLoaded()
{
    auto sourceElement = m_sourceElement.get();
    if (sourceElement && m_sourceElementLoadedToken.value == 0)
    {
        winrt::FrameworkElement sourceElementAsFrameworkElement = sourceElement.try_as<winrt::FrameworkElement>();

        if (sourceElementAsFrameworkElement)
        {
            m_sourceElementLoadedToken = sourceElementAsFrameworkElement.Loaded({ this, &ScrollInputHelper::OnSourceElementLoaded });
        }
    }
}

void ScrollInputHelper::UnhookSourceElementLoaded()
{
    auto sourceElement = m_sourceElement.safe_get();
    if (sourceElement && m_sourceElementLoadedToken.value != 0)
    {
        winrt::FrameworkElement sourceElementAsFrameworkElement = sourceElement.try_as<winrt::FrameworkElement>();

        if (sourceElementAsFrameworkElement)
        {
            sourceElementAsFrameworkElement.Loaded(m_sourceElementLoadedToken);
            m_sourceElementLoadedToken.value = 0;
        }
    }
}

void ScrollInputHelper::HookSourceControlTemplateChanged()
{
    auto sourceElement = m_sourceElement.get();
    if (sourceElement && m_sourceControlTemplateChangedToken.value == 0)
    {
        m_sourceControlTemplateChangedToken.value = sourceElement.RegisterPropertyChangedCallback(
            winrt::Control::TemplateProperty(), { this, &ScrollInputHelper::OnSourceElementPropertyChanged });
    }
}

void ScrollInputHelper::UnhookSourceControlTemplateChanged()
{
    auto sourceElement = m_sourceElement.safe_get();
    if (sourceElement && m_sourceControlTemplateChangedToken.value != 0)
    {
        sourceElement.UnregisterPropertyChangedCallback(winrt::Control::TemplateProperty(), m_sourceControlTemplateChangedToken.value);
        m_sourceControlTemplateChangedToken.value = 0;
    }
}

void ScrollInputHelper::HookTargetElementLoaded()
{
    auto targetElement = m_targetElement.get();

    if (targetElement && m_targetElementLoadedToken.value == 0)
    {
        winrt::FrameworkElement targetElementAsFrameworkElement = targetElement.try_as<winrt::FrameworkElement>();

        if (targetElementAsFrameworkElement)
        {
            m_targetElementLoadedToken = targetElementAsFrameworkElement.Loaded({ this, &ScrollInputHelper::OnTargetElementLoaded });
        }
    }
}

void ScrollInputHelper::UnhookTargetElementLoaded()
{
    auto targetElement = m_targetElement.safe_get();
    if (targetElement && m_targetElementLoadedToken.value != 0)
    {
        winrt::FrameworkElement targetElementAsFrameworkElement = targetElement.try_as<winrt::FrameworkElement>();

        if (targetElementAsFrameworkElement)
        {
            targetElementAsFrameworkElement.Loaded(m_targetElementLoadedToken);
            m_targetElementLoadedToken.value = 0;
        }
    }
}

void ScrollInputHelper::HookScrollViewerPropertyChanged()
{
    auto scrollViewer = m_scrollViewer.get();
    if (scrollViewer)
    {
        MUX_ASSERT(m_scrollViewerContentChangedToken.value == 0);
        MUX_ASSERT(m_scrollViewerHorizontalContentAlignmentChangedToken.value == 0);
        MUX_ASSERT(m_scrollViewerVerticalContentAlignmentChangedToken.value == 0);
        MUX_ASSERT(m_scrollViewerZoomModeChangedToken.value == 0);
        MUX_ASSERT(m_sourceSizeChangedToken.value == 0);

        m_scrollViewerContentChangedToken.value = scrollViewer.RegisterPropertyChangedCallback(
            winrt::ContentControl::ContentProperty(), { this, &ScrollInputHelper::OnScrollViewerPropertyChanged });
        m_scrollViewerHorizontalContentAlignmentChangedToken.value = scrollViewer.RegisterPropertyChangedCallback(
            winrt::Control::HorizontalContentAlignmentProperty(), { this, &ScrollInputHelper::OnScrollViewerPropertyChanged });
        m_scrollViewerVerticalContentAlignmentChangedToken.value = scrollViewer.RegisterPropertyChangedCallback(
            winrt::Control::VerticalContentAlignmentProperty(), { this, &ScrollInputHelper::OnScrollViewerPropertyChanged });
        m_scrollViewerZoomModeChangedToken.value = scrollViewer.RegisterPropertyChangedCallback(
            winrt::FxScrollViewer::ZoomModeProperty(), { this, &ScrollInputHelper::OnScrollViewerPropertyChanged });
        m_sourceSizeChangedToken = scrollViewer.SizeChanged({ this, &ScrollInputHelper::OnSourceSizeChanged });
    }
}

void ScrollInputHelper::HookScrollPresenterPropertyChanged()
{
    auto scrollPresenter = m_scrollPresenter.get();

    if (scrollPresenter)
    {
        MUX_ASSERT(m_scrollPresenterContentChangedToken.value == 0);
        MUX_ASSERT(m_sourceSizeChangedToken.value == 0);

        m_scrollPresenterContentChangedToken.value = scrollPresenter.RegisterPropertyChangedCallback(
            winrt::ScrollPresenter::ContentProperty(), { this, &ScrollInputHelper::OnScrollPresenterPropertyChanged });
        m_sourceSizeChangedToken = scrollPresenter.SizeChanged({ this, &ScrollInputHelper::OnSourceSizeChanged });
    }
}

void ScrollInputHelper::UnhookScrollViewerPropertyChanged()
{
    auto scrollViewer = m_scrollViewer.safe_get();
    if (scrollViewer)
    {
        if (m_scrollViewerContentChangedToken.value != 0)
        {
            scrollViewer.UnregisterPropertyChangedCallback(winrt::ContentControl::ContentProperty(), m_scrollViewerContentChangedToken.value);
            m_scrollViewerContentChangedToken.value = 0;
        }
        if (m_scrollViewerHorizontalContentAlignmentChangedToken.value != 0)
        {
            scrollViewer.UnregisterPropertyChangedCallback(winrt::Control::HorizontalContentAlignmentProperty(), m_scrollViewerHorizontalContentAlignmentChangedToken.value);
            m_scrollViewerHorizontalContentAlignmentChangedToken.value = 0;
        }
        if (m_scrollViewerVerticalContentAlignmentChangedToken.value != 0)
        {
            scrollViewer.UnregisterPropertyChangedCallback(winrt::Control::VerticalContentAlignmentProperty(), m_scrollViewerVerticalContentAlignmentChangedToken.value);
            m_scrollViewerVerticalContentAlignmentChangedToken.value = 0;
        }
        if (m_scrollViewerZoomModeChangedToken.value != 0)
        {
            scrollViewer.UnregisterPropertyChangedCallback(winrt::FxScrollViewer::ZoomModeProperty(), m_scrollViewerZoomModeChangedToken.value);
            m_scrollViewerZoomModeChangedToken.value = 0;
        }
        if (m_sourceSizeChangedToken.value != 0)
        {
            scrollViewer.SizeChanged(m_sourceSizeChangedToken);
            m_sourceSizeChangedToken.value = 0;
        }
    }
}

void ScrollInputHelper::UnhookScrollPresenterPropertyChanged()
{
    auto scrollPresenter = m_scrollPresenter.safe_get();

    if (scrollPresenter)
    {
        if (m_scrollPresenterContentChangedToken.value != 0)
        {
            scrollPresenter.UnregisterPropertyChangedCallback(winrt::ScrollPresenter::ContentProperty(), m_scrollPresenterContentChangedToken.value);
            m_scrollPresenterContentChangedToken.value = 0;
        }
        if (m_sourceSizeChangedToken.value != 0)
        {
            scrollPresenter.SizeChanged(m_sourceSizeChangedToken);
            m_sourceSizeChangedToken.value = 0;
        }
    }
}

void ScrollInputHelper::HookScrollViewerContentPropertyChanged()
{
    auto sourceContent = m_sourceContent.get();
    if (sourceContent)
    {
        if (m_scrollViewerContentHorizontalAlignmentChangedToken.value == 0)
        {
            m_scrollViewerContentHorizontalAlignmentChangedToken.value = sourceContent.RegisterPropertyChangedCallback(
                winrt::FrameworkElement::HorizontalAlignmentProperty(), { this, &ScrollInputHelper::OnScrollViewerContentPropertyChanged });
        }
        if (m_scrollViewerContentVerticalAlignmentChangedToken.value == 0)
        {
            m_scrollViewerContentVerticalAlignmentChangedToken.value = sourceContent.RegisterPropertyChangedCallback(
                winrt::FrameworkElement::VerticalAlignmentProperty(), { this, &ScrollInputHelper::OnScrollViewerContentPropertyChanged });
        }
        if (m_sourceContentSizeChangedToken.value == 0)
        {
            m_sourceContentSizeChangedToken = sourceContent.SizeChanged({ this, &ScrollInputHelper::OnSourceContentSizeChanged });
        }
    }
}

void ScrollInputHelper::HookScrollPresenterContentPropertyChanged()
{
    auto sourceContent = m_sourceContent.get();

    if (sourceContent)
    {
        if (m_sourceContentSizeChangedToken.value == 0)
        {
            m_sourceContentSizeChangedToken = sourceContent.SizeChanged({ this, &ScrollInputHelper::OnSourceContentSizeChanged });
        }
    }
}

void ScrollInputHelper::UnhookScrollViewerContentPropertyChanged()
{
    auto sourceContent = m_sourceContent.safe_get();

    if (sourceContent)
    {
        if (m_scrollViewerContentHorizontalAlignmentChangedToken.value != 0)
        {
            sourceContent.UnregisterPropertyChangedCallback(winrt::FrameworkElement::HorizontalAlignmentProperty(), m_scrollViewerContentHorizontalAlignmentChangedToken.value);
            m_scrollViewerContentHorizontalAlignmentChangedToken.value = 0;
        }
        if (m_scrollViewerContentVerticalAlignmentChangedToken.value != 0)
        {
            sourceContent.UnregisterPropertyChangedCallback(winrt::FrameworkElement::VerticalAlignmentProperty(), m_scrollViewerContentVerticalAlignmentChangedToken.value);
            m_scrollViewerContentVerticalAlignmentChangedToken.value = 0;
        }
        if (m_sourceContentSizeChangedToken.value != 0)
        {
            sourceContent.SizeChanged(m_sourceContentSizeChangedToken);
            m_sourceContentSizeChangedToken.value = 0;
        }
    }
}

// Note that if in the future the ScrollPresenter supports a virtual mode where the extent does not
// correspond to its Content size, the ScrollPresenter will need to raise an event when its virtual extent
// changes so that the ScrollPresenter.ExpressionAnimationSources's Extent composition property can
// be read. This should replace hooking up the SizeChanged event on the ScrollPresenter.Content altogether.
void ScrollInputHelper::UnhookScrollPresenterContentPropertyChanged()
{
    auto sourceContent = m_sourceContent.safe_get();

    if (sourceContent)
    {
        if (m_sourceContentSizeChangedToken.value != 0)
        {
            sourceContent.SizeChanged(m_sourceContentSizeChangedToken);
            m_sourceContentSizeChangedToken.value = 0;
        }
    }
}

void ScrollInputHelper::HookScrollViewerDirectManipulationStarted()
{
    auto scrollViewer = m_scrollViewer.get();
    if (scrollViewer)
    {
        MUX_ASSERT(m_scrollViewerDirectManipulationStartedToken.value == 0);

        m_scrollViewerDirectManipulationStartedToken = scrollViewer.DirectManipulationStarted({ this, &ScrollInputHelper::OnScrollViewerDirectManipulationStarted });
    }
}

void ScrollInputHelper::UnhookScrollViewerDirectManipulationStarted()
{
    auto scrollViewer = m_scrollViewer.safe_get();
    if (scrollViewer && m_scrollViewerDirectManipulationStartedToken.value != 0)
    {
        scrollViewer.DirectManipulationStarted(m_scrollViewerDirectManipulationStartedToken);
        m_scrollViewerDirectManipulationStartedToken.value = 0;
    }
}

void ScrollInputHelper::HookScrollViewerDirectManipulationCompleted()
{
    auto scrollViewer = m_scrollViewer.get();
    if (scrollViewer)
    {
        MUX_ASSERT(m_scrollViewerDirectManipulationCompletedToken.value == 0);

        m_scrollViewerDirectManipulationCompletedToken = scrollViewer.DirectManipulationCompleted({ this, &ScrollInputHelper::OnScrollViewerDirectManipulationCompleted });
    }
}

void ScrollInputHelper::UnhookScrollViewerDirectManipulationCompleted()
{
    auto scrollViewer = m_scrollViewer.safe_get();
    if (scrollViewer && m_scrollViewerDirectManipulationCompletedToken.value != 0)
    {
        scrollViewer.DirectManipulationCompleted(m_scrollViewerDirectManipulationCompletedToken);
        m_scrollViewerDirectManipulationCompletedToken.value = 0;
    }
}

void ScrollInputHelper::HookRichEditBoxTextChanged()
{
    auto richEditBox = m_richEditBox.get();
    if (richEditBox)
    {
        MUX_ASSERT(m_richEditBoxTextChangedToken.value == 0);

        m_richEditBoxTextChangedToken = richEditBox.TextChanged({ this, &ScrollInputHelper::OnRichEditBoxTextChanged });
    }
}

void ScrollInputHelper::UnhookRichEditBoxTextChanged()
{
    auto richEditBox = m_richEditBox.safe_get();
    if (richEditBox && m_richEditBoxTextChangedToken.value != 0)
    {
        richEditBox.TextChanged(m_richEditBoxTextChangedToken);
        m_richEditBoxTextChangedToken.value = 0;
    }
}

void ScrollInputHelper::HookCompositionTargetRendering()
{
    if (m_renderingToken.value == 0)
    {
        winrt::Windows::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };

        m_renderingToken = compositionTarget.Rendering({ this, &ScrollInputHelper::OnCompositionTargetRendering });
    }
}

void ScrollInputHelper::UnhookCompositionTargetRendering()
{
    if (m_renderingToken.value != 0)
    {
        winrt::Windows::UI::Xaml::Media::CompositionTarget compositionTarget{ nullptr };

        compositionTarget.Rendering(m_renderingToken);
        m_renderingToken.value = 0;
    }
}
