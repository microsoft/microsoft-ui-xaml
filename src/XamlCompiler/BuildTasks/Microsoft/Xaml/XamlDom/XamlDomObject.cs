// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Xaml;
    using System.Xaml.Schema;
    using DirectUI;
    using Properties;
    using Utilities;

    [DebuggerDisplay("<{Type.Name}>")]
    [System.Windows.Markup.ContentProperty("MemberNodes")]
    internal class XamlDomObject : XamlDomItem, System.Windows.Markup.IXamlTypeResolver, IXamlNamespaceResolver
    {
        private SealableNamespaceCollection namespaces;
        private XamlSchemaContext schemaContext;
        private XamlType type;
        private bool isGetObject;
        private Type unresolvedType;
        private XamlNodeCollection<XamlDomMember> memberNodes;
        
        public XamlDomObject(bool isGetObject, XamlType xamlType, string sourceFilePath)
            : base(sourceFilePath)
        {
            this.type = xamlType;
            this.isGetObject = isGetObject;
            this.schemaContext = xamlType?.SchemaContext;
            this.ApiInformation = (xamlType as DirectUIXamlType)?.ApiInformation;
        }

        public ApiInformation ApiInformation { get; }

        // The dom object for the x:Properties value.  Because of our syntax, this is originally
        // parsed as the value of the Content property, but we process it out during validation
        // and instead set it here.
        [DefaultValue(null)]
        public xPropertyInfo XPropertyInfo { get; set; }

        [DefaultValue(null)]
        public virtual XamlType Type
        {
            get { return this.type; }
            set
            {
                this.CheckSealed();
                this.type = value;
                this.SchemaContext = this.type.SchemaContext;
            }
        }

        [DefaultValue(false)]
        public virtual bool IsGetObject
        {
            get { return this.isGetObject; }
            set
            {
                this.CheckSealed();
                this.isGetObject = value;
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
        public virtual KeyedCollection<String, XamlDomNamespace> Namespaces
        {
            get
            {
                if (this.namespaces == null)
                {
                    this.namespaces = new SealableNamespaceCollection();

                    if (this.IsSealed)
                    {
                        this.namespaces.Seal();
                    }
                }
                return this.namespaces;
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
        public virtual IList<XamlDomMember> MemberNodes
        {
            get { return this.Internal_MemberNodes; }
        }

        private IList<XamlDomMember> Internal_MemberNodes
        {
            get
            {
                if (this.memberNodes == null)
                {
                    this.memberNodes = new XamlNodeCollection<XamlDomMember>(this);

                    if (this.IsSealed)
                    {
                        this.memberNodes.Seal();
                    }
                }
                return this.memberNodes;
            }
        }

        public virtual XamlSchemaContext SchemaContext
        {
            get { return this.schemaContext; }
            set
            {
                this.CheckSealed();

                if (this.Type != null && this.Type.SchemaContext != XamlLanguage.Array.SchemaContext && this.Type.SchemaContext != value)
                {
                    throw new InvalidOperationException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_TypeDifferentSchemas));
                }

                this.schemaContext = value;
                this.Resolve();
            }
        }
        
        public IEnumerable<XamlDomNamespace> AllXmlnsDefinitions
        {
            get
            {
                XamlDomObject current = this;
                while (current != null)
                {
                    foreach (XamlDomNamespace domNs in current.Namespaces)
                    {
                        yield return domNs;
                    }

                    current = (current.Parent == null) ? null : current.Parent.Parent;
                }
            }
        }

        public override void Seal()
        {
            base.Seal();

            if (this.memberNodes != null)
            {
                this.memberNodes.Seal();
            }

            if (this.namespaces != null)
            {
                this.namespaces.Seal();
            }
        }

        public bool HasMember(string instanceMember)
        {
            if (instanceMember == null)
            {
                throw new ArgumentNullException("instanceMember");
            }

            if (instanceMember.Contains("."))
            {
                throw new NotSupportedException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_UseHasAttachedMember));
            }

            return this.Logic_HasMember((XamlType)null, instanceMember);
        }

        public bool HasAttachableMember(XamlType declaringType, string attachableMember)
        {
            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            // If declaringXamlType was an unresolved "unknown" type then the xamlMember here will be null.  And possibly existant
            // attachable members will not be found here.  The caller should search the children by name
            return this.HasMember(declaringType.GetAttachableMember(attachableMember));
        }

        public bool HasAttachableMember(Type declaringType, string attachableMember)
        {
            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            return this.Logic_HasMember(declaringType, attachableMember);
        }

        public virtual bool HasMember(XamlMember xamlMember)
        {
            if (xamlMember == null)
            {
                throw new ArgumentNullException("xamlMember");
            }

            return this.GetMemberNode(xamlMember) != null;
        }

        public XamlDomMember GetMemberNode(string instanceMember)
        {
            if (instanceMember == null)
            {
                throw new ArgumentNullException("instanceMember");
            }

            if (instanceMember.Contains("."))
            {
                throw new NotSupportedException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_UseHasGetAttachedMember));
            }

            return this.Logic_GetMemberNode((XamlType)null, instanceMember);
        }

        public XamlDomMember GetAttachableMemberNode(XamlType declaringType, string attachableMember)
        {
            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            if (declaringType == null)
            {
                throw new ArgumentNullException("declaringType");
            }

            return this.Logic_GetMemberNode(declaringType, attachableMember);
        }

        public XamlDomMember GetAttachableMemberNode(Type declaringType, string attachableMember)
        {
            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            if (declaringType == null)
            {
                throw new ArgumentNullException("declaringType");
            }

            return this.Logic_GetMemberNode(declaringType, attachableMember);
        }

        public virtual XamlDomMember GetMemberNode(XamlMember xamlMember, bool allowPropertyAliasing = true)
        {
            if (xamlMember == null)
            {
                throw new ArgumentNullException("xamlMember");
            }

            XamlDirective directive = xamlMember as XamlDirective;
            XamlMember aliasedMember = null;
            if (directive != null && this.Type != null && allowPropertyAliasing)
            {
                aliasedMember = this.Type.GetAliasedProperty(directive);
            }

            if (this.memberNodes != null)
            {
                foreach (XamlDomMember memberNode in this.memberNodes)
                {
                    if (memberNode.Member == xamlMember || (aliasedMember != null && memberNode.Member == aliasedMember))
                    {
                        return memberNode;
                    }
                }
            }

            return null;
        }

        public void SetAttachableMemberValue(Type declaringType, string attachableMember, object value)
        {
            if (declaringType == null)
            {
                throw new ArgumentNullException("declaringType");
            }

            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            XamlType xamlType = this.SchemaContext.GetXamlType(declaringType);

            this.Logic_SetMember(xamlType, attachableMember, value);
        }

        public void SetAttachableMemberValue(XamlType declaringType, string attachableMember, object value)
        {
            if (declaringType == null)
            {
                throw new ArgumentNullException("declaringType");
            }

            if (attachableMember == null)
            {
                throw new ArgumentNullException("attachableMember");
            }

            this.Logic_SetMember(declaringType, attachableMember, value);
        }

        public void SetMemberValue(string instanceMember, object value)
        {
            if (instanceMember == null)
            {
                throw new ArgumentNullException("instanceMember");
            }

            this.Logic_SetMember((XamlType)null, instanceMember, value);
        }

        public virtual void SetMemberValue(XamlMember xamlMember, Object value)
        {
            if (xamlMember == null)
            {
                throw new ArgumentNullException("xamlMember");
            }

            XamlDomMember node = this.GetMemberNode(xamlMember);

            if (node == null)
            {
                node = new XamlDomMember(xamlMember, this.SourceFilePath);
                this.MemberNodes.Add(node);
            }

            node.Item = new XamlDomValue(value, this.SourceFilePath);
        }

        public virtual XamlDomMember RemoveMember(XamlMember xamlMember)
        {
            if (xamlMember == null)
            {
                throw new ArgumentNullException("xamlMember");
            }

            return this.RemoveMemberNode(this.GetMemberNode(xamlMember));
        }

        public virtual XamlDomMember RemoveMemberNode(XamlDomMember node)
        {
            if (node == null)
            {
                throw new ArgumentNullException("node");
            }

            if (this.memberNodes != null)
            {
                if (this.memberNodes.Remove(node))
                {
                    return node;
                }
            }
            return null;
        }

        public string ResolveXmlPrefix(string prefix)
        {
            foreach (XamlDomNamespace domNs in this.AllXmlnsDefinitions)
            {
                if (domNs.NamespaceDeclaration.Prefix == prefix)
                {
                    return domNs.NamespaceDeclaration.Namespace;
                }
            }

            return null;
        }

        public XamlTypeName ResolveXmlNameToTypeName(string xName)
        {
            string prefix;
            string typeName;

            XamlDomObject.SplitQualifiedName(xName, out prefix, out typeName);
            string xmlNs = this.ResolveXmlPrefix(prefix);
            return xmlNs == null ? null : new XamlTypeName(xmlNs, typeName);
        }

        public virtual XamlType ResolveXmlName(string xName)
        {
            string prefix;
            string typeName;

            XamlDomObject.SplitQualifiedName(xName, out prefix, out typeName);
            string xmlNs = this.ResolveXmlPrefix(prefix);

            return xmlNs == null ? null : this.SchemaContext.GetXamlType(new XamlTypeName(xmlNs, typeName));
        }

        public XamlMember ResolveMemberName(XamlType xamlTargetType, string longPropertyName)
        {
            XamlMember member = null;
            int dotIndex = longPropertyName.IndexOf('.');
            if (dotIndex == -1)
            {
                member = xamlTargetType.GetMember(longPropertyName);
            }
            else
            {
                string typeXmlName = longPropertyName.Substring(0, dotIndex);
                XamlType memberType = this.ResolveXmlName(typeXmlName);
                if (memberType != null)
                {
                    string shortPropertyName = longPropertyName.Substring(dotIndex + 1);
                    member = this.ResolveMemberName(xamlTargetType, memberType, shortPropertyName);
                }
            }

            return member;
        }

        public XamlMember ResolveMemberName(string longPropertyName)
        {
            int dotIndex = longPropertyName.IndexOf('.');

            if (dotIndex == -1)
            {
                // Must have a '.'
                throw new ArgumentOutOfRangeException(longPropertyName);
            }

            string memberName = longPropertyName.Substring(dotIndex + 1);
            string typeXmlName = longPropertyName.Substring(0, dotIndex);
            XamlType memberType = this.ResolveXmlName(typeXmlName);

            return this.ResolveMemberName(memberType, memberType, memberName);
        }

        /// <summary>
        /// Resolve the member name
        /// </summary>
        /// <param name="xamlTargetType">This is the type of the Element tag the property is associated with.</param>
        /// <param name="memberType">This is the type of the first half of the '.' syntax.  Ie: MemberType.PropName</param>
        /// <param name="shortPropertyName">Name of the Property, no '.'s</param>
        /// <returns></returns>
        public XamlMember ResolveMemberName(XamlType xamlTargetType, XamlType memberType, String shortPropertyName)
        {
            XamlMember member = null;
            if (xamlTargetType.CanAssignTo(memberType))
            {
                member = memberType.GetMember(shortPropertyName);
            }

            // The property might be attachable even if the types are assignable for example: Grid.Row on Grid
            if (member == null)
            {
                member = memberType.GetAttachableMember(shortPropertyName);
            }

            return member;
        }

        public virtual Type Resolve(string qualifiedTypeName)
        {
            string xmlNs;
            string typeName;

            XamlDomObject.SplitQualifiedName(qualifiedTypeName, out xmlNs, out typeName);

            XamlType referencedXamlType = this.SchemaContext.GetXamlType(new XamlTypeName(xmlNs, typeName));

            return referencedXamlType != null ? referencedXamlType.UnderlyingType : null;
        }

        public virtual string GetNamespace(string prefix)
        {
            if (this.namespaces != null && this.namespaces.Contains(prefix))
            {
                return this.namespaces[prefix].NamespaceDeclaration.Namespace;
            }
            else
            {
                return this.Parent == null ? null : this.Parent.LookupNamespaceByPrefix(prefix);
            }
        }

        public IEnumerable<NamespaceDeclaration> GetNamespacePrefixes()
        {
            XamlDomObject objectNode = this;
            List<string> prefixes = new List<string>();

            while (objectNode != null)
            {
                if (objectNode.namespaces != null)
                {
                    foreach (XamlDomNamespace nsNode in objectNode.Namespaces)
                    {
                        if (!prefixes.Contains(nsNode.NamespaceDeclaration.Prefix))
                        {
                            prefixes.Add(nsNode.NamespaceDeclaration.Prefix);
                            yield return nsNode.NamespaceDeclaration;
                        }
                    }
                }

                if (objectNode.Parent != null)
                {
                    objectNode = objectNode.Parent.Parent;
                }
                else
                {
                    // If we don't have a parent member, then set objectNode to null
                    objectNode = null;
                }
            }
        }

        // This is called from constructors so it cannot call the virtual getters for SchemaContext, Items and Member
        internal void Resolve()
        {
            if (this.schemaContext == null && this.Parent != null && this.Parent.SchemaContext != null)
            {
                this.schemaContext = this.Parent.SchemaContext;
            }

            Debug.Assert(this.schemaContext != null);

            if (this.type == null && this.unresolvedType != null)
            {
                this.type = this.schemaContext.GetXamlType(this.unresolvedType);
                this.unresolvedType = null;
            }

            foreach (XamlDomMember memberNode in this.Internal_MemberNodes)
            {
                memberNode.Resolve();
            }
        }

        private XamlMember ResolveXamlMember(XamlType declaringType, string member)
        {
            if (declaringType != null)
            {
                return declaringType.GetAttachableMember(member);
            }

            if (!this.IsGetObject)
            {
                return this.Type.GetMember(member);
            }
            else
            {
                if (this.Parent != null)
                {
                    this.Parent.Member.Type.GetMember(member);
                }
            }

            return null;
        }

        private XamlDomMember Logic_GetMemberNode(Type declaringType, string member)
        {
            return this.Logic_GetMemberNode(declaringType != null ? this.SchemaContext.GetXamlType(declaringType) : null, member);
        }

        private XamlDomMember Logic_GetMemberNode(XamlType declaringXamlType, string member)
        {
            XamlMember xamlMember = this.ResolveXamlMember(declaringXamlType, member);
            if (xamlMember == null)
            {
                // If declaringXamlType was an unresolved "unknown" type then the xamlMember here will be null.
                // And possibly existant attachable members will not be found here.
                // The caller should search the children by name.
                return null;
            }

            return this.GetMemberNode(xamlMember);
        }

        private bool Logic_HasMember(Type declaringType, string member)
        {
            return this.Logic_HasMember(declaringType != null ? this.SchemaContext.GetXamlType(declaringType) : null, member);
        }

        private bool Logic_HasMember(XamlType declaringXamlType, string member)
        {
            XamlMember xamlMember = this.ResolveXamlMember(declaringXamlType, member);

            if (xamlMember == null)
            {
                // If declaringXamlType was an unresolved "unknown" type then the xamlMember here will be null.
                // And possibly existant attachable members will not be found here.
                // The caller should search the children by name.
                return false;
            }
            return this.HasMember(xamlMember);
        }

        private void Logic_SetMember(XamlType declaringXamlType, string member, object value)
        {
            XamlMember xamlMember = this.ResolveXamlMember(declaringXamlType, member);
            if (xamlMember == null)
            {
                // If declaringXamlType was an unresolved "unknown" type then the xamlMember here will be null
                return;
            }

            this.SetMemberValue(xamlMember, value);
        }

        private static void SplitQualifiedName(string qualifiedName, out string prefix, out string name)
        {
            prefix = string.Empty;
            name = qualifiedName;

            int colonIdx = qualifiedName.IndexOf(':');
            if (colonIdx != -1)
            {
                prefix = qualifiedName.Substring(0, colonIdx);
                name = qualifiedName.Substring(colonIdx + 1);
            }
        }
    }
}