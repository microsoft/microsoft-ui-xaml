// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{

    public class DirectUIXamlType
    {
        static ProxyHelper _proxy;
        static PropertyInfo _propertyIsCodeGenType;

        object _instance;

        static DirectUIXamlType()
        {
            _proxy = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.DirectUIXamlType");
            _propertyIsCodeGenType = _proxy.GetProperty("IsCodeGenType");
        }

        public DirectUIXamlType(Type underlyingType, DirectUISchemaContext schemaContext)
        {
            try
            {
                Object[] args = new Object[] { underlyingType, schemaContext.Instance};
                _instance = _proxy.CreateInstance(args);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public object Instance
        {
            get { return _instance; }
        }

        public bool IsCodeGenType
        {
            get
            {
                return (bool)_propertyIsCodeGenType.GetValue(_instance);
            }
            set
            {
                _propertyIsCodeGenType.SetValue(_instance, value);
            }
        }
    }

}