// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// Just a placeholder to force the build to create type infomration.
#pragma once

#include "Controls.CustomPage.g.h"

namespace Private { namespace Tests {
    namespace Sample {

        // Custom types MUST be sealed and have a public constructor
        // if you'd like them to be activatable. 
        [::Windows::Foundation::Metadata::WebHostHidden]
        public ref class CustomPage sealed
        {
        public:
            CustomPage() {}
        };
    }
} }