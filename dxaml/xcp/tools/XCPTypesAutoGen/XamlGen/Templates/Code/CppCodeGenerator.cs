// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using OM;

namespace XamlGen.Templates.Code
{
    /// <summary>
    /// Base class for all metadata code generators.
    /// </summary>
    public abstract class CppCodeGenerator<TModel> : XamlCodeGenerator<TModel>
    {
        protected string GetEventSourceKind(EventDefinition ev)
        {
            return ev.IsRouted ? "CRoutedEventSource" : "CEventSource";
        }

        protected string GetBaseClassName(ClassDefinition type)
        {
            if (type.BaseClass.IsObjectType)
            {
                return "ctl::WeakReferenceSource";
            }
            return AsCppType(type.BaseClass.AbiImplementationFullName);
        }

        protected string GetBaseFactoryFullNameForValidateFactoryCreateInstance(ClassDefinition type)
        {
            return GetBaseFactoryFullNamePrivate(type, true /* includeInterfaceName */);
        }

        protected string GetBaseFactoryFullName(ClassDefinition type)
        {
            return GetBaseFactoryFullNamePrivate(type, false /* includeInterfaceName */);
        }

        private string GetBaseFactoryFullNamePrivate(ClassDefinition type, bool includeInterfaceName)
        {
            if (type.IsValueType
                || ((type.IsAbstract || !type.IdlClassInfo.HasDefaultConstructor)
                && !type.IdlClassInfo.IsComposable))
            {
                return "ctl::AbstractActivationFactory";
            }
            else
            {
                string abstractOrPlain = type.IsAbstract ? "Abstract" : "";
                string aggregableOrPlain = type.IdlClassInfo.IsComposable ? "Aggregable" : "";
                string factoryName = aggregableOrPlain + abstractOrPlain;
                if (!type.IsExcludedFromTypeTable && type.IsADependencyObject)
                {
                    factoryName = string.Format("ctl::Better{0}CoreObjectActivationFactory", factoryName);
                    return factoryName;
                }
                else
                {
                    factoryName = string.Format("ctl::{0}ActivationFactory<{1}{2}>",
                        factoryName,
                        AsCppType(type.AbiImplementationFullName),
                        includeInterfaceName ? "," + AsCppType(type.IdlClassInfo.AbiInterfaceFullName) : String.Empty
                        );
                    return factoryName;
                }
            }
        }

        protected bool IsBaseFactoryTypeGeneric(ClassDefinition type)
        {
            return GetBaseFactoryTypeName(type).Substring(5).Contains('<');
        }

        protected string GetBaseFactoryTypeName(ClassDefinition type)
        {
            return GetBaseFactoryFullName(type).Substring(5); // Cut off the "ctl::" from the front
        }

        protected string GetBaseFactoryTypeNameForValidateFactoryCreateInstance(ClassDefinition type)
        {
            return GetBaseFactoryFullNameForValidateFactoryCreateInstance(type).Substring(5); // Cut off the "ctl::" from the front
        }

        protected string GetCoreFieldInitializerString(ClassDefinition type, bool isContinuationOfInitializationList = false)
        {
            IEnumerable<PropertyDefinition> fields = type.CoreInstanceFields;
            if (!fields.Any())
            {
                return string.Empty;
            }

            bool first = true;
            StringBuilder builder = new StringBuilder(isContinuationOfInitializationList ? ", " : ": ");
            foreach (PropertyDefinition field in fields)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    builder.Append(", ");
                }

                builder.Append(field.FieldName);
                if (field.PropertyType.IsValueType)
                {
                    builder.Append("()");
                }
                else
                {
                    builder.Append("(nullptr)");
                }
            }
            return builder.ToString();
        }

        protected string GetFieldInitializerString(ClassDefinition type)
        {
            IEnumerable<PropertyDefinition> valueTypeFields = type.InstanceFields.Where(f => f.PropertyType.IsValueType && !f.PropertyType.Type.IsStringType);
            if (!valueTypeFields.Any())
            {
                return string.Empty;
            }

            bool first = true;
            StringBuilder builder = new StringBuilder(": ");
            foreach (PropertyDefinition field in valueTypeFields)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    builder.Append(", ");
                }

                builder.Append(field.FrameworkFieldName);
                builder.Append("()");
            }
            return builder.ToString();
        }

        protected string GetFactoryFactory(ClassDefinition type)
        {
            if (!type.HasCustomFactory && type.IsADependencyObject && !type.IsExcludedFromTypeTable)
            {
                return string.Format("ctl::BetterActivationFactoryCreator::GetForDO({0})", type.IndexName);
            }
            return string.Format("ctl::ActivationFactoryCreator<{0}>::CreateActivationFactory()", GetFactoryFullName(type));
        }

        protected string GetFactoryFullName(ClassDefinition type)
        {
            if (type.HasCustomFactory)
            {
                return AsCppType(type.FactoryTypeName);
            }
            return GetBaseFactoryFullName(type);
        }

        protected TypeReference MakeInReference(TypeDefinition type, string name = null)
        {
            return new TypeReference(type)
            {
                Name = name
            };
        }

        protected TypeReference MakeReturnReference(TypeDefinition type, string name = null)
        {
            return new TypeReference(type)
            {
                Name = name,
                IsReturnType = true
            };
        }

        protected string GetPInvokeArgumentListAsString(MethodDefinition method)
        {
            StringBuilder builder = new StringBuilder();

            bool first = true;
            foreach (ParameterDefinition parameter in method.Parameters)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                first = false;
                builder.Append(AsCppType(parameter.ParameterType.CoreParameterName));
            }

            if (method.ReturnType != null && !method.ReturnType.IsVoid)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                builder.Append('&');
                builder.Append(AsCppType(method.ReturnType.CoreParameterName));
            }

            return builder.ToString();
        }

        protected string GetArgumentListAsString(ConstructorDefinition ctor)
        {
            StringBuilder builder = new StringBuilder();
            if (ctor.Parameters.Any())
            {
                builder.Append(GetArgumentListAsString(ctor.Parameters));
                builder.Append(", ");
            }

            if (!ctor.DeclaringClass.IsSealed)
            {
                builder.Append("pOuter, ppInner, ");
            }

            builder.Append(MakeReturnReference(ctor.DeclaringType, "instance").AbiReturnParameterName);
            return builder.ToString();
        }

        protected string GetArgumentListAsString(IEnumerable<ParameterDefinition> parameters, TypeReference returnType = null)
        {
            StringBuilder builder = new StringBuilder();

            bool first = true;
            foreach (ParameterDefinition parameter in parameters)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                first = false;
                builder.Append(AsCppType(parameter.ParameterType.AbiArgumentName));
            }

            if (returnType != null && !returnType.IsVoid)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                builder.Append(AsCppType(returnType.AbiReturnArgumentName));
            }

            return builder.ToString();
        }

        protected string GetConstructorParameterListAsString(ConstructorDefinition ctor)
        {
            StringBuilder builder = new StringBuilder();
            if (ctor.Parameters.Any())
            {
                builder.Append(GetParameterListAsString(ctor.Parameters));
                builder.Append(", ");
            }

            if (!ctor.DeclaringClass.IsSealed)
            {
                builder.Append("_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, ");
            }

            builder.Append(AsCppType(MakeReturnReference(ctor.DeclaringType, "instance").AnnotatedAbiReturnParameterName));
            return builder.ToString();
        }

        protected string GetParameterListAsString(IEnumerable<ParameterDefinition> parameters, TypeReference returnType = null)
        {
            StringBuilder builder = new StringBuilder();

            bool first = true;
            foreach (ParameterDefinition parameter in parameters)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                first = false;
                builder.Append(AsCppType(parameter.ParameterType.AnnotatedAbiParameterName));
            }

            if (returnType != null && !returnType.IsVoid)
            {
                if (!first)
                {
                    builder.Append(", ");
                }
                builder.Append(AsCppType(returnType.AnnotatedAbiReturnParameterName));
            }

            return builder.ToString();
        }
    }
}
