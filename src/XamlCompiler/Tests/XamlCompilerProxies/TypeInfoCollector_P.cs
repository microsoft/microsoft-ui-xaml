// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

namespace Win8Xaml.CompilerProxies
{
    public class TypeInfoCollector
    {
        static ProxyHelper _typeInfoCollectorType;
        static MethodInfo _collectMethod;
        static MethodInfo _addMetadataAndBindableTypestMethod;
        static PropertyInfo _schemaInfoProperty;
        static PropertyInfo _rootLog;

        object _instance;

        static TypeInfoCollector()
        {
            _typeInfoCollectorType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.TypeInfoCollector");
            _collectMethod = _typeInfoCollectorType.GetMethod("Collect");
            _addMetadataAndBindableTypestMethod = _typeInfoCollectorType.GetMethod("AddMetadataAndBindableTypes");
            _schemaInfoProperty = _typeInfoCollectorType.GetProperty("SchemaInfo");
            _rootLog = _typeInfoCollectorType.GetProperty("RootLog");
        }

        public TypeInfoCollector(DirectUISchemaContext schema)
        {
            var targetPlatformType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Platform");
            Object[] args = new Object[] { schema.Instance, targetPlatformType.CreateInstance() };
            _instance = _typeInfoCollectorType.CreateInstance(args);
        }

        public void Collect(CompilerDomRootToken domRootToken)
        {
            Object[] args = new Object[] { domRootToken.Instance };
            Object inst = _collectMethod.Invoke(_instance, args);
            return;
        }

        public XamlSchemaCodeInfo SchemaInfo
        {
            get
            {
                object obj = _schemaInfoProperty.GetValue(_instance, null);
                XamlSchemaCodeInfo schemaInfo = new XamlSchemaCodeInfo(obj);
                return schemaInfo;
            }
        }

        public Roots RootLog
        {
            get
            {
                object obj = _rootLog.GetValue(_instance, null);
                Roots rootLog = new Roots(obj);
                return rootLog;
            }
        }

        public void AddMetadataAndBindableTypes(List<Assembly> loadedAssemblies, Assembly localAssembly)
        {
            Object[] args = new Object[] { loadedAssemblies, localAssembly };
            _addMetadataAndBindableTypestMethod.Invoke(_instance, args);
        }
    }
}
