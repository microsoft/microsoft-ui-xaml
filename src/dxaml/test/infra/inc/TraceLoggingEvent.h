// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <string>
#include <utility>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    struct TraceLoggingEvent
    {
    public:
        TraceLoggingEvent()
            : name(L"")
            , provider(L"")
            , id(0)
            , opcode(0)
            , level(0)
        { }

        TraceLoggingEvent(const std::wstring& name, const std::wstring& provider, int id, int opcode, int level)
            : name(name)
            , provider(provider)
            , id(id)
            , opcode(opcode)
            , level(level)
        { }

        TraceLoggingEvent(TraceLoggingEvent&& other) noexcept
            : name(std::move(other.name))
            , provider(std::move(other.provider))
            , id(other.id)
            , opcode(other.opcode)
            , level(other.level)
        { }

        TraceLoggingEvent& operator=(TraceLoggingEvent&& other) noexcept
        {
            if (this != &other)
            {
                name = std::move(other.name);
                provider = std::move(other.provider);
                id = std::move(other.id);
                opcode = std::move(other.opcode);
                level = std::move(other.level);
            }
            return *this;
        }

        std::wstring name;
        std::wstring provider;
        int id;
        int opcode;
        int level;

    };
} } } } }