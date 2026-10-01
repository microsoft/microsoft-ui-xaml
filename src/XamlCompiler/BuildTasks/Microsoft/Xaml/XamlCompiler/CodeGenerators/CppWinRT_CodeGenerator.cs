// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal static class CppWinRTProjectionDependency
    {
        public static string GetNamespace(Type type)
        {
            Type adjustedType = ((type != null) && type.IsArray) ? type.GetElementType() : type;

            if (adjustedType == null || XamlSchemaCodeInfo.IsProjectedPrimitiveCppType(adjustedType.FullName))
            {
                return null;
            }

            return adjustedType.Namespace;
        }

        public static IEnumerable<string> GetNamespaces(Type type)
        {
            Type adjustedType = ((type != null) && type.IsArray) ? type.GetElementType() : type;
            string projectionNamespace = GetNamespace(adjustedType);

            if (projectionNamespace == null)
            {
                yield break;
            }

            yield return projectionNamespace;

            if (adjustedType.IsGenericType)
            {
                foreach (var genericArgument in adjustedType.GetGenericArguments())
                {
                    foreach (var nestedNamespace in GetNamespaces(genericArgument))
                    {
                        yield return nestedNamespace;
                    }
                }
            }
        }

        public static IEnumerable<string> GetNamespaces(Type type, string unresolvedNamespace)
        {
            if (type == null)
            {
                if (!String.IsNullOrWhiteSpace(unresolvedNamespace))
                {
                    yield return unresolvedNamespace;
                }
                yield break;
            }

            foreach (var projectionNamespace in GetNamespaces(type))
            {
                yield return projectionNamespace;
            }
        }

        public static string GetHeaderFile(string projectionNamespace)
        {
            return $"winrt/{projectionNamespace}.h";
        }

        public static string GetModuleName(string projectionNamespace)
        {
            return $"winrt.{projectionNamespace}";
        }

        public static string GetXamlPrimaryModuleName(string rootNamespace)
        {
            return string.IsNullOrWhiteSpace(rootNamespace)
                ? "Application_Xaml"
                : $"{rootNamespace}.Application_Xaml";
        }

        public static string GetXamlPartitionName(string runtimeClassName)
        {
            return runtimeClassName.Replace("::", ".");
        }

        public static string GetXamlPartitionModuleName(string rootNamespace, string partitionName)
        {
            return $"{GetXamlPrimaryModuleName(rootNamespace)}:{GetXamlPartitionName(partitionName)}";
        }

    }

    internal class CppWinRT_CodeGenerator<T> : NativeCodeGenerator<T>
    {
        public override string ToStringWithCulture(ICodeGenOutput codegenOutput)
        {
            return codegenOutput.CppWinRTName();
        }

        public override string ToStringWithCulture(XamlType type)
        {
            return type.CppWinRTName();
        }

        public string GetCppWinRTProjectionDependencyDirective(string projectionNamespace, bool optionalHeader = false)
        {
            string headerFile = CppWinRTProjectionDependency.GetHeaderFile(projectionNamespace);

            if (ProjectInfo.UseCppWinRTNamedModules)
            {
                return $"import {CppWinRTProjectionDependency.GetModuleName(projectionNamespace)};";
            }

            if (optionalHeader)
            {
                return $"#if __has_include(<{headerFile}>)\n#include <{headerFile}>\n#endif";
            }

            return $"#include <{headerFile}>";
        }

        public string GetCppWinRTNamedModuleImportDirective(string projectionNamespace)
        {
            return ProjectInfo.UseCppWinRTNamedModules
                ? $"import {CppWinRTProjectionDependency.GetModuleName(projectionNamespace)};"
                : String.Empty;
        }

        public string GetCppWinRTModuleCompatibilityDefinition()
        {
            if (!ProjectInfo.UseCppWinRTNamedModules)
            {
                return String.Empty;
            }

            return "#ifndef WINRT_IMPORT_MODULE\n#define WINRT_IMPORT_MODULE\n#endif";
        }

        public static String Projection(string typeName)
        {
            string newName = Globalize(typeName);

            // The check of "::winrt::" instead of "::winrt" is deliberate here - otherwise user-defined types from a namespace starting
            // with "winrt" could be confused as already having the "::winrt" prefix (e.g. a "winrt_UserNamespace::UserType" that we want to convert to "::winrt::winrt_UserNamespace::UserType").
            if (!newName.StartsWith("::winrt::"))
            {
                newName = "::winrt" + newName;
            }
            else
            {
                throw new ArgumentException("Name should not already contain ::winrt prefix");
            }
            newName = newName.Replace("<::", "<::winrt::");
            newName = newName.Replace("::winrt::winrt::", "::winrt::");
            return newName;
        }

        protected string GetBindingTrackingClassName(BindUniverse bindUniverse, XamlClassCodeInfo codeInfo)
        {
            return Colonize(bindUniverse.NeedsCppBindingTrackingClass ?
                bindUniverse.BindingsTrackingClassName :
                Projection($"{ProjectInfo.RootNamespace}::implementation::XamlBindingTrackingBase"));
        }

        public IEnumerable<string> GetCacheDeclarations(BindUniverse bindUniverse)
        {
            foreach (var step in bindUniverse.BindPathSteps.Values.Where(step => step.IsIncludedInUpdate == true && step.NeedsUpdateChildListeners))
            {
                if (step.ImplementsINPC && step.RequiresChildNotification)
                {
                    if (step is RootStep)
                    {
                        yield return $"::winrt::weak_ref<{Projection(KnownNamespaces.XamlData)}::INotifyPropertyChanged> cachePC_{step.CodeName};";
                    }
                    else
                    {
                        yield return $"{Projection(KnownNamespaces.XamlData)}::INotifyPropertyChanged cachePC_{step.CodeName}{{nullptr}};";
                    }
                }
                if (step.ImplementsINDEI)
                {
                    yield return $"{Projection(KnownNamespaces.XamlData)}::INotifyDataErrorInfo cacheEC_{step.CodeName}{{nullptr}};";
                }
                if (step.ImplementsIObservableVector && step.RequiresChildNotification)
                {
                    yield return step.ValueType.CppWinRTName() + " cacheVC_" + step.CodeName + "{nullptr};";
                }
                if (step.ImplementsIObservableMap && step.RequiresChildNotification)
                {
                    yield return step.ValueType.CppWinRTName() + " cacheMC_" + step.CodeName + "{nullptr};";
                }
                else if (step.ImplementsINCC)
                {
                    yield return $"{Projection(KnownNamespaces.XamlInterop)}::INotifyCollectionChanged cacheCC_{step.CodeName}{{nullptr}};";
                }
                foreach (var child in step.TrackingSteps.OfType<DependencyPropertyStep>())
                {
                    if (step is RootStep)
                    {
                        yield return $"::winrt::weak_ref<{Projection(KnownNamespaces.Xaml)}::DependencyObject> cacheDPC_{child.CodeName};";
                    }
                    else
                    {
                        yield return $"{Projection(KnownNamespaces.Xaml)}::DependencyObject cacheDPC_{child.CodeName}{{nullptr}};";
                    }
                }
            }
        }

        public IEnumerable<string> GetTokenDeclarations(BindUniverse bindUniverse)
        {
            foreach (var step in bindUniverse.BindPathSteps.Values.Where(step => step.IsIncludedInUpdate == true && step.NeedsUpdateChildListeners))
            {
                if (step.ImplementsINPC)
                {
                    yield return $"::winrt::event_token tokenPC_{step.CodeName} {{}};";
                }
                if (step.ImplementsINDEI)
                {
                    yield return $"::winrt::event_token tokenEC_{step.CodeName} {{}};";
                }
                if (step.ImplementsIObservableVector)
                {
                    yield return $"::winrt::event_token tokenVC_{step.CodeName} {{}};";
                }
                if (step.ImplementsIObservableMap)
                {
                    yield return $"::winrt::event_token tokenMC_{step.CodeName} {{}};";
                }
                else if (step.ImplementsINCC)
                {
                    yield return $"::winrt::event_token tokenCC_{step.CodeName} {{}};";
                }
            }

            foreach (var step in bindUniverse.BindPathSteps.Values.Where(step => step.IsIncludedInUpdate == true && step.NeedsUpdateChildListeners))
            {
                foreach (var child in step.TrackingSteps.OfType<DependencyPropertyStep>())
                {
                    yield return $"std::int64_t tokenDPC_{child.CodeName}{{0}};";
                }
            }
        }
    }
}
