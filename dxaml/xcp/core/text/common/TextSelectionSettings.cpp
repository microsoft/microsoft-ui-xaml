// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextSelectionSettings.h"

TextSelectionSettings TextSelectionSettings::m_Instance;

//------------------------------------------------------------------------
//
//  Constructor for the settings. Loads the default values.
//
//------------------------------------------------------------------------
TextSelectionSettings::TextSelectionSettings()
{
    SetDefaults();
}

//------------------------------------------------------------------------
//
//  Resets the settings to default values. Used in constructor and
//  to reset values in case of an error.
//
//------------------------------------------------------------------------
void TextSelectionSettings::SetDefaults()
{
    m_rHandleTopToCenterHeight          = 0.6f;
    m_rHandleInternalOutlineDiameter    = 3.7f;
    m_rHandleExternalOutlineDiameter    = 4.8f;
    m_rHandleOutlineThickness           = 0.4f;
    m_rHandlePostThickness              = 0.6f;
    m_rHandleTouchTargetWidth           = 8.0f; // TODO: this the next one are smaller
    m_rHandleTouchTargetHeight          = 9.3f; // than Win8 (12.0f). Check with PM.
    m_rCaretAnimationRampUpFraction     = 0.12f;
    m_rCaretAnimationRampDownFraction   = 0.12f;
    m_rCaretAnimationUpPlateauFraction  = 0.38f;
    m_rCaretAnimationDownPlateauFraction= 0.38f;
    m_rCaretAnimationMaxBlockOpacity    = 0.7f;
    m_rCaretBlinkTimeout                = 5.0f;
    m_rCaretBlinkStart                  = 1.0f;
}

//------------------------------------------------------------------------
//
//  Returns a pointer to an instance of settings object. Always succeeds.
//  Checks for overrides once.
//
//------------------------------------------------------------------------
const TextSelectionSettings* TextSelectionSettings::Get()
{
    return &m_Instance;
}

