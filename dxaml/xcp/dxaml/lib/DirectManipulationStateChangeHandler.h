// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Callback object used to notify one interested party that a DirectManipulation state has changed.
//      This mechanism is meant for internal controls like SemanticZoom and ListViewBase. They become the
//      handler. The handler is not meant to be a child of the ScrollViewer. Extending the feature to
//      ScrollViewer children would require some reference counting work on the listener.
//      Once the ScrollViewer control exposes public events like ViewChanging and ViewChanged, those
//      listeners will be able to adopt them and this internal mechanism can be retired.

#pragma once

namespace DirectUI
{
    enum DMManipulationState;

    class DirectManipulationStateChangeHandler
    {
    public:
        // zCumulativeFactor: if the zoom factor was 1.5 at the beginning of the manipulation,
        // and the current zoom factor is 3.0, then zCumulativeFactor is 2.0.
        // xCenter/yCenter: these coordinates are in relation to the top/left corner of the
        // manipulated element. They might be negative if the ScrollViewer content is smaller
        // than the viewport.
        _Check_return_ virtual HRESULT NotifyStateChange(
            _In_ DMManipulationState state,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation,
            _In_ FLOAT zCumulativeFactor,
            _In_ FLOAT xCenter,
            _In_ FLOAT yCenter,
            _In_ BOOLEAN isInertial,
            _In_ BOOLEAN isTouchConfigurationActivated,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated) = 0;

    protected:
        virtual ~DirectManipulationStateChangeHandler() { }
    };
}

