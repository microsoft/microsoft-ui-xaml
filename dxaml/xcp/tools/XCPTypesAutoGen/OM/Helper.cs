// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Linq;
using System.Reflection;
using System.Text;

namespace OM
{
    public class Helper
    {
        internal static string GetAbiRuntimeClassFullName(TypeDefinition type, bool isGenericArgument = false)
        {
            if (isGenericArgument && !string.IsNullOrEmpty(type.AbiGenericArgumentName))
            {
                return type.AbiGenericArgumentName;
            }

            ClassDefinition typeAsClass = type as ClassDefinition;
            if (typeAsClass != null && !typeAsClass.IsValueType)
            {
                if (typeAsClass.IdlClassInfo.IsExcluded)
                {
                    return typeAsClass.AbiImplementationFullName;
                }

                return typeAsClass.IdlClassInfo.RuntimeClassFullName;
            }
            else
            {
                DelegateDefinition typeAsDelegate = type as DelegateDefinition;
                if (typeAsDelegate != null)
                {
                    return typeAsDelegate.IdlDelegateInfo.GenericFullName;
                }

                if (type.IsPrimitive && !string.IsNullOrEmpty(type.PrimitiveCppName))
                {
                    return type.PrimitiveCppName;
                }
                return type.FullName;
            }
        }

        internal static string GetAbiReferenceFullName(TypeDefinition type, bool isGenericArgument = false)
        {
            if (!type.IsGenericType)
            {
                return PrefixAbiIfNeeded(GetAbiNonGenericFullName(type, isGenericArgument));
            }

            StringBuilder builder = new StringBuilder(GetAbiNonGenericFullName(type, isGenericArgument));
            builder.Append('<');
            bool first = true;
            foreach (TypeReference arg in type.GenericArguments)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                first = false;

                if (arg.IsNullable)
                {
                    builder.Append(PrefixAbiIfNeeded("Windows.Foundation.IReference<"));
                }
                builder.Append(GetAbiReferenceFullName(arg.Type, true));
                if (arg.IsNullable)
                {
                    builder.Append(">");
                }

                // Write type argument pointers.
                if (!arg.IsValueType)
                {
                    builder.Append('*');
                }
            }

            // Avoid generating ">>", because it will confuse the compiler.
            if (builder[builder.Length - 1] == '>')
            {
                builder.Append(' ');
            }
            builder.Append('>');

            return PrefixAbiIfNeeded(builder.ToString());
        }

        private static string GetAbiNonGenericFullName(TypeDefinition type, bool isGenericArgument = false)
        {
            if (isGenericArgument)
            {
                return GetAbiRuntimeClassFullName(type, isGenericArgument);
            }

            ClassDefinition typeAsClass = type as ClassDefinition;
            if (typeAsClass != null && !typeAsClass.IsValueType)
            {
                if ((typeAsClass.IsGenericType && !typeAsClass.IsInterface) || (typeAsClass.Modifier <= Modifier.Internal))
                {
                    return typeAsClass.AbiImplementationFullName;
                }
                else
                {
                    return typeAsClass.IdlClassInfo.AbiInterfaceFullName;
                }
            }

            DelegateDefinition typeAsDelegate = type as DelegateDefinition;
            if (typeAsDelegate != null)
            {
                return typeAsDelegate.IdlDelegateInfo.FullInterfaceName;
            }

            EnumDefinition typeAsEnum = type as EnumDefinition;
            if (typeAsEnum != null)
            {
                if (typeAsEnum.IdlEnumInfo.IsExcluded)
                {
                    return typeAsEnum.AbiImplementationName;
                }
                else
                {
                    return typeAsEnum.FullName;
                }
            }

            if (type.IsPrimitive && !string.IsNullOrEmpty(type.PrimitiveCppName))
            {
                return type.PrimitiveCppName;
            }
            return type.IdlTypeInfo.FullName;
        }

        internal static void ShallowCopyProperties<TCommonBase>(TCommonBase from, TCommonBase to)
        {
            foreach (PropertyInfo property in typeof(TCommonBase).GetProperties().Where(p => p.CanRead && p.CanWrite && p.GetSetMethod(nonPublic: false) != null))
            {
                object value = property.GetValue(from, null);
                property.SetValue(to, value, null);
            }
        }

        internal static string ToPointerName(string name, int dimensions = 1)
        {
            if (dimensions == 0)
            {
                return name;
            }

            return new string('p', dimensions) + char.ToUpper(name[0]) + name.Substring(1);
        }

        internal static bool IsLessVisible(TypeDefinition left, TypeDefinition right)
        {
            if (left.IdlTypeInfo.IsPrivateIdlOnly)
            {
                return false;
            }

            return right.IdlTypeInfo.IsPrivateIdlOnly;
        }

        public static bool ShouldPrefixWithABI()
        {
            return Environment.GetEnvironmentVariable("PREFIXABI") != "/noabi";
        }

        // TODO 22728412 - Remove SetShouldGenerateWUXNamespace, ShouldGenerateWUXNamespace, ShouldGenerateContracts, EnsureCorrectNamespace,
        // EnsureTypeModelNamespace, EnsureWUXNamespace.  GetRootNamespace and GetRootNamespaceCpp may still have some value for code-cleanliness
        // if they use a hard-coded Microsoft.UI.Xaml namespace
        public static bool ShouldGenerateContracts()
        {
            return true;
        }

        public static string GetRootNamespace()
        {
            return "Microsoft.UI.Xaml";
        }

        public static string GetRootNamespaceCpp()
        {
            return GetRootNamespace().Replace(".", "::");
        }

        public static string PrefixAbi(string expression)
        {
            if (!ShouldPrefixWithABI())
            {
                return expression;
            }

            if (expression.StartsWith("ABI") == false)
            {
                if (expression.Contains("."))
                {
                    return "ABI." + expression;
                }
                if (expression.Contains("::"))
                {
                    return "ABI::" + expression;
                }
            }

            return expression;
        }

        public static string PrefixAbiIfNeeded(string expression) 
        {
            if (!ShouldPrefixWithABI())
            {
                return expression;
            }

            if (expression.StartsWith("_Out") || 
            expression.StartsWith("_In") || 
            expression.StartsWith("ctl") ||
            expression.StartsWith("wrl") ||
            expression.StartsWith("Microsoft::WRL::Wrappers")|| 
            expression.StartsWith("DirectUI") ||
            expression.StartsWith("TrackerPtr") ||
            expression.StartsWith("ABI")
            )
            {
                return expression;
            }

           return PrefixAbi(expression);
        }
    }
}
