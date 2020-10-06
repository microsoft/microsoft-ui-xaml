// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using Microsoft.VisualStudio.DesignTools.Extensibility.Metadata;

namespace Microsoft.UI.Xaml.DesignTools.ControlProvider
{
    abstract class ControlPropertyProvider
    {
        private static AttributeTableBuilder builder;
        private static CategoryAttribute CommonCategory = new CategoryAttribute("Common");
        
        /// <summary>
        /// The control type this control property provider will work with and add properties to.
        /// </summary>
        public abstract string ControlName { get; }

        internal static void SetGlobalTableBuilder(AttributeTableBuilder newBuilder)
        {
            builder = newBuilder;
        }

        /// <summary>
        /// This method adds the properties of the control to the builder it was created with
        /// </summary>
        public abstract void AddProperties();

        /// <summary>
        /// Adds a property to the list of common properties.
        /// </summary>
        /// <param name="name">The name of the property</param>
        protected void RegisterCommonProperty(string name)
        {
            builder.AddCustomAttributes(
                ControlReferenceLookupTable.GetReference(ControlName),
                name,
                CommonCategory
            );
        }

    }
}
