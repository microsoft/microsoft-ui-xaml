// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{

_Check_return_ HRESULT GetTransitionContext(
    _In_ xaml::IUIElement* element,
    _Out_ BOOLEAN* pRelevantInformation,
    _Out_ ThemeTransitionContext* pContext);

}
