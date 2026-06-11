// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef __DISABLE_EVENT_WAIT

#include <chrono>

using namespace std::chrono_literals;

#define DefaultTextInputTimeout                     500ms
#define DefaultTimeout                               10s
#define BVT_DefaultTimeout                            2min
#define BVT_DefaultTextInputTimeout  BVT_DefaultTimeout

#endif
