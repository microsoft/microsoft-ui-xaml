// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

// Declare one mode inside BEGIN_TEST_CLASS/END_TEST_CLASS. Tests using TestServices
// also call XAML_HOSTING_MODE_CLASS_SETUP() from ClassSetup to initialize that host.
#define TEST_CLASS_HOSTING_MODE(mode) TEST_CLASS_HOSTING_MODE_IMPL_##mode()

// Keep the default's launch metadata and runtime mode together.
#define TEST_CLASS_HOSTING_MODE_DEFAULT() TEST_CLASS_HOSTING_MODE_DEFAULT_WITH_THREADING(L"STA")
#define TEST_CLASS_HOSTING_MODE_DEFAULT_WITH_THREADING(threadingModel) \
    TEST_CLASS_HOSTING_MODE_IMPL_WPF_WITH_THREADING(threadingModel)

// Explicit hosts own process-wide Application state.
#define TEST_CLASS_HOSTING_MODE_IMPL_Win32Explicit() \
    TEST_CLASS_PROPERTY(L"Hosting:Mode", L"Win32Explicit") \
    TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class") \
    TEST_CLASS_PROPERTY(L"UAP:Host", L"PackagedCwa") \
    TEST_CLASS_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)

#define TEST_CLASS_HOSTING_MODE_IMPL_WPF() TEST_CLASS_HOSTING_MODE_IMPL_WPF_WITH_THREADING(L"STA")
#define TEST_CLASS_HOSTING_MODE_IMPL_WPF_WITH_THREADING(threadingModel) \
    TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF") \
    TEST_CLASS_PROPERTY(L"ThreadingModel", threadingModel) \
    TEST_CLASS_PROPERTY(L"UAP:Host", L"PackagedCwa") \
    TEST_CLASS_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)

#define TEST_CLASS_HOSTING_MODE_IMPL_UAP() \
    TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") \
    TEST_CLASS_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT)
