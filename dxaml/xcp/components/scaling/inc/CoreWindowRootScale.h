// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RootScale.h"

class CoreWindowRootScale : public RootScale
{
public:
    CoreWindowRootScale(RootScaleConfig config, CCoreServices* pCoreServices, VisualTree* pVisualTree);
protected:
    _Check_return_ HRESULT ApplyScaleProtected(bool scaleChanged) override;
private:
    _Check_return_ HRESULT CreateReverseTransform(_Out_ CValue* result);
    _Check_return_ HRESULT CreateTransform(_Out_ CValue* result);
};
