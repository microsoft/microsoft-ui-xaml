// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <string>

#include <wexstring.h>

namespace ErrorHandling {
    struct XamlFailureInfo;
}

namespace Private { namespace Infrastructure {

    // Simple class that prints the stack where an error ocurred.
    class SmartStackLogger
    {
    public:
        SmartStackLogger() = default;
        void LogStackIfNovel(const ErrorHandling::XamlFailureInfo& failure);
        void LogStackFromContext(CONTEXT context);

        bool IsEnabled() const;
        void SetIsEnabled(bool value);

    private:
        WEX::Common::String GetStack(DWORD stackFormat, CONTEXT context);
        bool m_enabled = false;
    };

} }
