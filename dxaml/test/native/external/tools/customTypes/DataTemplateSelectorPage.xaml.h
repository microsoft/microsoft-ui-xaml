// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataTemplateSelectorPage.g.h"
#include "ValueConverters.h" // include this here so XamlTypeInfo.g.cpp knows about the value converters
#include "DataTemplateSelectors.h" // ^^
namespace Tests { namespace Tools { namespace Shared {

    public ref class DataTemplateSelectorPage sealed
    {
    public:
        DataTemplateSelectorPage();
    };
} } }
