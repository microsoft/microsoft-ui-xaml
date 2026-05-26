// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Shut up Intellisense.
#if (__INTELLISENSE__)
#define L__FUNCTION__ L""
#endif

#include <windows.h>

#include <inspectable.h>
#include <strsafe.h>
#include <initguid.h>
#include <ppltasks.h>

// STL
#include <list>
#include <vector>
#include <queue>
#include <memory>
#include <map>
#include <functional>
#include <sstream>
#include <chrono>
#include <unordered_map>

#include <wrl.h>

#include <wil\resource.h>
#include <wil\result_macros.h>

#include <AssertMacros.h>

#include <wex.common.h>
#include <WexTestClass.h>

#include <NamespaceAliasesTest.h>
#include <RpcServer.h>