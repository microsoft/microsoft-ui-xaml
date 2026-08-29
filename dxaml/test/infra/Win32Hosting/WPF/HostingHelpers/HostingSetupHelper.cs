// Copyright (c) Microsoft Corporation. Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Private.Infrastructure.Hosting.HostingHelpers
{
    [Windows.Foundation.Metadata.Version(1)]
    public sealed class HostingSetupHelper
    {
        public void InitializeWPFHostFactory()
        {
        #if !_ARM64_
            var hf = new global::Private.Infrastructure.Hosting.WPF.HostFactory();
            global::Private.Infrastructure.TestHostSettings.Win32HostFactory = hf;
        #else
            throw new NotImplementedException("WPF hosting mode is not currently supported for ARM64");
        #endif
        }
    }
}