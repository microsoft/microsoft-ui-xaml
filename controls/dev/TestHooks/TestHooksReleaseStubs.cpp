// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// In release builds, test hook implementations are excluded but their IDL types
// remain in the WinMD so that test apps can compile against them. These stubs
// satisfy the winrt_make_* symbols that module.g.cpp references. Activation of
// these types at runtime in release will return null (no factory), which is safe
// because tests that use test hooks only run against debug builds.

#include "pch.h"

void* winrt_make_Microsoft_UI_Private_Controls_AnimatedIconTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_TeachingTipTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_RadioButtonsTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_SwipeTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_ItemsViewTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_ScrollViewTestHooks() { return nullptr; }
void* winrt_make_Microsoft_UI_Private_Controls_ScrollPresenterTestHooks() { return nullptr; }
