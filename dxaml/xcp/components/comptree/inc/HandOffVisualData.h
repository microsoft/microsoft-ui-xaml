// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CUIElement;

// HandOffVisualData holds several things:
// -The HandOff visual
// -The object that listens for DComp property changes
// -Simple cache of properties about the HandOff visual, these help the CompNode know when to set property values
//       See usage in HWCompTreeNodeWinRT::EnsureVisual() and HWCompTreeNodeWinRT::CleanupHandOffVisual().

class HandOffVisualData
{
public:
    HandOffVisualData()
    : cachedPropertiesInUse(false)
    {
    }

    Microsoft::WRL::ComPtr<WUComp::IVisual> handOffVisual;
    Microsoft::WRL::ComPtr<DCompPropertyChangedListener> dcompPropertyChangedListener;
    bool cachedPropertiesInUse : 1;
    bool previousHasWUCInsetClip : 1;
    float previousWUCOpacity;
    wfn::Vector3 previousWUCOffset;
    WUComp::CompositionCompositeMode previousWUCCompositeMode;
    wfn::Matrix4x4 previousWUCTransformMatrix;
    XRECTF_RB previousWUCInsetClip;
    wfn::Matrix3x2 previousWUCClipTransformMatrix{};
};

typedef containers::vector_map<CUIElement*, HandOffVisualData> HandOffVisualDataMap;