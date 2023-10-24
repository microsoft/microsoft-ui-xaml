// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System;
    using System.Collections.Generic;
    using System.Xaml;

    internal class XamlDomReader : XamlReader, IXamlLineInfo
    {
        IEnumerator<XamlNode> nodes;
        XamlSchemaContext schemaContext;
        private bool doNotReorder;

        public XamlDomReader(IXamlDomNode domNode, XamlSchemaContext schemaContext)
            : this(domNode, schemaContext, null)
        {
        }

        public XamlDomReader(IXamlDomNode domNode, XamlSchemaContext schemaContext, XamlDomReaderSettings settings)
        {
            if (schemaContext == null)
            {
                throw new ArgumentNullException("schemaContext");
            }
            if (domNode == null)
            {
                throw new ArgumentNullException("domNode");
            }

            this.schemaContext = schemaContext;
            if (settings != null)
            {
                this.doNotReorder = settings.DoNotReorderMembers;
            }

            this.nodes = this.WalkDom(domNode).GetEnumerator();
        }

        public override bool IsEof { get { return this.NodeType != XamlNodeType.None; } }

        public override XamlMember Member { get { return this.NodeType == XamlNodeType.StartMember ? this.nodes.Current.Member : null; } }

        public override NamespaceDeclaration Namespace { get { return this.nodes.Current.Namespace; } }

        public override XamlNodeType NodeType { get { return this.nodes.Current.NodeType; } }

        public override XamlSchemaContext SchemaContext { get { return this.schemaContext; } }

        public override XamlType Type { get { return this.NodeType == XamlNodeType.StartObject ? this.nodes.Current.Type : null; } }

        public override object Value { get { return this.NodeType == XamlNodeType.Value ? this.nodes.Current.Value : null; } }

        bool IXamlLineInfo.HasLineInfo { get { return true; } }

        int IXamlLineInfo.LineNumber { get { return this.nodes.Current.LineNumber; } }

        int IXamlLineInfo.LinePosition { get { return this.nodes.Current.LinePosition; } }

        public override bool Read()
        {
            return this.nodes.MoveNext();
        }

        private IEnumerable<XamlNode> WalkDom(IXamlDomNode domNode)
        {
            XamlDomObject objectNode = domNode as XamlDomObject;
            if (objectNode != null)
            {
                foreach (XamlNode node in this.ReadObjectNode(objectNode))
                {
                    yield return node;
                }
            }
            else
            {
                XamlDomMember memberNode = domNode as XamlDomMember;
                if (memberNode != null)
                {
                    foreach (XamlNode node in this.ReadMemberNode(memberNode))
                    {
                        yield return node;
                    }
                }
                else
                {
                    foreach (XamlNode node in this.ReadValueNode(domNode as XamlDomValue))
                    {
                        yield return node;
                    }
                }
            }
        }

        private IEnumerable<XamlNode> ReadValueNode(XamlDomValue xamlDomValue)
        {
            yield return XamlNode.GetValue(xamlDomValue);
        }

        private IEnumerable<XamlNode> ReadMemberNode(XamlDomMember memberNode)
        {
            if (memberNode.Items == null || memberNode.Items.Count == 0)
            {
                yield break;
            }

            yield return XamlNode.GetStartMember(memberNode);

            foreach (XamlDomItem itemNode in memberNode.Items)
            {
                XamlDomObject objectNode = itemNode as XamlDomObject;

                IEnumerable<XamlNode> enumerable = objectNode != null ? this.ReadObjectNode(objectNode) : this.ReadValueNode(itemNode as XamlDomValue);

                foreach (XamlNode node in enumerable)
                {
                    yield return node;
                }
            }

            yield return XamlNode.GetEndMember(memberNode);

        }

        private IEnumerable<XamlNode> ReadObjectNode(XamlDomObject objectNode)
        {
            foreach (XamlDomNamespace nsNode in objectNode.Namespaces)
            {
                yield return XamlNode.GetNamespaceDeclaration(nsNode);
            }

            yield return XamlNode.GetStartObject(objectNode);

            // We want to write out simple things that could be attributes out first if setting is set
            // We write out single values and things that are MEs
            if (!this.doNotReorder)
            {
                foreach (XamlNode node in this.WritePossibleAttributes(objectNode))
                {
                    yield return node;
                }

                foreach (XamlNode node in this.WriteElementMembers(objectNode))
                {
                    yield return node;
                }
            }
            else
            {
                foreach (XamlDomMember memberNode in objectNode.MemberNodes)
                {
                    foreach (XamlNode node in this.ReadMemberNode(memberNode))
                    {
                        yield return node;
                    }
                }
            }

            yield return XamlNode.GetEndObject(objectNode);
        }

        private IEnumerable<XamlNode> WriteElementMembers(XamlDomObject objectNode)
        {
            foreach (XamlDomMember memberNode in objectNode.MemberNodes)
            {
                if (XamlDomReader.IsAttribute(memberNode))
                {
                    continue;
                }

                foreach (XamlNode node in this.ReadMemberNode(memberNode))
                {
                    yield return node;
                }
            }
        }

        private IEnumerable<XamlNode> WritePossibleAttributes(XamlDomObject objectNode)
        {
            foreach (XamlDomMember memberNode in objectNode.MemberNodes)
            {
                if (XamlDomReader.IsAttribute(memberNode))
                {
                    foreach (var node in this.ReadMemberNode(memberNode))
                    {
                        yield return node;
                    }
                }
            }
        }

        private static bool IsAttribute(XamlDomMember memberNode)
        {
            if (memberNode.Items.Count == 1)
            {
                if (memberNode.Item is XamlDomValue)
                {
                    return true;
                }
                else
                {
                    XamlType objectType = ((XamlDomObject)memberNode.Item).Type;
                    if (objectType != null && objectType.IsMarkupExtension)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        private class XamlNode
        {
            private static XamlNode _xamlNode = new XamlNode();

            public XamlType Type;
            public XamlMember Member;
            public NamespaceDeclaration Namespace;
            public XamlNodeType NodeType = XamlNodeType.None;
            public object Value;
            public int LineNumber;
            public int LinePosition;

            public void Clear()
            {
                Type = null;
                Member = null;
                Namespace = null;
                NodeType = XamlNodeType.None;
                Value = null;
                LineNumber = 0;
                LinePosition = 0;
            }

            public static XamlNode GetNamespaceDeclaration(XamlDomNamespace nsNode)
            {
                _xamlNode.Clear();
                _xamlNode.Namespace = nsNode.NamespaceDeclaration;
                _xamlNode.NodeType = XamlNodeType.NamespaceDeclaration;
                _xamlNode.LineNumber = nsNode.StartLineNumber;
                _xamlNode.LinePosition = nsNode.StartLinePosition;
                return _xamlNode;
            }

            public static XamlNode GetStartObject(XamlDomObject objectNode)
            {
                _xamlNode.Clear();
                if (objectNode.IsGetObject)
                {
                    _xamlNode.NodeType = XamlNodeType.GetObject;
                }
                else
                {
                    _xamlNode.NodeType = XamlNodeType.StartObject;
                    _xamlNode.Type = objectNode.Type;
                }
                _xamlNode.LineNumber = objectNode.StartLineNumber;
                _xamlNode.LinePosition = objectNode.StartLinePosition;
                return _xamlNode;
            }

            internal static XamlNode GetEndObject(XamlDomObject objectNode)
            {
                _xamlNode.Clear();
                _xamlNode.NodeType = XamlNodeType.EndObject;
                _xamlNode.LineNumber = objectNode.EndLineNumber;
                _xamlNode.LinePosition = objectNode.EndLinePosition;
                return _xamlNode;
            }

            internal static XamlNode GetStartMember(XamlDomMember memberNode)
            {
                _xamlNode.Clear();
                _xamlNode.NodeType = XamlNodeType.StartMember;
                _xamlNode.Member = memberNode.Member;
                _xamlNode.LineNumber = memberNode.StartLineNumber;
                _xamlNode.LinePosition = memberNode.StartLinePosition;
                return _xamlNode;
            }


            internal static XamlNode GetEndMember(XamlDomMember memberNode)
            {
                _xamlNode.Clear();
                _xamlNode.NodeType = XamlNodeType.EndMember;
                _xamlNode.LineNumber = memberNode.EndLineNumber;
                _xamlNode.LinePosition = memberNode.EndLinePosition;
                return _xamlNode;
            }

            internal static XamlNode GetValue(XamlDomValue XamlDomValue)
            {
                _xamlNode.Clear();
                _xamlNode.NodeType = XamlNodeType.Value;
                _xamlNode.Value = XamlDomValue.Value;
                _xamlNode.LineNumber = XamlDomValue.StartLineNumber;
                _xamlNode.LinePosition = XamlDomValue.StartLinePosition;
                return _xamlNode;
            }
        }
    }
}