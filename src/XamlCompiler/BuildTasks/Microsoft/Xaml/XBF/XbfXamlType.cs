// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Runtime.InteropServices.Marshalling;
    using System.Xaml;
    using DirectUI;

    // A XAML type of XbfMetadataProvider
    [GeneratedComClass]
    internal sealed partial class XbfXamlType : IXbfType
    {
        private DirectUIXamlType _xamlType;
        private XbfMetadataProvider _xmp;
        private Dictionary<string, XbfXamlMember> _memberMap = new Dictionary<string, XbfXamlMember>();
        private bool? isBindable;
        private string _fullName;

        public XbfXamlType(DirectUIXamlType xamlType, XbfMetadataProvider xmp)
        {
            _xamlType = xamlType;
            _xmp = xmp;
        }

        public XbfXamlType BaseType => _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.BaseType);

        public XbfXamlMember ContentProperty
        {
            get
            {
                XamlMember cp = _xamlType.ContentProperty;
                if (cp == null)
                {
                    return null;
                }
                return GetMember(cp.Name);
            }
        }

        public string FullName
        {
            get
            {
                if (_fullName == null)
                {
                    _fullName = XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, XamlSchemaCodeInfo.SetAirityOnGenericTypeNames);
                }
                return _fullName;
            }
        }

        public XbfXamlMember GetMember(string name)
        {
            XbfXamlMember mdpXamlMember;
            if (!_memberMap.TryGetValue(name, out mdpXamlMember))
            {
                XamlMember member = _xamlType.GetMember(name);
                if (member == null)
                {
                    member = _xamlType.GetAttachableMember(name);
                }

                // Only return the properties on this type as the declaring type.
                DirectUIXamlType declaringType = (DirectUIXamlType)member.DeclaringType;
                if (declaringType != _xamlType)
                {
                    member = null;
                }

                if (member != null)
                {
                    mdpXamlMember = new XbfXamlMember((DirectUIXamlMember)member, _xmp);
                    _memberMap.Add(name, mdpXamlMember);
                }
            }
            return mdpXamlMember;
        }

        public bool IsArray => _xamlType.IsArray;

        public bool IsBindable
        {
            get
            {
                if (!isBindable.HasValue)
                {
                    isBindable = HasBindableAttribute();
                }
                return isBindable.Value;
            }
        }

        public bool IsCollection => _xamlType.IsCollection;

        public bool IsConstructible => _xamlType.IsConstructible;

        public bool IsDictionary => _xamlType.IsDictionary;

        public bool IsMarkupExtension => _xamlType.IsMarkupExtension;

        public XbfXamlType ItemType => _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.ItemType);

        public XbfXamlType KeyType => _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.KeyType);

        private bool HasBindableAttribute()
        {
            Type type = _xamlType.UnderlyingType;
            foreach (CustomAttributeData attr in DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, KnownTypes.BindableAttribute))
            {
                return true;
            }
            return false;
        }

        IXbfType IXbfType.get_BaseType() => BaseType;

        IXbfMember IXbfType.get_ContentProperty() => ContentProperty;

        string IXbfType.get_FullName() => FullName;

        bool IXbfType.get_IsArray() => IsArray;

        bool IXbfType.get_IsCollection() => IsCollection;

        bool IXbfType.get_IsConstructible() => IsConstructible;

        bool IXbfType.get_IsDictionary() => IsDictionary;

        bool IXbfType.get_IsMarkupExtension() => IsMarkupExtension;

        bool IXbfType.get_IsBindable() => IsBindable;

        IXbfType IXbfType.get_ItemType() => ItemType;

        IXbfType IXbfType.get_KeyType() => KeyType;

        IXbfType IXbfType.get_BoxedType() => null;

        unsafe void IXbfType.get_UnderlyingType(XbfTypeName* value) => throw new NotImplementedException();

        IntPtr IXbfType.ActivateInstance() => throw new NotImplementedException();

        IntPtr IXbfType.CreateFromString(string value) => throw new NotImplementedException();

        IXbfMember IXbfType.GetMember(string name) => GetMember(name);

        void IXbfType.AddToVector(IntPtr instance, IntPtr value) => throw new NotImplementedException();

        void IXbfType.AddToMap(IntPtr instance, IntPtr key, IntPtr value) => throw new NotImplementedException();

        void IXbfType.RunInitializer() => throw new NotImplementedException();
    }
}