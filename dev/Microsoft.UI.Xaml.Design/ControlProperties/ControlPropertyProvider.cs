// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Windows.Design.Metadata;
using System;
using System.ComponentModel;

namespace Microsoft.UI.Xaml.Design.ControlProvider
{
    abstract class ControlPropertyProvider
    {
        private AttributeTableBuilder builder;
        private static CategoryAttribute CommonCategory = new CategoryAttribute("Common2");
        
        /// <summary>
        /// The control type this control property provider will work with and add properties to.
        /// </summary>
        public abstract Type controlType { get; }

        public ControlPropertyProvider(AttributeTableBuilder builder)
        {
            this.builder = builder;
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
                controlType,
                name,
                CommonCategory
            );
        }

    }
}
