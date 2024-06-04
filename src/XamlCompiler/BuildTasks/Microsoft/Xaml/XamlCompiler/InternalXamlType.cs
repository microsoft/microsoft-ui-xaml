// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Markup;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using DirectUI;
    using XamlDom;

    [DebuggerDisplay("{StandardName} Sys={IsSystemType}")]
    class InternalTypeEntry
    {
        String _name;
        int _typeIndex;
        TypeForCodeGen _type;

        public InternalTypeEntry(XamlType xamlType)
        {
            UserTypeInfo = null;
            _typeIndex = -1;
            _type = new TypeForCodeGen(xamlType);
            var duiXamlType = xamlType as DirectUIXamlType;
            if (duiXamlType != null)
            {
                IsValueType = duiXamlType.IsValueType;
            }
        }

        public Type UnderlyingType { get { return _type.UnderlyingType; } }
        public String SystemName { get { return _type.SystemName; } }
        public String StandardName { get { return _type.StandardName; } }
        public LanguageSpecificString FullName { get { return _type.FullName; } }
        public string MemberFriendlyName { get { return _type.MemberFriendlyName; } }
        public InternalXamlUserTypeInfo UserTypeInfo { set; get; }

        public bool IsSystemType
        {
            get { return (UserTypeInfo == null); }
        }

        public int TypeIndex
        {
            get { Debug.Assert(_typeIndex != -1); return _typeIndex; }
            // Assert that the TypeIndex is set only once.
            set { Debug.Assert(_typeIndex == -1); _typeIndex = value; }
        }

        public bool IsValueType { get; set; }

        public String RefHat
        {
            get { return IsValueType ? String.Empty : "^"; }
        }

        public String Name
        {
            get
            {
                if (_name == null)
                {
                    int idx = StandardName.IndexOf('`');
                    _name = (idx == -1) ? StandardName : StandardName.Substring(0, idx);

                    idx = _name.LastIndexOf('.');
                    if (idx != -1)
                        _name = _name.Substring(idx + 1);
                }
                return _name;
            }
        }

        public bool IsDeprecated { get { return (UserTypeInfo == null) ? false : UserTypeInfo.IsDeprecated; } }

        public bool IsHandledByOtherProviders { get; set; }
    }

    [DebuggerDisplay("{StandardName}")]
    [ContentProperty("Members")]
    class InternalXamlUserTypeInfo
    {
        List<InternalXamlUserMemberInfo> _members;
        List<String> _enumValues;
        XamlSchemaCodeInfo _schemaInfo;
        InternalTypeEntry _simpleTypeEntry;

        public InternalXamlUserTypeInfo(InternalTypeEntry entry, XamlSchemaCodeInfo schemaInfo)
        {
            _simpleTypeEntry = entry;
            _members = new List<InternalXamlUserMemberInfo>();
            _schemaInfo = schemaInfo;
        }

        public void Init(XamlType xamlType)
        {
            IsArray = xamlType.IsArray;
            IsCollection = xamlType.IsCollection;
            IsDictionary = xamlType.IsDictionary;
            IsConstructible = xamlType.IsConstructible;
            IsMarkupExtension = xamlType.IsMarkupExtension;

            // Base Interfaces (IEnumerable) have no base class
            if (xamlType.BaseType != null)
            {
                BaseType = _schemaInfo.AddType(xamlType.BaseType);
            }

            if (xamlType.ContentProperty != null)
            {
                // Add the Content property member, but ensure the declaring type (which is really this type) is added as a return type stub
                // so we don't accidentally un-stub this type when AddMember() looks at its declaring type
                ContentProperty = _schemaInfo.AddMember(this._simpleTypeEntry, xamlType.ContentProperty, true);
            }

            if (xamlType.ItemType != null)
            {
                ItemType = _schemaInfo.AddTypeAndProperties(xamlType.ItemType);
            }

            if (xamlType.KeyType != null)
            {
                KeyType = _schemaInfo.AddTypeAndProperties(xamlType.KeyType);
            }
            if (xamlType.IsCollection || xamlType.IsDictionary)
            {
                AddMethodName = "Add";
                var duiType = xamlType as DirectUIXamlType;
                if (duiType != null && !String.IsNullOrEmpty(duiType.AddMethodName))
                {
                    AddMethodName = duiType.AddMethodName;
                }
            }

            DirectUIXamlType duiXamlType = xamlType as DirectUIXamlType;

            // Check if the type represents a nullable/boxed type
            Type boxedType;
            IsDeprecated = (duiXamlType == null) ? false : duiXamlType.IsDeprecated;

            if (duiXamlType != null && duiXamlType.IsNullableGeneric(out boxedType))
            {
                BoxedType = _schemaInfo.AddType(duiXamlType.SchemaContext.GetXamlType(boxedType));
            }

            if (xamlType.UnderlyingType.IsEnum)
            {
                Type enumType = xamlType.UnderlyingType;
                foreach (String enumValue in Enum.GetNames(enumType))
                {
                    EnumValues.Add(enumValue);
                }
            }

            IsLocalType = DomHelper.IsLocalType(xamlType);
            CreateFromStringMethod = xamlType.GetCreateFromStringMethod();
            HasCreateFromStringMethod = xamlType.HasCreateFromStringMethod();
        }

        public InternalTypeEntry BaseType { get; set; }
        public InternalTypeEntry BoxedType { get; set; }
        public InternalXamlUserMemberInfo ContentProperty { get; set; }
        public InternalTypeEntry TypeEntry { get { return _simpleTypeEntry; } }
        public String Name { get { return _simpleTypeEntry.Name; } }
        public int TypeIndex { get { return _simpleTypeEntry.TypeIndex; } }
        public bool IsReturnTypeStub { get; set; }

        public String SystemName { get { return _simpleTypeEntry.SystemName; } }
        public String StandardName { get { return _simpleTypeEntry.StandardName; } }
        public LanguageSpecificString FullName { get { return _simpleTypeEntry.FullName; } }
        public String MemberFriendlyName { get { return _simpleTypeEntry.MemberFriendlyName; } }
        public CreateFromStringMethod CreateFromStringMethod { get; set; }
        public bool HasCreateFromStringMethod { get; set; }
        public bool IsArray { get; set; }
        public bool IsCollection { get; set; }
        public bool IsConstructible { get; set; }
        public bool IsDictionary { get; set; }
        public bool IsMarkupExtension { get; set; }
        public bool IsBindable { get; set; }
        public bool IsLocalType { get; set; }
        public bool IsDeprecated { get; set; }
        public InternalTypeEntry ItemType { get; set; }
        public InternalTypeEntry KeyType { get; set; }
        public String AddMethodName { get; set; }

        public List<InternalXamlUserMemberInfo> Members
        {
            get { return _members; }
        }

        public void AddMember(InternalXamlUserMemberInfo userMember, InternalTypeEntry declaringType, XamlSchemaCodeInfo schemaInfo)
        {
            Debug.Assert(!String.IsNullOrWhiteSpace(userMember.Name) && userMember.DeclaringType != null);

            InternalXamlUserMemberInfo memberInfo;
            if (!TryFindMember(userMember.Name, declaringType, out memberInfo))
            {
                _members.Add(userMember);
            }
        }

        public bool HasMembers
        {
            get { return _members.Count > 0; }
        }

        public List<String> EnumValues
        {
            get
            {
                if (_enumValues == null)
                {
                    _enumValues = new List<string>();
                }
                return _enumValues;
            }
        }

        public bool HasEnumValues
        {
            get { return _enumValues != null && _enumValues.Count > 0; }
        }

        private bool TryFindIndexOfMember(String name, InternalTypeEntry declaringType, out int idx)
        {
            for (int i = 0; i < _members.Count; i++)
            {
                InternalXamlUserMemberInfo member = _members[i];
                if (member.Name == name && member.DeclaringType == declaringType)
                {
                    idx = i;
                    return true;
                }
            }
            idx = -1;
            return false;
        }

        private bool TryFindMember(string name, InternalTypeEntry declaringType, out InternalXamlUserMemberInfo member)
        {
            int idx;

            if (TryFindIndexOfMember(name, declaringType, out idx))
            {
                member = _members[idx];
                return true;
            }
            member = null;
            return false;
        }
    }

    [DebuggerDisplay("{Type.Name}  {DeclaringType.Name}.{Name}")]
    class InternalXamlUserMemberInfo
    {
        public void Init(XamlMember xamlMember, XamlSchemaCodeInfo schemaInfo)
        {
            // Name and Declaring type must already be set, and correct.
            Debug.Assert(Name == xamlMember.Name
                      && DeclaringType == schemaInfo.AddType(xamlMember.DeclaringType));

            // We can't call AddTypeAndProperties() here because that would cause us to
            // codegen the properties of every type of every property, and every property
            // of all those types etc...   The closure could be very large (and likely useless).
            // Also not every type works well with XAML generated code definitions.
            //  But generating a Property-less type definition causes us to generate a "bad"
            // type, that will cause problem if the type is actually used in XAML (in a library).
            // The solution is: if the Type is a Stub then look in the Libraries for an
            // implementation that isn't a stub before returning the stub.
            Type = schemaInfo.AddReturnTypeStub(xamlMember.Type);
            IsAttachable = xamlMember.IsAttachable;
            IsEvent = xamlMember.IsEvent;
            DirectUIXamlMember duiMember = xamlMember as DirectUIXamlMember;

            if (IsEvent)
            {
                HasPublicGetter = false;
                HasPublicSetter = false;
            }
            else
            {
                HasPublicSetter = (duiMember == null) ? xamlMember.IsWritePublic : duiMember.HasPublicSetter;
                HasPublicGetter = (duiMember == null) ? xamlMember.IsReadPublic : duiMember.HasPublicGetter;
            }

            IsDependencyProperty = (duiMember == null) ? false : duiMember.IsDependencyProperty;

            DirectUIXamlType duiType = xamlMember.Type as DirectUIXamlType;

            IsValueType = (duiType == null) ? false : duiType.IsValueType;
            IsString = (duiType == null) ? false : duiType.IsString();
            IsSignedChar = (duiType == null) ? false : duiType.IsSignedChar;
            IsInvalidType = (duiType == null) ? false : duiType.IsInvalidType;
            IsEnum = (duiType == null) ? false : duiType.UnderlyingType.IsEnum;

            // A property will provoke a "deprecated" error message if:
            // - it is deprecated
            // - it's return type is deprecated
            // - it's declaring type is deprecated.
            IsDeprecated = ((duiMember == null) ? false : duiMember.IsDeprecated);
            IsDeprecated |= ((duiType == null) ? false : duiType.IsDeprecated);

            DirectUIXamlType duiDeclaringType = duiMember.DeclaringType as DirectUIXamlType;
            IsDeprecated |= ((duiDeclaringType == null) ? false : duiDeclaringType.IsDeprecated);

            if (xamlMember.IsAttachable && xamlMember.TargetType != null)
            {
                TargetType = schemaInfo.AddTypeAndProperties(xamlMember.TargetType);
            }
        }

        public String Name { get; set; }
        public InternalTypeEntry Type { get; set; }
        public bool IsValueType { get; set; }
        public bool IsString { get; set; }
        public bool IsSignedChar { get; set; }
        public bool IsInvalidType { get; set; }
        public bool IsEnum { get; set; }
        public bool IsDeprecated { get; set; }
        public InternalTypeEntry DeclaringType { get; set; }
        public bool IsDependencyProperty { get; set; }
        public bool HasPublicSetter { get; set; }
        public bool HasPublicGetter { get; set; }
        public bool IsAttachable { get; set; }
        public InternalTypeEntry TargetType { get; set; }
        public bool IsEvent { get; set; }
    }
}
