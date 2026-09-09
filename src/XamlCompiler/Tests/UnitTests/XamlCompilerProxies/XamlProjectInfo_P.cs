// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Win8Xaml.CompilerProxies
{
    public class XamlProjectInfo
    {
        static ProxyHelper _xamlProjectInfoType;

        static PropertyInfo _rootNamespaceProperty;
        static PropertyInfo _projectNameProperty;
        static PropertyInfo _isLibraryProperty;
        static PropertyInfo _classToHeaderFileMapProperty;
        static PropertyInfo _genXbf32Path;
        static PropertyInfo _genXbf64Path;
        static PropertyInfo _genXbfArm64Path;
        static PropertyInfo _targetPlatformMinVersion;

        object _instance;

        static XamlProjectInfo()
        {
            _xamlProjectInfoType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlProjectInfo");
            _rootNamespaceProperty = _xamlProjectInfoType.GetProperty("RootNamespace");
            _projectNameProperty = _xamlProjectInfoType.GetProperty("ProjectName");
            _isLibraryProperty = _xamlProjectInfoType.GetProperty("IsLibrary");
            _classToHeaderFileMapProperty = _xamlProjectInfoType.GetProperty("ClassToHeaderFileMap");
            _genXbf32Path = _xamlProjectInfoType.GetProperty("GenXbf32Path");
            _genXbf64Path = _xamlProjectInfoType.GetProperty("GenXbf64Path");
            _genXbfArm64Path = _xamlProjectInfoType.GetProperty("GenXbfArm64Path");
            _targetPlatformMinVersion = _xamlProjectInfoType.GetProperty("TargetPlatformMinVersion");
        }

        public XamlProjectInfo()
        {
            _instance = _xamlProjectInfoType.CreateInstance();
        }

        public XamlProjectInfo(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public String GenXbf32Path
        {
            get { return (string)_genXbf32Path.GetValue(_instance, null); }
            set { _genXbf32Path.SetValue(_instance, value); }
        }

        public String GenXbf64Path
        {
            get { return (string)_genXbf64Path.GetValue(_instance, null); }
            set { _genXbf64Path.SetValue(_instance, value); }
        }

        public String GenXbfArm64Path
        {
            get { return (string)_genXbfArm64Path.GetValue(_instance, null); }
            set { _genXbfArm64Path.SetValue(_instance, value); }
        }

        public String RootNamespace
        {
            get { return (string) _rootNamespaceProperty.GetValue(_instance, null); }
            set { _rootNamespaceProperty.SetValue(_instance, value); }
        }

        public Version TargetPlatformMinVersion
        {
            get { return (Version)_targetPlatformMinVersion.GetValue(_instance, null); }
            set { _targetPlatformMinVersion.SetValue(_instance, value); }
        }

        public String ProjectName
        {
            get { return (string)_projectNameProperty.GetValue(_instance, null); }
            set { _projectNameProperty.SetValue(_instance, value); }
        }

        public bool IsLibrary
        {
            get { return (bool)_isLibraryProperty.GetValue(_instance, null); }
            set { _isLibraryProperty.SetValue(_instance, value); }
        }

        public Dictionary<String, String> ClassToHeaderFileMap
        {
            get { return (Dictionary<String, String>)_classToHeaderFileMapProperty.GetValue(_instance, null); }
            set { _classToHeaderFileMapProperty.SetValue(_instance, value); }
        }

    }
}
