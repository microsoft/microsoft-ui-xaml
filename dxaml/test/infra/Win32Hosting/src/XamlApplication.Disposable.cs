// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using System.Collections.Generic;
// CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
// Due to namespace clashing I had to use variable names with them.
using WUX =  Microsoft.UI.Xaml;
// CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

namespace Microsoft.Toolkit.Win32.UI.XamlHost
{
    /// <summary>
    /// XamlApplication is a custom <see cref="Microsoft.UI.Xaml.Application" /> that implements <see cref="Microsoft.UI.Xaml.Markup.IXamlMetadataProvider" />. The
    /// metadata provider implemented on the application is known as the 'root metadata provider'.  This provider
    /// has the responsibility of loading all other metadata for custom UWP XAML types.  In this implementation,
    /// reflection is used at runtime to probe for metadata providers in the working directory, allowing any
    /// type that includes metadata (compiled in to a .NET framework assembly) to be used without explicit
    /// metadata handling by the developer.
    /// </summary>
    partial class XamlApplication : IDisposable
    {
        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
        // Added constructor here which will take care of instantiating WindowsXamlManager
        public XamlApplication()
        {
            _windowsXamlManager = WUX.Hosting.WindowsXamlManager.InitializeForCurrentThread();            
        }

        private readonly WUX.Hosting.WindowsXamlManager _windowsXamlManager;
        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
        // Added Disposable pattern
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~XamlApplication()
        {
            Dispose(false);
        }

        /// <summary>
        /// Gets or sets a value indicating whether this application been disposed
        /// </summary>
        public bool IsDisposed { get; private set; }

        protected virtual void Dispose(bool disposing)
        {
            if (IsDisposed)
                return;

            IsDisposed = true;

            if (disposing)
            {
                // Free any other managed objects here.
                //
            }

            // Free any unmanaged objects here.
            //
            try
            {
                _windowsXamlManager.Dispose();
            }
            catch(Exception)
            {
            }
        }       
    }
}
