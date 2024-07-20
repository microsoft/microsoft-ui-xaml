// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBarPanningInfo.h"
#include "AnnotatedScrollBarTrace.h"
#include "ScrollControllerPanRequestedEventArgs.h"

AnnotatedScrollBarPanningInfo::AnnotatedScrollBarPanningInfo()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

AnnotatedScrollBarPanningInfo::~AnnotatedScrollBarPanningInfo()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

#pragma region IScrollControllerPanningInfo

bool AnnotatedScrollBarPanningInfo::IsRailEnabled()
{
    return true;
}

winrt::Orientation AnnotatedScrollBarPanningInfo::PanOrientation()
{
    return winrt::Orientation::Vertical;
}

winrt::UIElement AnnotatedScrollBarPanningInfo::PanningElementAncestor()
{
    return m_panningElementAncestor.get();
}

void AnnotatedScrollBarPanningInfo::SetPanningElementExpressionAnimationSources(
    const winrt::CompositionPropertySet& propertySet,
    const winrt::hstring& minOffsetPropertyName,
    const winrt::hstring& maxOffsetPropertyName,
    const winrt::hstring& offsetPropertyName,
    const winrt::hstring& multiplierPropertyName)
{
    m_expressionAnimationSources = propertySet;

    if (m_expressionAnimationSources)
    {
        m_minOffsetPropertyName = minOffsetPropertyName;
        m_maxOffsetPropertyName = maxOffsetPropertyName;
        m_offsetPropertyName = offsetPropertyName;
        m_multiplierPropertyName = multiplierPropertyName;
    
        UpdatePanningElementOffsetMultiplier();
    
        if (!m_thumbOffsetAnimation)
        {
            EnsureThumbAnimation();
            UpdateThumbExpression();
            StartThumbAnimation();
        }
    }
    else
    {
        m_minOffsetPropertyName = L"";
        m_maxOffsetPropertyName = L"";
        m_offsetPropertyName = L"";
        m_multiplierPropertyName = L"";
        m_thumbOffsetAnimation = nullptr;
        StopThumbAnimation();
    }
}

winrt::event_token AnnotatedScrollBarPanningInfo::Changed(const winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::IInspectable>& handler)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");

    return m_changedEventSource.add(handler);
}

void AnnotatedScrollBarPanningInfo::Changed(const winrt::event_token& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");

    m_changedEventSource.remove(token);
}

winrt::event_token AnnotatedScrollBarPanningInfo::PanRequested(const winrt::TypedEventHandler<winrt::IScrollControllerPanningInfo, winrt::ScrollControllerPanRequestedEventArgs>& handler)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Add Handler");

    return m_panRequestedEventSource.add(handler);
}

void AnnotatedScrollBarPanningInfo::PanRequested(const winrt::event_token& token)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO_DBG(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, L"Remove Handler");

    m_panRequestedEventSource.remove(token);
}

#pragma endregion

void AnnotatedScrollBarPanningInfo::PanningFrameworkElement(const winrt::FrameworkElement& value)
{
    if (m_panningFrameworkElement.get() != value)
    {
        m_panningFrameworkElement.set(value);

        if (m_panningFrameworkElement)
        {
            m_panningVisual.set(winrt::ElementCompositionPreview::GetElementVisual(m_panningFrameworkElement.get()));
            winrt::ElementCompositionPreview::SetIsTranslationEnabled(m_panningFrameworkElement.get(), true);
            StartThumbAnimation();
        }
        else
        {
            StopThumbAnimation();
            m_panningVisual.set(nullptr);
        }
    }
}

void AnnotatedScrollBarPanningInfo::PanningElementAncestor(const winrt::UIElement& value)
{
    if (m_panningElementAncestor.get() != value)
    {
        m_panningElementAncestor.set(value);
        RaiseChanged();
    }
}

void AnnotatedScrollBarPanningInfo::PanningElementOffsetMultiplier(float value)
{
    if (m_panningElementOffsetMultiplier != value)
    {
        m_panningElementOffsetMultiplier = value;

        UpdatePanningElementOffsetMultiplier();
    }
}

void AnnotatedScrollBarPanningInfo::RaiseChanged()
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_changedEventSource)
    {
        m_changedEventSource(*this, nullptr);
    }
}

bool AnnotatedScrollBarPanningInfo::RaisePanRequested(const winrt::PointerPoint& pointerPoint)
{
    ANNOTATEDSCROLLBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    if (m_panRequestedEventSource)
    {
        auto scrollControllerPanRequestedEventArgs = winrt::make<ScrollControllerPanRequestedEventArgs>(pointerPoint);

        m_panRequestedEventSource(*this, scrollControllerPanRequestedEventArgs);

        return scrollControllerPanRequestedEventArgs.Handled();
    }

    return false;
}

// Creates the thumbOffsetAnimation Composition expression animation used to 
// position the vertical pannable thumb within its track.
void AnnotatedScrollBarPanningInfo::EnsureThumbAnimation()
{
    if (!m_thumbOffsetAnimation &&
        m_expressionAnimationSources &&
        m_offsetPropertyName != L"" &&
        m_minOffsetPropertyName != L"" &&
        m_maxOffsetPropertyName != L"" &&
        m_multiplierPropertyName != L"")
    {
        m_thumbOffsetAnimation = m_expressionAnimationSources.Compositor().CreateExpressionAnimation();
        m_thumbOffsetAnimation.SetReferenceParameter(L"sources", m_expressionAnimationSources);
    }
}

void AnnotatedScrollBarPanningInfo::StartThumbAnimation()
{
    if (m_panningVisual && m_thumbOffsetAnimation)
    {
        m_panningVisual.get().StartAnimation(L"Translation.Y", m_thumbOffsetAnimation);
    }
}

void AnnotatedScrollBarPanningInfo::StopThumbAnimation()
{
    if (m_panningVisual)
    {
        m_panningVisual.get().StopAnimation(L"Translation.Y");
    }
}

// Updates the thumbOffsetAnimation Composition expression animation using the PropertySet provided
// in the SetPanningElementExpressionAnimationSources call.
void AnnotatedScrollBarPanningInfo::UpdateThumbExpression()
{
    if (m_thumbOffsetAnimation)
    {
        m_thumbOffsetAnimation.Expression(
            L"min(sources." + m_maxOffsetPropertyName +
            L",max(sources." + m_minOffsetPropertyName +
            L",sources." + m_offsetPropertyName +
            L"))/(-sources." + m_multiplierPropertyName + L")");
    }
}

void AnnotatedScrollBarPanningInfo::UpdatePanningElementOffsetMultiplier()
{
    if (m_expressionAnimationSources && m_multiplierPropertyName != L"")
    {
        m_expressionAnimationSources.InsertScalar(m_multiplierPropertyName, m_panningElementOffsetMultiplier);
    }
}
