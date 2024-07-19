// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class EnumGenInfo
    {
        public TypeGenInfo TypeInfo { get; private set; }
        public String ValueName { get; private set; }

        public EnumGenInfo(TypeGenInfo typeInfo, string valueName)
        {
            this.TypeInfo = typeInfo;
            this.ValueName = valueName;
        }
    };

    internal class TypeGenInfo
    {
        public InternalTypeEntry TypeEntry { get; protected set; }
        public bool IncrementalTypeInfo { get; }
        public InternalXamlUserTypeInfo UserTypeInfo { protected get; set; }

        public TypeGenInfo(InternalTypeEntry typeEntry, bool incrementalTypeInfo)
        {
            this.IncrementalTypeInfo = incrementalTypeInfo;
            this.TypeEntry = typeEntry;
            this.UserTypeInfo = null;
        }

        public bool IsSystemType
        {
            get
            {
                return this.TypeEntry.IsSystemType;
            }
        }

        public String StandardName
        {
            get
            {
                return this.TypeEntry.StandardName;
            }
        }

        public String BaseTypeStandardName
        {
            get
            {
                return this.UserTypeInfo != null && this.UserTypeInfo.BaseType != null ? this.UserTypeInfo.BaseType.StandardName : "";
            }
        }

        public String BoxedTypeStandardName
        {
            get
            {
                return this.UserTypeInfo != null && this.UserTypeInfo.BoxedType != null ? this.UserTypeInfo.BoxedType.StandardName : "";
            }
        }

        public bool IsLocalType
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsLocalType : false;
            }
        }

        public bool IsBindable
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsBindable : false;
            }
        }

        public bool IsMarkupExtension
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsMarkupExtension : false;
            }
        }

        public bool HasCreateFromStringMethod
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.HasCreateFromStringMethod : false;
            }
        }

        public CreateFromStringMethod CreateFromStringMethod
        {
            get
            {
                return this.UserTypeInfo?.CreateFromStringMethod;
            }
        }

        public bool IsReturnTypeStub
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsReturnTypeStub : false;
            }
        }

        public bool IsConstructible
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsConstructible : false;
            }
        }

        public bool IsCollection
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsCollection : false;
            }
        }

        public bool IsDictionary
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsDictionary : false;
            }
        }

        public bool IsDeprecated
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.IsDeprecated : false;
            }
        }

        public bool HasEnumValues
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.HasEnumValues : false;
            }
        }

        public bool HasMembers
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.HasMembers : false;
            }
        }

        public LanguageSpecificString FullName
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.FullName : this.TypeEntry.FullName;
            }
        }

        public String ContentPropertyName
        {
            get
            {
                return this.UserTypeInfo != null && this.UserTypeInfo.ContentProperty != null ? String.Format("{0}.{1}", this.UserTypeInfo.ContentProperty.DeclaringType.StandardName, this.UserTypeInfo.ContentProperty.Name) : "";
            }
        }

        public InternalTypeEntry KeyType
        {
            get
            {
                return this.UserTypeInfo.KeyType;
            }
        }

        public InternalTypeEntry ItemType
        {
            get
            {
                return this.UserTypeInfo.ItemType;
            }
        }

        public IEnumerable<MemberGenInfo> Members
        {
            get
            {
                var members = this.UserTypeInfo != null ? this.UserTypeInfo.Members : new List<InternalXamlUserMemberInfo>();
                foreach(var member in members)
                {
                    yield return new MemberGenInfo(this, member);
                }
            }
        }

        public List<String> EnumValues
        {
            get
            {
                return this.UserTypeInfo != null ? this.UserTypeInfo.EnumValues : new List<String>();
            }
        }

        public bool HasActivator
        {
            get { return IsConstructible && !IsReturnTypeStub; }
        }

        public LanguageSpecificString ActivatorName
        {
            get
            {
                return IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"ActivateType_{StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"ActivateType<{FullName.CppCXName()}>",
                        () => IsLocalType ? $"ActivateLocalType<{FullName.CppWinRTName().ToLocalCppWinRTTypeName()}>" : $"ActivateType<{FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public LanguageSpecificString CollectionAddName
        {
            get
            {
                return IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"CollectionAdd_{StandardName.GetMemberFriendlyName()}_{ItemType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"CollectionAdd<{FullName.CppCXName()}, {ItemType.FullName.CppCXName()}{ItemType.RefHat}>",
                        () => $"CollectionAdd<{FullName.CppWinRTName()}, {ItemType.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public LanguageSpecificString DictionaryAddName
        {
            get
            {
                return IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"DictionaryAdd_{StandardName.GetMemberFriendlyName()}_{KeyType.StandardName.GetMemberFriendlyName()}_{ItemType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"DictionaryAdd<{FullName.CppCXName()}, {KeyType.FullName.CppCXName()}{KeyType.RefHat}, {ItemType.FullName.CppCXName()}{ItemType.RefHat}>",
                        () => $"DictionaryAdd<{FullName.CppWinRTName()}, {KeyType.FullName.CppWinRTName()}, {ItemType.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public LanguageSpecificString FromStringConverterName
        {
            get
            {
                return IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"FromStringConverter_{StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"FromStringConverter<{FullName.CppCXName()}>",
                        () => $"FromStringConverter<{FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }
    };

    internal class MemberGenInfo
    {
        private TypeGenInfo typeInfo;
        private InternalXamlUserMemberInfo memberInfo;

        public MemberGenInfo(TypeGenInfo typeInfo, InternalXamlUserMemberInfo memberInfo)
        {
            this.typeInfo = typeInfo;
            this.memberInfo = memberInfo;
        }

        public bool IsAttachable
        {
            get { return memberInfo.IsAttachable; }
        }

        public bool IsDeprecated
        {
            get { return memberInfo.IsDeprecated; }
        }

        public bool IsDependencyProperty
        {
            get { return memberInfo.IsDependencyProperty; }
        }

        public bool IsValueType
        {
            get { return memberInfo.IsValueType; }
        }

        public bool IsEnum
        {
            get { return memberInfo.IsEnum; }
        }

        public string Name
        {
            get { return memberInfo.Name; }
        }

        public bool HasPublicGetter
        {
            get { return memberInfo.HasPublicGetter; }
        }

        public bool HasPublicSetter
        {
            get { return memberInfo.HasPublicSetter; }
        }

        public InternalTypeEntry DeclaringType
        {
            get { return memberInfo.DeclaringType; }
        }

        public InternalTypeEntry TargetType
        {
            get { return memberInfo.TargetType; }
        }

        public InternalTypeEntry Type
        {
            get { return memberInfo.Type; }
        }

        public bool HasGetAttachableMember
        {
            get { return HasPublicGetter && IsAttachable; }
        }

        public LanguageSpecificString GetAttachableMemberName
        {
            get
            {
                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"GetAttachableMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}_{TargetType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"GetAttachableMember_{Name}<{DeclaringType.FullName.CppCXName()}, {TargetType.FullName.CppCXName()}>",
                        () => $"GetAttachableMember_{Name}<{DeclaringType.FullName.CppWinRTName()}, {TargetType.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasGetValueTypeMember
        {
            get { return HasPublicGetter && !IsAttachable && IsValueType; }
        }

        public LanguageSpecificString GetValueTypeMemberName
        {
            get
            {
                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"GetValueTypeMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"GetValueTypeMember_{Name}<{DeclaringType.FullName.CppCXName()}, {Type.FullName.CppCXName()}>",
                        () => $"GetValueTypeMember_{Name}<{DeclaringType.FullName.CppWinRTName()}, {Type.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasGetReferenceTypeMember
        {
            get { return HasPublicGetter && !IsAttachable && !IsValueType; }
        }

        public LanguageSpecificString GetReferenceTypeMemberName
        {
            get
            {
                // Uniquely qualify generated member/getter names in case multiple members have the same name,
                // but require different functionality.  For C++/WinRT string types, the implementation needs
                // to call ::winrt::Windows::Foundation::PropertyValue::CreateString when setting/getting
                // properties of type strings, which differs from the normal reference type case
                // of just casting with .As
                string memberName;
                if (memberInfo != null && memberInfo.IsString)
                {
                    memberName = "StringMember";
                }
                else
                {
                    memberName = "Member";
                }

                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"GetReferenceTypeMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"GetReferenceTypeMember_{Name}<{DeclaringType.FullName.CppCXName()}>",
                        () => $"GetReferenceType{memberName}_{Name}<{DeclaringType.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasSetAttachableMember
        {
            get { return HasPublicSetter && IsAttachable; }
        }

        public LanguageSpecificString SetAttachableMemberName
        {
            get
            {
                string hatNeeded = IsValueType ? String.Empty : "^";
                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"SetAttachableMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}_{TargetType.StandardName.GetMemberFriendlyName()}_{Type.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"SetAttachableMember_{Name}<{DeclaringType.FullName.CppCXName()}, {TargetType.FullName.CppCXName()}, {Type.FullName.CppCXName()}{hatNeeded}>",
                        () => $"SetAttachableMember_{Name}<{DeclaringType.FullName.CppWinRTName()}, {TargetType.FullName.CppWinRTName()}, {Type.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasSetValueTypeMember
        {
            get { return HasPublicSetter && !IsAttachable && IsValueType && !IsEnum; }
        }

        public LanguageSpecificString SetValueTypeMemberName
        {
            get
            {
                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"SetValueTypeMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"SetValueTypeMember_{Name}<{DeclaringType.FullName.CppCXName()}, {Type.FullName.CppCXName()}>",
                        () => $"SetValueTypeMember_{Name}<{DeclaringType.FullName.CppWinRTName()}, {Type.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasSetEnumMember
        {
            get { return HasPublicSetter && !IsAttachable && IsValueType && IsEnum; }
        }

        public LanguageSpecificString SetEnumMemberName
        {
            get
            {
                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"SetEnumMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}_{Type.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"SetEnumMember_{Name}<{DeclaringType.FullName.CppCXName()}, {Type.FullName.CppCXName()}>",
                        () => $"SetEnumMember_{Name}<{DeclaringType.FullName.CppWinRTName()}, {Type.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }

        public bool HasSetReferenceTypeMember
        {
            get { return HasPublicSetter && !IsAttachable && !IsValueType; }
        }

        public LanguageSpecificString SetReferenceTypeMemberName
        {
            get
            {
                // Uniquely qualify generated member/getter names in case multiple members have the same name,
                // but require different functionality.  For C++/WinRT string types, the implementation needs
                // to call ::winrt::Windows::Foundation::PropertyValue::CreateString when setting/getting
                // properties of type strings, which differs from the normal reference type case
                // of just casting with .As
                string memberName;
                if (memberInfo != null && memberInfo.IsString)
                {
                    memberName = "StringMember";
                }
                else
                {
                    memberName = "Member";
                }

                return typeInfo.IncrementalTypeInfo ?
                    new LanguageSpecificString(() => $"SetReferenceTypeMember_{Name.GetMemberFriendlyName()}_{DeclaringType.StandardName.GetMemberFriendlyName()}") :
                    new LanguageSpecificString(
                        () => $"SetReferenceTypeMember_{Name}<{DeclaringType.FullName.CppCXName()}, {Type.FullName.CppCXName()}>",
                        () => $"SetReferenceType{memberName}_{Name}<{DeclaringType.FullName.CppWinRTName()}, {Type.FullName.CppWinRTName()}>",
                        null,
                        null);
            }
        }
    };

}
