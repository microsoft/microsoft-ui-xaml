// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    internal class XamlDomWriter : XamlWriter, IXamlLineInfoConsumer
    {
        string _sourceFilePath;

        public XamlDomWriter(XamlSchemaContext schemaContext, string sourceFilePath)
        {
            _schemaContext = schemaContext;
            _sourceFilePath = sourceFilePath;
        }

        Stack<XamlDomNode> writerStack = new Stack<XamlDomNode>();

        #region XamlWriter Members
        public XamlDomNode RootNode { get; set; }
        XamlSchemaContext _schemaContext;

        public override void WriteGetObject()
        {
            WriteObject(null, true);
        }

        public override void WriteStartObject(XamlType type)
        {
            WriteObject(type, false);
        }

        void WriteObject(XamlType xamlType, bool isGetObject)
        {
            XamlDomObject objectNode = new XamlDomObject(isGetObject, xamlType, _sourceFilePath);
            objectNode.IsGetObject = isGetObject;
            objectNode.StartLinePosition = _linePosition;
            objectNode.StartLineNumber = _lineNumber;
            // If it's a GetObject or a Directive, we need to store the actual XamlSchemaContext
            if (objectNode.IsGetObject ||objectNode.Type.SchemaContext == XamlLanguage.Object.SchemaContext)
            {
                objectNode.SchemaContext = SchemaContext;
            }

            if (_namespaces != null)
            {
                foreach (XamlDomNamespace xdns in _namespaces)
                {
                    objectNode.Namespaces.Add(xdns);
                }

                _namespaces.Clear();
            }

            // If Root Node is null then this is the root node.
            // If Root Node is not null, then add this to the parent member.

            if (RootNode == null)
            {
                RootNode = objectNode;
            }
            else
            {
                XamlDomMember propertyNode = (XamlDomMember)writerStack.Peek();
                propertyNode.Items.Add(objectNode);
                objectNode.Parent = propertyNode;
                if (isGetObject)
                {
                    objectNode.Type = propertyNode.Member.Type;
                }
            }
            writerStack.Push(objectNode);
        }

        public override void WriteEndObject()
        {
            Debug.Assert(CurrentStackNode is XamlDomObject);
            CurrentStackNode.EndLineNumber = _lineNumber;
            CurrentStackNode.EndLinePosition = _linePosition;
            writerStack.Pop();
        }

        public override void WriteStartMember(XamlMember xamlMember)
        {
            XamlDomMember propertyNode = new XamlDomMember(xamlMember, _sourceFilePath);

            // Only need to set the SchemaContext if it's a XamlDirective
            if (xamlMember.IsDirective)
            {
                propertyNode.SchemaContext = SchemaContext;
            }

            if (RootNode != null)
            {

                XamlDomObject objectNode = (XamlDomObject)writerStack.Peek();

                objectNode.MemberNodes.Add(propertyNode);
            }
            else
            {
                RootNode = propertyNode;
            }
            propertyNode.StartLineNumber = _lineNumber;
            propertyNode.StartLinePosition = _linePosition;

            writerStack.Push(propertyNode);
        }

        public override void WriteEndMember()
        {
            Debug.Assert(CurrentStackNode is XamlDomMember);
            CurrentStackNode.EndLineNumber = _lineNumber;
            CurrentStackNode.EndLinePosition = _linePosition;
            writerStack.Pop();
        }

        public override void WriteValue(object value)
        {
            XamlDomValue valueNode = new XamlDomValue(_sourceFilePath);
            valueNode.Value = value;

            if (RootNode != null)
            {
                //text should always be inside of a property...
                XamlDomMember propertyNode = (XamlDomMember)writerStack.Peek();
                propertyNode.Items.Add(valueNode);
            }
            else
            {
                RootNode = valueNode;
            }

            valueNode.StartLineNumber = _lineNumber;
            valueNode.StartLinePosition = _linePosition;
            valueNode.EndLineNumber = _lineNumber;
            valueNode.EndLinePosition = _linePosition;
        }

        public override void WriteNamespace(NamespaceDeclaration namespaceDeclaration)
        {
            XamlDomNamespace nsNode;
            if (_namespaces == null)
            {
                _namespaces = new List<XamlDomNamespace>();
            }

            nsNode = new XamlDomNamespace(namespaceDeclaration, _sourceFilePath);

            nsNode.StartLineNumber = _lineNumber;
            nsNode.StartLinePosition = _linePosition;
            nsNode.EndLineNumber = _lineNumber;
            nsNode.EndLinePosition = _linePosition;

            _namespaces.Add(nsNode);
        }

        public override XamlSchemaContext SchemaContext
        {
            get
            {
                return _schemaContext;
            }
        }

        #endregion

        #region IXamlLineInfoConsumer Members

        void IXamlLineInfoConsumer.SetLineInfo(int lineNumber, int linePosition)
        {
            _lineNumber = lineNumber;
            _linePosition = linePosition;
        }

        bool IXamlLineInfoConsumer.ShouldProvideLineInfo
        {
            get { return true; }
        }

        #endregion

        private XamlDomNode CurrentStackNode
        {
            get
            {
                if (writerStack.Count > 0)
                {
                    return writerStack.Peek();
                }
                else
                {
                    return null;
                }
            }
        }

        private int _lineNumber;
        private int _linePosition;
        private List<XamlDomNamespace> _namespaces;

    }
}
