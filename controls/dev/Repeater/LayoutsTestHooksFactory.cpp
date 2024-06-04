// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LayoutsTestHooksFactory.h"
#include "LayoutsTestHooks.properties.cpp"

LayoutsTestHooks* LayoutsTestHooks::s_testHooks = nullptr;

void LayoutsTestHooks::EnsureHooks()
{
    if (!s_testHooks)
    {
        s_testHooks = winrt::make_self<LayoutsTestHooks>().detach();
    }
}
