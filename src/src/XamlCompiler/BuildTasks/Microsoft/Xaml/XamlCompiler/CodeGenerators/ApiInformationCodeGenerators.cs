// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    internal interface IApiInformationCodeGen
    {
        ICodeGenOutput CallExpression { get; }
    }

    internal static class ApiInformationCodeGenExtensions
    {
        private static Dictionary<ApiInformation, IApiInformationCodeGen> apiInformationCache =
            new Core.InstanceCache<ApiInformation, IApiInformationCodeGen>();
        private static Dictionary<ApiInformationMethod, IApiInformationCodeGen> apiInformationMethodCache =
            new Core.InstanceCache<ApiInformationMethod, IApiInformationCodeGen>();
        private static Dictionary<ApiInformationParameter, IApiInformationCodeGen> apiInformationParameterCache =
            new Core.InstanceCache<ApiInformationParameter, IApiInformationCodeGen>();

        public static IApiInformationCodeGen CodeGen(this ApiInformation instance)
        {
            IApiInformationCodeGen codeGen = null;
            if (!apiInformationCache.TryGetValue(instance, out codeGen))
            {
                codeGen = new ApiInformationCodeGenerator() { Instance = instance };
                apiInformationCache.Add(instance, codeGen);
            }
            return codeGen;
        }

        public static IApiInformationCodeGen CodeGen(this ApiInformationMethod instance)
        {
            IApiInformationCodeGen codeGen = null;
            if (!apiInformationMethodCache.TryGetValue(instance, out codeGen))
            {
                if (instance.IsCustomAPI)
                {
                    codeGen = new CustomPredicateCodeGenerator() { Instance = instance };
                }
                else
                {
                    codeGen = new ApiInformationMethodCodeGenerator() { Instance = instance };
                }
                apiInformationMethodCache.Add(instance, codeGen);
            }
            return codeGen;
        }

        public static IApiInformationCodeGen CodeGen(this ApiInformationParameter instance)
        {
            IApiInformationCodeGen codeGen = null;
            if (!apiInformationParameterCache.TryGetValue(instance, out codeGen))
            {
                codeGen = new ApiInformationParameterCodeGenerator() { Instance = instance };
                apiInformationParameterCache.Add(instance, codeGen);
            }
            return codeGen;
        }
    }

    internal class ApiInformationCodeGenerator : CodeGeneratorBase<ApiInformation>, IApiInformationCodeGen
    {
        public ICodeGenOutput CallExpression
        {
            get
            {
                var callExpression = Instance.Method.CodeGen().CallExpression;
                if (Instance.Method.IsCustomAPI)
                {
                    return new LanguageSpecificString(
                    () => string.Format("{0}->Evaluate(ref new Platform::Collections::Vector<Platform::String^>({{ {1} }})->GetView())", callExpression.CppCXName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CppCXName()))),
                    () => string.Format("{0}.Evaluate(winrt::single_threaded_vector<winrt::hstring>({{ {1} }}).GetView())", callExpression.CppWinRTName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CppWinRTName()))),
                    () => string.Format("{0}.Evaluate(new System.Collections.Generic.List<string> {{ {1} }})", callExpression.CSharpName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CSharpName()))),
                    () => string.Format("{0}.Evaluate(New System.Collections.Generic.List(Of String) From {{ {1} }})", callExpression.VBName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.VBName())))
                    );
                }
                else
                {
                    return new LanguageSpecificString(
                    () => string.Format("{0}({1})", callExpression.CppCXName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CppCXName()))),
                    () => string.Format("{0}({1})", callExpression.CppWinRTName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CppWinRTName()))),
                    () => string.Format("{0}({1})", callExpression.CSharpName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.CSharpName()))),
                    () => string.Format("{0}({1})", callExpression.VBName(), string.Join(", ", Instance.Parameters.Select(p => p.CodeGen().CallExpression.VBName())))
                    );
                }
            }
        }
    }

    internal class ApiInformationMethodCodeGenerator : CodeGeneratorBase<ApiInformationMethod>, IApiInformationCodeGen
    {
        public ICodeGenOutput CallExpression =>
            new LanguageSpecificString(
                () => string.Format("{1}::Windows::Foundation::Metadata::ApiInformation::{0}", Instance.MethodName, Instance.Condition ? "" : "!"),
                () => string.Format("{1}::winrt::Windows::Foundation::Metadata::ApiInformation::{0}", Instance.MethodName, Instance.Condition ? "" : "!"),
                () => string.Format("{1}global::Windows.Foundation.Metadata.ApiInformation.{0}", Instance.MethodName, Instance.Condition ? "" : "!"),
                () => string.Format("{1}Global.Windows.Foundation.Metadata.ApiInformation.{0}", Instance.MethodName, Instance.Condition ? "" : "Not ")
            );
    }

    internal class CustomPredicateCodeGenerator : CodeGeneratorBase<ApiInformationMethod>, IApiInformationCodeGen
    {
        public ICodeGenOutput CallExpression
        {
            get
            {
                string csNamespace = string.Empty;
                if (Instance.Namespace != null && Instance.Namespace.StartsWith("using:", StringComparison.OrdinalIgnoreCase))
                {
                    csNamespace = Instance.Namespace.Substring(6); // Remove "using:"
                }

                // For C++, replace dots with ::
                string cppNamespace = string.IsNullOrEmpty(csNamespace) ? string.Empty : csNamespace.Replace(".", "::");

                return new LanguageSpecificString(
                    () => string.Format("ref new {0}::{1}()", cppNamespace, Instance.MethodName),
                    () => string.Format("::winrt::{0}::{1}()", cppNamespace, Instance.MethodName),
                    () => string.Format("new {0}.{1}()", csNamespace, Instance.MethodName),
                    () => string.Format("New Global.{0}.{1}()", csNamespace, Instance.MethodName)
                );
            }
        }
    }

    internal class ApiInformationParameterCodeGenerator : CodeGeneratorBase<ApiInformationParameter>, IApiInformationCodeGen
    {
        public ICodeGenOutput CallExpression =>
            new LanguageSpecificString(
                () => string.Format("{0}{1}{0}", Instance.ParameterType == typeof(string) ? "\"" : "", Instance.ParameterValue),
                () => string.Format("{0}{1}{2}", Instance.ParameterType == typeof(string) ? "L\"" : "", Instance.ParameterValue, Instance.ParameterType == typeof(string) ? "\"" : ""),
                () => string.Format("{0}{1}{0}", Instance.ParameterType == typeof(string) ? "\"" : "", Instance.ParameterValue),
                () => string.Format("{0}{1}{0}", Instance.ParameterType == typeof(string) ? "\"" : "", Instance.ParameterValue)
            );
    }
}