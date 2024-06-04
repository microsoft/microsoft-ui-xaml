// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GestureOutputMapper.h"

class ElementGestureTracker;

// This class serves as an adapter that allows for XAML and GestureRecognizer to talk to each other
class GestureRecognizerAdapter
{
public:
    ~GestureRecognizerAdapter();
    void EnsureInitialized(_In_ ElementGestureTracker* const elementGestureTracker);
    void Reset(_In_opt_ ElementGestureTracker* const elementGestureTracker);

    void ConfigurationBuilder(
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode) const;

    wrl::ComPtr<ixp::IGestureRecognizer> m_gestureRecognizer;

    GestureOutputMapper m_gestureOutputMapper;

private:
    void Init(_In_ ElementGestureTracker* const elementGestureTracker);

    void SetPivotRadius(_In_ bool pivot, _In_ float floatPivot) const;
    void SetInertiaParameters(_In_ GestureOutputMapper::InertiaParameters& params) const;

    void OnTapped(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::ITappedEventArgs* args);
    void OnRightTapped(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IRightTappedEventArgs* args);
    void OnHolding(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IHoldingEventArgs* args);
    void OnManipulationStarted(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationStartedEventArgs* args);
    void OnManipulationUpdated(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationUpdatedEventArgs* args);
    void OnManipulationInertiaStarting(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationInertiaStartingEventArgs* args);
    void OnManipulationCompleted(_In_ ElementGestureTracker* elementGestureTracker, _In_ ixp::IGestureRecognizer* gr, _In_ ixp::IManipulationCompletedEventArgs* args);

    EventRegistrationToken m_tappedToken = {};
    EventRegistrationToken m_rightTappedToken = {};
    EventRegistrationToken m_holdingToken = {};
    EventRegistrationToken m_manipStartedToken = {};
    EventRegistrationToken m_manipUpdatedToken = {};
    EventRegistrationToken m_manipInteriaStartingToken = {};
    EventRegistrationToken m_manipCompletedToken = {};
};