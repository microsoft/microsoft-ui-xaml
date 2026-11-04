// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Win8Xaml.CompilerProxies
{
    public class CompileXaml : Microsoft.Build.Utilities.Task
    {
        static ProxyHelper _compileXamlType;
        //static MethodInfo _executeMethod;

        // Required Properties
        static PropertyInfo _projectPathProperty;
        static PropertyInfo _languageProperty;
        static PropertyInfo _languageSourceExtensionProperty;
        static PropertyInfo _outputPathProperty;
        static PropertyInfo _referenceAssembliesProperty;
        static PropertyInfo _referenceAssemblyPathsProperty;

        static PropertyInfo _forceSharedStateShutdown;
        static PropertyInfo _compileModeProperty;
        static PropertyInfo _generatedCodeFilesProperty;
        static PropertyInfo _generatedXamlFilesProperty;

        object _instance;

        static CompileXaml()
        {
            _compileXamlType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Tasks.CompileXaml");
            //_executeMethod = _compileXamlType.GetMethod("Execute");

            // Required Properties
            _projectPathProperty = _compileXamlType.GetProperty("ProjectPath");
            _languageProperty = _compileXamlType.GetProperty("Language");
            _languageSourceExtensionProperty = _compileXamlType.GetProperty("LanguageSourceExtension");
            _outputPathProperty = _compileXamlType.GetProperty("OutputPath");
            _referenceAssembliesProperty = _compileXamlType.GetProperty("ReferenceAssemblies");
            _referenceAssemblyPathsProperty = _compileXamlType.GetProperty("ReferenceAssemblyPaths");

            _forceSharedStateShutdown = _compileXamlType.GetProperty("ForceSharedStateShutdown");
            _compileModeProperty = _compileXamlType.GetProperty("CompileMode");
            _generatedCodeFilesProperty = _compileXamlType.GetProperty("GeneratedCodeFiles");
            _generatedXamlFilesProperty = _compileXamlType.GetProperty("GeneratedXamlFiles");
        }

        public CompileXaml()
        {
            _instance = _compileXamlType.CreateInstance();
        }

        public override bool Execute()
        {
            // We need the following if we're to compile anything.
            if (ProjectPath == null)
                return false;

            if (Language == null)
                return false;

            if (LanguageSourceExtension == null)
                return false;

            if (ReferenceAssemblies == null)
                return false;

            if (ReferenceAssemblyPaths == null)
                return false;

            return ((Task)_instance).Execute();
        }

        public string ProjectPath
        {
            get { return (String)_projectPathProperty.GetValue(_instance, null); }
            set { _projectPathProperty.SetValue(_instance, value); }
        }

        /// <summary>
        /// Legal Values C++, C#, VB
        /// </summary>
        public string Language
        {
            get { return (String)_languageProperty.GetValue(_instance, null); }
            set { _languageProperty.SetValue(_instance, value); }
        }

        /// <summary>
        /// .cpp, .cs, .vb
        /// </summary>
        public string LanguageSourceExtension
        {
            get { return (String)_languageSourceExtensionProperty.GetValue(_instance, null); }
            set { _languageSourceExtensionProperty.SetValue(_instance, value); }
        }

        public string OutputPath
        {
            get { return (String)_outputPathProperty.GetValue(_instance, null); }
            set { _outputPathProperty.SetValue(_instance, value); }
        }

        public ITaskItem[] ReferenceAssemblies
        {
            get { return (TaskItem[])_referenceAssembliesProperty.GetValue(_instance, null); }
            set { _referenceAssembliesProperty.SetValue(_instance, value); }
        }

        public ITaskItem[] ReferenceAssemblyPaths 
        {
            get { return (TaskItem[])_referenceAssemblyPathsProperty.GetValue(_instance, null); }
            set { _referenceAssemblyPathsProperty.SetValue(_instance, value); }
        }

        public bool ForceSharedStateShutdown
        {
            get { return (bool)_forceSharedStateShutdown.GetValue(_instance, null); }
            set { _forceSharedStateShutdown.SetValue(_instance, value); }
        }

        public String CompileMode
        {
            get { return (String)_compileModeProperty.GetValue(_instance, null); }
            set { _compileModeProperty.SetValue(_instance, value); }
        }

        public ITaskItem[] GeneratedCodeFiles
        {
            get { return (ITaskItem[])_generatedCodeFilesProperty.GetValue(_instance, null); }
        }

        public ITaskItem[] GeneratedXamlFiles
        {
            get { return (ITaskItem[])_generatedXamlFilesProperty.GetValue(_instance, null); }
        }
    }
}
