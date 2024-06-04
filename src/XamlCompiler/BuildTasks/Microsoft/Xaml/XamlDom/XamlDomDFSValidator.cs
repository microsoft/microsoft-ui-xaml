// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using DirectUI;

    internal class XamlDomDFSValidator
    {
        private XamlDomObject _owner;
        private DirectUISchemaContext _schema;

        internal XamlDomDFSValidator(XamlDomObject owner, DirectUISchemaContext schema)
        {
            if (owner == null)
            {
                throw new ArgumentNullException("owner");
            }

            _owner = owner;
            _schema = schema;
        }

        internal virtual List<XamlCompileError> EnsureNoXBindIsUsedInsideAStyle()
        {
            Stack<XamlDomNode> nodes = new Stack<XamlDomNode>();
            Stack<int> styleStartDepths = new Stack<int>();

            nodes.Push(_owner);

            List<XamlCompileError> ret = new List<XamlCompileError>();

            while (nodes.Count > 0)
            {
                var node = nodes.Pop();
                XamlDomObject domObject = node as XamlDomObject;
                XamlDomMember domMember = node as XamlDomMember;

                if (domObject != null)
                {
                    if (_schema.DirectUISystem.Style.IsAssignableFrom(domObject.Type.UnderlyingType))
                    {
                        // we are entering a style tag.
                        styleStartDepths.Push(nodes.Count);
                    }
                    else if (_schema.DirectUIXamlLanguage.BindExtension == domObject.Type && styleStartDepths.Count > 0)
                    {
                        ret.Add(new XamlXBindUsedInStyleError(domObject));
                    }

                    foreach (var child in domObject.MemberNodes)
                    {
                        nodes.Push(child);
                    }
                }
                else if (domMember != null)
                {
                    foreach (var child in domMember.Items)
                    {
                        nodes.Push(child);
                    }
                }

                if (styleStartDepths.Count > 0 && styleStartDepths.Peek() == nodes.Count)
                {
                    // we are exiting a style tag.
                    styleStartDepths.Pop();
                }
            }

            return ret;
        }
    }
}
