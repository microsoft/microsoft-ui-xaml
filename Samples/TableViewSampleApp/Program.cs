// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using System.Threading;
#if !CSWINRT3_0
using WinRT;
#endif

namespace TableViewSampleApp;

public static class Program
{
    [System.STAThread]
    public static int Main(string[] args)
    {
#if !CSWINRT3_0
        ComWrappersSupport.InitializeComWrappers();
#endif
        Application.Start(p =>
        {
            var context = new DispatcherQueueSynchronizationContext(DispatcherQueue.GetForCurrentThread());
            SynchronizationContext.SetSynchronizationContext(context);
            _ = new App(args);
        });

        return 0;
    }
}
