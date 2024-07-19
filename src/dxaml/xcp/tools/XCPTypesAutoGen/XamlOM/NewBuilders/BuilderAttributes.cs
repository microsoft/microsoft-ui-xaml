// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM.NewBuilders
{
    public interface IPattern
    {
    }

    public interface INamespaceBuilder
    {
        void BuildNewNamespace(NamespaceDefinition definition);
    }

    public interface ITypeBuilder
    {
        void BuildNewType(TypeDefinition definition, Type source);
    }

    public interface IContractBuilder
    {
        void BuildContract(ContractDefinition definition, Type source);
    }

    public interface IClassBuilder
    {
        void BuildNewClass(ClassDefinition definition, Type source);
    }

    public interface IEndClassBuilder
    {
        void BuildEndClass(ClassDefinition definition, Type source);
    }

    public interface IAttributeBuilder
    {
        void BuildNewAttribute(AttributeDefinition definition, Type source);
    }

    public interface IDelegateBuilder
    {
        void BuildNewDelegate(DelegateDefinition definition, Type source);
    }

    public interface IEnumBuilder
    {
        void BuildNewEnum(EnumDefinition definition, Type source);
    }

    public interface IEnumValueBuilder
    {
        void BuildNewEnumValue(EnumValueDefinition definition, FieldInfo source);
    }

    public interface IConstructorBuilder
    {
        void BuildNewConstructor(ConstructorDefinition definition, ConstructorInfo source);
    }

    public interface IMemberBuilder
    {
        void BuildNewMember(MemberDefinition definition, MemberInfo source);
    }

    public interface IAttachedPropertyBuilder
    {
        void BuildNewProperty(AttachedPropertyDefinition definition, PropertyInfo source);
    }

    public interface IPropertyBuilder
    {
        void BuildNewProperty(PropertyDefinition definition, PropertyInfo source);
    }

    public interface IEventBuilder
    {
        void BuildNewEvent(EventDefinition definition, EventInfo source);
    }

    public interface IAttachedEventBuilder
    {
        void BuildNewEvent(AttachedEventDefinition definition, EventInfo source);
    }

    public interface IMethodBuilder
    {
        void BuildNewMethod(MethodDefinition definition, MethodInfo source);
    }

    public interface IParameterBuilder
    {
        void BuildNewParameter(ParameterDefinition definition, ParameterInfo source);
    }

    public interface IOrderedBuilder
    {
        int Order { get; }
    }
}
