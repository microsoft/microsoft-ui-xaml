// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlCompilerPage.g.h"

namespace CustomTypes
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class XamlCompilerPage sealed
    {
    public:
        XamlCompilerPage();
        property Platform::String^ foo;
    };
}
