// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RefreshVisualizer.h"

#include "RefreshStateChangedEventArgs.g.h"
#include "RefreshRequestedEventArgs.g.h"

//////////////////////////////////////////////////////////
/////////        RefreshStateChanged        /////////////
//////////////////////////////////////////////////////////

class RefreshStateChangedEventArgs : 
    public winrt::implementation::RefreshStateChangedEventArgsT<RefreshStateChangedEventArgs>
{
public:
    RefreshStateChangedEventArgs(winrt::RefreshVisualizerState oldValue, winrt::RefreshVisualizerState newValue);

    //IRefreshStatusChangedEventArgs overrides
    winrt::RefreshVisualizerState OldState();
    winrt::RefreshVisualizerState NewState();

private:
    winrt::RefreshVisualizerState m_oldState { };
    winrt::RefreshVisualizerState m_newState { };
};


//////////////////////////////////////////////////////////
/////////          RefreshRequested          /////////////
//////////////////////////////////////////////////////////

class RefreshRequestedEventArgs :
    public ReferenceTracker<RefreshRequestedEventArgs, winrt::implementation::RefreshRequestedEventArgsT, winrt::composing, winrt::composable>
{
public:
    RefreshRequestedEventArgs(const winrt::Deferral& handler);

    winrt::Deferral GetDeferral();

    void IncrementDeferralCount();
    void DecrementDeferralCount();

private:
    tracker_ref<winrt::Deferral> m_deferral{ this };
    int m_deferralCount{ 0 };
};