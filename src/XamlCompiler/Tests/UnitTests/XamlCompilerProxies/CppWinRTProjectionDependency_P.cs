// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public static class CppWinRTProjectionDependency
    {
        static readonly ProxyHelper _projectionDependencyType;
        static readonly MethodInfo _getHeaderFile;
        static readonly MethodInfo _getModuleName;
        static readonly MethodInfo _getNamespaces;
        static readonly MethodInfo _getNamespacesWithFallback;
        static readonly MethodInfo _getXamlPrimaryModuleName;
        static readonly MethodInfo _getXamlClassModuleName;
        static readonly MethodInfo _writeAggregator;

        static CppWinRTProjectionDependency()
        {
            _projectionDependencyType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.CppWinRTProjectionDependency");
            _getHeaderFile = _projectionDependencyType.GetStaticMethod("GetHeaderFile", 1);
            _getModuleName = _projectionDependencyType.GetStaticMethod("GetModuleName", 1);
            _getNamespaces = _projectionDependencyType.GetStaticMethod("GetNamespaces", 1);
            _getNamespacesWithFallback = _projectionDependencyType.GetStaticMethod("GetNamespaces", 2);
            _getXamlPrimaryModuleName = _projectionDependencyType.GetStaticMethod("GetXamlPrimaryModuleName", 1);
            _getXamlClassModuleName = _projectionDependencyType.GetStaticMethod("GetXamlClassModuleName", 2);
            _writeAggregator = _projectionDependencyType.GetStaticMethod("WriteAggregator", 2);
        }

        public static bool[] GetCodeGenerationDecisionsBeforeProjectInfo(string flags)
        {
            var core = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CompileXamlInternal");
            object instance = core.CreateInstance();
            var flagProperty = core.GetProperty("CodeGenerationControlFlags");
            flagProperty.SetValue(instance, Enum.Parse(flagProperty.PropertyType, flags));
            var bindingFlags = BindingFlags.Instance | BindingFlags.NonPublic;
            return new[]
            {
                (bool)core.GetMethod("ShouldSuppressPageCodeGen", bindingFlags).Invoke(instance, null),
                (bool)core.GetMethod("ShouldSuppressTypeInfoCodeGen", bindingFlags).Invoke(instance, null)
            };
        }

        public static string GetHeaderFile(string projectionNamespace)
        {
            return (string)_getHeaderFile.Invoke(null, new object[] { projectionNamespace });
        }

        public static string GetModuleName(string projectionNamespace)
        {
            return (string)_getModuleName.Invoke(null, new object[] { projectionNamespace });
        }

        public static string[] GetNamespaces(Type type)
        {
            return ((IEnumerable<string>)_getNamespaces.Invoke(null, new object[] { type })).ToArray();
        }

        public static string[] GetNamespaces(Type type, string unresolvedNamespace)
        {
            return ((IEnumerable<string>)_getNamespacesWithFallback.Invoke(null, new object[] { type, unresolvedNamespace })).ToArray();
        }

        public static string GetXamlPrimaryModuleName(string rootNamespace)
        {
            return (string)_getXamlPrimaryModuleName.Invoke(null, new object[] { rootNamespace });
        }

        public static string GetXamlClassModuleName(string rootNamespace, string runtimeClassName)
        {
            return (string)_getXamlClassModuleName.Invoke(null, new object[] { rootNamespace, runtimeClassName });
        }

        public static string WriteAggregator(string rootNamespace, IEnumerable<string> classes)
        {
            return (string)_writeAggregator.Invoke(null, new object[] { rootNamespace, classes });
        }
    }
}
