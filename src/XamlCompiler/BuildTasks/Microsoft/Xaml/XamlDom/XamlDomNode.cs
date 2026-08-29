// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System;
    using Properties;
    using Utilities;

    internal abstract class XamlDomNode : IXamlDomNode
    {
        private bool isSealed;
        private int startLineNumber;
        private int startLinePosition;
        private int endLineNumber;
        private int endLinePosition;

        public string SourceFilePath { get; }

        public XamlDomNode(string sourceFilePath)
        {
            SourceFilePath = sourceFilePath;
        }

        public int StartLineNumber
        {
            get { return this.startLineNumber; }
            set { this.CheckSealed(); this.startLineNumber = value; }
        }

        public int StartLinePosition
        {
            get { return this.startLinePosition; }
            set { this.CheckSealed(); this.startLinePosition = value; }
        }

        public int EndLineNumber
        {
            get { return this.endLineNumber; }
            set { this.CheckSealed(); this.endLineNumber = value; }
        }

        public int EndLinePosition
        {
            get { return this.endLinePosition; }
            set { this.CheckSealed(); this.endLinePosition = value; }
        }

        public bool IsSealed { get { return this.isSealed; } }

        public virtual void Seal()
        {
            this.isSealed = true;
        }

        protected void CheckSealed()
        {
            if (this.IsSealed)
            {
                throw new InvalidOperationException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_SealedXamlDomNode));
            }
        }
    }
}