// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using Windows.Foundation;
using Microsoft.Windows.Interop;
using Private.Infrastructure.Hosting;

namespace Private.Infrastructure.Hosting.WinForms
{
    public sealed class HostFactory: IWin32HostFactory
    {
#if BUILD_WINDOWS
        [CLSCompliant(false)]
#endif
        public void SetExceptionHandler(ExceptionHandler handler)
        {
            AppDomainExceptionHandler.SetExceptionHandler(handler);
        }

        private async Task<object> CreateInternal()
        {
            var winFormsHost = new WinFormsHost();
            await winFormsHost.Init();
            return winFormsHost;
        }

#if BUILD_WINDOWS
        [CLSCompliant(false)]
#endif
        public IAsyncOperation<object> Create(DpiAwarenessContext dpiAwarenessContext /* ignored */, bool initCore /* ignored */)
        {
            return CreateInternal().AsAsyncOperation();
        }
    }
}

