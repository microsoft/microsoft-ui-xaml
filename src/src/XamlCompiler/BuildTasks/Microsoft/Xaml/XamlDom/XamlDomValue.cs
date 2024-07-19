// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Diagnostics;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    [DebuggerDisplay("{Value}")]
    internal class XamlDomValue : XamlDomItem
    {
        public XamlDomValue(string sourceFilePath)
            : base(sourceFilePath)
        {
        }

        public XamlDomValue(object value, string sourceFilePath)
            :base(sourceFilePath)
        {
            _value = value;
        }

        [DefaultValue(null)]
        public virtual object Value
        {
            get { return _value; }
            set { CheckSealed(); _value = value; }
        }

        private object _value;
    }
}
