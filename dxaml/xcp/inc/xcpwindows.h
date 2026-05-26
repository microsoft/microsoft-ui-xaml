// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Includes windows.h and undefs definitions that are used in this code-base.

#pragma once

// Including new here for components that wish to use the in-place new operator
// but don't currently include any STL headers.
#include <new>

#include <windows.h>
#include <wrl.h>

// Windows headers will #define these method names with A/W suffixes, which wreaks havoc on the link.
#ifdef GetClassName
#undef GetClassName
#endif

#include <Unknwn.h>
#include <inspectable.h>
#include "Windows.Foundation.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "XamlStructCollectionDeclarations.h"
#include "HeaderTrimming.h"

#include "Windows.Foundation.Numerics.h"

// Standard Windows error handling helpers.
#include <wil\Result.h>
