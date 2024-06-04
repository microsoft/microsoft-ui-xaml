// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//---------------------------------------------------------------------------
//
//  TextSelectionSettings
//
//  Singleton-like. Contains the hard coded parameters.
//
//---------------------------------------------------------------------------

// TODO (OS TFS 11359798): None of these values ever change, so these should be changed to just be constants without a class instance.
class TextSelectionSettings
{
public:
    static const TextSelectionSettings * Get();

    XFLOAT m_rHandleTopToCenterHeight;          // The distance between the top of the handle and the center point of the gripper.
                                                // This value is used to adjust the safety zone processing vs the selection location.
    XFLOAT m_rHandleInternalOutlineDiameter;    // Diameter of the internal outline of the handle in millimeters
    XFLOAT m_rHandleExternalOutlineDiameter;    // Diameter of the external outline of the handle in millimeters
    XFLOAT m_rHandleOutlineThickness;           // Thickness of the internal outline of the handle in millimeters
    XFLOAT m_rHandlePostThickness;              // Thickness of the handle's post in millimeters
    XFLOAT m_rHandleTouchTargetWidth;           // Width of the rectangular touch target of handle in millimeters
    XFLOAT m_rHandleTouchTargetHeight;          // Height of the rectangular touch target of handle in millimeters
    XFLOAT m_rCaretAnimationRampUpFraction;     // Fraction of the caret blink interval when the caret fades in [0..1]
    XFLOAT m_rCaretAnimationRampDownFraction;   // Fraction of the caret blink interval when the caret fades out [0..1]
    XFLOAT m_rCaretAnimationUpPlateauFraction;  // Fraction of the caret blink interval when the caret stays on [0..1]
    XFLOAT m_rCaretAnimationDownPlateauFraction;// Fraction of the caret blink interval when the caret stays off [0..1]
    XFLOAT m_rCaretAnimationMaxBlockOpacity;    // Maximum opacity of the caret when it is wider than 1 pixel [0..1]
    XFLOAT m_rCaretBlinkTimeout;                // The caret will stop this many seconds. When <= 0, the timeout is disabled
    XFLOAT m_rCaretBlinkStart;                  // The caret will wait this many seconds before it starts blinking
private:
    TextSelectionSettings();
    void SetDefaults();
    static TextSelectionSettings m_Instance; // Instead of allocating an object, will use a static instance
};

