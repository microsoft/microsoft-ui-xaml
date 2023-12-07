// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Xaml;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    using DirectUI;

    class TypeForCodeGen
    {
        DirectUIXamlType _xamlType;
        String _standardName = null;
        String _systemName = null;
        String _memberFriendlyName = null;
        LanguageSpecificString _languageSpecificString;

        public TypeForCodeGen(XamlType xamlType)
        {
            _xamlType = (DirectUIXamlType)xamlType;
        }

        public Type UnderlyingType
        {
            get { return _xamlType.UnderlyingType; }
        }

        // Standard name aka WinRT name
        public string StandardName
        {
            get
            {
                if (_standardName == null)
                {
                    _standardName = XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, ProgrammingLanguage.IdlWinRT, false);
                }
                return _standardName;
            }
        }

        // System Name type.FullName, including ugly Generic`[[ syntax
        public string SystemName
        {
            get
            {
                if (_systemName == null)
                {
                    _systemName = _xamlType.UnderlyingType.FullName;
                }
                return _systemName;
            }
        }

        public LanguageSpecificString FullName
        {
            get
            {
                if (_languageSpecificString == null)
                {
                    _languageSpecificString = new LanguageSpecificString(
                        () => XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, ProgrammingLanguage.CppCX, true),
                        () => XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, ProgrammingLanguage.CppWinRT, true),
                        () => XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, ProgrammingLanguage.CSharp, true),
                        () => XamlSchemaCodeInfo.GetFullGenericNestedName(_xamlType.UnderlyingType, ProgrammingLanguage.VB, true)
                        );
                }
                return _languageSpecificString;
            }
        }

        // Full Type name usable as a C++ variable or function name (i.e. '_' instead of namespace/generic/template separators).
        public string MemberFriendlyName
        {
            get
            {
                if (_memberFriendlyName == null)
                {
                    _memberFriendlyName = this.StandardName.GetMemberFriendlyName();
                }
                return _memberFriendlyName;
            }
        }
    }

    public static class TypeForCodeGenExtensionMethods
    {
        static Dictionary<XamlType, TypeForCodeGen> cache = new Dictionary<XamlType, TypeForCodeGen>();

        private static TypeForCodeGen GetTypeForCodeGen(this XamlType type)
        {
            TypeForCodeGen tfcg;
            if (cache.ContainsKey(type))
            {
                tfcg = cache[type];
            }
            else
            {
                tfcg = new TypeForCodeGen(type);
                cache.Add(type, tfcg);
            }
            return tfcg;
        }

        public static string CSharpName(this XamlType type)
        {
            return GetTypeForCodeGen(type).FullName.CSharpName();
        }

        public static string CppCXName(this XamlType type, bool IncludeHatIfApplicable = true)
        {
            TypeForCodeGen tfcg = GetTypeForCodeGen(type);
            if (type.UnderlyingType.IsValueType || !IncludeHatIfApplicable)
            {
                return tfcg.FullName.CppCXName();
            }
            else
            {
                return tfcg.FullName.CppCXName() + "^";
            }
        }

        public static string CppWinRTName(this XamlType type)
        {
            return GetTypeForCodeGen(type).FullName.CppWinRTName();
        }

        public static string MemberFriendlyName(this XamlType type)
        {
            return GetTypeForCodeGen(type).MemberFriendlyName;
        }

        public static string VBName(this XamlType type)
        {
            return GetTypeForCodeGen(type).FullName.VBName();
        }

        public static IEnumerable<Parameter> TryGetInvokeParameters(this Type multicastDelegate)
        {
            return GetInvokeParameters(multicastDelegate, false);
        }

        public static IEnumerable<Parameter> GetInvokeParameters(this Type multicastDelegate, bool throwOnError = true)
        {
            if (multicastDelegate.BaseType.FullName != KnownTypes.MulticastDelegate)
            {
                if (throwOnError)
                {
                    throw new ArgumentException($"Type '{multicastDelegate.BaseType.FullName}' is not a multi cast delegate.");
                }
                else
                {
                    return null;
                }
            }
            var invokeMethodInfo = multicastDelegate.GetMethod(KnownMembers.Invoke);
            if (invokeMethodInfo == null)
            {
                if (throwOnError)
                {
                    throw new ArgumentException($"Type '{multicastDelegate.BaseType.FullName}' does not have an Invoke method.");
                }
                else
                {
                    return null;
                }
            }
            return invokeMethodInfo.GetParameters().Select(p => new Parameter(p));
        }

        public static string ForCall(this IEnumerable<Parameter> parameters)
        {
            return parameters.Select(p => p.Name).ToCommaSeparatedValues();
        }

        public static LanguageSpecificString Declaration(this IEnumerable<Parameter> parameters)
        {
            var cppDeclarations = parameters.Select(p => $"{XamlSchemaCodeInfo.GetFullGenericNestedName(p.ParameterType, ProgrammingLanguage.CppWinRT, true)} const& {p.Name}");
            var csDeclarations = parameters.Select(p => $"{XamlSchemaCodeInfo.GetFullGenericNestedName(p.ParameterType, ProgrammingLanguage.CSharp, true)} {p.Name}");
            var vbDeclarations = parameters.Select(p => $"{p.Name} As {XamlSchemaCodeInfo.GetFullGenericNestedName(p.ParameterType, ProgrammingLanguage.VB, true)}");
            var cxDeclarations = parameters.Select(p => string.Format("{0}{1} {2}",
                XamlSchemaCodeInfo.GetFullGenericNestedName(p.ParameterType, ProgrammingLanguage.CppCX, true),
                p.ParameterType.IsValueType ? "" : " ^ ", p.Name));

            return new LanguageSpecificString(
                () => cxDeclarations.ToCommaSeparatedValues(),
                () => cppDeclarations.ToCommaSeparatedValues(),
                () => csDeclarations.ToCommaSeparatedValues(),
                () => vbDeclarations.ToCommaSeparatedValues());
        }
    }
}
