// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Xaml;
    using System.Reflection.Adds;
    using DirectUI;
    using Properties;
    using Utilities;

    [DebuggerDisplay("{Member.Name}")]
    [System.Windows.Markup.ContentProperty("ItemNodes")]
    internal class XamlDomMember : XamlDomNode, IXamlDomMember
    {
        private XamlMember member;
        private XamlNodeCollection<XamlDomItem> items;
        private XamlDomObject parent;
        private XamlSchemaContext schemaContext;
        private string unresolvedMemberName;
        private Type unresolvedDeclaringType;

        public XamlDomMember(XamlMember xamlMember, string sourceFilePath)
            :base(sourceFilePath)
        {
            this.member = xamlMember;
            if (xamlMember != null)
            {
                try
                {
                    this.schemaContext = xamlMember.Type.SchemaContext;
                }
                catch (UnresolvedAssemblyException)
                {
                    this.schemaContext = xamlMember.DeclaringType.SchemaContext;
                }
                catch (TypeLoadException)
                {
                    this.schemaContext = xamlMember.DeclaringType.SchemaContext;
                }

                this.ApiInformation = (xamlMember as DirectUIXamlMember)?.ApiInformation;
            }
        }

        public ApiInformation ApiInformation { get; }
       
        public virtual XamlSchemaContext SchemaContext
        {
            get
            {
                return this.schemaContext;
            }
            set
            {
                this.CheckSealed();
                if (this.Member != null && !this.Member.IsDirective && this.Member.Type.SchemaContext != value)
                {
                    throw new InvalidOperationException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_MemberDifferentSchemas));
                }
                this.schemaContext = value;
            }
        }

        [DefaultValue(null)]
        public virtual XamlMember Member
        {
            get
            {
                return this.member;
            }
            set
            {
                this.CheckSealed();
                this.member = value;
                this.schemaContext = this.member.Type.SchemaContext;
                this.Resolve();
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public XamlDomObject Parent
        {
            get
            {
                return this.parent;
            }
            set
            {
                this.CheckSealed();
                this.parent = value;
            }
        }

        public virtual XamlDomItem Item
        {
            get
            {
                return this.Internal_Item;
            }
            set
            {
                this.Internal_Item = value;
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
        public virtual IList<XamlDomItem> Items
        {
            get
            {
                return this.Internal_Items;
            }
        }

        private XamlDomItem Internal_Item
        {
            get
            {
                if (this.Internal_Items.Count > 1)
                {
                    throw new NotSupportedException(ResourceUtilities.FormatString(XamlCompilerResources.XamlDom_MemberHasMoreThanOneItem, this.Member.Name));
                }
                if (this.Internal_Items.Count == 0)
                {
                    return null;
                }
                return this.Internal_Items[0];
            }
            set
            {
                this.Internal_Items.Clear();
                this.Internal_Items.Add(value);
            }
        }

        private IList<XamlDomItem> Internal_Items
        {
            get
            {
                if (this.items == null)
                {
                    this.items = new XamlNodeCollection<XamlDomItem>(this);
                    if (this.IsSealed)
                    {
                        this.items.Seal();
                    }
                }
                return this.items;
            }
        }

        public override void Seal()
        {
            base.Seal();
            if (this.items != null)
            {
                this.items.Seal();
            }
        }

        // This is called from constructors so it cannot call the virtual
        // getters for SchemaContext, Items and Member.
        internal void Resolve()
        {
            if (this.schemaContext == null && this.Parent != null && this.Parent.SchemaContext != null)
            {
                this.schemaContext = this.Parent.SchemaContext;
            }
            if (this.member == null && this.unresolvedMemberName != null)
            {
                if (this.unresolvedDeclaringType != null)
                {
                    this.member = this.schemaContext.GetXamlType(this.unresolvedDeclaringType).GetAttachableMember(this.unresolvedMemberName);
                }
                else
                {
                    this.member = this.Parent.Type.GetMember(this.unresolvedMemberName);
                }
                this.unresolvedMemberName = null;
                this.unresolvedDeclaringType = null;
            }

            foreach (XamlDomItem itemNode in this.Internal_Items)
            {
                XamlDomObject objNode = itemNode as XamlDomObject;
                if (objNode != null)
                {
                    objNode.Resolve();
                }
            }
        }

        internal string LookupNamespaceByPrefix(string prefix)
        {
            return this.Parent.GetNamespace(prefix);
        }
    }
}