// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;
    using XamlDom;

    internal class PhaseAssignment : ILineNumberAndErrorInfo, IComparable<PhaseAssignment>
    {
        public ConnectionIdElement ConnectionIdElement { get; private set; }
        public LineNumberInfo LineNumberInfo { get; set; }
        public int Phase { get; set; }

        public PhaseAssignment(XamlDomMember phaseMember, ConnectionIdElement connectionIdElement)
        {
            this.LineNumberInfo = new LineNumberInfo(phaseMember);

            this.ConnectionIdElement = connectionIdElement;

            string phase = DomHelper.GetStringValueOfProperty(phaseMember);
            int value = 0;
            Int32.TryParse(phase, out value);
            this.Phase = value;
        }

        public XamlCompileError GetAttributeProcessingError()
        {
            return new XamlRewriterErrorDataTypeLongForm(this.LineNumberInfo.StartLineNumber, this.LineNumberInfo.StartLinePosition);
        }

        public int CompareTo(PhaseAssignment other)
        {
            return this.Phase - other.Phase;
        }
    }
}

