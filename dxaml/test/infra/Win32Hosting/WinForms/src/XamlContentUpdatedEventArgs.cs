// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Xaml = Microsoft.UI.Xaml;

namespace Microsoft.Windows.Interop
{
    /// <summary>
    ///     Contains root FrameworkElement for XamlContentUpdated event
    /// </summary>
    partial class XamlContentUpdatedEventArgs : EventArgs
    {
        /// <summary>
        /// Root Windows.UI.FrameworkElement assigned to XamlContentHost
        /// </summary>
        private Xaml.FrameworkElement rootXAMLElement;

        /// <summary>
        /// Creates a XamlContentUpdatedEventArgs instance
        /// </summary>
        /// <param name="e">New root Windows.UI.FrameworkElement assigned to XamlContentHost</param>
        internal XamlContentUpdatedEventArgs(Xaml.FrameworkElement e)
        {
            this.rootXAMLElement = e;
        }

        /// <summary>
        /// Gets root Windows.UI.FrameworkElement assigned to XamlContentHost
        /// </summary>
        public Xaml.FrameworkElement RootXAMLElement
        {
            get { return this.rootXAMLElement; }
        }
    }
}
