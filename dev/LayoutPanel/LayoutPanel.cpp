// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "LayoutPanel.h"
#include "LayoutPanelLayoutContext.h"

GlobalDependencyProperty LayoutPanel::s_layoutProperty{ nullptr };
GlobalDependencyProperty LayoutPanel::s_borderBrushProperty{ nullptr };
GlobalDependencyProperty LayoutPanel::s_borderThicknessProperty{ nullptr };
GlobalDependencyProperty LayoutPanel::s_cornerRadiusProperty{ nullptr };
GlobalDependencyProperty LayoutPanel::s_paddingProperty{ nullptr };

/*static*/
void LayoutPanel::EnsureProperties()
{
    if (!s_layoutProperty)
    {
        s_layoutProperty =
            InitializeDependencyProperty(
                L"Layout",
                winrt::name_of<winrt::Layout>(),
                winrt::name_of<winrt::LayoutPanel>(),
                false /* isAttached */,
                nullptr, /* defaultValue */
                winrt::PropertyChangedCallback(&LayoutPanel::OnPropertyChanged));
    }
    if (!s_borderBrushProperty)
    {
        s_borderBrushProperty =
            InitializeDependencyProperty(
                L"BorderBrush",
                winrt::name_of<winrt::Brush>(),
                winrt::name_of<winrt::LayoutPanel>(),
                false /* isAttached */,
                nullptr, /* defaultValue */
                winrt::PropertyChangedCallback(&LayoutPanel::OnPropertyChanged));
    }
    if (!s_borderThicknessProperty)
    {
        s_borderThicknessProperty =
            InitializeDependencyProperty(
                L"BorderThickness",
                winrt::name_of<winrt::Thickness>(),
                winrt::name_of<winrt::LayoutPanel>(),
                false /* isAttached */,
                box_value(winrt::Thickness{ 0,0,0,0 }), /* defaultValue */
                winrt::PropertyChangedCallback(&LayoutPanel::OnPropertyChanged));
    }
    if (!s_cornerRadiusProperty)
    {
        s_cornerRadiusProperty =
            InitializeDependencyProperty(
                L"CornerRadius",
                winrt::name_of<winrt::CornerRadius>(),
                winrt::name_of<winrt::LayoutPanel>(),
                false /* isAttached */,
                box_value(winrt::CornerRadius{ 0,0,0,0 }), /* defaultValue */
                winrt::PropertyChangedCallback(&LayoutPanel::OnPropertyChanged));
    }
    if (!s_paddingProperty)
    {
        s_paddingProperty =
            InitializeDependencyProperty(
                L"Padding",
                winrt::name_of<winrt::Thickness>(),
                winrt::name_of<winrt::LayoutPanel>(),
                false /* isAttached */,
                box_value(winrt::Thickness{ 0,0,0,0 }), /* defaultValue */
                winrt::PropertyChangedCallback(&LayoutPanel::OnPropertyChanged));
    }
}

/*static*/
void LayoutPanel::ClearProperties()
{
    s_layoutProperty = nullptr;
    s_borderBrushProperty = nullptr;
    s_borderThicknessProperty = nullptr;
    s_cornerRadiusProperty = nullptr;
    s_paddingProperty = nullptr;
}

/*static*/
void LayoutPanel::OnPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::get_self<LayoutPanel>(sender.as<winrt::LayoutPanel>())->OnPropertyChanged(args);
}

void LayoutPanel::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    const winrt::IDependencyProperty dependencyProperty = args.Property();

    if (dependencyProperty == s_layoutProperty)
    {
        OnLayoutChanged(args.OldValue().as<winrt::Layout>(), args.NewValue().as<winrt::Layout>());
    }
#ifdef USE_INTERNAL_SDK
    else if (dependencyProperty == s_borderBrushProperty)
    {
        if (auto panelProtected = try_as<winrt::Windows::UI::Xaml::Controls::IPanelProtectedFeature_WUXCPreviewTypes>())
        {
            panelProtected.BorderBrushProtected(safe_cast<winrt::Brush>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_borderThicknessProperty)
    {
        if (auto panelProtected = try_as<winrt::Windows::UI::Xaml::Controls::IPanelProtectedFeature_WUXCPreviewTypes>())
        {
            panelProtected.BorderThicknessProtected(unbox_value<winrt::Thickness>(args.NewValue()));
        }
    }
    else if (dependencyProperty == s_cornerRadiusProperty)
    {
        if (auto panelProtected = try_as<winrt::Windows::UI::Xaml::Controls::IPanelProtectedFeature_WUXCPreviewTypes>())
        {
            panelProtected.CornerRadiusProtected(unbox_value<winrt::CornerRadius>(args.NewValue()));
        }
    }
#endif
    else if (dependencyProperty == s_paddingProperty)
    {
        InvalidateMeasure();
    }
}

winrt::Layout LayoutPanel::Layout()
{
    return m_layout.get();
}

void LayoutPanel::Layout(winrt::Layout const& value)
{
    SetValue(s_layoutProperty, value);
}

winrt::Brush LayoutPanel::BorderBrush()
{
    return GetValue(s_borderBrushProperty).as<winrt::Brush>();
}

void LayoutPanel::BorderBrush(winrt::Brush const& value)
{
    SetValue(s_borderBrushProperty, value);
}

winrt::Thickness LayoutPanel::BorderThickness()
{
    return auto_unbox(GetValue(s_borderThicknessProperty));
}

void LayoutPanel::BorderThickness(winrt::Thickness const& value)
{
    SetValue(s_borderThicknessProperty, box_value(value));
}

winrt::CornerRadius LayoutPanel::CornerRadius()
{
    return auto_unbox(GetValue(s_cornerRadiusProperty));
}

void LayoutPanel::CornerRadius(winrt::CornerRadius const& value)
{
    SetValue(s_cornerRadiusProperty, box_value(value));
}

winrt::Thickness LayoutPanel::Padding()
{
    return auto_unbox(GetValue(s_paddingProperty));
}

void LayoutPanel::Padding(winrt::Thickness const& value)
{
    SetValue(s_paddingProperty, box_value(value));
}

winrt::Size LayoutPanel::MeasureOverride(winrt::Size const& availableSize)
{
    winrt::Size desiredSize{};

    // We adjust availableSize based on our Padding and BorderThickness:
    auto padding = Padding();
    auto borderThickness = BorderThickness();
    auto effectiveHorizontalPadding = static_cast<float>(padding.Left + padding.Right + borderThickness.Left + borderThickness.Right);
    auto effectiveVerticalPadding = static_cast<float>(padding.Top + padding.Bottom + borderThickness.Top + borderThickness.Bottom);

    auto adjustedSize = availableSize;
    adjustedSize.Width -= effectiveHorizontalPadding;
    adjustedSize.Height -= effectiveVerticalPadding;

    adjustedSize.Width = std::max(0.0f, adjustedSize.Width);
    adjustedSize.Height = std::max(0.0f, adjustedSize.Height);

    if (auto layout = Layout())
    {
        auto layoutDesiredSize = layout.Measure(m_layoutContext, adjustedSize);
        layoutDesiredSize.Width += effectiveHorizontalPadding;
        layoutDesiredSize.Height += effectiveVerticalPadding;
        desiredSize = layoutDesiredSize;
    }
    else
    {
        winrt::Size desiredSizeUnpadded{};
        for (winrt::UIElement const& child : Children())
        {
            child.Measure(adjustedSize);
            auto childDesiredSize = child.DesiredSize();
            desiredSizeUnpadded.Width = std::max(desiredSizeUnpadded.Width, childDesiredSize.Width);
            desiredSizeUnpadded.Height = std::max(desiredSizeUnpadded.Height, childDesiredSize.Height);
        }
        desiredSize = desiredSizeUnpadded;
        desiredSize.Width += effectiveHorizontalPadding;
        desiredSize.Height += effectiveVerticalPadding;
    }
    return desiredSize;
}

winrt::Size LayoutPanel::ArrangeOverride(winrt::Size const& finalSize)
{
    winrt::Size result = finalSize;

    auto padding = Padding();
    auto borderThickness = BorderThickness();

    auto effectiveHorizontalPadding = static_cast<float>(padding.Left + padding.Right + borderThickness.Left + borderThickness.Right);
    auto effectiveVerticalPadding = static_cast<float>(padding.Top + padding.Bottom + borderThickness.Top + borderThickness.Bottom);
    auto leftAdjustment = static_cast<float>(padding.Left + borderThickness.Left);
    auto topAdjustment = static_cast<float>(padding.Top + borderThickness.Top);

    auto adjustedSize = finalSize;
    adjustedSize.Width -= effectiveHorizontalPadding;
    adjustedSize.Height -= effectiveVerticalPadding;

    adjustedSize.Width = std::max(0.0f, adjustedSize.Width);
    adjustedSize.Height = std::max(0.0f, adjustedSize.Height);
    
    if (auto layout = Layout())
    {
        auto layoutSize = layout.Arrange(m_layoutContext, adjustedSize);
        layoutSize.Width += effectiveHorizontalPadding;
        layoutSize.Height += effectiveVerticalPadding;

        // We need to reposition the child elements if we have top or left padding:
        if (leftAdjustment != 0 || topAdjustment != 0)
        {
            for (winrt::UIElement const& child: Children())
            {
                if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
                {
                    auto layoutSlot = winrt::LayoutInformation::GetLayoutSlot(childAsFe);
                    layoutSlot.X += leftAdjustment;
                    layoutSlot.Y += topAdjustment;
                    childAsFe.Arrange(layoutSlot);
                }
            }
        }

        result = layoutSize;
    }
    else
    {
        winrt::Rect arrangeRect = { leftAdjustment, topAdjustment, adjustedSize.Width, adjustedSize.Height };
        for (winrt::UIElement const& child : Children())
        {
            child.Arrange(arrangeRect);
        }
    }

    return result;
}

void LayoutPanel::OnLayoutChanged(winrt::Layout const& oldValue, winrt::Layout const& newValue)
{
    if (!m_layoutContext)
    {
        m_layoutContext = winrt::make<LayoutPanelLayoutContext>(*this);
    }

    if (oldValue)
    {
        oldValue.UninitializeForContext(m_layoutContext);
        m_measureInvalidated.revoke();
        m_arrangeInvalidated.revoke();
    }

    m_layout.set(newValue);

    if (newValue)
    {
        newValue.InitializeForContext(m_layoutContext);
        m_measureInvalidated = newValue.MeasureInvalidated(winrt::auto_revoke, { this, &LayoutPanel::InvalidateMeasureForLayout });
        m_arrangeInvalidated = newValue.ArrangeInvalidated(winrt::auto_revoke, { this, &LayoutPanel::InvalidateArrangeForLayout });
    }

    InvalidateMeasure();
}

void LayoutPanel::InvalidateMeasureForLayout(winrt::Layout const& sender, winrt::IInspectable const& args)
{
    InvalidateMeasure();
}

void LayoutPanel::InvalidateArrangeForLayout(winrt::Layout const& sender, winrt::IInspectable const& args)
{
    InvalidateArrange();
}
