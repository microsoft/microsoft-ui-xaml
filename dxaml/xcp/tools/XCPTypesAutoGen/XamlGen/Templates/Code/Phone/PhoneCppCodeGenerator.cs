// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.Code
{
    /// <summary>
    /// Base class for all metadata code generators.
    /// </summary>
    public abstract class PhoneCppCodeGenerator<TModel> : CppCodeGenerator<TModel>
    {
        protected string GetPhoneInitializeImplParameterListAsString(ClassDefinition cls, IEnumerable<ParameterDefinition> parameters)
        {
            string parameterListAsString = GetParameterListAsString(parameters);

            if (cls.IdlClassInfo.IsComposable)
            {
                if (parameterListAsString.Length > 0)
                {
                    parameterListAsString += ", ";
                }

                parameterListAsString += "_In_opt_ IInspectable* pOuter";
            }

            return parameterListAsString;
        }

        protected virtual string GetPhoneInitializeImplArgumentListAsString(ClassDefinition type, ConstructorDefinition ctor)
        {
            string argumentListAsString = GetArgumentListAsString(ctor.Parameters);

            if (type.IdlClassInfo.IsComposable)
            {
                if (argumentListAsString.Length > 0)
                {
                    argumentListAsString += ", ";
                }

                argumentListAsString += "nullptr";
            }

            return argumentListAsString;
        }

        protected string GetPhoneActivationFactoryTemplateParameters(ClassDefinition cls)
        {
            List<string> implementedInterfaces = new List<string>();
            foreach (var version in cls.Versions.OrderBy(v => v.Version).Select(v => v.GetProjection()))
            {
                if (version.IdlClassInfo.HasFactoryMethods)
                {
                    implementedInterfaces.Add(AsCppType(version.IdlClassInfo.FullFactoryInterfaceName));
                }
                if (version.IdlClassInfo.HasStaticMembers)
                {
                    implementedInterfaces.Add(AsCppType(version.IdlClassInfo.FullStaticMembersInterfaceName));
                }
                foreach (var explicitlyImplementedInterface in version.FactoryExplicitlyImplementedInterfaces)
                {
                    implementedInterfaces.Add(AsCppType(explicitlyImplementedInterface.AbiFullName));
                }
            }

            // AgileActivationFactory takes at most 3 interfaces as template parameters:
            if (implementedInterfaces.Count <= 3)
            {
                return string.Join(", ", implementedInterfaces);
            }
            else
            {
                // We can extend beyond this limit by wrapping interfaces in wrl::Implements
                return WrapInterfaceListInWrlImplements(implementedInterfaces);
            }
        }

        private string WrapInterfaceListInWrlImplements(List<string> interfaces)
        {
            // wrl::Implements takes up to 10 interfaces as template parameters:
            if (interfaces.Count <= 10)
            {
                return string.Format("wrl::Implements<{0}>", string.Join(", ", interfaces));
            }
            else
            {
                // We can extend this limit indefinitely by recursively wrapping lists of interfaces in wrl::Implements:
                return string.Format("wrl::Implements<{0}, {1}>", 
                    string.Join(", ", interfaces.Take(9)),
                    WrapInterfaceListInWrlImplements(interfaces.Skip(9).ToList()));
            }
        }

        protected string AsCppNamespaceDeclarationBegin(NamespaceDefinition ns)
        {
            string prefix = "namespace ";

            if (Helper.ShouldPrefixWithABI())
            {
                prefix = "namespace ABI { namespace ";
            }

            return prefix + ns.Name.Replace(".", " { namespace ");
        }

        protected string AsCppNamespaceDeclarationEnd(NamespaceDefinition ns)
        {
            string  namespaceDeclarationEnd = ns.Name.Where(c => c == '.').Aggregate("", (current, c) => current + "}");

            if (Helper.ShouldPrefixWithABI())
            {
                namespaceDeclarationEnd +="}";
            }

            return namespaceDeclarationEnd;
        }

        protected static string AsCppDefine(string str)
        {
            return str.Replace(".", "_").Replace("`", "_").Replace("<", "_").Replace(">", "_").Replace(",", "_");
        }

        protected static string AsResourceIndex(string str)
        {
            return $"INDEX_{AsCppDefine(str).ToUpper()}";
        }

        protected static string AsResourceKey(string str)
        {
            return $"IDS_{AsCppDefine(str)}";
        }

        protected static string AsResourcePropertyIndex(MemberDefinition m)
        {
            return string.Format("PROPERTYINDEX_{0}_{1}", m.DeclaringType.TypeTableName, m.Name).ToUpper();
        }

        public string LowerCaseFirstLetter(string aWord)
        {
            var firstLetter = aWord.Substring(0, 1);
            var lowerCaseFirstLetter = firstLetter.ToLower() + aWord.Substring(1);
            return lowerCaseFirstLetter;
        }
    }
}
