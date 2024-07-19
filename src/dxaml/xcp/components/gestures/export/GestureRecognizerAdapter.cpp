// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>

#include "GestureRecognizerAdapter.h"
#include "ElementGestureTracker.h"
#include "WrlHelper.h"
#include "RootScale.h"

#include <GestureConfigurationBuilder.h>

GestureRecognizerAdapter::~GestureRecognizerAdapter()
{
    Reset(nullptr);
}

void GestureRecognizerAdapter::EnsureInitialized(_In_ ElementGestureTracker* const elementGestureTracker)
{
    if (m_gestureRecognizer == nullptr)
    {
        IFCFAILFAST(wf::ActivateInstance(
            wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Microsoft.UI.Input.GestureRecognizer")).Get(),
            &m_gestureRecognizer));
        IFCFAILFAST(m_gestureRecognizer->put_ShowGestureFeedback(false));
        Init(elementGestureTracker);
    }
}

void GestureRecognizerAdapter::Init(_In_ ElementGestureTracker* const elementGestureTracker)
{
    IFCFAILFAST(m_gestureRecognizer->add_Tapped(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::TappedEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::ITappedEventArgs* args) -> HRESULT
    {
        OnTapped(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_tappedToken));

    IFCFAILFAST(m_gestureRecognizer->add_RightTapped(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::RightTappedEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IRightTappedEventArgs* args) -> HRESULT
    {
        OnRightTapped(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_rightTappedToken));

    IFCFAILFAST(m_gestureRecognizer->add_Holding(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::HoldingEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IHoldingEventArgs* args) -> HRESULT
    {
        OnHolding(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_holdingToken));

    IFCFAILFAST(m_gestureRecognizer->add_ManipulationStarted(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::ManipulationStartedEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IManipulationStartedEventArgs* args) -> HRESULT
    {
        OnManipulationStarted(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_manipStartedToken));

    IFCFAILFAST(m_gestureRecognizer->add_ManipulationUpdated(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::ManipulationUpdatedEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IManipulationUpdatedEventArgs* args) -> HRESULT
    {
        OnManipulationUpdated(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_manipUpdatedToken));

    IFCFAILFAST(m_gestureRecognizer->add_ManipulationInertiaStarting(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::ManipulationInertiaStartingEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IManipulationInertiaStartingEventArgs* args) -> HRESULT
    {
        OnManipulationInertiaStarting(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_manipInteriaStartingToken));

    IFCFAILFAST(m_gestureRecognizer->add_ManipulationCompleted(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::GestureRecognizer*, ixp::ManipulationCompletedEventArgs*>>(
            [this, elementGestureTracker](ixp::IGestureRecognizer* gr, ixp::IManipulationCompletedEventArgs* args) -> HRESULT
    {
        OnManipulationCompleted(elementGestureTracker, gr, args);
        return S_OK;
    }).Get(), &m_manipCompletedToken));
}

void GestureRecognizerAdapter::Reset(_In_opt_ ElementGestureTracker* const elementGestureTracker)
{
    m_gestureOutputMapper.Reset();

    if (m_tappedToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_Tapped(m_tappedToken)); }
    if (m_rightTappedToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_RightTapped(m_rightTappedToken)); }
    if (m_holdingToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_Holding(m_holdingToken)); }
    if (m_manipStartedToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_ManipulationStarted(m_manipStartedToken)); }
    if (m_manipInteriaStartingToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_ManipulationInertiaStarting(m_manipInteriaStartingToken)); }
    if (m_manipUpdatedToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_ManipulationUpdated(m_manipUpdatedToken)); }
    if (m_manipCompletedToken.value != 0) { IFCFAILFAST(m_gestureRecognizer->remove_ManipulationCompleted(m_manipCompletedToken)); }

    m_tappedToken = {};
    m_rightTappedToken = {};
    m_holdingToken = {};
    m_manipStartedToken = {};
    m_manipUpdatedToken = {};
    m_manipInteriaStartingToken = {};
    m_manipCompletedToken = {};

    if (m_gestureRecognizer != nullptr)
    {
        IGNOREHR(m_gestureRecognizer->CompleteGesture());
    }

    if (elementGestureTracker != nullptr)
    {
        Init(elementGestureTracker);
    }
}

void GestureRecognizerAdapter::SetPivotRadius(_In_ bool pivot, _In_ float floatPivot) const
{
    if (pivot && m_gestureOutputMapper.IsInteractionStopping() == false)
    {
        m_gestureRecognizer->put_PivotRadius(floatPivot);
    }
}

void GestureRecognizerAdapter::SetInertiaParameters(_In_ GestureOutputMapper::InertiaParameters& params) const
{
    if (params.m_translationDeceleration > 0) { m_gestureRecognizer->put_InertiaTranslationDeceleration(params.m_translationDeceleration); }
    if (params.m_translationDisplacement > 0) { m_gestureRecognizer->put_InertiaTranslationDisplacement(params.m_translationDisplacement); }
    if (params.m_rotationDeceleration > 0) { m_gestureRecognizer->put_InertiaRotationDeceleration(params.m_rotationDeceleration); }
    if (params.m_rotationAngle > 0) { m_gestureRecognizer->put_InertiaRotationAngle(params.m_rotationAngle); }
    if (params.m_expansionDeceleration > 0) { m_gestureRecognizer->put_InertiaExpansionDeceleration(params.m_expansionDeceleration); }
    if (params.m_expansionExpansion > 0) { m_gestureRecognizer->put_InertiaExpansion(params.m_expansionExpansion); }
}

void GestureRecognizerAdapter::ConfigurationBuilder(
    _In_ bool bTapEnabled,
    _In_ bool bDoubleTapEnabled,
    _In_ bool bRightTapEnabled,
    _In_ bool bHoldEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode) const
{
    ixp::GestureSettings settings = static_cast<ixp::GestureSettings>(
        GestureConfig::GestureConfigurationBuilder(bTapEnabled, bDoubleTapEnabled, bRightTapEnabled, bHoldEnabled, manipulationMode));

    IFCFAILFAST(m_gestureRecognizer->put_GestureSettings(settings));
}

void GestureRecognizerAdapter::OnTapped(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::ITappedEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    UINT32 tapCount;
    IFCFAILFAST(args->get_TapCount(&tapCount));

    TouchInteractionMsg msg = m_gestureOutputMapper.OnTapped(position, tapCount);

    xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
    if (element.get() != nullptr && msg.m_msgID != XCP_NULL)
    {
        IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnRightTapped(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IRightTappedEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    TouchInteractionMsg msg = m_gestureOutputMapper.OnSecondaryTapped(position);

    xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
    if (element.get() != nullptr && msg.m_msgID != XCP_NULL)
    {
        IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnHolding(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IHoldingEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    ixp::HoldingState holdingState;
    IFCFAILFAST(args->get_HoldingState(&holdingState));

    TouchInteractionMsg msg = m_gestureOutputMapper.OnHolding(position, holdingState);

    xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
    if (element.get() != nullptr && msg.m_msgID != XCP_NULL)
    {
        IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnManipulationStarted(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationStartedEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    ixp::ManipulationDelta cumulativeDelta;
    IFCFAILFAST(args->get_Cumulative(&cumulativeDelta));

    if (elementGestureTracker->m_manipulationMode != DirectUI::ManipulationModes::None)
    {
        xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
        const float scale = (element.get() != nullptr) ? RootScale::GetRasterizationScaleForElement(element) : 1.0f;

        TouchInteractionMsg msg = m_gestureOutputMapper.OnManipulationStarted(position, cumulativeDelta, scale);

        if (element.get() != nullptr && msg.m_msgID != XCP_NULL && m_gestureOutputMapper.ShouldProcessManipulationEvents())
        {
            IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
            SetPivotRadius(elementGestureTracker->m_bPivot, elementGestureTracker->m_floatPivotRadius);
        }
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnManipulationUpdated(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationUpdatedEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    ixp::ManipulationDelta delta;
    IFCFAILFAST(args->get_Delta(&delta));

    ixp::ManipulationDelta cumulativeDelta;
    IFCFAILFAST(args->get_Cumulative(&cumulativeDelta));

    ixp::ManipulationVelocities velocities;
    IFCFAILFAST(args->get_Velocities(&velocities));

    if (elementGestureTracker->m_manipulationMode != DirectUI::ManipulationModes::None)
    {
        xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
        const float scale = (element.get() != nullptr) ? RootScale::GetRasterizationScaleForElement(element) : 1.0f;

        TouchInteractionMsg msg = m_gestureOutputMapper.OnManipulationUpdated(position, delta, cumulativeDelta, velocities, scale);

        if (element.get() != nullptr && msg.m_msgID != XCP_NULL && m_gestureOutputMapper.ShouldProcessManipulationEvents())
        {
            IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
            SetPivotRadius(elementGestureTracker->m_bPivot, elementGestureTracker->m_floatPivotRadius);
        }
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnManipulationInertiaStarting(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationInertiaStartingEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    ixp::ManipulationDelta delta;
    IFCFAILFAST(args->get_Delta(&delta));

    ixp::ManipulationDelta cumulativeDelta;
    IFCFAILFAST(args->get_Cumulative(&cumulativeDelta));

    ixp::ManipulationVelocities velocities;
    IFCFAILFAST(args->get_Velocities(&velocities));

    if (elementGestureTracker->m_manipulationMode != DirectUI::ManipulationModes::None)
    {
        xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
        const float scale = (element.get() != nullptr) ? RootScale::GetRasterizationScaleForElement(element) : 1.0f;

        TouchInteractionMsg msg = m_gestureOutputMapper.OnManipulationInertiaStarting(position, delta, cumulativeDelta, velocities, scale);

        if (element.get() != nullptr && msg.m_msgID != XCP_NULL)
        {
            if (m_gestureOutputMapper.ShouldProcessManipulationEvents())
            {
                IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
            }
            GestureOutputMapper::InertiaParameters params = m_gestureOutputMapper.GetInertiaParametersFromMsg(msg);
            SetInertiaParameters(params);
        }

        msg = m_gestureOutputMapper.OnManipulationUpdated(position, delta, cumulativeDelta, velocities, scale);
        if (m_gestureOutputMapper.ShouldProcessManipulationEvents())
        {
            IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
        }

        SetPivotRadius(elementGestureTracker->m_bPivot, elementGestureTracker->m_floatPivotRadius);
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}

void GestureRecognizerAdapter::OnManipulationCompleted(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationCompletedEventArgs* args)
{
    wf::Point position;
    IFCFAILFAST(args->get_Position(&position));

    ixp::ManipulationDelta cumulativeDelta;
    IFCFAILFAST(args->get_Cumulative(&cumulativeDelta));

    ixp::ManipulationVelocities velocities;
    IFCFAILFAST(args->get_Velocities(&velocities));

    xref_ptr<CUIElement> element = elementGestureTracker->m_pElement.lock();
    const float scale = (element.get() != nullptr) ? RootScale::GetRasterizationScaleForElement(element) : 1.0f;

    bool isInteractionStopping = false;
    TouchInteractionMsg msg = m_gestureOutputMapper.OnManipulationCompleted(position, elementGestureTracker->m_manipulationMode, cumulativeDelta, velocities, &isInteractionStopping, scale);

    if (element.get() != nullptr && msg.m_msgID != XCP_NULL)
    {
        if (m_gestureOutputMapper.ShouldProcessManipulationEvents())
        {
            IFCFAILFAST(element->GetContext()->ProcessTouchInteractionCallback(element, &msg));
        }

        if (elementGestureTracker->m_manipulationMode != DirectUI::ManipulationModes::None && elementGestureTracker->m_bPivot && isInteractionStopping == false)
        {
            m_gestureRecognizer->put_PivotRadius(elementGestureTracker->m_floatPivotRadius);
        }

        m_gestureOutputMapper.ResetInteraction();
    }

    m_gestureOutputMapper.ResetProcessingOutput();
}
