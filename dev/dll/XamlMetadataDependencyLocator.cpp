// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include <initguid.h>
#include <wrl\module.h>

#include <minerror.h>
#include <DependencyLocator.h>

#include "XamlTypeInfo.h"
#include "XamlTypeInfo.g.h"

using namespace Microsoft::WRL;

namespace Private
{
    PROVIDE_DEPENDENCY(XamlRuntimeType);
}

// Need it to export services
extern "C" __declspec(dllexport) DependencyLocator::Internal::ILocalDependencyStorage* __cdecl GetDependencyLocatorStorage()
{
    return &(DependencyLocator::Internal::GetDependencyLocatorStorage().Get());
}
