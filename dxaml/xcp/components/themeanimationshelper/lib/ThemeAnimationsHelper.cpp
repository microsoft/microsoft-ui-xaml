// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Microsoft.UI.Xaml.h>

#include "ThemeAnimationsHelper.h"
//#include "TranslateTransform.g.h"

using namespace DirectUI;
using namespace DirectUI::Components;

bool ThemeAnimationsHelper::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(
    _In_ float containerWidth,
    _In_ float containerHeight,
    _Inout_ float& controlWidth,
    _Inout_ float& controlHeight,
    _Out_ float& halfDifferenceX,
    _Out_ float& halfDifferenceY)
{
    bool needsTransformGroup = false;
    halfDifferenceX = 0.0f;
    halfDifferenceY = 0.0f;

    if (containerWidth < controlWidth)
    {
        halfDifferenceX = (controlWidth - containerWidth) / 2.0f;

        controlWidth = containerWidth;

        needsTransformGroup = true;
    }

    if (containerHeight < controlHeight)
    {
        halfDifferenceY = (controlHeight - containerHeight) / 2.0f;

        controlHeight = containerHeight;

        needsTransformGroup = true;
    }

    return needsTransformGroup;
}