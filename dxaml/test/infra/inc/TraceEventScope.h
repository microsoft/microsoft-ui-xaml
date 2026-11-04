// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    struct TraceEventScope
    {
        // A zero process keeps legacy capture; scoped timestamps use TAEF's real-time QPC clock.
        std::uint32_t processId = 0;
        std::int64_t startTimestamp = 0;

        bool Includes(std::uint32_t eventProcessId, std::int64_t eventTimestamp) const
        {
            return processId == 0 || (eventProcessId == processId && eventTimestamp >= startTimestamp);
        }
    };

} } } } }
