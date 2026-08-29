// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using XamlDom;

    internal class xProperty
    {
        public string Name = null;
        public bool IsReadOnly = false;

        // This is only guaranteed to be valid/non-null in Pass 2
        public System.Xaml.XamlType PropertyType = null;

        public XamlDomObject OriginalXProperty = null;
        private string _changedHandler = null;

        public string ChangedHandler
        {
            get
            {
                if (_changedHandler == null)
                {
                    return Name + "Changed";
                }
                return _changedHandler;
            }
            set
            {
                _changedHandler = value;
            }
        }

        public string FullTypeName = null;

        // If the default value exists and is a dom object instead of a simple string, the markup needed for XamlReader.Load will be stored here.
        // Mutually exclusive with DefaultValueString
        public string DefaultValueMarkup = null;

        // If the default value exists and is a simple string, this string is its value.
        // Mutually exclusive with DefaultValueMarkup
        public string DefaultValueString = null;

        // A string to display in code-gen describing where the x:Property is defined
        public string CodegenComment = null;

        public xProperty()
        {
        }
    }

    internal class xPropertyInfo
    {
        public XamlDomObject xPropertiesNode;
        public XamlDomObject xPropertiesRoot; // The page root
        public System.Collections.Generic.List<xProperty> xProperties;
    }
}
