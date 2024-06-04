// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using XamlDom;

    internal class StrippableNamespace : ILineNumberAndErrorInfo
    {
        public LineNumberInfo LineNumberInfo { get; set; }

        // For namespaces with a non-UWP plaform conditional, we need to strip the entire namespace declaration before passing it off to UWP GenXBF.
        // If the namespace is platform-conditional for UWP, we only need to strip the platform-conditional part of the namespace.
        public bool StripWholeNamespace { get; }

        public StrippableNamespace(XamlDomNamespace nameSpace, bool stripWholeNamespace)
        {
            this.LineNumberInfo = new LineNumberInfo(nameSpace);
            StripWholeNamespace = stripWholeNamespace;
        }

        public XamlCompileError GetAttributeProcessingError()
        {
            return new XamlRewriterErrorDataTypeLongForm(this.LineNumberInfo.StartLineNumber, this.LineNumberInfo.StartLinePosition);
        }
    }
}
