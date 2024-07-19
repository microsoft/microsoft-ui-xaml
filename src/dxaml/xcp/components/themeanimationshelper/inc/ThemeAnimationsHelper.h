// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI { namespace Components { namespace ThemeAnimationsHelper {

    bool DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(
        _In_ float containerWidth,
        _In_ float containerHeight,
        _Inout_ float& controlWidth,
        _Inout_ float& controlHeight,
        _Out_ float& halfDifferenceX,
        _Out_ float& halfDifferenceY);

} } }