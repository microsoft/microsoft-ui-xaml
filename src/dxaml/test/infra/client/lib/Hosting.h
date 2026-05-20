// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Infrastructure {
    namespace Hosting {
        enum HostingMode
        {
            UAP = 0x00,
            WPF = 0x01,
            WinForms = 0x02,
            Win32Explicit = 0x03,   // Explicit hosting - the test is responsible for creating threads, hwnds, and DesktopWindowXamlSources
        };

        HRESULT GetHostingMode(HostingMode* hostingMode);
    }
}}
