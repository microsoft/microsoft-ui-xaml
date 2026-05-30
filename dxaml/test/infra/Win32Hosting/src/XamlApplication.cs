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
#if !BUILD_WINDOWS
    public
#endif
    partial class XamlApplication : WUX.Application, WUX.Markup.IXamlMetadataProvider
    {
        private static readonly List<Type> FilteredTypes = new List<Type>
        {
            typeof(XamlApplication),
            typeof(WUX.Markup.IXamlMetadataProvider)
        };

        // Metadata provider identified by the root metadata provider
        private List<WUX.Markup.IXamlMetadataProvider> _metadataProviders = null;

        /// <summary>
        /// Gets XAML <see cref="Microsoft.UI.Xaml.Markup.IXamlType"/> interface from all cached metadata providers for the <paramref name="type"/>.
        /// </summary>
        /// <param name="type">Type of requested type</param>
        /// <returns>IXamlType interface or null if type is not found</returns>
        public WUX.Markup.IXamlType GetXamlType(Type type)
        {
            EnsureMetadataProviders();

            foreach (var provider in _metadataProviders)
            {
                var result = provider.GetXamlType(type);
                if (result != null)
                {
                    return result;
                }
            }

            return null;
        }

        /// <summary>
        /// Gets XAML IXamlType interface from all cached metadata providers by full type name
        /// </summary>
        /// <param name="fullName">Full name of requested type</param>
        /// <returns><see cref="Microsoft.UI.Xaml.Markup.IXamlType"/> if found; otherwise, null.</returns>
        public WUX.Markup.IXamlType GetXamlType(string fullName)
        {
            EnsureMetadataProviders();

            foreach (var provider in _metadataProviders)
            {
                var result = provider.GetXamlType(fullName);
                if (result != null)
                {
                    return result;
                }
            }

            return null;
        }

        /// <summary>
        /// Gets all XAML namespace definitions from metadata providers
        /// </summary>
        /// <returns>Array of namespace definitions</returns>
        public WUX.Markup.XmlnsDefinition[] GetXmlnsDefinitions()
        {
            EnsureMetadataProviders();

            var definitions = new List<WUX.Markup.XmlnsDefinition>();
            foreach (var provider in _metadataProviders)
            {
                definitions.AddRange(provider.GetXmlnsDefinitions());
            }

            return definitions.ToArray();
        }

        /// <summary>
        /// Probes file system for UWP XAML metadata providers
        /// </summary>
        private void EnsureMetadataProviders()
        {
            if (_metadataProviders == null)
            {
                // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
                // Our tests do not need metadata. Replace actual call with empty MetadataProvider, as it's taking long while loading data
                // Do not copy this into the GitHub version. 
                _metadataProviders = this.GetMetadataProviders().ToList();
                // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

            }
        }

        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION        
        protected virtual IEnumerable<WUX.Markup.IXamlMetadataProvider> GetMetadataProviders()
        {
            // Our tests do not use MetadataProvider, hence skipping them from loading.
            // return MetadataProviderDiscovery.DiscoverMetadataProviders(FilteredTypes);
            return Enumerable.Empty<WUX.Markup.IXamlMetadataProvider>();
        }
        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION


        /// <summary>
        /// Gets and returns the current UWP XAML Application instance in a reference parameter.
        /// If the current XAML Application instance has not been created for the process (is null),
        /// a new <see cref="Microsoft.Toolkit.Win32.UI.XamlHost.XamlApplication" /> instance is created and returned.
        /// </summary>
#if BUILD_WINDOWS
        internal
#else
        public
#endif
        static void GetOrCreateXamlApplicationInstance(ref WUX.Application application)
        {
            // Instantiation of the application object must occur before creating the DesktopWindowXamlSource instance.
            // DesktopWindowXamlSource will create a generic Application object unable to load custom UWP XAML metadata.
            if (application == null)
            {
                try
                {
                    // Microsoft.UI.Xaml.Application.Current may throw if DXamlCore has not been initialized.
                    // Treat the exception as an uninitialized Microsoft.UI.Xaml.Application condition.
                    application = WUX.Application.Current;
                }
                catch
                {
                    // Create a custom UWP XAML Application object that implements reflection-based XAML metadata probing.
                    application = new XamlApplication();
                }
            }
        }
    }
}
