// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace XamlOM.NewBuilders
{
    public static class ModelFactory
    {
        private static Dictionary<string, NamespaceDefinition> s_namespaces = new Dictionary<string, NamespaceDefinition>();
        private static Dictionary<Type, TypeDefinition> s_types = new Dictionary<Type, TypeDefinition>();
        private static Dictionary<Tuple<Type, int>, ContractReference> s_contractReferences = new Dictionary<Tuple<Type, int>, ContractReference>();

        public static OMContext Create(IEnumerable<Assembly> assemblies)
        {
            s_namespaces.Clear();
            s_types.Clear();
            Helper.Initialize(assemblies);

            OMContext model = new OMContext();

            // Build namespaces and types.
            foreach (NamespaceDefinition ns in GetNamespaces(assemblies))
            {
                model.Namespaces.Add(ns);
            }

            // Add declares.
            foreach (Tuple<string, Type[]> declareSection in Declares.GetDeclares(assemblies))
            {
                model.DeclareSections.Add(
                    new Tuple<string, IEnumerable<TypeReference>>(
                        declareSection.Item1,
                        declareSection.Item2.Select(t => Helper.GetTypeRef(t))));
            }

            var moduleDefs = assemblies.SelectMany(a => a.GetCustomAttributes(typeof(ModuleAttribute), false).OfType<ModuleAttribute>());
            if (moduleDefs.Count() == 1)
            {
                var modDef = moduleDefs.Single();
                model.Module = modDef.OMModule;
            }

            return model;
        }

        private static IEnumerable<NamespaceDefinition> GetNamespaces(IEnumerable<Assembly> assemblies)
        {
            Assembly coreAssembly = typeof(ModelFactory).Assembly;
            var assemblyTypes = assemblies.SelectMany(assembly => assembly.GetTypes()).ToArray();

            IEnumerable<NamespaceAttribute> namespaceAttributes = Helper.GetCustomAttributes<NamespaceAttribute>(coreAssembly);
            foreach (NamespaceAttribute nsBuilder in namespaceAttributes)
            {
                // Force namespace entries in cache.
                GetOrCreateNamespace(nsBuilder.Name);
            }

            // Create types for each namespace.
            foreach (NamespaceAttribute nsBuilder in namespaceAttributes.OrderBy(x => x.Order))
            {
                NamespaceDefinition ns = CreateTypesForNamespace(assemblyTypes, nsBuilder.Name);
                nsBuilder.BuildNewNamespace(ns);
                yield return ns;
            }

            // Lastly, add all of the types we created (including referenced types) to the right namespace.
            foreach (TypeDefinition definition in s_types
                .Where(entry => Helper.GetCustomAttribute<HideFromNewCodeGen>(entry.Key) == null)
                .Select(entry => entry.Value)
                .OrderBy(d => d.Name))
            {
                if (definition is EnumDefinition)
                {
                    definition.DeclaringNamespace.Enums.Add((EnumDefinition)definition);
                }
                else if (definition is DelegateDefinition)
                {
                    definition.DeclaringNamespace.Delegates.Add((DelegateDefinition)definition);
                }
                else if (definition is AttributeDefinition)
                {
                    definition.DeclaringNamespace.Attributes.Add((AttributeDefinition)definition);
                }
                else if (definition is ContractDefinition)
                {
                    definition.DeclaringNamespace.Contracts.Add((ContractDefinition)definition);
                }
                else
                {
                    definition.DeclaringNamespace.Classes.Add((ClassDefinition)definition);
                }
            }
        }

        private static NamespaceDefinition CreateTypesForNamespace(Type[] assemblyTypes, string namespaceName)
        {
            NamespaceDefinition ns = s_namespaces[namespaceName];
            foreach (Type type in assemblyTypes.Where(t => t.Namespace == namespaceName))
            {
                if (Helper.GetCustomAttribute<HideFromNewCodeGen>(type) != null)
                {
                    continue;
                }

                if (type.IsEnum)
                {
                    GetOrCreateEnum(type);
                }
                else if (typeof(Delegate).IsAssignableFrom(type))
                {
                    GetOrCreateDelegate(type);
                }
                else if (typeof(Attribute).IsAssignableFrom(type))
                {
                    GetOrCreateAttribute(type);
                }
                else if (typeof(Contract).IsAssignableFrom(type))
                {
                    GetOrCreateContract(type);
                }
                else
                {
                    GetOrCreateClass(type);
                }
            }

            return ns;
        }

        private static NamespaceDefinition GetOrCreateNamespace(string namespaceName)
        {
            NamespaceDefinition definition;
            if (!s_namespaces.TryGetValue(namespaceName, out definition))
            {
                definition = new NamespaceDefinition()
                {
                    Name = namespaceName
                };
                s_namespaces.Add(namespaceName, definition);
            }
            return definition;
        }

        private static T GetOrCreateType<T>(Type type, Action<T> initializer) where T : TypeDefinition, new()
        {
            TypeDefinition definition;
            if (!s_types.TryGetValue(type, out definition))
            {
                definition = new T();
                s_types.Add(type, definition);
                initializer((T)definition);
            }
            return (T)definition;
        }

        internal static TypeDefinition GetOrCreateType(Type type)
        {
            if (type.IsEnum)
            {
                return GetOrCreateEnum(type);
            }
            else if (typeof(Delegate).IsAssignableFrom(type))
            {
                return GetOrCreateDelegate(type);
            }
            else
            {
                return GetOrCreateClass(type);
            }
        }

        internal static EnumDefinition GetOrCreateEnum(Type type)
        {
            type = Helper.NormalizeType(type);
            return GetOrCreateType<EnumDefinition>(type, definition =>
                {
                    BuildType(definition, type);
                    BuildEnum(definition, type);

                    FieldInfo previousField = null;
                    foreach (FieldInfo field in type.GetFields(BindingFlags.Public | BindingFlags.Static))
                    {
                        definition.Values.Add(CreateEnumValue(definition, field, previousField));
                        previousField = field;
                    }
                });
        }

        private static EnumValueDefinition CreateEnumValue(TypeDefinition declaringType, FieldInfo field, FieldInfo previousField)
        {
            EnumValueDefinition definition = new EnumValueDefinition()
            {
                DeclaringType = declaringType
            };
            BuildEnumValue(definition, field);

            if (previousField == null && definition.Value == 0)
            {
                definition.IsInOrder = true;
            }
            else if (previousField != null)
            {
                definition.IsInOrder = (int)previousField.GetRawConstantValue() == (definition.Value - 1);
            }

            return definition;
        }

        internal static AttributeDefinition GetOrCreateAttribute(Type type)
        {
            type = Helper.NormalizeType(type);
            return GetOrCreateType<AttributeDefinition>(type, definition =>
            {
                BuildType(definition, type);
                BuildAttribute(definition, type);

                BindingFlags flags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly;
                foreach (PropertyInfo property in type.GetProperties(flags).OrderBy(p => p.MetadataToken))
                {
                    PropertyDefinition pd = CreateProperty(definition, property);
                    Helper.FixUpAttributeProperty(pd, property);
                    definition.Properties.Add(pd);
                }
            });
        }

        internal static DelegateDefinition GetOrCreateDelegate(Type type)
        {
            type = Helper.NormalizeType(type);
            return GetOrCreateType<DelegateDefinition>(type, definition =>
                {
                    BuildType(definition, type);
                    BuildDelegate(definition, type);
                });
        }

        internal static ContractDefinition GetOrCreateContract(Type type)
        {
            type = Helper.NormalizeType(type);
            return GetOrCreateType<ContractDefinition>(type, definition =>
                {
                    BuildType(definition, type);
                    BuildContract(definition, type);
                });
        }

        internal static ClassDefinition GetOrCreateClass(Type type)
        {
            type = Helper.NormalizeType(type);
            return GetOrCreateType<ClassDefinition>(type, definition =>
                {
                    BuildType(definition, type);
                    BuildClass(definition, type);

                    BindingFlags flags = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly;
                    foreach (PropertyInfo property in type.GetProperties(flags).Where(p => p.GetGetMethod(true).GetBaseDefinition() == p.GetGetMethod(true)).OrderBy(p => p.MetadataToken))
                    {
                        if (Helper.GetCustomAttribute<HideFromNewCodeGen>(property) != null)
                        {
                            continue;
                        }

                        if (Helper.IsStructPropertyHelper(property))
                        {
                            foreach (MethodDefinition md in CreateStructPropertyMethods(definition, property))
                            {
                                InferTypeData(md, definition);
                                definition.AllMethods.Add(md);
                            }
                        }
                        else
                        {
                            PropertyDefinition pd = CreateProperty(definition, property);
                            InferTypeData(pd, definition);
                            definition.AllProperties.Add(pd);
                        }
                    }

                    foreach (EventInfo ev in type.GetEvents(flags).Where(ev => ev.GetAddMethod(true).GetBaseDefinition() == ev.GetAddMethod(true)).OrderBy(ev => ev.MetadataToken))
                    {
                        if (Helper.GetCustomAttribute<HideFromNewCodeGen>(ev) != null)
                        {
                            continue;
                        }

                        EventDefinition ed = CreateEvent(definition, ev);
                        InferTypeData(ed, definition);
                        definition.AllEvents.Add(ed);
                    }

                    foreach (MethodInfo m in type.GetMethods(flags).Where(m => !m.IsSpecialName && m == m.GetBaseDefinition()).OrderBy(m => m.MetadataToken))
                    {
                        if (Helper.GetCustomAttribute<HideFromNewCodeGen>(m) != null)
                        {
                            continue;
                        }

                        MethodDefinition md = CreateMethod(definition, m);
                        InferTypeData(md, definition);
                        definition.AllMethods.Add(md);
                    }

                    foreach (ConstructorInfo c in type.GetConstructors(flags).OrderBy(c => c.MetadataToken))
                    {
                        if (Helper.GetCustomAttribute<HideFromNewCodeGen>(c) != null)
                        {
                            continue;
                        }

                        if (!c.IsPrivate)
                        {
                            ConstructorDefinition cd = CreateConstructor(definition, c);
                            InferTypeData(cd, definition);
                            definition.Constructors.Add(cd);
                        }
                    }

                    Helper.ApplyAttributes<IEndClassBuilder>(type,
                        builder => builder.BuildEndClass(definition, type));

                    AssociateContentProperty(definition, type);
                    AssociateInputProperty(definition, type);
                    ImplementIVectorIfNecessary(definition, type);

                    Helper.SetCollectionInfo(definition, type);
                });
        }

        private static void ImplementIVectorIfNecessary(ClassDefinition definition, Type type)
        {
            if (definition.IsCollectionImplementationClass && !definition.IsGenericType)
            {
                TypeDefinition elementType = Helper.GetCollectionElementType(definition);
            
                ClassDefinition collectionType;
                if (definition.XamlClassFlags.IsObservable)
                {
                    collectionType = Helper.GetCollectionType(CollectionKind.Observable, elementType);
                }
                else
                {
                    collectionType = Helper.GetCollectionType(CollectionKind.Vector, elementType);
                }

                TypeReference implementedInterface = NewBuilders.Helper.ImplementInterface(definition, Helper.GetClrType(collectionType));
                implementedInterface.IsImplicitInterface = true;

                if (!definition.IdlClassInfo.HasPrimaryInterface)
                {
                    definition.IdlClassInfo.NonFinalInterfaceName = definition.IdlClassInfo.InterfaceName;
                    definition.IdlClassInfo.InterfaceName = collectionType.IdlTypeInfo.GenericFullName;
                    definition.IdlClassInfo.AbiInterfaceName = new TypeReference(collectionType).AbiFullName;
                    definition.IdlClassInfo.IsInterfaceNameFinal = true;
                }
            }
        }

        private static void AssociateContentProperty(ClassDefinition definition, Type type)
        {
            Microsoft.UI.Xaml.Markup.ContentPropertyAttribute contentPropertyAtt = Helper.GetCustomAttribute<Microsoft.UI.Xaml.Markup.ContentPropertyAttribute>(type);
            if (contentPropertyAtt != null)
            {
                PropertyDefinition property = definition.GetProperty(contentPropertyAtt.Name);
                Helper.SetContentProperty(definition, (DependencyPropertyDefinition)property);
            }

            if (definition.ContentProperty != null && definition.IsCollectionImplementationClass)
            {
                definition.ContentProperty.IdlMemberInfo.IsExcluded = true;
            }
        }

        private static void AssociateInputProperty(ClassDefinition definition, Type type)
        {
            Microsoft.UI.Xaml.Controls.InputPropertyAttribute contentPropertyAtt = Helper.GetCustomAttribute<Microsoft.UI.Xaml.Controls.InputPropertyAttribute>(type);
            if (contentPropertyAtt != null)
            {
                PropertyDefinition property = definition.GetProperty(contentPropertyAtt.Name);
                definition.InputProperty = (DependencyPropertyDefinition)property;
            }
        }

        private static void InferTypeData(MemberDefinition member, ClassDefinition type)
        {
            if (!type.IsInterface && member.Name == "ContentProperty")
            {
                Helper.SetContentProperty(type, (DependencyPropertyDefinition)member, isImplicitContentProperty: true);
                member.IdlMemberInfo.IsExcluded = true;
            }
        }

        private static ConstructorDefinition CreateConstructor(TypeDefinition declaringType, ConstructorInfo ctor)
        {
            ConstructorDefinition definition = new ConstructorDefinition()
            {
                DeclaringType = declaringType
            };
            BuildMember(definition, ctor);
            BuildConstructor(definition, ctor);

            foreach (ParameterInfo p in ctor.GetParameters())
            {
                definition.Parameters.Add(CreateParameter(p));
            }

            return definition;
        }

        private static PropertyDefinition CreateProperty(TypeDefinition declaringType, PropertyInfo property)
        {
            PropertyDefinition definition;

            if (Helper.GetCustomAttribute<SimplePropertyAttribute>(property) != null)
            {
                definition = new DependencyPropertyDefinition();
            }
            else if (Helper.GetCustomAttribute<AttachedAttribute>(property) != null || property.Name.StartsWith("Attached"))
            {
                definition = new AttachedPropertyDefinition()
                {
                    TargetType = Helper.GetDPTargetTypeRef(typeof(Microsoft.UI.Xaml.UIElement)),
                };
            }
            else
            {
                PropertyKind kind = PropertyKind.Both;
                MethodInfo getMethodTemp = property.GetGetMethod(true);
                if (declaringType.IsExcludedFromTypeTable ||
                    getMethodTemp.IsStatic ||
                    (!property.DeclaringType.IsInterface && getMethodTemp.IsAbstract) ||
                    (!property.DeclaringType.IsValueType && !property.DeclaringType.IsInterface && !typeof(Microsoft.UI.Xaml.DependencyObject).IsAssignableFrom(property.DeclaringType)))
                {
                    kind = PropertyKind.PropertyOnly;
                }
                else
                {
                    TypeTableAttribute typeTableAtt = Helper.GetCustomAttribute<TypeTableAttribute>(property);
                    if (typeTableAtt != null && typeTableAtt.IsExcludedFromNewTypeTable)
                    {
                        kind = PropertyKind.PropertyOnly;
                    }
                    else
                    {
                        PropertyKindAttribute propertyKindAtt = Helper.GetCustomAttribute<PropertyKindAttribute>(property);
                        if (propertyKindAtt != null)
                        {
                            kind = propertyKindAtt.Kind;
                        }
                    }
                }

                switch (kind)
                {
                    case PropertyKind.PropertyOnly:
                        definition = new PropertyDefinition()
                        {
                            FrameworkFieldName = Helper.GetFieldName(property)
                        };
                        break;

                    default:
                        definition = new DependencyPropertyDefinition();
                        break;

                }
            }

            definition.DeclaringType = declaringType;
            BuildMember(definition, property);
            BuildProperty(definition, property);

            var setMethod = property.GetSetMethod(true);
            var getMethod = property.GetGetMethod(true);

            VersionAttribute setterVersionAttribute = setMethod != null ? Helper.GetCustomAttribute<VersionAttribute>(setMethod) : null;
            VersionAttribute getterVersionAttribute = getMethod != null ? Helper.GetCustomAttribute<VersionAttribute>(getMethod) : null;

            if(setterVersionAttribute != null)
            {
                definition.SetterVersion = setterVersionAttribute.Version;
            }

            if(getterVersionAttribute != null)
            {
                definition.GetterVersion = getterVersionAttribute.Version;
            }

            return definition;
        }

        private static EventDefinition CreateEvent(TypeDefinition declaringType, EventInfo ev)
        {
            EventDefinition definition;
            if (Helper.GetCustomAttribute<AttachedAttribute>(ev) != null)
            {
                definition = new AttachedEventDefinition()
                {
                    DeclaringType = declaringType,
                    TargetType = Helper.GetDPTargetTypeRef(typeof(Microsoft.UI.Xaml.UIElement)),
                };
            }
            else
            {
                definition = new EventDefinition()
                {
                    DeclaringType = declaringType
                };
            }

            BuildMember(definition, ev);
            BuildEvent(definition, ev);

            return definition;
        }

        private static IEnumerable<MethodDefinition> CreateStructPropertyMethods(ClassDefinition declaringType, PropertyInfo property)
        {
            MethodInfo getMethod = property.GetGetMethod(true);
            MethodInfo setMethod = property.GetSetMethod(true);

            yield return CreateStructPropertyMethodWithName(declaringType, getMethod, "Get" + property.Name);

            // We don't support extension property setters.
        }

        private static MethodDefinition CreateStructPropertyMethodWithName(ClassDefinition declaringType, MethodInfo m, string name)
        {
            MethodDefinition definition = CreateMethod(declaringType, m);
            definition.Name = definition.IdlMemberInfo.Name = name;
            definition.ReturnType.Name = "value"; // For compat reasons we use value instead of returnValue.
            return definition;
        }

        private static MethodDefinition CreateMethod(TypeDefinition declaringType, MethodInfo m)
        {
            MethodDefinition definition = new MethodDefinition()
            {
                DeclaringType = declaringType
            };

            if (declaringType.IsValueType && !m.IsStatic)
            {
                definition.Parameters.Add(new ParameterDefinition()
                {
                    ParameterType = Helper.GetStructHelperTargetTypeRef(m.DeclaringType)
                });
            }

            BuildMember(definition, m);
            BuildMethod(definition, m);

            foreach (ParameterInfo p in m.GetParameters())
            {
                definition.Parameters.Add(CreateParameter(p));
            }

            return definition;
        }

        private static ParameterDefinition CreateParameter(ParameterInfo parameter)
        {
            ParameterDefinition definition = new ParameterDefinition();
            BuildParameter(definition, parameter);
            return definition;
        }

        private static void BuildType(TypeDefinition definition, Type type)
        {
            definition.DeclaringNamespace = GetOrCreateNamespace(type.Namespace);
            definition.IsValueType = type.IsValueType;
            definition.XamlTypeFlags.AllowsMultipleAssociations = type.IsValueType || type == typeof(Windows.Foundation.Object);
            if (type.IsGenericType)
            {
                definition.IsGenericType = true;
                foreach (Type genericArg in type.GetGenericArguments())
                {
                    definition.GenericArguments.Add(Helper.GetTypeRef(genericArg));
                }

                definition.GenericClrName = type.ToString().Substring(type.Namespace.Length + 1);
                definition.Name = definition.IdlTypeInfo.Name = type.Name.Remove(type.Name.IndexOf("`"));
                definition.FriendlyGenericName = Helper.NormalizeType(type).GetFriendlyName();
                definition.FriendlyGenericFullName = type.GetFriendlyFullName();
            }
            else
            {
                definition.Name = definition.GenericClrName = definition.IdlTypeInfo.Name = type.Name;
            }

            if (type.IsGenericTypeDefinition || type.IsGenericParameter)
            {
                definition.IsExcludedFromTypeTable = true;
                definition.IsGenericType = true;
                definition.IsGenericTypeDefinition = type.IsGenericTypeDefinition;
                definition.IdlTypeInfo.IsExcluded = true;
            }


            definition.Guids = Guids.CreateFromType(definition);

            definition.TypeTableName = Helper.GetDefaultTypeTableName(type);
            definition.CoreName = "C" + definition.Name;
            definition.Modifier = Helper.GetModifier(type);
            if (definition.Modifier != Modifier.Public)
            {
                definition.IdlTypeInfo.IsExcluded = true;
            }
        }

        private static void BuildContract(ContractDefinition definition, Type type)
        {
            Helper.ApplyAttributes<ITypeBuilder>(type, builder => builder.BuildNewType(definition, type));
            Helper.ApplyAttributes<IContractBuilder>(type, builder => builder.BuildContract(definition, type));
        }

        public static ContractReference CreateContractReference(Type type, int version)
        {
            var key = Tuple.Create(type, version);
            if (!s_contractReferences.ContainsKey(key))
            {
                var contract = GetOrCreateContract(type);
                s_contractReferences.Add(key, new ContractReference(contract, version));
            }

            return s_contractReferences[key];
        }

        private static void BuildClass(ClassDefinition definition, Type type)
        {
            var factoryFormat = "{0}Factory";

            if (type.IsInterface)
            {
                definition.IdlClassInfo.InterfaceName = definition.Name;
            }
            else if (type.IsValueType)
            {
                definition.FactoryTypeName = string.Format(factoryFormat, definition.Name);
                definition.StructHelperName = string.Format("{0}Helper", definition.Name);
                definition.IdlClassInfo.InterfaceName = string.Format("I{0}Helper", definition.Name);
                definition.IdlClassInfo.StaticMembersInterfaceName = string.Format("I{0}HelperStatics", definition.Name);
            }
            else
            {
                definition.FactoryTypeName = string.Format(factoryFormat, definition.Name);
                Helper.SetIdlNames(definition, definition.Name);
            }

            if (type.IsGenericTypeDefinition)
            {
                definition.CoreCreationMethodName = string.Empty;
            }

            if (type.IsInterface)
            {
                definition.IsInterface = true;
                definition.IsCreateableFromXAML = false;
                definition.CoreCreationMethodName = string.Empty;
            }
            else if (type.IsAbstract && type.IsSealed)
            {
                definition.IsAbstract = true;
                definition.IsSealed = definition.IdlClassInfo.IsSealed = true;
                definition.IsStatic = true;
            }
            else
            {
                definition.IsAbstract = type.IsAbstract;
                definition.IsSealed = definition.IdlClassInfo.IsSealed = type.IsSealed;
                definition.IsValueType = type.IsValueType;
            }

            if (type.BaseType != null && !type.IsValueType && !type.IsInterface)
            {
                if (type != typeof(Windows.Foundation.Object))
                {
                    definition.BaseClass = GetOrCreateClass(type.BaseType);
                    definition.IdlClassInfo.BaseClass = Helper.GetBaseClassInIdl(definition);

                    if (type.BaseType.IsGenericType
                        && (type.BaseType.GetGenericTypeDefinition() == typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<>)
                        || type.BaseType.GetGenericTypeDefinition() == typeof(Microsoft.UI.Xaml.Collections.ObservablePresentationFrameworkCollection<>)))
                    {
                        definition.IsCollectionImplementationClass = true;
                    }
                    else if (typeof(Microsoft.UI.Xaml.EventArgs).IsAssignableFrom(type))
                    {
                        definition.IsAEventArgs = true;
                    }
                    else if (typeof(Microsoft.UI.Xaml.DependencyObject).IsAssignableFrom(type))
                    {
                        definition.IsADependencyObject = true;

                        if (typeof(Microsoft.UI.Xaml.UIElement).IsAssignableFrom(type))
                        {
                            definition.IsAUIElement = true;
                        }

                        if (typeof(Microsoft.UI.Xaml.Automation.Peers.AutomationPeer).IsAssignableFrom(type))
                        {
                            definition.IsAnAutomationPeer = true;
                        }
                    }
                }
                else
                {
                    definition.IsObjectType = true;
                    definition.IdlClassInfo.IsInterfaceNameFinal = true;
                }
            }
            else if (type == typeof(Windows.Foundation.String))
            {
                definition.IsStringType = true;
            }
            else if (type == typeof(Windows.UI.Xaml.Interop.TypeName))
            {
                definition.IsTypeNameType = true;
            }

            Helper.ApplyAttributes<ITypeBuilder, IClassBuilder>(type,
                builder => builder.BuildNewType(definition, type),
                builder => builder.BuildNewClass(definition, type));

            Helper.EnsureDefaultPlatforms(definition, type);

            if (definition.BaseClass != null)
            {
                definition.IsADependencyObject |= definition.BaseClass.IsADependencyObject;
                definition.IsAUIElement |= definition.BaseClass.IsAUIElement;
            }

            if (definition.BaseClass != null && definition.BaseClass.VelocityVersion != 0)
            {
                if (definition.IdlTypeInfo.IsPrivateIdlOnly)
                {
                    throw new InvalidOperationException(
                        string.Format(
                            "Internal class {0} cannot derive from class {1} since internal classes are available in all branches, whereas the velocity base class is only available in certain branches",
                            definition.Name, definition.BaseClass.Name));
                }
                else if (definition.Modifier != Modifier.Internal && definition.VelocityVersion != definition.BaseClass.VelocityVersion)
                {
                    // Modifier.Internal means the class isn't in any .idl and so we can ignore those.
                    throw new InvalidOperationException(string.Format("VelocityFeature attributes do not match between {0} and base class {1}.", definition.Name, definition.BaseClass.Name));
                }
            }

            if (type.IsGenericType && !definition.IdlClassInfo.IsImported)
            {
                definition.IdlClassInfo.IsExcluded = true;
            }

            definition.GenerateInCore &= ((definition.IsADependencyObject && !definition.IsGenericType && !definition.IsExcludedFromTypeTable) || definition.IsAEventArgs);

            if (definition.IsSimpleType)
            {
                if (definition.Modifier == Modifier.Internal)
                {
                    definition.GenerateStableIndex = false;
                    definition.ExcludeGuidFromTypeTable = true;
                    definition.TypeTableName = null;
                    definition.GenerateTypeCheckInfo = false;
                }
            }
        }

        private static void BuildEnum(EnumDefinition definition, Type type)
        {
            Helper.ApplyAttributes<ITypeBuilder, IEnumBuilder>(type,
                builder => builder.BuildNewType(definition, type),
                builder => builder.BuildNewEnum(definition, type));

            Helper.EnsureDefaultPlatforms(definition, type);
        }

        private static void BuildEnumValue(EnumValueDefinition definition, FieldInfo field)
        {
            definition.Name = field.Name;
            definition.Value = (int)field.GetRawConstantValue();
            Helper.ApplyAttributes<IMemberBuilder, IEnumValueBuilder>(field,
                builder => builder.BuildNewMember(definition, field),
                builder => builder.BuildNewEnumValue(definition, field));
        }

        private static void BuildAttribute(AttributeDefinition definition, Type type)
        {
            AttributeUsageAttribute usageAtt = Helper.GetCustomAttribute<AttributeUsageAttribute>(type);
            if (usageAtt != null)
            {
                definition.AllowMultiple = usageAtt.AllowMultiple;
            }

            Helper.ApplyAttributes<ITypeBuilder, IAttributeBuilder>(type,
                builder => builder.BuildNewType(definition, type),
                builder => builder.BuildNewAttribute(definition, type));
            Helper.EnsureDefaultPlatforms(definition, type);
        }

        private static void BuildDelegate(DelegateDefinition definition, Type type)
        {
            MethodInfo invokeMethod = type.GetMethod("Invoke");

            foreach (ParameterInfo arg in invokeMethod.GetParameters())
            {
                TypeReference parameterTypeRef = Helper.GetTypeRef(arg.ParameterType);
                parameterTypeRef.Name = arg.Name;
                definition.Parameters.Add(new ParameterDefinition()
                {
                    ParameterType = parameterTypeRef
                });
            }

            definition.IdlDelegateInfo.InterfaceName = "I" + definition.Name;
            definition.ReturnType = Helper.GetReturnTypeRef(invokeMethod.ReturnType);
            definition.IsExcludedFromTypeTable = true;
            Helper.ApplyAttributes<ITypeBuilder, IDelegateBuilder>(type,
                builder => builder.BuildNewType(definition, type),
                builder => builder.BuildNewDelegate(definition, type));
            Helper.EnsureDefaultPlatforms(definition, type);

            if (type.IsGenericType && !definition.IdlDelegateInfo.IsImported)
            {
                definition.IdlDelegateInfo.IsExcluded = true;
            }
        }

        private static void BuildMember(MemberDefinition definition, MemberInfo member)
        {
            MethodBase method;
            if (member is PropertyInfo)
            {
                method = ((PropertyInfo)member).GetGetMethod(true);
            }
            else if (member is EventInfo)
            {
                method = ((EventInfo)member).GetAddMethod(true);
            }
            else
            {
                method = (MethodBase)member;
            }

            definition.Name = definition.IdlMemberInfo.Name = member.Name;
            if (!method.DeclaringType.IsInterface)
            {
                definition.IsAbstract = method.IsAbstract;
                definition.IsStatic = method.IsStatic;
                definition.IsVirtual = method.IsVirtual;

                definition.Modifier = Helper.GetModifier(method);
                if (definition.DeclaringType.IdlTypeInfo.IsExcluded || Helper.GetModifier(method) <= Modifier.Internal)
                {
                    definition.IdlMemberInfo.IsExcluded = true;
                }
            }
        }

        private static void BuildProperty(PropertyDefinition definition, PropertyInfo property)
        {
            definition.PropertyType = Helper.GetPropertyTypeRef(property.PropertyType);
            definition.PropertyType.IsOptional = true;

            if (property.Name == "ContentProperty")
            {
                // The type of content properties on value types is the actual value type. E.g. the 
                // type of the content property on Color is Color. The model cannot describe this though 
                // because a struct cannot reference itself.
                if (property.DeclaringType.IsValueType)
                {
                    definition.PropertyType = Helper.GetTypeRef(property.DeclaringType);
                    definition.IsContentProperty = true;
                }
            }

            if (property.CanWrite)
            {
                definition.SetterModifier = Helper.GetModifier(property.GetSetMethod(true));
                if (definition.SetterModifier <= Modifier.Internal)
                {
                    definition.IdlPropertyInfo.IsReadOnly = true;
                }
            }
            else
            {
                definition.IsReadOnly = true;
                definition.IdlPropertyInfo.IsReadOnly = true;
            }

            Helper.ApplyAttributes<IMemberBuilder, IPropertyBuilder>(property,
                builder => builder.BuildNewMember(definition, property),
                builder => builder.BuildNewProperty(definition, property));

            DependencyPropertyDefinition dp = definition as DependencyPropertyDefinition;
            if (dp != null)
            {
                AttachedPropertyDefinition attachedDefinition = definition as AttachedPropertyDefinition;
                if (attachedDefinition != null)
                {
                    // Remove "Attached" suffix.
                    if (property.Name.StartsWith("Attached"))
                    {
                        attachedDefinition.Name = attachedDefinition.IdlMemberInfo.Name = property.Name.Substring("Attached".Length);
                    }

                    Helper.ApplyAttributes<IAttachedPropertyBuilder>(property, builder => builder.BuildNewProperty(attachedDefinition, property));

                    ((ClassDefinition)attachedDefinition.TargetType.Type).AllProperties.Add(attachedDefinition);
                }

                if (Helper.IsVisualTreeProperty(dp))
                {
                    dp.IsVisualTreeProperty = true;
                }

                if (Helper.IsObjectOrDependencyObjectTreeProperty(dp))
                {
                    dp.IsObjectOrDependencyObjectTreeProperty = true;
                }

                if (dp.IsSimpleProperty)
                {
                    if (dp.Modifier == Modifier.Internal)
                    {
                        dp.GenerateStableIndex = false;
                        dp.AllowEnumeration = false;
                    }
                }
            }
        }

        private static void BuildEvent(EventDefinition definition, EventInfo ev)
        {
            definition.Modifier = Helper.GetModifier(ev.GetAddMethod(true));
            if (definition.Modifier <= Modifier.Internal)
            {
                definition.IdlMemberInfo.IsExcluded = true;
                definition.GenerateStableIndex = false;
            }
            definition.EventHandlerType = Helper.GetEventHandlerTypeRef(ev.EventHandlerType);
            MethodInfo invokeMethod = ev.EventHandlerType.GetMethod("Invoke");
            Type eventArgType = invokeMethod.GetParameters()[1].ParameterType;
            if (typeof(Microsoft.UI.Xaml.RoutedEventArgs).IsAssignableFrom(eventArgType))
            {
                definition.IsRouted = true;
            }

            if (!definition.DeclaringClass.IsADependencyObject)
            {
                definition.GenerateDefaultBody = false;
            }

            Helper.ApplyAttributes<IMemberBuilder, IEventBuilder>(ev,
                builder => builder.BuildNewMember(definition, ev),
                builder => builder.BuildNewEvent(definition, ev));

            var attachedEv = definition as AttachedEventDefinition;
            if (attachedEv != null)
            {
                Helper.ApplyAttributes<IAttachedEventBuilder>(ev, builder => builder.BuildNewEvent(attachedEv, ev));
            }

        }

        private static void BuildMethod(MethodDefinition definition, MethodInfo method)
        {
            definition.ReturnType = Helper.GetReturnTypeRef(method.ReturnType, definition.SupportsV2CodeGen);
            definition.GenerateDefaultBody = false;

            Helper.ApplyAttributes<IMemberBuilder, IMethodBuilder>(method,
                builder => builder.BuildNewMember(definition, method),
                builder => builder.BuildNewMethod(definition, method));
        }

        private static void BuildConstructor(ConstructorDefinition definition, ConstructorInfo ctor)
        {
            Helper.ApplyAttributes<IMemberBuilder, IConstructorBuilder>(ctor,
                builder => builder.BuildNewMember(definition, ctor),
                builder => builder.BuildNewConstructor(definition, ctor));
        }

        private static void BuildParameter(ParameterDefinition definition, ParameterInfo parameter)
        {
            definition.ParameterType = Helper.GetTypeRef(parameter.ParameterType);
            definition.ParameterType.Name = parameter.Name;
            Helper.ApplyAttributes<IParameterBuilder>(parameter, builder => builder.BuildNewParameter(definition, parameter));
        }
    }
}
