// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "RefreshVisualizer.h"
#include "RefreshVisualizerEventArgs.h"
#include "RuntimeProfiler.h"
#include "PTRTracing.h"


//The Opacity of the progress indicator in the non-pending non-executing states
#define MINIMUM_INDICATOR_OPACITY 0.4f

//The size of the default progress indicator
#define DEFAULT_INDICATOR_SIZE 30

//The position the progress indicator parallax animation places the indicator during manipulation
#define PARALLAX_POSITION_RATIO 0.5f

//////////////////////////////////////////////////////////
/////////          RefreshVisualizer         /////////////
//////////////////////////////////////////////////////////
RefreshVisualizer::~RefreshVisualizer()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (auto refreshInfoProvider = m_refreshInfoProvider.safe_get())
    {
        refreshInfoProvider.IsInteractingForRefreshChanged(m_RefreshInfoProvider_InteractingForRefreshChangedToken);
        refreshInfoProvider.InteractionRatioChanged(m_RefreshInfoProvider_InteractionRatioChangedToken);
    }
}

RefreshVisualizer::RefreshVisualizer()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RefreshVisualizer);

    SetDefaultStyleKey(this);
}

void RefreshVisualizer::OnApplyTemplate()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    // BEGIN: Populate template children
    winrt::IControlProtected thisAsControlProtected = *this;
    m_root.set(GetTemplateChildT<winrt::Panel>(L"Root", thisAsControlProtected));
    // END: Populate template children

    // BEGIN: Initialize our private backers to the dependency properties and initialize state.
    m_state = State();

    m_orientation = Orientation();
    OnOrientationChangedImpl();

    m_content.set(Content());
    if (!m_content)
    {
        winrt::SymbolIcon defaultContent = winrt::SymbolIcon(winrt::Symbol::Refresh);
        defaultContent.Height(DEFAULT_INDICATOR_SIZE);
        defaultContent.Width(DEFAULT_INDICATOR_SIZE);

        Content(defaultContent);
    }
    else
    {
        OnContentChangedImpl();
    }
    // END: Initialize

    UpdateContent();
}

void RefreshVisualizer::RequestRefresh()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    UpdateRefreshState(winrt::RefreshVisualizerState::Refreshing);
    if (m_refreshInfoProvider.get())
    {
        m_refreshInfoProvider.get().OnRefreshStarted();
    }
    RaiseRefreshRequested();
}

//Private Interface methods
winrt::IRefreshInfoProvider RefreshVisualizer::InfoProvider()
{
    winrt::IInspectable valueAsII = GetValue(s_InfoProviderProperty);
    return valueAsII.as<winrt::IRefreshInfoProvider>();
}

void RefreshVisualizer::InfoProvider(winrt::IRefreshInfoProvider const& value)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    SetValue(s_InfoProviderProperty, value);
}

void RefreshVisualizer::SetInternalPullDirection(winrt::RefreshPullDirection const& value)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, value);
    m_pullDirection = value;
    OnOrientationChangedImpl();
    UpdateContent();
}

//Privates
void RefreshVisualizer::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (property == s_InfoProviderProperty)
    {
        OnRefreshInfoProviderChanged(args);
    }
    else if (property == s_OrientationProperty)
    {
        OnOrientationChanged(args);
    }
    else if (property == s_StateProperty)
    {
        OnStateChanged(args);
    }
    else if (property == s_ContentProperty)
    {
        OnContentChanged(args);
    }
}

void RefreshVisualizer::OnRefreshInfoProviderChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_PTR, METH_NAME, this, args.OldValue(), args.NewValue());
    if (m_refreshInfoProvider)
    {
        m_refreshInfoProvider.get().IsInteractingForRefreshChanged(m_RefreshInfoProvider_InteractingForRefreshChangedToken);
        m_refreshInfoProvider.get().InteractionRatioChanged(m_RefreshInfoProvider_InteractionRatioChangedToken);
    }

    m_refreshInfoProvider.set(InfoProvider());

    if (m_refreshInfoProvider)
    {
        m_RefreshInfoProvider_InteractingForRefreshChangedToken = m_refreshInfoProvider.get().IsInteractingForRefreshChanged({ this, &RefreshVisualizer::RefreshInfoProvider_InteractingForRefreshChanged });
        m_RefreshInfoProvider_InteractionRatioChangedToken = m_refreshInfoProvider.get().InteractionRatioChanged({ this, &RefreshVisualizer::RefreshInfoProvider_InteractionRatioChanged });

        m_executionRatio = m_refreshInfoProvider.get().ExecutionRatio();
    }
    else
    {
        m_RefreshInfoProvider_InteractingForRefreshChangedToken = {};
        m_RefreshInfoProvider_InteractionRatioChangedToken = {};

        m_executionRatio = 1.0f;
    }
}

void RefreshVisualizer::OnOrientationChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.OldValue(), args.NewValue());
    m_orientation = Orientation();
    OnOrientationChangedImpl();
    UpdateContent();
}

void RefreshVisualizer::OnOrientationChangedImpl()
{
    switch (m_orientation)
    {
    case winrt::RefreshVisualizerOrientation::Auto:
        switch (m_pullDirection)
        {
        case winrt::RefreshPullDirection::TopToBottom:
        case winrt::RefreshPullDirection::BottomToTop:
            m_startingRotationAngle = 0.0f;
            break;
        case winrt::RefreshPullDirection::LeftToRight:
            m_startingRotationAngle = (float)-M_PI_2;
            break;
        case winrt::RefreshPullDirection::RightToLeft:
            m_startingRotationAngle = (float)M_PI_2;
            break;
        }
        break;
    case winrt::RefreshVisualizerOrientation::Normal:
        m_startingRotationAngle = 0.0f;
        break;
    case winrt::RefreshVisualizerOrientation::Rotate270DegreesCounterclockwise:
        m_startingRotationAngle = (float)-M_PI_2;
        break;
    case winrt::RefreshVisualizerOrientation::Rotate90DegreesCounterclockwise:
        m_startingRotationAngle = (float)M_PI_2;
        break;
    default:
        MUX_ASSERT(false);
    }
}

void RefreshVisualizer::OnStateChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, args.OldValue(), args.NewValue());
    const winrt::RefreshVisualizerState oldstate = m_state;
    m_state = State();
    UpdateContent();
    RaiseRefreshStateChanged(oldstate, m_state);
}


void RefreshVisualizer::OnContentChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_PTR_PTR, METH_NAME, this, args.OldValue(), args.NewValue());
    m_content.set(Content());
    OnContentChangedImpl();
    UpdateContent();
}

void RefreshVisualizer::OnContentChangedImpl()
{
    if (m_root)
    {
        // There is a slight parallax animation of the progress indicator as the IRefreshInfoProvider updates
        // the interaction ratio. Unfortunately, this composition animation would be clobbered by setting the alignment 
        // properties of the visual's Xaml object. To get around this we wrap the indicator in a container and set the
        // container's alignment properties instead. On RS2+ we can instead animate the Translation XAML property, if
        // the progress Indicator is a Framework Element and has the Vertical/Horizontal Alignment properties.
        m_root.get().Children().Clear();

        if (!m_content)
        {
            winrt::SymbolIcon defaultContent = winrt::SymbolIcon(winrt::Symbol::Refresh);
            defaultContent.Height(DEFAULT_INDICATOR_SIZE);
            defaultContent.Width(DEFAULT_INDICATOR_SIZE);

            m_content.set(defaultContent);
        }

        winrt::Panel m_containerPanel = m_root.get();
        auto contentAsFrameworkElement = m_content.try_as<winrt::FrameworkElement>();

        if (!SharedHelpers::IsRS2OrHigher() || !contentAsFrameworkElement)
        {
            m_containerPanel = winrt::Grid();
            m_root.get().Children().InsertAt(0, m_containerPanel);
            m_containerPanel.VerticalAlignment(winrt::VerticalAlignment::Center);
            m_containerPanel.HorizontalAlignment(winrt::HorizontalAlignment::Center);
        }
        else
        {
            winrt::ElementCompositionPreview::SetIsTranslationEnabled(m_content.get(), true);
            contentAsFrameworkElement.VerticalAlignment(winrt::VerticalAlignment::Center);
            contentAsFrameworkElement.HorizontalAlignment(winrt::HorizontalAlignment::Center);
        }

        m_containerPanel.Children().InsertAt(0, m_content.get());
    }
}

void RefreshVisualizer::UpdateContent()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_content)
    {
        const winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_content.get());

        const winrt::Size contentSize = m_content.get().RenderSize();
        contentVisual.CenterPoint({ (float)(contentSize.Height / 2), (float)(contentSize.Width / 2), 0.0f });

        switch (m_state)
        {
        case winrt::RefreshVisualizerState::Idle:
            contentVisual.Opacity(MINIMUM_INDICATOR_OPACITY);
            contentVisual.RotationAngle(m_startingRotationAngle);
            //On RS2 and above we achieve the parallax animation using the Translation property, so we set the appropriate field here.
            if (SharedHelpers::IsRS2OrHigher())
            {
                contentVisual.Properties().InsertVector3(L"Translation", { 0.0f, 0.0f, 0.0f });
            }
            else
            {
                contentVisual.Offset({ 0.0f, 0.0f, 0.0f });
            }

            break;
        case winrt::RefreshVisualizerState::Peeking:
            contentVisual.Opacity(1.0f);
            contentVisual.RotationAngle(m_startingRotationAngle);
            break;
        case winrt::RefreshVisualizerState::Interacting:
            contentVisual.Opacity(MINIMUM_INDICATOR_OPACITY);
            ExecuteInteractingAnimations();
            break;
        case winrt::RefreshVisualizerState::Pending:
            ExecuteScaleUpAnimation();
            contentVisual.Opacity(1.0f);
            contentVisual.RotationAngle(m_startingRotationAngle);
            break;
        case winrt::RefreshVisualizerState::Refreshing:
            ExecuteExecutingRotationAnimation();
            contentVisual.Opacity(1.0f);
            if (m_root)
            {
                float translationRatio = [this]() {
                    if (auto&& refreshInfoProvider = m_refreshInfoProvider.get())
                    {
                        return (1.0f - (float)(refreshInfoProvider.ExecutionRatio())) * PARALLAX_POSITION_RATIO;
                    }
                    return 1.0f;
                }();
                translationRatio = IsPullDirectionFar() ? -1.0f * translationRatio : translationRatio;
                //On RS2 and above we achieve the parallax animation using the Translation property, so we set the appropriate field here.
                if (SharedHelpers::IsRS2OrHigher())
                {
                    if (IsPullDirectionVertical())
                    {
                        contentVisual.Properties().InsertVector3(L"Translation", { 0.0f, translationRatio * (float)m_root.get().ActualHeight(), 0.0f });
                    }
                    else
                    {
                        contentVisual.Properties().InsertVector3(L"Translation", { translationRatio * (float)m_root.get().ActualWidth(), 0.0f, 0.0f });
                    }
                }
                else
                {
                    if (IsPullDirectionVertical())
                    {
                        contentVisual.Offset({ 0.0f, translationRatio * (float)m_root.get().ActualHeight(), 0.0f });
                    }
                    else
                    {
                        contentVisual.Offset({ translationRatio * (float)m_root.get().ActualHeight(), 0.0f, 0.0f });
                    }
                }
            }

            break;
        default:
            MUX_ASSERT(false);
        }
    }
}

void RefreshVisualizer::ExecuteInteractingAnimations()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_content && m_refreshInfoProvider)
    {
        winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_content.get());
        if (!m_compositor)
        {
            m_compositor.set(contentVisual.Compositor());
        }

        //Set up the InteractionRatioRotationAnimation
        const winrt::Size contentSize = m_content.get().RenderSize();
        contentVisual.CenterPoint({ (float)(contentSize.Height / 2), (float)(contentSize.Width / 2), 0.0f });

        const winrt::hstring interactionRatioPropertyName = m_refreshInfoProvider.get().InteractionRatioCompositionProperty();
        const winrt::CompositionPropertySet interactionRatioPropertySet = m_refreshInfoProvider.get().CompositionProperties();

        const winrt::ExpressionAnimation contentInteractionRatioRotationAnimation = m_compositor.get().CreateExpressionAnimation(
            L"startingRotationAngle + (Pi * (Clamp(RefreshInteractionRatioPropertySet." +
            static_cast<std::wstring>(interactionRatioPropertyName) +
            L", 0.0f, contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO) / contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO) * 2)");

        const auto thresholdRatioName{ L"DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO"sv };
        contentVisual.Properties().InsertScalar(thresholdRatioName, static_cast<float>(m_executionRatio));
        contentInteractionRatioRotationAnimation.SetReferenceParameter(L"contentVisual", contentVisual);
        contentInteractionRatioRotationAnimation.SetReferenceParameter(L"RefreshInteractionRatioPropertySet", interactionRatioPropertySet);
        contentInteractionRatioRotationAnimation.SetScalarParameter(L"startingRotationAngle", m_startingRotationAngle);

        contentVisual.StartAnimation(L"RotationAngle", contentInteractionRatioRotationAnimation);

        //Set up the InteractionRatioOpacityAnimation
        const winrt::ExpressionAnimation contentInteractionRatioOpacityAnimation = m_compositor.get().CreateExpressionAnimation(
            L"((1.0f - contentVisual.MINIMUM_INDICATOR_OPACITY) * RefreshInteractionRatioPropertySet."
            + static_cast<std::wstring>(interactionRatioPropertyName) +
            L") + contentVisual.MINIMUM_INDICATOR_OPACITY");
        const auto minOpacityName{ L"MINIMUM_INDICATOR_OPACITY"sv };
        contentVisual.Properties().InsertScalar(minOpacityName, MINIMUM_INDICATOR_OPACITY);
        contentInteractionRatioOpacityAnimation.SetReferenceParameter(L"contentVisual", contentVisual);
        contentInteractionRatioOpacityAnimation.SetReferenceParameter(L"RefreshInteractionRatioPropertySet", interactionRatioPropertySet);

        //contentVisual.StartAnimation(L"Opacity", contentInteractionRatioOpacityAnimation);

        //Set up the InteractionRatioParallaxAnimation
        winrt::ExpressionAnimation contentInteractionRatioParallaxAnimation{ nullptr };
        if (IsPullDirectionFar())
        {
            contentInteractionRatioParallaxAnimation = m_compositor.get().CreateExpressionAnimation(
                L"((1.0f - contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO) * rootSize * 0.5f * -1.0f) * min((RefreshInteractionRatioPropertySet."
                + static_cast<std::wstring>(interactionRatioPropertyName) +
                L" / contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO), 1.0f)");
        }
        else
        {
            contentInteractionRatioParallaxAnimation = m_compositor.get().CreateExpressionAnimation(
                L"((1.0f - contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO) * rootSize * 0.5f) * min((RefreshInteractionRatioPropertySet."
                + static_cast<std::wstring>(interactionRatioPropertyName) +
                L" / contentVisual.DEFAULT_REFRESHINDICATOR_THRESHOLD_RATIO), 1.0f)");
        }
        if (m_root)
        {
            contentInteractionRatioParallaxAnimation.SetScalarParameter(L"rootSize", (float)(IsPullDirectionVertical() ? m_root.get().ActualHeight() : m_root.get().ActualWidth()));
        }
        else
        {
            contentInteractionRatioParallaxAnimation.SetScalarParameter(L"rootSize", 0.0f);
        }

        contentInteractionRatioParallaxAnimation.SetReferenceParameter(L"contentVisual", contentVisual);
        contentInteractionRatioParallaxAnimation.SetReferenceParameter(L"RefreshInteractionRatioPropertySet", interactionRatioPropertySet);

        //On RS2 and above we achieve the parallax animation using the Translation property, so we animate the appropriate field here.
        if (!SharedHelpers::IsRS2OrHigher())
        {
            if (IsPullDirectionVertical())
            {
                contentVisual.StartAnimation(L"Offset.Y", contentInteractionRatioParallaxAnimation);
            }
            else
            {
                contentVisual.StartAnimation(L"Offset.X", contentInteractionRatioParallaxAnimation);
            }
        }
        else
        {
            if (IsPullDirectionVertical())
            {
                contentVisual.StartAnimation(L"Translation.Y", contentInteractionRatioParallaxAnimation);
            }
            else
            {
                contentVisual.StartAnimation(L"Translation.X", contentInteractionRatioParallaxAnimation);
            }
        }
    }
}

void RefreshVisualizer::ExecuteScaleUpAnimation()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_content)
    {
        const winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_content.get());
        if (!m_compositor)
        {
            m_compositor.set(contentVisual.Compositor());
        }

        const winrt::Vector2KeyFrameAnimation contentScaleAnimation = m_compositor.get().CreateVector2KeyFrameAnimation();
        contentScaleAnimation.InsertKeyFrame(0.5f, winrt::float2(1.50f, 1.50f));
        contentScaleAnimation.InsertKeyFrame(1.0f, winrt::float2(1.0f, 1.0f));
        contentScaleAnimation.Duration(300ms);

        const winrt::Size contentSize = m_content.get().RenderSize();
        contentVisual.CenterPoint({ (float)(contentSize.Height / 2), (float)(contentSize.Width / 2), 0.0f });

        contentVisual.StartAnimation(L"Scale.XY", contentScaleAnimation);
    }
}

void RefreshVisualizer::ExecuteExecutingRotationAnimation()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    if (m_content)
    {
        const winrt::Visual contentVisual = winrt::ElementCompositionPreview::GetElementVisual(m_content.get());
        if (!m_compositor)
        {
            m_compositor.set(contentVisual.Compositor());
        }

        const winrt::ScalarKeyFrameAnimation contentExecutionRotationAnimation = m_compositor.get().CreateScalarKeyFrameAnimation();
        contentExecutionRotationAnimation.InsertKeyFrame(0.0f, m_startingRotationAngle, m_compositor.get().CreateLinearEasingFunction());
        contentExecutionRotationAnimation.InsertKeyFrame(1.0f, m_startingRotationAngle + (float)(2.0f * M_PI), m_compositor.get().CreateLinearEasingFunction());
        contentExecutionRotationAnimation.Duration(500ms);
        contentExecutionRotationAnimation.IterationBehavior(winrt::AnimationIterationBehavior::Forever);

        const winrt::Size contentSize = m_content.get().RenderSize();
        contentVisual.CenterPoint({ (float)(contentSize.Height / 2), (float)(contentSize.Width / 2), 0.0f });

        contentVisual.StartAnimation(L"RotationAngle", contentExecutionRotationAnimation);
    }
}

void RefreshVisualizer::UpdateRefreshState(const winrt::RefreshVisualizerState& newState)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, newState);
    if (newState != m_state)
    {
        put_State(newState);
    }
}

void RefreshVisualizer::RaiseRefreshStateChanged(const winrt::RefreshVisualizerState& oldState, const winrt::RefreshVisualizerState& newState)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, oldState, newState);
    const auto e = winrt::make<RefreshStateChangedEventArgs>(oldState, newState);
    m_refreshStateChangedEventSource(*this, e);
}

void RefreshVisualizer::RaiseRefreshRequested()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    com_ptr<RefreshVisualizer> strongThis = get_strong();

    const winrt::Deferral instance{ [strongThis]()
        {
            strongThis->CheckThread();
            strongThis->RefreshCompleted();
        } 
    };

    const auto args = winrt::make_self<RefreshRequestedEventArgs>(instance);

    //This makes sure that everyone registered for this event can get access to the deferral
    //Otherwise someone could complete the deferral before someone else has had a chance to grab it
    args->IncrementDeferralCount();
    m_refreshRequestedEventSource(*this, *args);
    args->DecrementDeferralCount();
}

void RefreshVisualizer::RefreshCompleted()
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    UpdateRefreshState(winrt::RefreshVisualizerState::Idle);
    if (m_refreshInfoProvider)
    {
        m_refreshInfoProvider.get().OnRefreshCompleted();
    }
}

void RefreshVisualizer::RefreshInfoProvider_InteractingForRefreshChanged(const  winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*e*/)
{
    if (m_refreshInfoProvider)
    {
        m_isInteractingForRefresh = m_refreshInfoProvider.get().IsInteractingForRefresh();
        PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, m_isInteractingForRefresh);
        if (!m_isInteractingForRefresh)
        {
            switch (m_state)
            {
            case winrt::RefreshVisualizerState::Pending:
                // User stopped interaction after it hit the Pending state.
                RequestRefresh();
                break;
            case winrt::RefreshVisualizerState::Refreshing:
                // We don't want to interrupt a currently executing refresh.
                break;
            default:
                //Peeking, interacting, or idle results in idle.
                UpdateRefreshState(winrt::RefreshVisualizerState::Idle);
                break;
            }
        }
    }
}

void RefreshVisualizer::RefreshInfoProvider_InteractionRatioChanged(const winrt::IRefreshInfoProvider& /*sender*/, const winrt::RefreshInteractionRatioChangedEventArgs& e)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, e.InteractionRatio());
    const bool wasAtZero = m_interactionRatio == 0.0f;
    m_interactionRatio = e.InteractionRatio();
    if (m_isInteractingForRefresh)
    {
        if (m_state == winrt::RefreshVisualizerState::Idle)
        {
            if (wasAtZero)
            {
                if (m_interactionRatio > m_executionRatio)
                {
                    //Sometimes due to missed frames in the interplay of comp and xaml the interaction tracker will 'jump' passed the executionRatio on the first Value changed.
                    UpdateRefreshState(winrt::RefreshVisualizerState::Pending);
                }
                else if (m_interactionRatio > 0.0f)
                {
                    UpdateRefreshState(winrt::RefreshVisualizerState::Interacting);
                }
            }
            else if (m_interactionRatio > 0.0f)
            {
                // TODO: IRefreshInfoProvider does not raise InteractionRatioChanged yet when DManip is overpanning. Thus we do not yet 
                // enter the Peeking state when DManip overpans in inertia.
                UpdateRefreshState(winrt::RefreshVisualizerState::Peeking);
            }
        }
        else if (m_state == winrt::RefreshVisualizerState::Interacting)
        {
            if (m_interactionRatio <= 0.0f)
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Idle);
            }
            else if (m_interactionRatio > m_executionRatio)
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Pending);
            }
        }
        else if (m_state == winrt::RefreshVisualizerState::Pending)
        {
            if (m_interactionRatio <= m_executionRatio)
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Interacting);
            }
            else if (m_interactionRatio <= 0.0f)
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Idle);
            }
        }
        //If we are in Refreshing or Peeking we want to stay in those states.
    }
    else
    {
        //If we are not refreshing or interacting for refresh then the only valid states are Peeking and Idle
        if (m_state != winrt::RefreshVisualizerState::Refreshing)
        {
            if (m_interactionRatio > 0.0f)
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Peeking);
            }
            else 
            {
                UpdateRefreshState(winrt::RefreshVisualizerState::Idle);
            }
        }
    }
}

void RefreshVisualizer::put_State(const winrt::RefreshVisualizerState& value)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, value);
    SetValue(s_StateProperty, box_value(value));
}

bool RefreshVisualizer::IsPullDirectionVertical()
{
    return m_pullDirection == winrt::RefreshPullDirection::TopToBottom || m_pullDirection == winrt::RefreshPullDirection::BottomToTop;
}

bool RefreshVisualizer::IsPullDirectionFar()
{
    return m_pullDirection == winrt::RefreshPullDirection::BottomToTop || m_pullDirection == winrt::RefreshPullDirection::RightToLeft;
}
