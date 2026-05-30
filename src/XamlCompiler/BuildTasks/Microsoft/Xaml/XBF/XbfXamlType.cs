// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Xaml;
    using DirectUI;

    internal class XbfXamlType : IXbfType
    {
        private DirectUIXamlType _xamlType;
        private IXmpXamlType _xmp;
        private Dictionary<string, XbfXamlMember> _memberMap = new Dictionary<string, XbfXamlMember>();
        private bool? isBindable;
        private string _fullName;

        public XbfXamlType(DirectUIXamlType xamlType, IXmpXamlType xmp)
        {
            _xamlType = xamlType;
            _xmp = xmp;
        }

        public IXbfType BaseType
        {
            get
            {
                return _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.BaseType);
            }
        }

        public IXbfMember ContentProperty
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
                    _fullName = XamlSchemaCodeInfo.GetFullGenericNestedName(UnderlyingType, XamlSchemaCodeInfo.SetAirityOnGenericTypeNames);
                }
                return _fullName;
            }
        }

        public IXbfMember GetMember(string name)
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

        public bool IsArray
        {
            get { return _xamlType.IsArray; }
        }

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

        public bool IsCollection
        {
            get { return _xamlType.IsCollection; }
        }

        public bool IsConstructible
        {
            get { return _xamlType.IsConstructible; }
        }

        public bool IsDictionary
        {
            get { return _xamlType.IsDictionary; }
        }

        public bool IsMarkupExtension
        {
            get { return _xamlType.IsMarkupExtension; }
        }

        public IXbfType ItemType
        {
            get
            {
                return _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.ItemType);
            }
        }

        public IXbfType KeyType
        {
            get
            {
                return _xmp.GetXmpXamlType((DirectUIXamlType)_xamlType.KeyType);
            }
        }

        public IXbfType BoxedType
        {
            get
            {
                return null;
            }
        }

        public Type UnderlyingType
        {
            get { return _xamlType.UnderlyingType; }
        }

        public object ActivateInstance()
        {
            throw new NotImplementedException();    // not used
        }

        public void AddToMap(object instance, object key, object value)
        {
            throw new NotImplementedException();    // not used
        }

        public void AddToVector(object instance, object value)
        {
            throw new NotImplementedException();    // not used
        }

        public object CreateFromString(string value)
        {
            throw new NotImplementedException();    // not used
        }

        public void RunInitializer()
        {
            throw new NotImplementedException();    // not used
        }

        private bool HasBindableAttribute()
        {
            Type type = _xamlType.UnderlyingType;
            foreach (CustomAttributeData attr in DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, KnownTypes.BindableAttribute))
            {
                return true;
            }
            return false;
        }
    }
}