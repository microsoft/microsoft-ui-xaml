// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MockExternalDependency.h>
#include <ExternalDependency.h>

static int s_iGetStorageCounter = 0;
extern "C" __declspec(dllexport) DependencyLocator::Internal::LocalDependencyStorage* __cdecl GetDependencyLocatorStorage()
{
    s_iGetStorageCounter++;
    return &(DependencyLocator::Internal::GetDependencyLocatorStorage().Get());
}

static wrl::ComPtr<IInspectable> s_item;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        PROVIDE_DEPENDENCY(ExternalSimpleObject1);
        PROVIDE_DEPENDENCY(ExternalSimpleObject2);

        int ExternalSimpleObject1::GetLoadedCount() const
        {
            return s_iGetStorageCounter;
        }

        int ExternalSimpleObject1::ReturnTheNumberOne() const
        {
            return 1;
        }

        int ExternalSimpleObject2::GetLoadedCount() const
        {
            return s_iGetStorageCounter;
        }

        int ExternalSimpleObject2::ReturnTheNumberTwo() const
        {
            return 2;
        }

        void ExternalSimpleObject2::SetItem(IInspectable* object)
        {
            s_item = object;
        }

    }

} } } }

