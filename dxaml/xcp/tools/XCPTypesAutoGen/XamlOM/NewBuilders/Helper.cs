// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Text;

namespace XamlOM.NewBuilders
{
    internal static class Helper
    {
        private static IEnumerable<Assembly> assemblies = Enumerable.Empty<Assembly>();

        internal static void Initialize(IEnumerable<Assembly> newAssemblies)
        {
            assemblies = assemblies.Concat(newAssemblies);
        }

        private static Dictionary<Type, Type> s_typeMapping = new Dictionary<Type, Type>()
        {
            { typeof(Object), typeof(Windows.Foundation.Object) },
            { typeof(Boolean), typeof(Windows.Foundation.Boolean) },
            { typeof(Byte), typeof(Windows.Foundation.Byte) },
            { typeof(Char), typeof(Windows.Foundation.Char16) },
            { typeof(Uri), typeof(Windows.Foundation.Uri) },
            { typeof(String), typeof(Windows.Foundation.String) },
            { typeof(Int32), typeof(Windows.Foundation.Int32) },
            { typeof(Int64), typeof(Windows.Foundation.Int64) },
            { typeof(Single), typeof(Windows.Foundation.Float) },
            { typeof(Double), typeof(Windows.Foundation.Double) },
            { typeof(UInt32), typeof(Windows.Foundation.UInt32) },
            { typeof(UInt16), typeof(Windows.Foundation.UInt16) },
            { typeof(UInt64), typeof(Windows.Foundation.UInt64) },
            { typeof(DateTime), typeof(Windows.Foundation.DateTime) },
            { typeof(TimeSpan), typeof(Windows.Foundation.TimeSpan) },
            { typeof(Type), typeof(Windows.UI.Xaml.Interop.TypeName) },
            { typeof(Nullable<>), typeof(Windows.Foundation.IReference<>) }
        };

        private static Dictionary<Type, string> s_typeNameMapping = new Dictionary<Type, string>()
        {
            { typeof(Windows.Foundation.Byte), "Windows.Foundation.UInt8" },
            { typeof(Windows.Foundation.Char16), "Windows.Foundation.Char" },
            { typeof(Windows.Foundation.Float), "Windows.Foundation.Single" },
        };

        internal static IEnumerable<TAttribute> GetCustomAttributes<TAttribute>(this Assembly assembly) where TAttribute : class
        {
            return (TAttribute[])assembly.GetCustomAttributes(typeof(TAttribute), false);
        }

        internal static TAttribute[] GetCustomAttributes<TAttribute>(this ICustomAttributeProvider attributeProvider) where TAttribute : class
        {
            return (TAttribute[])attributeProvider.GetCustomAttributes(typeof(TAttribute), false);
        }

        internal static TAttribute GetCustomAttribute<TAttribute>(this ICustomAttributeProvider attributeProvider) where TAttribute : class
        {
            return (TAttribute)attributeProvider.GetCustomAttributes(typeof(TAttribute), false).SingleOrDefault();
        }

        internal static TypeReference GetTypeRef(Type type, int version = 1)
        {
            if (type == typeof(void))
            {
                return new TypeReference(null)
                {
                    IsVoid = true
                };
            }

            TypeReference typeRef = new TypeReference(null);
            Type typeToUse = type;

            if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                typeRef.IsNullable = true;
                typeToUse = type.GetGenericArguments()[0];
            }
            else if (type.IsArray)
            {
                typeRef.IsArray = true;
                typeToUse = type.GetElementType();
            }
            else if (type.IsByRef)
            {
                typeRef.IsOut = true;
                typeToUse = type.GetElementType();
            }
            else if (type == typeof(IntPtr))
            {
                typeRef.IsIntPtr = true;
                typeToUse = typeof(int);
            }

            typeRef.Type = ModelFactory.GetOrCreateType(typeToUse);
            IdlTypeAttribute idlTypeAtt = GetCustomAttribute<IdlTypeAttribute>(typeToUse);
            if (idlTypeAtt != null)
            {
                typeRef.IdlInfo.Type = ModelFactory.GetOrCreateType(idlTypeAtt.IdlType);
            }
            else
            {
                typeRef.IdlInfo.Type = typeRef.Type;
            }
            typeRef.Version = version;
            return typeRef;
        }

        internal static void ApplyAttributes<TBuilder>(ICustomAttributeProvider attributeProvider, Action<TBuilder> builder)
        {
            // First execute all the IPatterns.
            foreach (TBuilder att in attributeProvider.GetCustomAttributes(true).OfType<IPattern>().OfType<TBuilder>().OrderBy(att => GetOrder(att)))
            {
                builder(att);
            }

            // Then execute the non-IPatterns.
            foreach (TBuilder att in attributeProvider.GetCustomAttributes(false).Where(b => !(b is IPattern)).OfType<TBuilder>().OrderBy(att => GetOrder(att)))
            {
                builder(att);
            }
        }

        internal static void ApplyAttributes<TBuilder1, TBuilder2>(Type attributeProvider, Action<TBuilder1> builder1, Action<TBuilder2> builder2)
        {
            object[] typeAttributes = attributeProvider.GetCustomAttributes(false);

            // First execute all the IPatterns, starting with the base types.
            ForEachBaseType(attributeProvider, baseType =>
                {
                    object[] baseTypeAttributes = baseType.GetCustomAttributes(false);
                    foreach (TBuilder1 att in baseTypeAttributes.OfType<IPattern>().OfType<TBuilder1>().OrderBy(att => GetOrder(att)))
                    {
                        if (baseType == attributeProvider || AllowInheritance(att))
                        {
                            builder1(att);
                        }
                    }

                    foreach (TBuilder2 att in baseTypeAttributes.OfType<IPattern>().OfType<TBuilder2>().OrderBy(att => GetOrder(att)))
                    {
                        if (baseType == attributeProvider || AllowInheritance(att))
                        {
                            builder2(att);
                        }
                    }
                });

            // Then execute the non-IPatterns.
            foreach (TBuilder1 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder1>().OrderBy(att => GetOrder(att)))
            {
                builder1(att);
            }

            foreach (TBuilder2 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder2>().OrderBy(att => GetOrder(att)))
            {
                builder2(att);
            }
        }

        internal static void ApplyAttributes<TBuilder1, TBuilder2>(ICustomAttributeProvider attributeProvider, Action<TBuilder1> builder1, Action<TBuilder2> builder2)
        {
            object[] allAttributes = attributeProvider.GetCustomAttributes(true);
            object[] typeAttributes = attributeProvider.GetCustomAttributes(false);

            // First execute all the IPatterns.
            foreach (TBuilder1 att in allAttributes.OfType<IPattern>().OfType<TBuilder1>())
            {
                builder1(att);
            }

            foreach (TBuilder2 att in allAttributes.OfType<IPattern>().OfType<TBuilder2>())
            {
                builder2(att);
            }

            // Then execute the non-IPatterns.
            foreach (TBuilder1 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder1>())
            {
                builder1(att);
            }

            foreach (TBuilder2 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder2>())
            {
                builder2(att);
            }
        }

        internal static void ApplyAttributes<TBuilder1, TBuilder2, TBuilder3>(ICustomAttributeProvider attributeProvider, Action<TBuilder1> builder1, Action<TBuilder2> builder2, Action<TBuilder3> builder3)
        {
            object[] allAttributes = attributeProvider.GetCustomAttributes(true);
            object[] typeAttributes = attributeProvider.GetCustomAttributes(false);

            // First execute all the IPatterns.
            foreach (TBuilder1 att in allAttributes.OfType<IPattern>().OfType<TBuilder1>())
            {
                builder1(att);
            }

            foreach (TBuilder2 att in allAttributes.OfType<IPattern>().OfType<TBuilder2>())
            {
                builder2(att);
            }

            foreach (TBuilder3 att in allAttributes.OfType<IPattern>().OfType<TBuilder3>())
            {
                builder3(att);
            }

            // Then execute the non-IPatterns.
            foreach (TBuilder1 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder1>())
            {
                builder1(att);
            }

            foreach (TBuilder2 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder2>())
            {
                builder2(att);
            }

            foreach (TBuilder3 att in typeAttributes.Where(b => !(b is IPattern)).OfType<TBuilder3>())
            {
                builder3(att);
            }
        }

        internal static Modifier GetModifier(MethodBase method)
        {
            if (method.IsPrivate)
            {
                return Modifier.Private;
            }

            if (method.IsFamily)
            {
                return Modifier.Protected;
            }

            if (method.IsAssembly || method.IsFamilyOrAssembly || method.IsFamilyAndAssembly)
            {
                return Modifier.Internal;
            }

            if (method.IsPublic)
            {
                return Modifier.Public;
            }

            throw new NotSupportedException(method.Name);
        }

        internal static Modifier GetModifier(Type type)
        {
            if (type.IsPublic)
            {
                return Modifier.Public;
            }
            else
            {
                return Modifier.Internal;
            }
        }

        internal static bool GetDefaultBoolValue(PropertyInfo property)
        {
            DefaultValueAttribute defaultValueAtt = GetCustomAttribute<DefaultValueAttribute>(property);
            if (defaultValueAtt != null)
            {
                return (bool)defaultValueAtt.Value;
            }
            return false;
        }

        internal static void ApplyFlagProperties<TFlagsAttribute, TDefinition>(TFlagsAttribute flags, TDefinition definition)
        {
            foreach (PropertyInfo property in typeof(TFlagsAttribute).GetProperties(BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.Instance))
            {
                bool value = (bool)property.GetValue(flags, null);

                if (value != GetDefaultBoolValue(property))
                {
                    PropertyInfo targetProperty = typeof(TDefinition).GetProperty(property.Name);
                    if (targetProperty != null)
                    {
                        targetProperty.SetValue(definition, value, null);
                    }
                }
            }
        }

        private static bool AllowInheritance(object attribute)
        {
            AttributeUsageAttribute usage = GetCustomAttribute<AttributeUsageAttribute>(attribute.GetType());
            if (usage != null)
            {
                return usage.Inherited;
            }

            // By default, inheritance is allowed.
            return true;
        }

        private static void ForEachBaseType(Type type, Action<Type> action)
        {
            if (type.BaseType != null && type.BaseType != typeof(object))
            {
                ForEachBaseType(type.BaseType, action);
            }

            action(type);
        }

        internal static Type NormalizeType(Type type)
        {
            Type normalizedType;
            if (s_typeMapping.TryGetValue(type, out normalizedType))
            {
                return normalizedType;
            }
            return type;
        }

        internal static string NormalizeTypeFullName(Type normalizedType)
        {
            string normalizedTypeName;
            if (s_typeNameMapping.TryGetValue(normalizedType, out normalizedTypeName))
            {
                return normalizedTypeName;
            }
            return normalizedType.FullName ?? normalizedType.Name;
        }
        internal static string NormalizeTypeName(Type normalizedType)
        {
            string normalizedTypeName;
            if (s_typeNameMapping.TryGetValue(normalizedType, out normalizedTypeName))
            {
                return normalizedTypeName.Substring(normalizedTypeName.LastIndexOf(".") + 1);
            }
            return normalizedType.Name;
        }

        internal static string GetImplementedMemberIndexName(ClassDefinition declaringType, MemberDefinition member)
        {
            StringBuilder builder = new StringBuilder();
            if (member is PropertyDefinition || member is EventDefinition)
            {
                builder.Append("PROPERTY");
            }
            else if (member is MethodDefinition)
            {
                builder.Append("METHOD");
            }

            builder.Append(declaringType.IndexName);
            builder.Append('_');
            builder.Append(member.Name.ToUpper());
            return builder.ToString();
        }

        internal static void SetIdlNames(ClassDefinition definition, string name)
        {
            definition.IdlClassInfo.FactoryInterfaceName = $"I{name}Factory";
            definition.IdlClassInfo.InterfaceName = $"I{name}";
            definition.IdlClassInfo.ProtectedMembersInterfaceName = $"I{name}Protected";
            definition.IdlClassInfo.StaticMembersInterfaceName = $"I{name}Statics";
            definition.IdlClassInfo.VirtualMembersInterfaceName = $"I{name}Overrides";
        }

        internal static void EnsureDefaultPlatforms(TypeDefinition definition, Type type)
        {
            // If we don't have any explicit platform attributes for version 1, then we'll assume version 1 was in UWP from the beginning
            if (!GetCustomAttributes<PlatformAttribute>(type).Any(p => p.Version == 1))
            {
                if (definition.DeclaringNamespace.Name.StartsWith("Microsoft.UI.Xaml") || definition.DeclaringNamespace.Name.StartsWith("Windows.UI.Xaml"))
                {
                    // By default, we support the first OS version where WUX ship
                    var winAtt = new PlatformAttribute(typeof(Microsoft.UI.Xaml.WinUIContract), OSVersions.InitialUAPVersion);

                    winAtt.BuildNewType(definition, type);

                    var definitionAsClass = definition as ClassDefinition;
                    if (definitionAsClass != null)
                    {
                        winAtt.BuildNewClass(definitionAsClass, type);
                    }
                }
            }
        }

        internal static Type GetClrType(TypeDefinition type)
        {
            if (!assemblies.Any())
            {
                throw new InvalidOperationException("Helpers.Initialize has not been called");
            }

            var typeName = type.GenericClrFullName;
            var result = Type.GetType(
                typeName,
                (asmName) => typeof(Helper).Assembly,
                (asm, name, casing) =>
                    assemblies
                        .Select(a => a.GetType(name, casing))
                        .FirstOrDefault(t => t != null));

            if (result == null)
            {
                throw new TypeLoadException(typeName);
            }

            return result;
        }

        internal static void SetContentProperty(ClassDefinition type, DependencyPropertyDefinition property, bool isImplicitContentProperty = false)
        {
            type.ContentProperty = property;
            property.IsContentProperty = true;
            property.IsImplicitContentProperty = isImplicitContentProperty;
        }


        internal static TypeDefinition GetCollectionElementType(ClassDefinition collectionType)
        {
            // Assume the base class is PresentationFrameworkCollection<T>.
            return collectionType.BaseClass.GenericArguments[0].Type;
        }

        internal static ClassDefinition GetCollectionType(CollectionKind collectionType, TypeDefinition elementType)
        {
            switch (collectionType)
            {
                case OM.CollectionKind.Enumerable:
                    return ModelFactory.GetOrCreateClass(typeof(Windows.Foundation.Collections.IIterable<>).MakeGenericType(GetClrType(elementType)));
                case OM.CollectionKind.Indexable:
                    return ModelFactory.GetOrCreateClass(typeof(Windows.Foundation.Collections.IVectorView<>).MakeGenericType(GetClrType(elementType)));
                case OM.CollectionKind.Observable:
                    return ModelFactory.GetOrCreateClass(typeof(Windows.Foundation.Collections.IObservableVector<>).MakeGenericType(GetClrType(elementType)));
                case OM.CollectionKind.Vector:
                    return ModelFactory.GetOrCreateClass(typeof(Windows.Foundation.Collections.IVector<>).MakeGenericType(GetClrType(elementType)));
                default:
                    throw new NotSupportedException("Invalid CollectionKind.");
            }
        }

        internal static void FixUpAttributeProperty(PropertyDefinition pd, PropertyInfo source)
        {
            if (source.PropertyType == typeof(Type))
            {
                pd.PropertyType.IdlInfo.AttributePropertyName = "type";
            }
            else
            {
                pd.PropertyType.IdlInfo.AttributePropertyName = pd.PropertyType.IdlInfo.GenericFullName;
            }
        }

        internal static TypeReference GetPropertyTypeRef(Type type)
        {
            TypeReference typeRef = GetTypeRef(type);
            typeRef.CountParameterName = "count";
            typeRef.Name = "value";
            return typeRef;
        }

        internal static TypeReference GetEventHandlerTypeRef(Type type)
        {
            TypeReference typeRef = GetTypeRef(type);
            typeRef.Name = "value";
            return typeRef;
        }

        internal static TypeReference GetDPTargetTypeRef(Type type)
        {
            TypeReference typeRef = GetTypeRef(type);
            typeRef.Name = "element";
            return typeRef;
        }

        internal static TypeReference GetReturnTypeRef(Type type, bool v2CodeGen = false)
        {
            // v2CodeGen of true means use the RS4+ name corrections

            TypeReference typeRef = GetTypeRef(type);
            typeRef.CountParameterName = v2CodeGen ? "resultCount" : "returnValueCount";
            typeRef.IsReturnType = true;
            typeRef.Name = v2CodeGen ? "result" : "returnValue";
            return typeRef;
        }

        internal static TypeReference GetStructHelperTargetTypeRef(Type type)
        {
            TypeReference typeRef = GetTypeRef(type);
            typeRef.Name = "target";
            return typeRef;
        }

        internal static bool IsStructPropertyHelper(PropertyInfo property)
        {
            if (property.DeclaringType.IsValueType)
            {
                PropertyFlagsAttribute propertyFlags = Helper.GetCustomAttribute<PropertyFlagsAttribute>(property);
                if (propertyFlags != null && propertyFlags.IsHelper)
                {
                    MethodInfo getMethod = property.GetGetMethod(true);
                    if (!getMethod.IsStatic)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        internal static TypeReference ImplementInterface(ClassDefinition definition, Type clrInterfaceTypeToImplement, int version = 1)
        {
            TypeReference typeRef = NewBuilders.Helper.GetTypeRef(clrInterfaceTypeToImplement, version);
            ImplementInterface(definition, typeRef);
            return typeRef;
        }

        private static void ImplementInterface(ClassDefinition declaringType, TypeReference interfaceReference)
        {
            declaringType.ImplementedInterfaces.Add(interfaceReference);

            ClassDefinition interfaceToImplement = (ClassDefinition)interfaceReference.Type;

            // Only implicitly implement interfaces for classes.
            if (!declaringType.IsInterface)
            {
                // Copy interface members to class.
                foreach (EventDefinition interfaceEvent in interfaceToImplement.Events.Where(m => !declaringType.AllEvents.Any(existingEvent => existingEvent.Name == m.Name)))
                {
                    EventDefinition eventDefinition = ImplementEvent(declaringType, interfaceEvent);
                    eventDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        eventDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(eventDefinition.Version))
                    {
                        eventDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(eventDefinition.Version);
                    }
                    declaringType.AllEvents.Add(eventDefinition);
                }

                foreach (PropertyDefinition interfaceProperty in interfaceToImplement.Properties.Where(p => !declaringType.AllProperties.Any(existingProperty => existingProperty.Name == p.Name)))
                {
                    PropertyDefinition propertyDefinition = ImplementProperty(declaringType, interfaceProperty);
                    propertyDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        propertyDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(propertyDefinition.Version))
                    {
                        propertyDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(propertyDefinition.Version);
                    }
                    declaringType.AllProperties.Add(propertyDefinition);
                }

                foreach (MethodDefinition interfaceMethod in interfaceToImplement.Methods.Where(m => !declaringType.AllMethods.Any(existingMethod => existingMethod.Name == m.Name && existingMethod.Parameters.Count == m.Parameters.Count)))
                {
                    MethodDefinition methodDefinition = ImplementMethod(declaringType, interfaceMethod);
                    methodDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        methodDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(methodDefinition.Version))
                    {
                        methodDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(methodDefinition.Version);
                    }
                    declaringType.AllMethods.Add(methodDefinition);
                }

                // If the interface we're implementing requires other interfaces, implement those as well.
                foreach (TypeReference requiredInterface in interfaceToImplement.ImplementedInterfaces)
                {
                    TypeReference implicitInterface = ImplementInterface(declaringType, GetClrType(requiredInterface.Type), interfaceReference.Version);
                    implicitInterface.IdlInfo.IsExcluded = true;
                }
            }
        }

        internal static TypeReference ImplementStaticInterface(ClassDefinition definition, Type clrInterfaceTypeToImplement, int version = 1)
        {
            TypeReference typeRef = NewBuilders.Helper.GetTypeRef(clrInterfaceTypeToImplement, version);
            ImplementStaticInterface(definition, typeRef);
            return typeRef;
        }

        private static void ImplementStaticInterface(ClassDefinition declaringType, TypeReference interfaceReference)
        {
            declaringType.FactoryImplementedInterfaces.Add(interfaceReference);

            // Only implicitly implement interfaces for classes.
            if (!declaringType.IsInterface)
            {
                ClassDefinition interfaceToImplement = (ClassDefinition)interfaceReference.Type;

                // Copy interface members to class as statics.
                // Exclude them from public IDL if interface is PrivateIdl.
                foreach (EventDefinition interfaceEvent in interfaceToImplement.Events.Where(e => !declaringType.AllEvents.Any(existingEvent => existingEvent.Name == e.Name)))
                {
                    EventDefinition eventDefinition = ImplementEvent(declaringType, interfaceEvent);
                    eventDefinition.IsStatic = true;
                    eventDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        eventDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(eventDefinition.Version))
                    {
                        eventDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(eventDefinition.Version);
                    }
                    declaringType.AllEvents.Add(eventDefinition);
                }

                foreach (PropertyDefinition interfaceProperty in interfaceToImplement.Properties.Where(p => !declaringType.AllProperties.Any(existingProperty => existingProperty.Name == p.Name)))
                {
                    PropertyDefinition propertyDefinition = ImplementProperty(declaringType, interfaceProperty);
                    propertyDefinition.IsStatic = true;
                    propertyDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        propertyDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(propertyDefinition.Version))
                    {
                        propertyDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(propertyDefinition.Version);
                    }
                    declaringType.AllProperties.Add(propertyDefinition);
                }

                foreach (MethodDefinition interfaceMethod in interfaceToImplement.Methods.Where(m => !declaringType.AllMethods.Any(existingMethod => existingMethod.Name == m.Name && existingMethod.Parameters.Count == m.Parameters.Count)))
                {
                    MethodDefinition methodDefinition = ImplementMethod(declaringType, interfaceMethod);
                    methodDefinition.IsStatic = true;
                    methodDefinition.Version = interfaceReference.Version;
                    if (interfaceToImplement.IdlTypeInfo.IsPrivateIdlOnly)
                    {
                        methodDefinition.IdlMemberInfo.IsExcluded = true;
                    }

                    if (VelocityFeatures.IsVelocityVersion(methodDefinition.Version))
                    {
                        methodDefinition.VelocityFeatureName = VelocityFeatures.GetFeatureName(methodDefinition.Version);
                    }
                    declaringType.AllMethods.Add(methodDefinition);
                }
            }
        }

        private static PropertyDefinition ImplementProperty(ClassDefinition declaringType, PropertyDefinition interfaceProperty)
        {
            PropertyDefinition implementedProperty;

            AttachedPropertyDefinition attachedPropertyToImplement = interfaceProperty as AttachedPropertyDefinition;
            if (attachedPropertyToImplement != null)
            {
                // Implement once we use this.
                throw new NotImplementedException();
            }
            else
            {
                DependencyPropertyDefinition dpToImplement = interfaceProperty as DependencyPropertyDefinition;
                if (dpToImplement != null)
                {
                    DependencyPropertyDefinition dp = new DependencyPropertyDefinition()
                    {
                        DependencyPropertyModifier = dpToImplement.DependencyPropertyModifier,
                        InterfaceMember = dpToImplement,
                        IsReadOnly = dpToImplement.IsReadOnly,
                        IsStatic = dpToImplement.IsStatic,
                        IsVirtual = dpToImplement.IsVirtual,
                        Modifier = dpToImplement.Modifier,
                        Name = dpToImplement.Name,
                        PropertyType = dpToImplement.PropertyType,
                        SetterModifier = dpToImplement.SetterModifier
                    };
                    implementedProperty = dp;
                }
                else
                {
                    implementedProperty = new PropertyDefinition();
                }
            }

            CopyMemberData(declaringType, interfaceProperty, implementedProperty);
            implementedProperty.FieldName = interfaceProperty.FieldName;
            implementedProperty.FrameworkFieldName = interfaceProperty.FrameworkFieldName;
            implementedProperty.IsReadOnly = interfaceProperty.IsReadOnly;
            implementedProperty.PropertyType = interfaceProperty.PropertyType;
            implementedProperty.SetterModifier = interfaceProperty.SetterModifier;
            implementedProperty.IdlPropertyInfo.IsReadOnly = interfaceProperty.IdlPropertyInfo.IsReadOnly;
            implementedProperty.XamlPropertyFlags = interfaceProperty.XamlPropertyFlags;
            return implementedProperty;
        }

        private static EventDefinition ImplementEvent(ClassDefinition declaringType, EventDefinition interfaceEvent)
        {
            EventDefinition implementedEvent = new EventDefinition();
            CopyMemberData(declaringType, interfaceEvent, implementedEvent);
            implementedEvent.EventHandlerType = interfaceEvent.EventHandlerType;
            implementedEvent.IsRouted = interfaceEvent.IsRouted;
            implementedEvent.GenerateDefaultBody = declaringType.IsADependencyObject;
            implementedEvent.XamlEventFlags = interfaceEvent.XamlEventFlags;
            return implementedEvent;
        }

        private static MethodDefinition ImplementMethod(ClassDefinition declaringType, MethodDefinition interfaceMethod)
        {
            MethodDefinition implementedMethod = new MethodDefinition();
            CopyMemberData(declaringType, interfaceMethod, implementedMethod);
            implementedMethod.IsDefaultOverload = interfaceMethod.IsDefaultOverload;
            implementedMethod.Parameters.AddRange(interfaceMethod.Parameters);
            implementedMethod.ReturnType = interfaceMethod.ReturnType;
            implementedMethod.GenerateDefaultBody = false;
            return implementedMethod;
        }

        private static void CopyMemberData(ClassDefinition declaringType, MemberDefinition interfaceMember, MemberDefinition implementedMember)
        {
            implementedMember.AllowCrossThreadAccess = interfaceMember.AllowCrossThreadAccess;
            implementedMember.Comment = interfaceMember.Comment;
            implementedMember.DeclaringType = declaringType;
            implementedMember.Deprecations.AddRange(interfaceMember.Deprecations);
            implementedMember.InterfaceMember = interfaceMember;
            implementedMember.IdlMemberInfo.IsExcluded = interfaceMember.IdlMemberInfo.IsExcluded;
            implementedMember.IsStatic = interfaceMember.IsStatic;
            implementedMember.IsVirtual = interfaceMember.IsVirtual;
            implementedMember.Modifier = interfaceMember.Modifier;
            implementedMember.Name = interfaceMember.Name;
            implementedMember.IdlMemberInfo.Name = interfaceMember.IdlMemberInfo.Name;
            implementedMember.GenerateDefaultBody = interfaceMember.GenerateDefaultBody;
            implementedMember.GenerateStub = interfaceMember.GenerateStub;
        }

        internal static ClassDefinition GetBaseClassInIdl(ClassDefinition definition)
        {
            ClassDefinition baseClass = definition.BaseClass;
            while (baseClass != null && baseClass.IdlTypeInfo.IsExcluded)
            {
                baseClass = baseClass.IdlClassInfo.BaseClass;
            }
            return baseClass;
        }

        internal static bool IsAObject(TypeDefinition type)
        {
            return !type.IsValueType;
        }

        private static int GetOrder(object att)
        {
            IOrderedBuilder orderedAtt = att as IOrderedBuilder;
            return (orderedAtt != null) ? orderedAtt.Order : 0;
        }

        internal static string GetFieldName(PropertyInfo property)
        {
            int dimensions = 0;
            if (!property.PropertyType.IsValueType)
            {
                dimensions++;
            }
            return "m_" + ToPointerName(ToCamelCase(property.Name), dimensions);
        }

        internal static string ToPointerName(string name, int dimensions = 1)
        {
            if (dimensions == 0)
            {
                return name;
            }

            return new string('p', dimensions) + ToPascalCase(name);
        }

        private static string ToPascalCase(string str)
        {
            if (str.Length > 1)
            {
                return char.ToUpper(str[0]) + str.Substring(1);
            }
            return str;
        }

        private static string ToCamelCase(string str)
        {
            if (str.Length > 2 && char.IsLower(str[1]))
            {
                return char.ToLower(str[0]) + str.Substring(1);
            }
            return str;
        }

        internal static string GetDefaultTypeTableName(Type type)
        {
            if (type.IsGenericType)
            {
                return string.Empty;
            }

            return type.Name;
        }

        internal static bool IsADependencyObject(TypeDefinition type)
        {
            if (type.IsPrimitive)
            {
                return false;
            }

            ClassDefinition typeAsClass = type as ClassDefinition;
            if (typeAsClass != null)
            {
                return typeAsClass.IsADependencyObject;
            }
            return false;
        }

        /// <summary>
        /// Determines whether the specified property may hold an object that is part of the visual tree.
        /// </summary>
        /// <param name="dp"></param>
        /// <returns></returns>
        internal static bool IsVisualTreeProperty(DependencyPropertyDefinition dp)
        {
            if (dp.PropertyType.Type.IsExcludedFromVisualTreeNodeConsideration)
            {
                return false;
            }

            if (dp.DeclaringType.IsValueType && dp.IsContentProperty)
            {
                return false;
            }

            if (dp.XamlPropertyFlags.IsExcludedFromVisualTree)
            {
                return false;
            }

            if (dp.XamlPropertyFlags.IsForcedIntoVisualTree)
            {
                return true;
            }

            return Helper.IsADependencyObject(dp.PropertyType.Type);
        }

        /// <summary>
        /// Determines whether the specified property may hold an object (CValue) or dependency object.
        /// This is different from the VisualTreeProperty because these properties need not be realized
        /// as part of the VisualTree (eg. Properties on ExternalObjectReference).
        /// </summary>
        /// <param name="dp"></param>
        /// <returns></returns>
        internal static bool IsObjectOrDependencyObjectTreeProperty(DependencyPropertyDefinition dp)
        {
            if (dp.PropertyType.Type.IsExcludedFromReferenceTrackerWalk)
            {
                return false;
            }

            if (dp.DeclaringType.IsValueType && dp.IsContentProperty)
            {
                return false;
            }

            return Helper.IsADependencyObject(dp.PropertyType.Type) || dp.PropertyType.Type.IsObjectType;
        }

        internal static void SetCollectionInfo(ClassDefinition definition, Type type)
        {
            var vectorTypeArguments = GetCollectionTypes(definition, new List<string> { "IVector", "IObservableVector" });
            var mapsTypeArguments = GetCollectionTypes(definition, new List<string> { "IMap" });

            if (vectorTypeArguments != null)
            {
                Debug.Assert(vectorTypeArguments.Length == 1);
                definition.VectorGenericArgument = vectorTypeArguments[0];
            }

            if (mapsTypeArguments != null)
            {
                Debug.Assert(mapsTypeArguments.Length == 2);
                definition.MapGenericArguments = new KeyValuePair<TypeReference, TypeReference>(
                    mapsTypeArguments[0],
                    mapsTypeArguments[1]);
            }


        }

        private static TypeReference[] GetCollectionTypes(ClassDefinition definition, List<string> collectionTypeNames)
        {
            TypeReference[] types = null;

            if (definition.IsGenericType &&
                collectionTypeNames.Contains(definition.Name))
            {
                types = definition.GenericArguments.ToArray();
            }
            else
            {
                foreach (var implementedInterface in definition.ImplementedInterfaces)
                {
                    types = GetCollectionTypes((ClassDefinition)implementedInterface.Type, collectionTypeNames);
                    if (types != null) break;
                }
            }

            return types;
        }
    }
}
