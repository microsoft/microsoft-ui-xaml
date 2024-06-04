// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

/*
 ElementGestureTracker serves as the entry point into gesture recognition for XAML. This class is responsible for keeping track of state,
 such as whether we are in the middle of an interaction/manipulation, listening for the requested gestures, and firing the appropriate XAML gesture events
 when a gesture has been identified. This class manages gestures for a single UIElement, but has the capability to reset itself to start managing
 gestures on a new element when needed. Much of the logic in listening and appropriately firing XAML events are deferred to the Adapters.

 Currently, XAML has a reliance on the TouchInteractionMsg struct to fire the XAML events corresponding to the various gestures. When we receive gesture output
 data from the GetstureRecognizerAdapter, we need to map this into a TouchInterationMsg before we are able to fire any XAML events. GestureOutputMapper takes on this role,
 and also keeps track of any necessary state concerning these gestures, such as if we've received a ManipulationStarted, but not a ManipulationCompleted. In the past the
 GetstureOutputMapper create TouchInteractionMsgs for not only the GestureRecognizer, but also for other gesture related inputs.  These other inputs have been deprectated
 and removed so now we are strictly using GestureRecognizer and we'd like to have XAML be able to fire events directly using the data outputted by GestureRecogizer.
 When we've reached this point, we can delete GestureOutputMapper.

 Layout of how the components interact with each other:
                                                                                                 +-----------------+
                                                                                                 |GestureRecognizer|
                                                +---------------------+                          +-----+-----------+
                                                |ElementGestureTracker|                                ^
                                                +---+-------+---------+                                |
                                                            ^                                          |
                                                            |                                          |
                                                            |      +------------------------+          |
                                                            +--->  |GestureRecognizerAdapter| <--------+
                                                                   +------------+-----------+
                                                                                 ^
                                                                                 |
                                                                                 |
                                                                                 v
                                                                      +-------------------+
                                                                      |GestureOutputMapper|
                                                                      +-------------------+
*/



#pragma once

#include "UIElementStructs.h"
#include "GestureRecognizerAdapter.h"

class CTouchInteractionElement;

// This class is responsible for taking XAML pointer information and appropriately passing it to
// the Getsture Recognizer to fire Gesture events for a specific element
class ElementGestureTracker final
{
    friend class GestureRecognizerAdapter;
public:
    ElementGestureTracker(
        _In_ CUIElement *pElement,
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode);
    ~ElementGestureTracker();

public:
    _Check_return_ HRESULT ProcessPointerMessage(
        _In_ const InputMessage& msg);
    void UpdatePeg(bool peg);
    void Destroy();
    _Check_return_ HRESULT Reset();
    _Check_return_ HRESULT ResetAndReinitialize(
        _In_ CUIElement *pElement,
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode);
    _Check_return_ HRESULT SetConfiguration(
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode,
        _In_ XPOINTF ptPivotCenter,
        _In_ float fPivotRadius);
    bool IsIdle();
    void Stop(
        _In_ bool bCallbackForManipulationCompleted);
    bool IsStopped();
    bool IsInertial();
    void ProcessInertiaInteraction();
    void SetManipulationStarting(_In_ bool bManipulationStarting);
    bool IsManipulationStarting();
    bool IsManipulationStarted();
    bool IsPegged();
    bool IsProcessingInput() const { return m_isProcessingInput; }

public:

    // This Deleter is here for using a custom deleter in objects like std::unique_ptr. Since this object
    // is cleaned up through calling Destroy on it, rather than deleting it explicitly.
    struct Deleter
    {
        void operator()(_In_opt_ ElementGestureTracker* pObj) const
        {
            if (pObj)
            {
                pObj->Destroy();
            }
        }
    };

private:
    _Check_return_ HRESULT InitializeInteractionEngine();

    _Check_return_ HRESULT ProcessPointerInformation(
        _In_ const InputMessage& msg);

    _Check_return_ HRESULT UpdateSettings(
        _In_ bool bTapEnabled,
        _In_ bool bDoubleTapEnabled,
        _In_ bool bRightTapEnabled,
        _In_ bool bHoldEnabled,
        _In_ DirectUI::ManipulationModes manipulationMode);

    void SetManipulationStarted(bool value);
    void SetInteractionStopping(bool value);
    void SetInteractionStopped(bool value);
    void SetPointerInputType(XPointerInputType type);

private:
    xref::weakref_ptr<CUIElement>      m_pElement;
    float                              m_floatPivotRadius = 0.0f;
    unsigned int                       m_frameId = 0;
    unsigned int                       m_cPointerInteractionContexts = 0;
    DirectUI::ManipulationModes        m_manipulationMode = DirectUI::ManipulationModes::None;
    bool                               m_bForceUpdateSettings = false;
    bool                               m_bPivot = false;
    bool                               m_bDelayDestroyInteractionEngine = false;
    bool                               m_bPegged = false;
    bool                               m_bTapEnabled = false;
    bool                               m_bDoubleTapEnabled = false;
    bool                               m_bRightTapEnabled = false;
    bool                               m_bHoldEnabled = false;
    bool                               m_isProcessingInput = false;

    GestureRecognizerAdapter m_gestureRecognizerAdapter;
    GestureOutputMapper* m_gestureOutputMapper;
    wrl::ComPtr<ixp::IExpPointerPointStatics> m_expPointerPointStatics;
};