// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <collection.h>

namespace XamlTypeInfo
{
    namespace InfoProvider
    {
        ref class XamlTypeInfoProvider sealed
        {
        internal:
            ::Microsoft::UI::Xaml::Markup::IXamlType^ CreateXamlType(::Platform::String^ typeName);
            ::Microsoft::UI::Xaml::Markup::IXamlMember^ CreateXamlMember(::Platform::String^ longMemberName);

            ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlTypeByName(::Platform::String^ typeName);
            ::Microsoft::UI::Xaml::Markup::IXamlType^ GetXamlTypeByType(::Windows::UI::Xaml::Interop::TypeName t);
            ::Microsoft::UI::Xaml::Markup::IXamlMember^ GetMemberByLongName(::Platform::String^ longMemberName);

        private:
            std::map<::Platform::String^, ::Platform::WeakReference> _xamlTypes;
            std::map<::Platform::String^, ::Microsoft::UI::Xaml::Markup::IXamlMember^> _xamlMembers;

        };

        ref class XamlSystemBaseType sealed : public ::Microsoft::UI::Xaml::Markup::IXamlType
        {
        internal:
            XamlSystemBaseType(::Platform::String^ name);

        public:
            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ BaseType 
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property Microsoft::UI::Xaml::Markup::IXamlMember^ ContentProperty 
            {
                ::Microsoft::UI::Xaml::Markup::IXamlMember^ get();
            }

            virtual property ::Platform::String^ FullName
            {
                ::Platform::String^ get();
            }

            virtual property ::Platform::String^ Name
            {
                ::Platform::String^ get();
            }

            virtual property bool IsArray
            {
                bool get();
            }

            virtual property bool IsCollection
            {
                bool get();
            }

            virtual property bool IsConstructible
            {
                bool get();
            }

            virtual property bool IsDictionary
            {
                bool get();
            }

            virtual property bool IsMarkupExtension
            {
                bool get();
            }

            virtual property bool IsEnum
            {
                bool get();
            }

            virtual property bool IsSystemType
            {
                bool get();
            }

            virtual property bool IsBindable
            {
                bool get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ ItemType
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ KeyType
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ BoxedType
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Windows::UI::Xaml::Interop::TypeName UnderlyingType
            {
                ::Windows::UI::Xaml::Interop::TypeName get();
            }

            virtual ::Platform::Object^ ActivateInstance();
            virtual ::Microsoft::UI::Xaml::Markup::IXamlMember^ GetMember(::Platform::String^ name);
            virtual void AddToVector(::Platform::Object^ instance, ::Platform::Object^ value);
            virtual void AddToMap(::Platform::Object^ instance, ::Platform::Object^ key, ::Platform::Object^ value);
            virtual void RunInitializer();
            virtual ::Platform::Object^ CreateFromString(::Platform::String^ value);

        private:
            ::Platform::String^ _fullName;
        };

        ref class XamlUserType sealed : public [::Platform::Metadata::RuntimeClassName] ::Microsoft::UI::Xaml::Markup::IXamlType
        {
        internal:
            XamlUserType(::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ provider, ::Platform::String^ fullName, ::Microsoft::UI::Xaml::Markup::IXamlType^ baseType);

        public:
            // --- Interface methods ----
            virtual property ::Platform::String^ FullName
            {
                ::Platform::String^ get();
            }

            virtual property ::Platform::String^ Name
            {
                ::Platform::String^ get();
            }

            virtual property ::Windows::UI::Xaml::Interop::TypeName UnderlyingType
            {
                ::Windows::UI::Xaml::Interop::TypeName get();
            }

            virtual property bool IsSystemType
            {
                bool get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ BaseType 
            { 
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property bool IsArray 
            { 
                bool get();
            }

            virtual property bool IsCollection 
            { 
                bool get();
            }

            virtual property bool IsConstructible 
            { 
                bool get();
            }

            virtual property bool IsDictionary 
            { 
                bool get();
            }

            virtual property bool IsMarkupExtension 
            { 
                bool get();
            }

            virtual property bool IsEnum 
            { 
                bool get();
            }

            virtual property bool IsBindable
            { 
                bool get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlMember^ ContentProperty 
            { 
                ::Microsoft::UI::Xaml::Markup::IXamlMember^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ ItemType 
            { 
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ KeyType 
            { 
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ BoxedType
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual ::Microsoft::UI::Xaml::Markup::IXamlMember^ GetMember(::Platform::String^ name);
            virtual ::Platform::Object^ ActivateInstance();
            virtual void AddToMap(::Platform::Object^ instance, ::Platform::Object^ key, ::Platform::Object^ value);
            virtual void AddToVector(::Platform::Object^ instance, ::Platform::Object^ value);
            virtual void RunInitializer();
            virtual ::Platform::Object^ CreateFromString(::Platform::String^ value);
            // --- End of Interface methods

        internal:
            typedef ::Platform::Object^ (*ActivatorFn)();
            typedef void (*AddToCollectionFn)(::Platform::Object^ instance, ::Platform::Object^ item);
            typedef void (*AddToDictionaryFn)(::Platform::Object^ instance, ::Platform::Object^ key, ::Platform::Object^ item);
            typedef ::Platform::Object^ (*StringConverterFn)(::XamlTypeInfo::InfoProvider::XamlUserType^ userType, ::Platform::String^ input);

            property ActivatorFn Activator;
            property AddToCollectionFn CollectionAdd;
            property AddToDictionaryFn DictionaryAdd;
            property ::Windows::UI::Xaml::Interop::TypeKind KindOfType;
            property StringConverterFn FromStringConverter;

            void SetContentPropertyName(::Platform::String^ contentPropertyName);
            void SetIsArray();
            void SetIsMarkupExtension();
            void SetIsEnum();
            void SetIsBindable();
            void SetItemTypeName(::Platform::String^ itemTypeName);
            void SetKeyTypeName(::Platform::String^ keyTypeName);
            void AddMemberName(::Platform::String^ shortName);
            void AddEnumValue(::Platform::String^ name, ::Platform::Object^ value);
            uint32 CreateEnumUIntFromString(::Platform::String^ input);

        private:
            ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ _provider;
            ::Microsoft::UI::Xaml::Markup::IXamlType^ _baseType;
            bool _isArray;
            bool _isConstructible;
            bool _isDictionary;
            bool _isMarkupExtension;
            bool _isEnum;
            bool _isBindable;

            ::Platform::String^ _contentPropertyName;
            ::Platform::String^ _itemTypeName;
            ::Platform::String^ _keyTypeName;
            ::Platform::String^ _fullName;
            std::map<::Platform::String^, ::Platform::String^> _memberNames;
            std::map<std::wstring, ::Platform::Object^> _enumValues;
        };

        ref class XamlMember sealed : public ::Microsoft::UI::Xaml::Markup::IXamlMember
        {
        internal:
            XamlMember(::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ provider, ::Platform::String^ name, ::Platform::String^ typeName);

            void SetIsAttachable();
            void SetIsDependencyProperty();
            void SetIsReadOnly();
            void SetTargetTypeName(::Platform::String^ targetTypeName);

            typedef ::Platform::Object^ (*PropertyGetterFn)(::Platform::Object^ instance);
            typedef void (*PropertySetterFn)(::Platform::Object^ instance, ::Platform::Object^ value);

            property PropertyGetterFn Getter;
            property PropertySetterFn Setter;

        public:
            virtual property bool IsAttachable
            { 
                bool get();
            }

            virtual property bool IsDependencyProperty 
            { 
                bool get();
            }

            virtual property bool IsReadOnly
            { 
                bool get();
            }

            virtual property ::Platform::String^ Name
            { 
                ::Platform::String^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ Type
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual property ::Microsoft::UI::Xaml::Markup::IXamlType^ TargetType
            {
                ::Microsoft::UI::Xaml::Markup::IXamlType^ get();
            }

            virtual ::Platform::Object^ GetValue(::Platform::Object^ instance);
            virtual void SetValue(::Platform::Object^ instance, ::Platform::Object^ value);

        private:
            bool _isAttachable;
            bool _isDependencyProperty;
            bool _isReadOnly; 
            ::Platform::String^ _name;
            ::Platform::String^ _targetTypeName;
            ::Platform::String^ _typeName;
            ::XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ _provider;
        };
    }
}

