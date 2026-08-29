// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    internal abstract class XamlDomItem : XamlDomNode
    {
        public XamlDomItem(string sourceFilePath)
            : base(sourceFilePath)
        {}

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        [DefaultValue(null)]
        public  XamlDomMember Parent
        {
            get { return _parent; }
            set
            {
                CheckSealed();
                _parent = value;
            }
        }

        private XamlDomMember _parent;
    }
}
