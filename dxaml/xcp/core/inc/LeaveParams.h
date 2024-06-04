// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CResourceDictionary;

//
// Structure used by Leave/LeaveImpl for
// propagating state flags that affect the object.
//
struct LeaveParams
{
    // These are control flags for Leave operations
    bool fIsLive                       : 1;
    bool fSkipNameRegistration         : 1;
    bool fCoercedIsEnabled             : 1;
    bool fUseLayoutRounding            : 1;
    bool fVisualTreeBeingReset         : 1;
    bool fIsForKeyboardAccelerator     : 1;
    bool fCheckForResourceOverrides    : 1;
    CResourceDictionary *pParentResourceDictionary;

    LeaveParams()
        : fIsLive(TRUE)
        , fSkipNameRegistration(FALSE)
        , fCoercedIsEnabled(TRUE)
        , fUseLayoutRounding(EnterParams::UseLayoutRoundingDefault)
        , fVisualTreeBeingReset(FALSE)
        , fIsForKeyboardAccelerator(FALSE)
        , fCheckForResourceOverrides(FALSE)
        , pParentResourceDictionary(nullptr)
    {
    }

    LeaveParams(
        bool isLive,
        bool skipNameRegistration,
        bool coercedIsEnabled,
        bool visualTreeBeingReset)
        : fIsLive(isLive)
        , fSkipNameRegistration(skipNameRegistration)
        , fCoercedIsEnabled(coercedIsEnabled)
        , fUseLayoutRounding(EnterParams::UseLayoutRoundingDefault)
        , fVisualTreeBeingReset(visualTreeBeingReset)
        , fIsForKeyboardAccelerator(FALSE)
        , fCheckForResourceOverrides(FALSE)
        , pParentResourceDictionary(nullptr)
    {}
};
