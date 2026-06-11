// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Windows.Forms;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Microsoft.Windows.Interop
{
    /// <summary>
    ///     A Windows Forms control that can be used to host XAML content
    /// </summary>
    partial class XamlContentHost
    {
        private static TraceSource traceSource = new TraceSource("XamlContentHost");
    }
}
