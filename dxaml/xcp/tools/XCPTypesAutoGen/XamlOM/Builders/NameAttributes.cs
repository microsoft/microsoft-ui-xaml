// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Overrides default names for a type or member. This attribute should be used going forward instead of 
    /// all the other name related attributes.
    /// </summary>
    [AttributeUsage(AttributeTargets.All, Inherited = false)]
    public class CustomNamesAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        public string AbiGenericArgumentName
        {
            get;
            set;
        }

        /// <summary>
        /// Many XAML primitive types as represented as dependency objects (e.g. CBoolean, CString, etc.). This lets us 
        /// specify the primitive name we use in the core to efficiently represent their values (e.g. bool, xstring_ptr, etc.).
        /// </summary>
        public string PrimitiveCoreName
        {
            get;
            set;
        }

        public string PrimitiveCppName
        {
            get;
            set;
        }

        /// <summary>
        /// Many of our code gen types aren't the real type names (example Windows.Foundation.Char16 instead of Windows.Foundation.Char). Set this on types where the names are
        /// incorrect so we produce the correct IDL.
        /// </summary>
        public string RealTypeName
        {
            get;
            set;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            if (!string.IsNullOrEmpty(AbiGenericArgumentName))
            {
                definition.AbiGenericArgumentName = AbiGenericArgumentName;
            }

            if (!string.IsNullOrEmpty(PrimitiveCoreName))
            {
                definition.PrimitiveCoreName = PrimitiveCoreName;
            }

            if (!string.IsNullOrEmpty(PrimitiveCppName))
            {
                definition.PrimitiveCppName = PrimitiveCppName;
            }

            if (!string.IsNullOrEmpty(RealTypeName))
            {
                definition.CorrectedTypeName = RealTypeName;
            }
        }
    }

    [AttributeUsage(AttributeTargets.All, Inherited = false)]
    public abstract class NameAttribute : Attribute
    {
        public string Name
        {
            get;
            protected set;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Struct, Inherited = false)]
    public class DefaultInterfaceNameAttribute : NameAttribute, NewBuilders.IClassBuilder
    {
        public DefaultInterfaceNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.IdlClassInfo.InterfaceName = Name;
        }
    }

    /// <summary>
    /// Specifies the return type parameter name. Only used for back compat.
    /// </summary>
    [AttributeUsage(AttributeTargets.Delegate | AttributeTargets.Method, Inherited = false)]
    public class ReturnTypeParameterNameAttribute : NameAttribute, NewBuilders.IDelegateBuilder, NewBuilders.IMethodBuilder
    {
        public ReturnTypeParameterNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewDelegate(OM.DelegateDefinition definition, Type source)
        {
            definition.ReturnType.Name = Name;
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.ReturnType.Name = Name;
        }
    }

    /// <summary>
    /// Specifies the count parameter name. Only used for back compat.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Parameter, Inherited = false)]
    public class CountParameterNameAttribute : NameAttribute, NewBuilders.IMethodBuilder, NewBuilders.IParameterBuilder
    {
        public CountParameterNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewParameter(OM.ParameterDefinition definition, ParameterInfo source)
        {
            definition.ParameterType.CountParameterName = Name;
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.ReturnType.CountParameterName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.All, Inherited = false)]
    public class DXamlNameAttribute :
        NameAttribute,
        NewBuilders.IPattern,
        NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder, NewBuilders.IDelegateBuilder, NewBuilders.IMethodBuilder
    {
        public string DefaultInterfaceName
        {
            get;
            private set;
        }

        public DXamlNameAttribute(string name, string defaultInterfaceName = null)
        {
            Name = name;
            DefaultInterfaceName = defaultInterfaceName;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IdlTypeInfo.Name = Name;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            NewBuilders.Helper.SetIdlNames(definition, Name);
            if (!string.IsNullOrEmpty(DefaultInterfaceName))
            {
                definition.IdlClassInfo.InterfaceName = DefaultInterfaceName;
            }
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.Name = Name;
            definition.IdlMemberInfo.Name = Name;
        }

        public void BuildNewDelegate(OM.DelegateDefinition definition, Type source)
        {
            definition.Name = Name;
            definition.IdlDelegateInfo.InterfaceName = "I" + Name;
        }
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class DXamlOverloadNameAttribute : NameAttribute, NewBuilders.IMethodBuilder
    {
        public DXamlOverloadNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.OverloadName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Struct, Inherited = false)]
    public class DXamlSystemNamespaceIndexNameAttribute : NameAttribute
    {
        public DXamlSystemNamespaceIndexNameAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public class DXamlTypeTableNameAttribute : NameAttribute, NewBuilders.ITypeBuilder
    {
        public DXamlTypeTableNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.TypeTableName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    // TODO: [Obsolete("Use ExternalIdlAttribute instead.")]
    public class DXamlInterfaceNameAttribute : NameAttribute
    {
        public DXamlInterfaceNameAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Constructor, Inherited = false)]
    public class FactoryMethodNameAttribute : NameAttribute, NewBuilders.IConstructorBuilder
    {
        public FactoryMethodNameAttribute(string name)
        {
            Name = name;
        }

        void NewBuilders.IConstructorBuilder.BuildNewConstructor(OM.ConstructorDefinition definition, ConstructorInfo source)
        {
            definition.FactoryMethodName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class NativeMethodAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public string NativeClassName
        {
            get;
            private set;
        }

        public string NativeMethodName
        {
            get;
            private set;
        }

        public NativeMethodAttribute(string nativeClassName, string nativeMethodName)
        {
            NativeClassName = nativeClassName;
            NativeMethodName = nativeMethodName;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.MethodName = NativeClassName + "::" + NativeMethodName;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Property | AttributeTargets.Event, Inherited = false)]
    public class NativeNameAttribute : NameAttribute,
        NewBuilders.ITypeBuilder,
        NewBuilders.IClassBuilder,
        NewBuilders.IEnumBuilder
    {
        public NativeNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            if (!(definition is OM.EnumDefinition))
            {
                definition.CoreName = Name;
            }
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            // For now, all classes with [NativeName] are implemented by hand.
            definition.GenerateInCore = false;
        }

        public void BuildNewEnum(OM.EnumDefinition definition, Type source)
        {
            definition.UIAName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum, Inherited = false)]
    public class TypeTableNameAttribute : NameAttribute
    {
        public TypeTableNameAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Field, Inherited = false)]
    public class NativeValueNameAttribute : NameAttribute, NewBuilders.IEnumValueBuilder
    {
        public NativeValueNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewEnumValue(OM.EnumValueDefinition definition, FieldInfo source)
        {
            definition.UIAName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public class NativeInteropNameAttribute : NameAttribute
    {
        public NativeInteropNameAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Enum, Inherited = false)]
    public class NativeValueNamespaceAttribute : NameAttribute
    {
        public NativeValueNamespaceAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class NativeClassNameAttribute : NameAttribute
    {
        public NativeClassNameAttribute(string name)
        {
            Name = name;
        }
    }

    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class NativeCreationMethodNameAttribute : NameAttribute, NewBuilders.IClassBuilder, NewBuilders.IOrderedBuilder
    {
        public NativeCreationMethodNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.CoreCreationMethodName = Name;
        }

        public int Order
        {
            get;
            set;
        }
    }

    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class OffsetFieldNameAttribute :
        NameAttribute,
        NewBuilders.IPropertyBuilder
    {
        public OffsetFieldNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.FieldName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class RenderDirtyFlagClassNameAttribute :
        NameAttribute,
        NewBuilders.IPropertyBuilder
    {
        public RenderDirtyFlagClassNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            ((OM.DependencyPropertyDefinition)definition).RenderDirtyFlagClassName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class RenderDirtyFlagMethodNameAttribute :
        NameAttribute,
        NewBuilders.IPropertyBuilder
    {
        public RenderDirtyFlagMethodNameAttribute(string name)
        {
            Name = name;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            ((OM.DependencyPropertyDefinition)definition).RenderDirtyFlagMethodName = Name;
        }
    }

    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class StorageGroupNamesAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public string EnsureMethodName
        {
            get;
            private set;
        }

        public string ClassName
        {
            get;
            private set;
        }

        public string FieldName
        {
            get;
            private set;
        }

        public StorageGroupNamesAttribute(string ensureMethodName, string className, string fieldName)
        {
            EnsureMethodName = ensureMethodName;
            ClassName = className;
            FieldName = fieldName;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.StorageGroupEnsureMethodName = EnsureMethodName;
            definition.StorageGroupOffsetClassName = ClassName;
            definition.StorageGroupOffsetFieldName = FieldName;
        }
    }
}
