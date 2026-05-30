// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System.ComponentModel;
    using XamlDom;

    [TypeConverter(typeof(LineNumberInfoTypeConverter))]
    internal class LineNumberInfo
    {
        public LineNumberInfo(IXamlDomNode domNode)
        {
            this.StartLineNumber = domNode.StartLineNumber;
            this.StartLinePosition = domNode.StartLinePosition;
            this.EndLineNumber = domNode.EndLineNumber;
            this.EndLinePosition = domNode.EndLinePosition;
        }

        public int StartLineNumber { get; set; }
        public int StartLinePosition { get; set; }
        public int EndLineNumber { get; set; }
        public int EndLinePosition { get; set; }
    }
}