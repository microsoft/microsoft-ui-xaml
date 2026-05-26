// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using Windows.Foundation;

namespace Private.Infrastructure.Hosting.WPF
{
    [global::Windows.Foundation.Metadata.MarshalingBehavior(global::Windows.Foundation.Metadata.MarshalingType.Agile)]
    public sealed partial class HostFactory : IWin32HostFactory 
    {
        public void SetExceptionHandler(ExceptionHandler handler)
        {
            AppDomainExceptionHandler.SetExceptionHandler(handler);
        }

        private async Task<object> CreateInternal(DpiAwarenessContext dpiAwarenessContext, bool initCore)
        {
            var wpfHost = new WPFHost(dpiAwarenessContext, initCore);
            await wpfHost.EnsureWindow();
            return wpfHost;
        }

        public IAsyncOperation<object> Create(DpiAwarenessContext dpiAwarenessContext, bool initCore)
        {
            return CreateInternal(dpiAwarenessContext, initCore).AsAsyncOperation();
        }
    }
}

