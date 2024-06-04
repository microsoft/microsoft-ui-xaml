// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ExternalDependency.h>


namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class __declspec(uuid("c763721f-036b-4a8a-8cbb-a672bbc50bc1")) ExternalSimpleObject1
        {
        public:
            virtual int ReturnTheNumberOne() const;
            virtual int GetLoadedCount() const;
        };

        class __declspec(uuid("16b1426f-eadc-490c-ab6d-60715f6025fa")) ExternalSimpleObject2
        {
        public:
            virtual int ReturnTheNumberTwo() const;
            virtual int GetLoadedCount() const;
            virtual void SetItem(IInspectable* keepAlive);
        };
    }

} } } }

