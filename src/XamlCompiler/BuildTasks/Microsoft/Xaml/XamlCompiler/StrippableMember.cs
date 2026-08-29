// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using XamlDom;

    internal class StrippableMember : ILineNumberAndErrorInfo
    {
        public LineNumberInfo LineNumberInfo { get; set; }

        public StrippableMember(XamlDomMember dataTypeMember)
        {
            this.LineNumberInfo = new LineNumberInfo(dataTypeMember);
        }

        public XamlCompileError GetAttributeProcessingError()
        {
            return new XamlRewriterErrorDataTypeLongForm(this.LineNumberInfo.StartLineNumber, this.LineNumberInfo.StartLinePosition);
        }
    }
}
