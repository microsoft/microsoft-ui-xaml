// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using DirectUI;
    using XamlDom;

    internal class FieldDefinition
    {
        public String FieldName { get; set; }

        //For use in Pass1 these must be strings
        // because for local types we don't have a real 'Type'
        public String FieldTypePath { get; private set; }
        public String FieldTypeShortName { get; private set; }
        public String FieldTypeName
        {
            get { return FieldTypePath + "." + FieldTypeShortName; }
        }

        // This can be null in pass1 if is a 'local' type.
        // only for use in pass2.
        public XamlType FieldXamlType { get { return _xamlType; } }
        public TypeForCodeGen FieldType { get; set; }
        public bool IsValueType { get; private set; }
        public bool IsDeprecated { get; private set; }
        public LanguageSpecificString _fieldModifier;

        private XamlType _xamlType;

        private FieldDefinition() { }

        public FieldDefinition(XamlDomObject namedObject)
        {
            InitFromType((DirectUIXamlType)namedObject.Type);

            XamlDomMember nameMember = DomHelper.GetAliasedMemberNode(namedObject, XamlLanguage.Name);
            FieldName = DomHelper.GetStringValueOfProperty(nameMember);

            XamlDomMember fieldModifierMember = namedObject.GetMemberNode(XamlLanguage.FieldModifier);
            if (fieldModifierMember != null)
            {
                string modifier = DomHelper.GetStringValueOfProperty(fieldModifierMember);
                _fieldModifier = new LanguageSpecificString(
                    () => modifier.ToLower(),
                    () => modifier.ToLower(),
                    () => modifier.ToLower(),
                    () => modifier.ToTitleCase()
                    );
            }
        }

        private void InitFromType(DirectUIXamlType xamlType)
        {
            _xamlType = xamlType;
            FieldType = new TypeForCodeGen(xamlType);
            FieldTypePath = xamlType.UnderlyingType.Namespace;
            FieldTypeShortName = xamlType.UnderlyingType.Name;
            IsValueType = xamlType.IsValueType;
            IsDeprecated = xamlType.IsDeprecated;
        }

        // Contructor for Named element with a Local Type in Pass1
        //
        public FieldDefinition(XamlDomObject namedObject, string clrPath)
        {
            _xamlType = namedObject.Type;
            FieldType = null;
            FieldTypePath = clrPath;
            FieldTypeShortName = namedObject.Type.Name;

            XamlDomMember nameMember = DomHelper.GetAliasedMemberNode(namedObject, XamlLanguage.Name);
            FieldName = DomHelper.GetStringValueOfProperty(nameMember);

            XamlDomMember fieldModifierMember = namedObject.GetMemberNode(XamlLanguage.FieldModifier);

            if (fieldModifierMember != null)
            {
                string modifier = DomHelper.GetStringValueOfProperty(fieldModifierMember);
                _fieldModifier = new LanguageSpecificString(() => modifier);
            }
        }

        public LanguageSpecificString FieldModifier
        {
            get
            {
                if (_fieldModifier == null)
                {
                    _fieldModifier = new LanguageSpecificString(
                        () => "private",
                        () => "protected",
                        () => "private",
                        () => "private");
                }
                return _fieldModifier;
            }
            set
            {
                _fieldModifier = value;
            }
        }

        /// <summary>
        /// Compare two FieldDefinitions to see if they are identical and don't need merging.
        /// Does not look at line number infomation
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public bool HasSameAttributes(FieldDefinition that)
        {
            if (!CanBeMerged(that))
                return false;

            // if there is a FieldType then compare that.
            if (this.FieldType != null)
            {
                if (this.FieldType != that.FieldType)
                    return false;
            }
            else  // if there is no Field type (pass1 on a local type) then compare names.
            {
                if (this.FieldTypeName != that.FieldTypeName)
                    return false;
            }

            return true;
        }
        /// <summary>
        /// Compare two FieldDefinitions check for Irreconcilable differneces.
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public bool CanBeMerged(FieldDefinition that)
        {
            if (this.FieldName != that.FieldName)
                return false;

            if (this.FieldModifier != that.FieldModifier)
                return false;

            return true;
        }

        /// <summary>
        /// Create a new FieldDefinition with a best common base class.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns></returns>
        public static FieldDefinition CreateMerged(FieldDefinition a, FieldDefinition b)
        {
            FieldDefinition merged = new FieldDefinition();

            // Proper error for this should have been handleed by the caller.
            if(!a.CanBeMerged(b))
                throw new InvalidOperationException("Field definitions cannot be merged");

            merged.FieldName = a.FieldName;
            merged.FieldModifier = a.FieldModifier;

            // Cannot merge the linenumber information (irreconcilably different)
            // Merged FieldDefinitions are NOT used for code gen, just for reconciling the field types.
            if (a.FieldType == null || b.FieldType == null)
            {
                merged.FieldType = new TypeForCodeGen(a.DirectUIXamlLanguage.Object);
            }
            DirectUIXamlType baseXamlType = (DirectUIXamlType)FindCommonBaseClass(a._xamlType, b._xamlType);

            merged.InitFromType(baseXamlType);

            return merged;
        }

        private IDirectUIXamlLanguage DirectUIXamlLanguage
        {
            get
            {
                if (_xamlType == null)
                {
                    return null;
                }
                DirectUISchemaContext duiSchema = (DirectUISchemaContext)_xamlType.SchemaContext;
                return duiSchema.DirectUIXamlLanguage;
            }
        }

        private static XamlType FindCommonBaseClass(XamlType a, XamlType b)
        {
            if (a == b)
            {
                return a;
            }

            if (a.UnderlyingType.IsAssignableFrom(b.UnderlyingType))
            {
                return a;
            }

            if (b.UnderlyingType.IsAssignableFrom(a.UnderlyingType))
            {
                return b;
            }

            return FieldDefinition.FindCommonBaseClass(a.BaseType, b.BaseType);
        }

    }
}
