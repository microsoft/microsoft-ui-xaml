// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.Build.Framework;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class DirectUISchemaContext
    {
        static ProxyHelper _directUiSchemaContextType;
        static MethodInfo _getXamlType1;
        static MethodInfo _getXamlType2;
        static MethodInfo _getAllXamlTypes;
        static PropertyInfo _schemaErrorsProperty;
        static PropertyInfo _directUISystemProperty;
        static PropertyInfo _typeResolver;

        object _instance;
        List<XamlCompileError> _errorList = new List<XamlCompileError>();

        static DirectUISchemaContext()
        {
            _directUiSchemaContextType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.DirectUI.DirectUISchemaContext");
            _getXamlType1 = _directUiSchemaContextType.GetMethod("GetXamlType", 1, new string[] { "System.Xaml.Schema.XamlTypeName" });
            _getXamlType2 = _directUiSchemaContextType.GetMethod("GetXamlType", 1, new Type[] { typeof(Type) });
            _getAllXamlTypes = _directUiSchemaContextType.GetMethod("GetAllXamlTypes");
            _schemaErrorsProperty = _directUiSchemaContextType.GetProperty("SchemaErrors", true);
            _directUISystemProperty = _directUiSchemaContextType.GetProperty("DirectUISystem", true);
            _typeResolver = _directUiSchemaContextType.GetProperty("TypeResolver", true);
        }

        public DirectUISchemaContext(IEnumerable<Assembly> refAssemblies, List<ITaskItem> systemExtraReferenceItems, Assembly localAssembly, ISet<string> staticLibraryAssemblies, bool isStringNullable)
        {
            try
            {
                object[] args = new object[] { refAssemblies, systemExtraReferenceItems, localAssembly, staticLibraryAssemblies, string.Empty, isStringNullable };
                _instance = _directUiSchemaContextType.CreateInstance(args);
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

        // A couple of extra convience properties cached on the client side
        List<Assembly> _userAssemblies = new List<Assembly>();
        public List<Assembly> UserAssemblies
        {
            get { return _userAssemblies; }
        }
        public Assembly LocalAssembly { get; set; }
        
        public XamlType GetXamlType(XamlTypeName xamlTypeName)
        {
            object[] args = new object[] { xamlTypeName.Instance };
            return new XamlType(_getXamlType1.Invoke(_instance, args));
        }

        public XamlType GetXamlType(Type type)
        {
            object[] args = new object[] { type };
            return new XamlType(_getXamlType2.Invoke(_instance, args));
        }

        public ICollection<XamlType> GetAllXamlTypes(string xamlNamespace)
        {
            object[] args = new object[] { xamlNamespace };
            IEnumerable objectList = (IEnumerable)_getAllXamlTypes.Invoke(_instance, args);
            List<XamlType> xamlTypes = new List<XamlType>();
            foreach (object type in objectList)
            {
                xamlTypes.Add(new XamlType(type));
            }
            return xamlTypes;
        }

        public List<XamlCompileError> SchemaErrors
        {
            get
            {
                IEnumerable listVal = (IEnumerable)_schemaErrorsProperty.GetValue(_instance, null);
                XamlDomValidator.ConvertListOfErrors(_errorList, listVal);
                return _errorList;
            }
        }

        public DirectUISystem DirectUISystem
        {
            get
            {
                Object directUiSystem = _directUISystemProperty.GetValue(_instance, null);
                return new DirectUISystem(directUiSystem);
            }
        }

        public TypeResolver TypeResolver
        {
            get
            {
                Object obj = _typeResolver.GetValue(_instance, null);
                return new TypeResolver(obj);
            }
            set
            {
                TypeResolver tr = value as TypeResolver;
                if (tr == null)
                    throw new Exception("expected TypeResolver Proxy");

                _typeResolver.SetValue(_instance, tr.Instance);
            }
        }
    }
}
