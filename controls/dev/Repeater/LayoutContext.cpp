// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "LayoutContext.h"

#pragma region ILayoutContext

winrt::IInspectable LayoutContext::LayoutState()
{
    return overridable().LayoutStateCore();
}

void LayoutContext::LayoutState(winrt::IInspectable const& value)
{
    overridable().LayoutStateCore(value);
}

#pragma endregion

#pragma region ILayoutContextOverrides

winrt::IInspectable LayoutContext::LayoutStateCore()
{
    throw winrt::hresult_not_implemented();
}

void LayoutContext::LayoutStateCore(winrt::IInspectable const& /*state*/)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion