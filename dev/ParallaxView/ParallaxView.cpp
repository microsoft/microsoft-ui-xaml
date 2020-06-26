// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ParallaxView.h"
#include "RuntimeProfiler.h"

using namespace std;

ParallaxView::~ParallaxView()
{
    UnhookChildPropertyChanged(true /* isInDestructor */);
}

ParallaxView::ParallaxView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ParallaxView);

    std::shared_ptr<ScrollInputHelper> scrollInputHelper(
        std::make_shared<ScrollInputHelper>(
            this,
            [this](bool horizontalInfoChanged, bool verticalInfoChanged)
            { 
                OnScrollInputHelperInfoChanged(horizontalInfoChanged, verticalInfoChanged);
            }));
    m_scrollInputHelper = scrollInputHelper;

    HookLoaded();
    HookSizeChanged();
}


void ParallaxView::RefreshAutomaticHorizontalOffsets()
{
    // This method is meant to be invoked when the ParallaxView is parallaxing horizontally, is
    // placed within a scrollPresenter content and its horizontal offset within that content has changed.
    if (HorizontalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative && HorizontalShift() != 0.0)
    {
        UpdateStartOffsetExpression(winrt::Orientation::Horizontal);
        UpdateEndOffsetExpression(winrt::Orientation::Horizontal);
    }
}

void ParallaxView::RefreshAutomaticVerticalOffsets()
{
    // This method is meant to be invoked when the ParallaxView is parallaxing vertically, is
    // placed within a scrollPresenter content and its vertical offset within that content has changed.
    if (VerticalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative && VerticalShift() != 0.0)
    {
        UpdateStartOffsetExpression(winrt::Orientation::Vertical);
        UpdateEndOffsetExpression(winrt::Orientation::Vertical);
    }
}
#pragma endregion

#pragma region IFrameworkElementOverridesHelper
winrt::Size ParallaxView::MeasureOverride(winrt::Size const& availableSize)
{
    winrt::Size childDesiredSize{ 0.0f, 0.0f };
    winrt::UIElement child = Child();

    if (child)
    {
        // Include the HorizontalShift/VerticalShift amounts in the available size so the desired child size
        // accounts for the parallaxing effect.
        const winrt::Size childAvailableSize { 
            static_cast<float>(availableSize.Width + abs(HorizontalShift())), 
            static_cast<float>(availableSize.Height + abs(VerticalShift())) };
        child.Measure(childAvailableSize);
        childDesiredSize = child.DesiredSize();
    }
    
    return {
        std::isinf(availableSize.Width) ? childDesiredSize.Width : availableSize.Width,
        std::isinf(availableSize.Height) ? childDesiredSize.Height : availableSize.Height };
}

winrt::Size ParallaxView::ArrangeOverride(winrt::Size const& finalSize)
{
    winrt::UIElement child = Child();

    if (child)
    {
        winrt::FrameworkElement childAsFE = child.try_as<winrt::FrameworkElement>();
        winrt::Rect finalRect = { 0.0f, 0.0f, child.DesiredSize().Width, child.DesiredSize().Height };

        if (HorizontalShift() != 0.0 && finalRect.Width < finalSize.Width + abs(HorizontalShift()))
        {
            // The child is parallaxing horizontally. Ensure that its arrange width exceeds the ParallaxView's arrange width by at least HorizontalShift.
            // Expand its height by the same ratio if it's stretched vertically.
            const float stretchRatio = finalRect.Width > 0.0f ? (finalSize.Width + static_cast<float>(abs(HorizontalShift()))) / finalRect.Width : 0.0f;
            finalRect.Width = finalSize.Width + static_cast<float>(abs(HorizontalShift()));
            if (stretchRatio != 0.0f && childAsFE && isnan(childAsFE.Height()) && childAsFE.VerticalAlignment() == winrt::VerticalAlignment::Stretch)
            {
                finalRect.Height *= stretchRatio;
            }
        }
        if (VerticalShift() != 0.0 && finalRect.Height < finalSize.Height + abs(VerticalShift()))
        {
            // The child is parallaxing vertically. Ensure that its arrange height exceeds the ParallaxView's arrange height by at least VerticalShift.
            // Expand its width by the same ratio if it's stretched horizontally.
            const float stretchRatio = finalRect.Height > 0.0f ? (finalSize.Height + static_cast<float>(abs(VerticalShift()))) / finalRect.Height : 0.0f;
            finalRect.Height = finalSize.Height + static_cast<float>(abs(VerticalShift()));
            if (stretchRatio != 0.0f && childAsFE && isnan(childAsFE.Width()) && childAsFE.HorizontalAlignment() == winrt::HorizontalAlignment::Stretch)
            {
                finalRect.Width *= stretchRatio;
            }
        }

        if (childAsFE)
        {
            // Ensure the child is properly aligned within the ParallaxView based on its
            // alignment properties. This alignment behavior is identical to the Border's behavior.
            float offset = 0.0f;

            switch (childAsFE.HorizontalAlignment())
            {
            case winrt::HorizontalAlignment::Center:
                offset = (finalSize.Width - finalRect.Width) / 2.0f;
                break;
            case winrt::HorizontalAlignment::Right:
                offset = finalSize.Width - finalRect.Width;
                break;
            case winrt::HorizontalAlignment::Stretch:
                if (finalRect.Width < finalSize.Width)
                {
                    offset = (finalSize.Width - finalRect.Width) / 2.0f;
                }
                break;
            }
            finalRect.X = offset;

            offset = 0.0f;

            switch (childAsFE.VerticalAlignment())
            {
            case winrt::VerticalAlignment::Center:
                offset = (finalSize.Height - finalRect.Height) / 2.0f;
                break;
            case winrt::VerticalAlignment::Bottom:
                offset = finalSize.Height - finalRect.Height;
                break;
            case winrt::VerticalAlignment::Stretch:
                if (finalRect.Height < finalSize.Height)
                {
                    offset = (finalSize.Height - finalRect.Height) / 2.0f;
                }
                break;
            }
            finalRect.Y = offset;
        }

        child.Arrange(finalRect);

        // Set a rectangular clip on this ParallaxView the same size as the arrange
        // rectangle so the child does not render beyond it.
        auto rectangleGeometry = Clip().as<winrt::RectangleGeometry>();

        if (!rectangleGeometry)
        {
            // Ensure that this ParallaxView has a rectangular clip.
            winrt::RectangleGeometry newRectangleGeometry;
            newRectangleGeometry.Rect();
            Clip(newRectangleGeometry);

            rectangleGeometry = newRectangleGeometry;
        }

        const winrt::Rect currentClipRect = rectangleGeometry.Rect();

        if (currentClipRect.X != 0.0f || currentClipRect.Width != finalSize.Width ||
            currentClipRect.Y != 0.0f || currentClipRect.Height != finalSize.Height)
        {
            const winrt::Rect newClipRect{ 0.0f, 0.0f, finalSize.Width, finalSize.Height };
            rectangleGeometry.Rect(newClipRect);
        }
    }

    return finalSize;
}
#pragma endregion

// Returns True on RedStone 2 and later versions, when the ElementCompositionPreview::SetIsTranslationEnabled method is available.
bool ParallaxView::IsVisualTranslationPropertyAvailable()
{
    return DownlevelHelper::SetIsTranslationEnabledExists();
}

// Returns the target property path, according to the availability of the ElementCompositionPreview::SetIsTranslationEnabled method.
wstring_view ParallaxView::GetVisualTargetedPropertyName(winrt::Orientation orientation)
{
    if (IsVisualTranslationPropertyAvailable())
    {
        return orientation == winrt::Orientation::Horizontal ? s_translationXPropertyName : s_translationYPropertyName;
    }
    else
    {
        return orientation == winrt::Orientation::Horizontal ? s_transformMatrixTranslateXPropertyName : s_transformMatrixTranslateYPropertyName;
    }
}


// Invoked when a dependency property of this ParallaxView has changed.
void ParallaxView::OnPropertyChanged(
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty dependencyProperty = args.Property();

    if (dependencyProperty == s_ChildProperty)
    {
        winrt::IInspectable oldChild = args.OldValue();
        winrt::IInspectable newChild = args.NewValue();
        UpdateChild(oldChild.as<winrt::UIElement>(), newChild.as<winrt::UIElement>());
    }
    else if (dependencyProperty == s_IsHorizontalShiftClampedProperty ||
             dependencyProperty == s_HorizontalSourceOffsetKindProperty ||
             dependencyProperty == s_HorizontalSourceStartOffsetProperty ||
             dependencyProperty == s_HorizontalSourceEndOffsetProperty ||
             dependencyProperty == s_MaxHorizontalShiftRatioProperty)
    {
        UpdateExpressionAnimation(winrt::Orientation::Horizontal);
    }
    else if (dependencyProperty == s_HorizontalShiftProperty)
    {
        InvalidateMeasure();
        UpdateExpressionAnimation(winrt::Orientation::Horizontal);
    }
    else if (dependencyProperty == s_SourceProperty)
    {
        if (m_scrollInputHelper)
        {
            winrt::IInspectable newSource = args.NewValue();
            m_scrollInputHelper->SetSourceElement(newSource.as<winrt::UIElement>());
        }
    }
    else if (dependencyProperty == s_IsVerticalShiftClampedProperty || 
             dependencyProperty == s_VerticalSourceOffsetKindProperty ||
             dependencyProperty == s_VerticalSourceStartOffsetProperty ||
             dependencyProperty == s_VerticalSourceEndOffsetProperty ||
             dependencyProperty == s_MaxVerticalShiftRatioProperty)
    {
        UpdateExpressionAnimation(winrt::Orientation::Vertical);
    }
    else if (dependencyProperty == s_VerticalShiftProperty)
    {
        InvalidateMeasure();
        UpdateExpressionAnimation(winrt::Orientation::Vertical);
    }
}

// Invoked by ScrollInputHelper when a characteristic changes requires a re-evaluation of the parallaxing expression animations.
void ParallaxView::OnScrollInputHelperInfoChanged(bool horizontalInfoChanged, bool verticalInfoChanged)
{
    if (horizontalInfoChanged)
        UpdateExpressionAnimation(winrt::Orientation::Horizontal);
    if (verticalInfoChanged)
        UpdateExpressionAnimation(winrt::Orientation::Vertical);
}

void ParallaxView::OnLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs& /*args*/)
{
    // Characteristics influencing the source start and end offsets are ready now.
    if (HorizontalShift() != 0.0)
    {
        UpdateStartOffsetExpression(winrt::Orientation::Horizontal);
        UpdateEndOffsetExpression(winrt::Orientation::Horizontal);
    }

    if (VerticalShift() != 0.0)
    {
        UpdateStartOffsetExpression(winrt::Orientation::Vertical);
        UpdateEndOffsetExpression(winrt::Orientation::Vertical);
    }
}

void ParallaxView::OnSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& /*args*/)
{
    // ParallaxView sizes influence the source end offset.
    if (HorizontalShift() != 0.0)
    {
        UpdateEndOffsetExpression(winrt::Orientation::Horizontal);
    }

    if (VerticalShift() != 0.0)
    {
        UpdateEndOffsetExpression(winrt::Orientation::Vertical);
    }
}

// Invoked when a tracked dependency property changes for the Child dependency object.
void ParallaxView::OnChildPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    winrt::FrameworkElement senderAsFrameworkElement = sender.try_as<winrt::FrameworkElement>();

    if (senderAsFrameworkElement &&
        (args == winrt::FrameworkElement::HorizontalAlignmentProperty() ||
         args == winrt::FrameworkElement::VerticalAlignmentProperty()))
    {
        senderAsFrameworkElement.InvalidateArrange();
    }
}

void ParallaxView::UpdateChild(const winrt::UIElement& oldChild, const winrt::UIElement& newChild)
{
    winrt::FrameworkElement childAsFrameworkElement = nullptr;
    
    UnhookChildPropertyChanged(false /* isInDestructor */);

    auto children = try_as<winrt::Panel>().Children();
    children.Clear();

    if (newChild)
    {
        children.Append(newChild);

        // Detect when a child alignment changes, so it can be re-arranged.
        childAsFrameworkElement = newChild.try_as<winrt::FrameworkElement>();
        HookChildPropertyChanged(childAsFrameworkElement);
    }

    if (m_scrollInputHelper && m_scrollInputHelper->TargetElement() != newChild)
    {
        m_scrollInputHelper->SetTargetElement(newChild);
        if (HorizontalShift() != 0.0)
        {
            UpdateExpressionAnimation(winrt::Orientation::Horizontal);
        }
        if (VerticalShift() != 0.0)
        {
            UpdateExpressionAnimation(winrt::Orientation::Vertical);
        }
    }
}

// Sets up the internal composition property set that tracks the animated source start & end offsets.
void ParallaxView::EnsureAnimatedVariables()
{
    if (!m_animatedVariables && m_targetVisual)
    {
        m_animatedVariables = m_targetVisual.Compositor().CreatePropertySet();
        m_animatedVariables.InsertScalar(L"HorizontalSourceStartOffset", 0.0f);
        m_animatedVariables.InsertScalar(L"HorizontalSourceEndOffset", 0.0f);
        m_animatedVariables.InsertScalar(L"VerticalSourceStartOffset", 0.0f);
        m_animatedVariables.InsertScalar(L"VerticalSourceEndOffset", 0.0f);
    }
}

// Updates the composition animation for the source start offset.
void ParallaxView::UpdateStartOffsetExpression(winrt::Orientation orientation)
{
    if (m_scrollInputHelper && m_scrollInputHelper->SourcePropertySet() && m_animatedVariables &&
        ((orientation == winrt::Orientation::Horizontal && HorizontalShift() != 0.0) ||
        (orientation == winrt::Orientation::Vertical && VerticalShift() != 0.0)))
    {
        winrt::ExpressionAnimation startOffsetExpressionAnimation = nullptr;

        if (orientation == winrt::Orientation::Horizontal)
        {
            if (!m_horizontalSourceStartOffsetExpression)
            {
                m_horizontalSourceStartOffsetExpression = m_targetVisual.Compositor().CreateExpressionAnimation();
            }
            startOffsetExpressionAnimation = m_horizontalSourceStartOffsetExpression;
        }
        else
        {
            if (!m_verticalSourceStartOffsetExpression)
            {
                m_verticalSourceStartOffsetExpression = m_targetVisual.Compositor().CreateExpressionAnimation();
            }
            startOffsetExpressionAnimation = m_verticalSourceStartOffsetExpression;
        }

        std::wstring startOffsetExpression;
        const float startOffset = static_cast<float>(orientation == winrt::Orientation::Horizontal ? HorizontalSourceStartOffset() : VerticalSourceStartOffset());

        startOffsetExpressionAnimation.SetScalarParameter(L"startOffset", startOffset);

        if ((orientation == winrt::Orientation::Horizontal && HorizontalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative) ||
            (orientation == winrt::Orientation::Vertical && VerticalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative))
        {
            // Horizontal/VerticalSourceStartOffset is added to automatic value

            const float maxUnderpanOffset = static_cast<float>(m_scrollInputHelper->GetMaxUnderpanOffset(orientation));

            startOffsetExpressionAnimation.SetScalarParameter(L"maxUnderpanOffset", maxUnderpanOffset);

            if (m_scrollInputHelper->IsTargetElementInSource())
            {
                // Target is inside the scrollPresenter.

                // startOffset = (ParallaxViewOffset + HorizontalSourceStartOffset) * ZoomFactor - ViewportWidth - MaxUnderpanOffset
                const float parallaxViewOffset = static_cast<float>(m_scrollInputHelper->GetOffsetFromScrollContentElement(*this, orientation));
                const float viewportSize = static_cast<float>(m_scrollInputHelper->GetViewportSize(orientation));

                startOffsetExpression = L"(parallaxViewOffset + startOffset) * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" - viewportSize - maxUnderpanOffset";
                startOffsetExpressionAnimation.SetScalarParameter(L"parallaxViewOffset", static_cast<float>(parallaxViewOffset));
                startOffsetExpressionAnimation.SetScalarParameter(L"viewportSize", static_cast<float>(viewportSize));
                startOffsetExpressionAnimation.SetReferenceParameter(L"source", m_scrollInputHelper->SourcePropertySet());
            }
            else
            {
                // Target is outside the scrollPresenter.

                // startOffset = HorizontalSourceStartOffset * ZoomFactor - MaxUnderpanOffset
                startOffsetExpression = L"startOffset * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" - maxUnderpanOffset";
                startOffsetExpressionAnimation.SetReferenceParameter(L"source", m_scrollInputHelper->SourcePropertySet());
            }
        }
        else
        {
            // Horizontal/VerticalSourceStartOffset is an absolute value

            // If HorizontalSourceStartOffset <= 0 Then
            //   startOffset = HorizontalSourceStartOffset
            // Else
            //   startOffset = HorizontalSourceStartOffset * ZoomFactor
            if (startOffset > 0.0f)
            {
                startOffsetExpression = L"startOffset * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName());
                startOffsetExpressionAnimation.SetReferenceParameter(L"source", m_scrollInputHelper->SourcePropertySet());
            }
            else
            {
                startOffsetExpression = L"startOffset";
            }
        }

        if (startOffsetExpressionAnimation.Expression() != startOffsetExpression)
        {
            startOffsetExpressionAnimation.Expression(startOffsetExpression);
        }

        m_animatedVariables.StopAnimation((orientation == winrt::Orientation::Horizontal) ? L"HorizontalSourceStartOffset" : L"VerticalSourceStartOffset");
        m_animatedVariables.StartAnimation((orientation == winrt::Orientation::Horizontal) ? L"HorizontalSourceStartOffset" : L"VerticalSourceStartOffset", startOffsetExpressionAnimation);
    }
}

// Updates the composition animation for the source end offset.
void ParallaxView::UpdateEndOffsetExpression(winrt::Orientation orientation)
{
    if (m_scrollInputHelper && m_scrollInputHelper->SourcePropertySet() && m_animatedVariables &&
        ((orientation == winrt::Orientation::Horizontal && HorizontalShift() != 0.0) ||
        (orientation == winrt::Orientation::Vertical && VerticalShift() != 0.0)))
    {
        winrt::ExpressionAnimation endOffsetExpressionAnimation = nullptr;

        if (orientation == winrt::Orientation::Horizontal)
        {
            if (!m_horizontalSourceEndOffsetExpression)
            {
                m_horizontalSourceEndOffsetExpression = m_targetVisual.Compositor().CreateExpressionAnimation();
            }
            endOffsetExpressionAnimation = m_horizontalSourceEndOffsetExpression;
        }
        else
        {
            if (!m_verticalSourceEndOffsetExpression)
            {
                m_verticalSourceEndOffsetExpression = m_targetVisual.Compositor().CreateExpressionAnimation();
            }
            endOffsetExpressionAnimation = m_verticalSourceEndOffsetExpression;
        }

        std::wstring endOffsetExpression;
        const float endOffset = static_cast<float>(orientation == winrt::Orientation::Horizontal ? HorizontalSourceEndOffset() : VerticalSourceEndOffset());

        endOffsetExpressionAnimation.SetScalarParameter(L"endOffset", endOffset);
        endOffsetExpressionAnimation.SetReferenceParameter(L"source", m_scrollInputHelper->SourcePropertySet());

        if ((orientation == winrt::Orientation::Horizontal && HorizontalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative) ||
            (orientation == winrt::Orientation::Vertical && VerticalSourceOffsetKind() == winrt::ParallaxSourceOffsetKind::Relative))
        {
            // Horizontal/VerticalSourceEndOffset is added to automatic value

            const float maxOverpanOffset = static_cast<float>(m_scrollInputHelper->GetMaxOverpanOffset(orientation));

            endOffsetExpressionAnimation.SetScalarParameter(L"maxOverpanOffset", maxOverpanOffset);

            if (m_scrollInputHelper->IsTargetElementInSource())
            {
                // Target is inside the scrollPresenter.

                // endOffset = (ParallaxViewOffset + ParallaxViewWidth + HorizontalSourceEndOffset) * ZoomFactor + MaxOverpanOffset
                const float parallaxViewOffset = static_cast<float>(m_scrollInputHelper->GetOffsetFromScrollContentElement(*this, orientation));
                const float parallaxViewSize = static_cast<float>(orientation == winrt::Orientation::Horizontal ? ActualWidth() : ActualHeight());

                endOffsetExpression = L"(parallaxViewOffset + parallaxViewSize + endOffset) * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" + maxOverpanOffset";
                endOffsetExpressionAnimation.SetScalarParameter(L"parallaxViewOffset", parallaxViewOffset);
                endOffsetExpressionAnimation.SetScalarParameter(L"parallaxViewSize", parallaxViewSize);
            }
            else
            {
                // Target is outside the scrollPresenter.

                const float viewportSize = static_cast<float>(m_scrollInputHelper->GetViewportSize(orientation));
                const float contentSize = static_cast<float>(m_scrollInputHelper->GetContentSize(orientation));

                // endOffset = Max(0, (ContentWidth + HorizontalSourceEndOffset) * ZoomFactor - ViewportWidth) + MaxOverpanOffset
                endOffsetExpression = L"Max(0.0f, (contentSize + endOffset) * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" - viewportSize) + maxOverpanOffset";
                endOffsetExpressionAnimation.SetScalarParameter(L"viewportSize", viewportSize);
                endOffsetExpressionAnimation.SetScalarParameter(L"contentSize", contentSize);
            }
        }
        else
        {
            // Horizontal/VerticalSourceEndOffset is an absolute value

            const float viewportSize = static_cast<float>(m_scrollInputHelper->GetViewportSize(orientation));
            const float contentSize = static_cast<float>(m_scrollInputHelper->GetContentSize(orientation));

            // If (ContentWidth > ViewportWidth) Then
            //   If (HorizontalSourceEndOffset <= ContentWidth - ViewportWidth) Then
            //     endOffset = Max(0, HorizontalSourceEndOffset * ZoomFactor)
            //   Else
            //     endOffset = Max(0, (ContentWidth - ViewportWidth) * ZoomFactor) + HorizontalSourceEndOffset - ContentWidth + ViewportWidth
            // Else
            //   If (HorizontalSourceEndOffset <= 0) Then
            //     endOffset = Max(0, (ContentWith + HorizontalSourceEndOffset) * ZoomFactor - ViewportWidth)
            //   Else
            //     endOffset = Max(0, ContentWidth * ZoomFactor - ViewportWidth) + HorizontalSourceEndOffset
            if (contentSize > viewportSize)
            {
                if (endOffset <= contentSize - viewportSize)
                {
                    endOffsetExpression = L"Max(0.0f, endOffset * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L")";
                }
                else
                {
                    endOffsetExpression = L"Max(0.0f, (contentSize - viewportSize) * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L") + endOffset - contentSize + viewportSize";
                    endOffsetExpressionAnimation.SetScalarParameter(L"contentSize", contentSize);
                    endOffsetExpressionAnimation.SetScalarParameter(L"viewportSize", viewportSize);
                }
            }
            else
            {
                if (endOffset <= 0.0f)
                {
                    endOffsetExpression = L"Max(0.0f, (contentSize + endOffset) * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" - viewportSize)";
                }
                else
                {
                    endOffsetExpression = L"Max(0.0f, contentSize * source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceScalePropertyName()) + L" - viewportSize) + endOffset";
                }
                endOffsetExpressionAnimation.SetScalarParameter(L"contentSize", contentSize);
                endOffsetExpressionAnimation.SetScalarParameter(L"viewportSize", viewportSize);
            }
        }

        if (endOffsetExpressionAnimation.Expression() != endOffsetExpression)
        {
            endOffsetExpressionAnimation.Expression(endOffsetExpression);
        }

        m_animatedVariables.StopAnimation((orientation == winrt::Orientation::Horizontal) ? L"HorizontalSourceEndOffset" : L"VerticalSourceEndOffset");
        m_animatedVariables.StartAnimation((orientation == winrt::Orientation::Horizontal) ? L"HorizontalSourceEndOffset" : L"VerticalSourceEndOffset", endOffsetExpressionAnimation);
    }
}

// Updates the parallaxing composition animation.
void ParallaxView::UpdateExpressionAnimation(winrt::Orientation orientation)
{
    if (SharedHelpers::IsTH2OrLower())
    {
        // The ParallaxView control is not supported on Windows versions prior to RS1.
        return;
    }

    if (m_scrollInputHelper && m_scrollInputHelper->TargetElement() && m_scrollInputHelper->SourcePropertySet())
    {
        winrt::Visual targetVisual = winrt::ElementCompositionPreview::GetElementVisual(m_scrollInputHelper->TargetElement());
        if (m_targetVisual != targetVisual)
        {
            m_targetVisual = targetVisual;
            if (IsVisualTranslationPropertyAvailable())
            {
                winrt::ElementCompositionPreview::SetIsTranslationEnabled(m_scrollInputHelper->TargetElement(), true);
            }
            EnsureAnimatedVariables();
        }
    }
    else if (m_targetVisual)
    {
        // Stop prior parallaxing animations.        
        m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Horizontal));
        m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Vertical));
        m_isHorizontalAnimationStarted = m_isVerticalAnimationStarted = false;

        if (IsVisualTranslationPropertyAvailable())
        {
            m_targetVisual.Properties().InsertVector3(s_translationPropertyName, { 0.0f, 0.0f, 0.0f });
        }
        else
        {
            auto m = m_targetVisual.TransformMatrix();
            m.m41 = m.m42 = 0.0f;
            m_targetVisual.TransformMatrix(m);
        }

        m_targetVisual = nullptr;
    }

    if (m_targetVisual)
    {
        if ((orientation == winrt::Orientation::Horizontal && HorizontalShift() == 0.0) ||
            (orientation == winrt::Orientation::Vertical && VerticalShift() == 0.0))
        {
            if (orientation == winrt::Orientation::Horizontal)
            {
                if (m_isHorizontalAnimationStarted)
                {
                    // Stop prior horizontal parallaxing animation.
                    m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Horizontal));
                    m_isHorizontalAnimationStarted = false;

                    if (IsVisualTranslationPropertyAvailable())
                    {
                        m_targetVisual.Properties().InsertVector3(s_translationPropertyName, { 0.0f, 0.0f, 0.0f });
                    }
                    else
                    {
                        auto m = m_targetVisual.TransformMatrix();
                        m.m41 = 0.0f;
                        m_targetVisual.TransformMatrix(m);
                    }

                    if (m_isVerticalAnimationStarted)
                    {
                        m_targetVisual.StartAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Vertical), m_verticalParallaxExpressionInternal);
                    }
                }
            }
            else if (m_isVerticalAnimationStarted)
            {
                // Stop prior vertical parallaxing animation.
                m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Vertical));
                m_isVerticalAnimationStarted = false;

                if (IsVisualTranslationPropertyAvailable())
                {
                    m_targetVisual.Properties().InsertVector3(s_translationPropertyName, { 0.0f, 0.0f, 0.0f });
                }
                else
                {
                    auto m = m_targetVisual.TransformMatrix();
                    m.m42 = 0.0f;
                    m_targetVisual.TransformMatrix(m);
                }

                if (m_isHorizontalAnimationStarted)
                {
                    m_targetVisual.StartAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Horizontal), m_horizontalParallaxExpressionInternal);
                }
            }
        }
        else
        {
            UpdateStartOffsetExpression(orientation);
            UpdateEndOffsetExpression(orientation);

            winrt::ExpressionAnimation parallaxExpressionInternal = (orientation == winrt::Orientation::Horizontal) ? m_horizontalParallaxExpressionInternal : m_verticalParallaxExpressionInternal;
            std::wstring source = L"source." + static_cast<std::wstring>(m_scrollInputHelper->GetSourceOffsetPropertyName(orientation));
            std::wstring startOffset = (orientation == winrt::Orientation::Horizontal) ? L"animatedVariables.HorizontalSourceStartOffset" : L"animatedVariables.VerticalSourceStartOffset";
            std::wstring endOffset = (orientation == winrt::Orientation::Horizontal) ? L"animatedVariables.HorizontalSourceEndOffset" : L"animatedVariables.VerticalSourceEndOffset";
            std::wstring parallaxExpression;
            const float shift = (float)(orientation == winrt::Orientation::Horizontal ? HorizontalShift() : VerticalShift());

            if ((orientation == winrt::Orientation::Horizontal && IsHorizontalShiftClamped()) ||
                (orientation == winrt::Orientation::Vertical && IsVerticalShiftClamped()))
            {
                // Clamped parallax offset case.

                if (shift > 0.0)
                {
                    // X <= startOffset --> P(X) = 0
                    parallaxExpression = L"(-" + static_cast<std::wstring>(source) + L" <= " + static_cast<std::wstring>(startOffset) + L") ? 0.0f : ";

                    // startOffset < X < endOffset --> P(X) = -Min(MaxRatio, shift / (endOffset - startOffset)) * (X - startOffset)
                    parallaxExpression += L"((-" + static_cast<std::wstring>(source) + L" < " + static_cast<std::wstring>(endOffset) + L") ? ";
                    parallaxExpression += L"(-Min(maxRatio, (shift / (" + static_cast<std::wstring>(endOffset) + L" - " + static_cast<std::wstring>(startOffset) + L"))) * (-" + static_cast<std::wstring>(source) + L" - " + static_cast<std::wstring>(startOffset) + L")) : ";

                    // X >= endOffset --> P(X) = -Min(MaxRatio * Max(0 , endOffset - startOffset), shift)
                    parallaxExpression += L"-Min(maxRatio * Max(0.0f, " + static_cast<std::wstring>(endOffset) + L" - " + static_cast<std::wstring>(startOffset) + L"), shift))";
                }
                else
                {
                    // shift < 0.0

                    // X <= startOffset --> P(X) = -Min(MaxRatio * Max(0 , endOffset - startOffset), -shift)
                    parallaxExpression = L"(-" + static_cast<std::wstring>(source) + L" <= " + static_cast<std::wstring>(startOffset) + L") ? -Min(maxRatio * Max(0.0f, " + static_cast<std::wstring>(endOffset) + L" - " + static_cast<std::wstring>(startOffset) + L"), -shift) : ";

                    // startOffset < X < endOffset --> P(X) = Min(MaxRatio, shift / (startOffset - endOffset)) * (X - endOffset)
                    parallaxExpression += L"((-" + static_cast<std::wstring>(source) + L" < " + static_cast<std::wstring>(endOffset) + L") ? ";
                    parallaxExpression += L"(Min(maxRatio, (shift / (" + static_cast<std::wstring>(startOffset) + L" - " + static_cast<std::wstring>(endOffset) + L"))) * (-" + static_cast<std::wstring>(source) + L" - " + static_cast<std::wstring>(endOffset) + L")) : ";

                    // X >= endOffset --> P(X) = 0
                    parallaxExpression += L"0.0f)";
                }
            }
            else
            {
                // Unclamped parallax offset case.

                if (shift > 0.0)
                {
                    // startOffset == endOffset --> P(X) = 0
                    parallaxExpression = L"(" + static_cast<std::wstring>(startOffset) + L" == " + static_cast<std::wstring>(endOffset) + L") ? 0.0f : ";

                    // startOffset != endOffset --> P(X) = -Min(MaxRatio, shift / (endOffset - startOffset)) * (X - startOffset)
                    parallaxExpression += L"-Min(maxRatio, shift / (" + static_cast<std::wstring>(endOffset) + L" - " + static_cast<std::wstring>(startOffset) + L")) * (-" + static_cast<std::wstring>(source) + L" - " + static_cast<std::wstring>(startOffset) + L")";
                }
                else
                {
                    // startOffset == endOffset --> P(X) = 0
                    parallaxExpression = L"(" + static_cast<std::wstring>(startOffset) + L" == " + static_cast<std::wstring>(endOffset) + L") ? 0.0f : ";

                    // startOffset != endOffset --> P(X) = Min(MaxRatio, shift / (startOffset - endOffset)) * (X - endOffset)
                    parallaxExpression += L"Min(maxRatio, shift / (" + static_cast<std::wstring>(startOffset) + L" - " + static_cast<std::wstring>(endOffset) + L")) * (-" + static_cast<std::wstring>(source) + L" - " + static_cast<std::wstring>(endOffset) + L")";
                }
            }

            if (!parallaxExpressionInternal)
            {
                parallaxExpressionInternal = m_targetVisual.Compositor().CreateExpressionAnimation(parallaxExpression);
                if (orientation == winrt::Orientation::Horizontal)
                {
                    m_horizontalParallaxExpressionInternal = parallaxExpressionInternal;
                }
                else
                {
                    m_verticalParallaxExpressionInternal = parallaxExpressionInternal;
                }
            }
            else if (parallaxExpressionInternal.Expression() != parallaxExpression)
            {
                parallaxExpressionInternal.Expression(parallaxExpression);
            }

            parallaxExpressionInternal.SetReferenceParameter(L"source", m_scrollInputHelper->SourcePropertySet());
            parallaxExpressionInternal.SetReferenceParameter(L"animatedVariables", m_animatedVariables);
            parallaxExpressionInternal.SetScalarParameter(L"maxRatio", static_cast<float>(max(0.0, (orientation == winrt::Orientation::Horizontal ? MaxHorizontalShiftRatio() : MaxVerticalShiftRatio()))));
            parallaxExpressionInternal.SetScalarParameter(L"shift", shift);

            if (orientation == winrt::Orientation::Horizontal)
            {
                if (m_isHorizontalAnimationStarted)
                {
                    m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Horizontal));
                }
                m_targetVisual.StartAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Horizontal), parallaxExpressionInternal);
                m_isHorizontalAnimationStarted = true;
            }
            else
            {
                if (m_isVerticalAnimationStarted)
                {
                    m_targetVisual.StopAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Vertical));
                }
                m_targetVisual.StartAnimation(GetVisualTargetedPropertyName(winrt::Orientation::Vertical), parallaxExpressionInternal);
                m_isVerticalAnimationStarted = true;
            }
        }
    }
}

void ParallaxView::HookLoaded()
{
    if (m_loadedToken.value == 0)
    {
        m_loadedToken = Loaded({ this, &ParallaxView::OnLoaded });
    }
}

void ParallaxView::HookSizeChanged()
{
    if (m_sizeChangedToken.value == 0)
    {
        m_sizeChangedToken = SizeChanged({ this, &ParallaxView::OnSizeChanged });
    }
}

void ParallaxView::HookChildPropertyChanged(const winrt::FrameworkElement& child)
{
    if (child)
    {
        m_currentListeningChild.set(child);
        if (m_childHorizontalAlignmentChangedToken.value == 0)
        {
            m_childHorizontalAlignmentChangedToken.value = child.RegisterPropertyChangedCallback(
                winrt::FrameworkElement::HorizontalAlignmentProperty(), { this, &ParallaxView::OnChildPropertyChanged });
        }
        if (m_childVerticalAlignmentChangedToken.value == 0)
        {
            m_childVerticalAlignmentChangedToken.value = child.RegisterPropertyChangedCallback(
                winrt::FrameworkElement::VerticalAlignmentProperty(), { this, &ParallaxView::OnChildPropertyChanged });
        }
    }
}

void ParallaxView::UnhookChildPropertyChanged(bool isInDestructor)
{
    if (auto child = m_currentListeningChild.safe_get(isInDestructor))
    {
        if (m_childHorizontalAlignmentChangedToken.value != 0)
        {
            child.UnregisterPropertyChangedCallback(winrt::FrameworkElement::HorizontalAlignmentProperty(), m_childHorizontalAlignmentChangedToken.value);
            m_childHorizontalAlignmentChangedToken.value = 0;
        }
        if (m_childVerticalAlignmentChangedToken.value != 0)
        {
            child.UnregisterPropertyChangedCallback(winrt::FrameworkElement::VerticalAlignmentProperty(), m_childVerticalAlignmentChangedToken.value);
            m_childVerticalAlignmentChangedToken.value = 0;
        }
        m_currentListeningChild.set(nullptr);
    }
}
