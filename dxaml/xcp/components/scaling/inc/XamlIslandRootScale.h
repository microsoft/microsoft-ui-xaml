// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RootScale.h"
#include <fwd/windows.ui.composition.h>

class XamlIslandRootScale : public RootScale
{
public:
    XamlIslandRootScale(CCoreServices* pCoreServices, VisualTree* pVisualTree);
protected:
    _Check_return_ HRESULT ApplyScaleProtected(bool scaleChanged) override;
};
