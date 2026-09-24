// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{

    public class XamlMember
    {
        static ProxyHelper _xamlMemberProxy;
        static PropertyInfo _typeProperty;
        static PropertyInfo _nameProperty;

        object _instance;

        static XamlMember()
        {
            _xamlMemberProxy = new ProxyHelper("System.Xaml.XamlMember");
            _typeProperty = _xamlMemberProxy.GetProperty("Type");
            _nameProperty = _xamlMemberProxy.GetProperty("Name");
        }

        public XamlMember(object instance)
        {
            _instance = instance;
        }

        public XamlType Type
        {
            get
            {
                return new XamlType(_typeProperty.GetValue(_instance));
            }
        }

        public string Name
        {
            get
            {
                return (string)_nameProperty.GetValue(_instance);
            }
        }
    }

}