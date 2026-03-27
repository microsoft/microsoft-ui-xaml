// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
namespace Microsoft.UI.Xaml.Tests.Common
{
    public class TestCleanupWrapper : IDisposable
    {
        void IDisposable.Dispose()
        {
            global::Private.Infrastructure.TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
        }
    }
}