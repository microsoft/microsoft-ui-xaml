// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using VKEYS = Windows.System;
using WUX = Microsoft.UI.Xaml;
using Microsoft.Toolkit.Wpf.UI.XamlHost;

namespace Private.Infrastructure.Hosting.WPF
{
#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    public sealed class WindowsXamlHostWrapper : IDisposable
    {
        private readonly WindowsXamlHost host;

        public WindowsXamlHostWrapper()
        {
            this.host = new WindowsXamlHost();
        }

        public Double Height
        {
            get { return host.ActualHeight; }
            set { this.host.Height = value; }
        }

        public Double Width
        {
            get { return host.ActualWidth; }
            set { this.host.Width = value; }
        }

        public WUX.UIElement Content
        {
            get { return host.Child; }
            set { this.host.Child = value; }
        }

        public void InsertInto(object container)
        {
            var c = container as System.Windows.Controls.UIElementCollection;
            c.Add(this.host);
        }

        public void Dispose()
        {
            var dispatcher = this.host.Dispatcher;
            dispatcher.Invoke(() =>
            {
                this.host.Dispose();
            });
        }
    }
}
