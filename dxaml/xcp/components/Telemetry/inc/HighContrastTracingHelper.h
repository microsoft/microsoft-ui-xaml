// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Theming {
    enum class Theme : uint8_t;
}

void TraceLoggingHCColor(_In_ CUIElement *element, _In_ Theming::Theme theme, _In_ const SetValueParams& args);