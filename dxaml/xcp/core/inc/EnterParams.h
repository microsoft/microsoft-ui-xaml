// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CResourceDictionary;
class VisualTree;

//
// Structure used by Enter/EnterImpl for
// propagating state flags that affect the object.
//
struct EnterParams
{
    // These are control flags for Enter operations
    bool fIsLive                       : 1;
    bool fSkipNameRegistration         : 1;
    bool fCoercedIsEnabled             : 1;
    bool fUseLayoutRounding            : 1;
    bool fIsForKeyboardAccelerator     : 1;
    bool fCheckForResourceOverrides    : 1;
    CResourceDictionary *pParentResourceDictionary;
    VisualTree* visualTree;

    EnterParams()
        : fIsLive(true)
        , fSkipNameRegistration(false)
        , fCoercedIsEnabled(true)
        , fUseLayoutRounding(UseLayoutRoundingDefault)
        , fIsForKeyboardAccelerator(false)
        , fCheckForResourceOverrides(false)
        , pParentResourceDictionary(nullptr)
        , visualTree(nullptr)
    {}

    EnterParams(
        bool isLive,
        bool skipNameRegistration,
        bool coercedIsEnabled,
        bool useLayoutRounding,
        VisualTree* enteringVisualTree)
        : fIsLive(isLive)
        , fSkipNameRegistration(skipNameRegistration)
        , fCoercedIsEnabled(coercedIsEnabled)
        , fUseLayoutRounding(useLayoutRounding)
        , fIsForKeyboardAccelerator(false)
        , fCheckForResourceOverrides(false)
        , pParentResourceDictionary(nullptr)
        , visualTree(enteringVisualTree)
    {}

    static constexpr bool UseLayoutRoundingDefault = true;
};
