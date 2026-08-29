// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class CppCX_CodeGenerator<T> : NativeCodeGenerator<T>
    {
        public override string ToStringWithCulture(ICodeGenOutput codegenOutput)
        {
            return codegenOutput.CppCXName();
        }

        public override string ToStringWithCulture(XamlType type)
        {
            return type.CppCXName();
        }

        public static new String Colonize(string typeName)
        {
            return KnownStrings.Colonize(typeName).Replace("System::", "Platform::");
        }

        /// <summary>
        /// Used by the C++ code gen to create a reference to a type
        /// </summary>
        /// <param name="typeName"></param>
        /// <returns></returns>
        internal static string ColonizeRef(EventAssignment ev)
        {
            string typeName = XamlSchemaCodeInfo.GetFullGenericNestedName(
                    ev.Event.Type.UnderlyingType, ProgrammingLanguage.CppCX, false);
            return typeName;
        }

        /// <summary>
        /// Used by the C++ code gen to create a reference to a type
        /// </summary>
        /// <param name="typeName"></param>
        /// <returns></returns>
        internal static string ColonizeRef(BoundEventAssignment ev)
        {
            string typeName = XamlSchemaCodeInfo.GetFullGenericNestedName(
                    ev.MemberType.UnderlyingType, ProgrammingLanguage.CppCX, false);
            return typeName;
        }

        public static String Projection(string typeName)
        {
            return Globalize(typeName);
        }

        protected string GetBindingFullClassName(BindUniverse bindUniverse, XamlClassCodeInfo codeInfo)
        {
            return Colonize($"{codeInfo.ClassName.FullName}.{bindUniverse.BindingsClassName}");
        }

        protected string GetBindingTrackingFullClassName(BindUniverse bindUniverse, XamlClassCodeInfo codeInfo)
        {
            return Colonize(bindUniverse.NeedsCppBindingTrackingClass ?
                $"{codeInfo.ClassName.Namespace}.{bindUniverse.BindingsTrackingClassName}" :
                "::XamlBindingInfo::XamlBindingTrackingBase");
        }

        public void OutputNamespaceBegin(string name)
        {
            foreach (var part in name.Split('.'))
            {
                WriteLine($"namespace {part}");
                WriteLine("{");
                PushIndent();
            }
        }

        public void OutputNamespaceEnd(string name)
        {
            foreach (var part in name.Split('.'))
            {
                PopIndent();
                WriteLine("}");
            }
        }
    }
}
