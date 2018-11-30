// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace People { namespace Controls {

    namespace StringUtil
    {
        /// <summary>
        /// Formats a given string with the desired parameter list.
        /// </summary>
        /// <param name="formatString">The format string.</param>
        /// <param name="...">The parameter list of format args.</param>
        /// <returns>The formatted string.</returns>
        Platform::String^ FormatString(_In_ Platform::String^ formatString, ...);
    }
}}}