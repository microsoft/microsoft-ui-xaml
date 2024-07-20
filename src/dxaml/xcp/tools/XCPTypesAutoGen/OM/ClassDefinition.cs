// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM.Idl;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class ClassDefinition : TypeDefinition
    {
        public static readonly ClassDefinition UnknownType;

        private List<AttributeInstantiation> _attributes = new List<AttributeInstantiation>();
        private DependencyPropertyDefinition[] _cachedSlottedProperties;
        private List<ConstructorDefinition> _constructors = new List<ConstructorDefinition>();
        private List<EventDefinition> _allEvents = new List<EventDefinition>();
        private List<MethodDefinition> _allMethods = new List<MethodDefinition>();
        private List<PropertyDefinition> _allProperties = new List<PropertyDefinition>();
        private List<TypeReference> _factoryImplementedInterfaces = new List<TypeReference>();
        private List<TypeReference> _implementedInterfaces = new List<TypeReference>();
        private List<ClassDefinition> _baseClassesVirtualMemberInterfaces = null;
        private List<ClassDefinition> _overrideInterfaces = null;
        private List<TemplatePartDefinition> _templateParts = new List<TemplatePartDefinition>();
        private List<ClassVersion> _versions = new List<ClassVersion>();
        private bool? _implementOverridesExplicitly = null;
        private Dictionary<int, bool> _interfaceForwardedVersions = new Dictionary<int, bool>();

        public string AbiImplementationFullName
        {
            get
            {
                StringBuilder builder = new StringBuilder(OMContext.DefaultImplementationNamespace);
                builder.Append('.');
                builder.Append(Name);
                if (IsGenericType)
                {
                    builder.Append('<');
                    bool first = true;
                    foreach (TypeReference arg in GenericArguments)
                    {
                        if (!first)
                        {
                            builder.Append(", ");
                        }
                        first = false;
                        builder.Append(Helper.GetAbiReferenceFullName(arg.Type, true)); 

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
                }

                return builder.ToString();
            }
        }

        public List<EventDefinition> AllEvents
        {
            get
            {
                return _allEvents;
            }
        }

        public List<PropertyDefinition> AllProperties
        {
            get
            {
                return _allProperties;
            }
        }

        public IEnumerable<MemberDefinition> AllPMEs
        {
            get
            {
                return AllEvents.Cast<MemberDefinition>().Concat(AllProperties.Where(p => !(p is AttachedPropertyDefinition)).Cast<MemberDefinition>()).Concat(AllMethods.Cast<MemberDefinition>());
            }
        }

        public List<MethodDefinition> AllMethods
        {
            get
            {
                return _allMethods;
            }
        }

        public List<AttributeInstantiation> Attributes
        {
            get
            {
                return _attributes;
            }
        }

        public ClassDefinition BaseClass
        {
            get;
            set;
        }

        public ClassDefinition BaseClassInTypeTable
        {
            get
            {
                ClassDefinition baseClass = BaseClass;
                while (baseClass != null && baseClass.IsExcludedFromTypeTable)
                {
                    baseClass = baseClass.BaseClass;
                }

                if (baseClass != null)
                {
                    return baseClass;
                }

                return UnknownType;
            }
        }

        public List<ConstructorDefinition> Constructors
        {
            get
            {
                return _constructors;
            }
        }

        public IEnumerable<ConstructorDefinition> IdlExposedConstructors
        {
            get
            {
                return Constructors.Where(c=> !c.IdlConstructorInfo.IsExcluded);
            }
        }

        public string CoreDefaultConstructor
        {
            get
            {
                if (!IsActivatableInCore)
                {
                    throw new InvalidOperationException();
                }

                return "OnCoreCreate" + Name;
            }
        }

        public string CoreDefaultConstructorPointer
        {
            get
            {
                if (IsActivatableInCore)
                {
                    return "&" + CoreDefaultConstructor;
                }
                return string.Empty;
            }
        }

        public IEnumerable<PropertyDefinition> CoreInstanceFields
        {
            get
            {
                if (IsAEventArgs)
                {
                    return InstanceFields;
                }
                else
                {
                    return FieldBackedDependencyProperties;
                }
            }
        }

        public IEnumerable<ConstructorDefinition> CustomConstructors
        {
            get
            {
                return _constructors.Where(c => !c.IsParameterless);
            }
        }

        public bool NeedsActivateInstance
        {
            get
            {
                return !IdlClassInfo.IsComposable &&
                        !HasCustomConstructors &&
                        !Constructors.Any(c => c.Modifier == Modifier.Internal);
            }
        }

        public IEnumerable<ClassDefinition> BaseClassesVirtualMemberInterfaces
        {
            get
            {
                if (_baseClassesVirtualMemberInterfaces == null)
                {
                    _baseClassesVirtualMemberInterfaces = new List<ClassDefinition>();
                    var baseClass = BaseClass;
                    while (baseClass != null)
                    {
                        foreach (var version in baseClass.VersionProjections.Where(v => v.IdlClassInfo.HasVirtualMembers))
                        {
                            _baseClassesVirtualMemberInterfaces.Add(version);
                        }
                        baseClass = baseClass.BaseClass;
                    }
                }
                return _baseClassesVirtualMemberInterfaces;
            }
        }

        public bool ImplementOverridesExplicitly
        {
            get
            {
                if (_implementOverridesExplicitly == null)
                {
                    const int maxInterfaces = 8;
                    int countInterfaces = 0;
                    foreach (var version in VersionProjections)
                    {
                        if (version.IdlClassInfo.HasPrimaryInterface)
                        {
                            countInterfaces++;
                        }
                        if (version.IdlClassInfo.HasVirtualMembers)
                        {
                            countInterfaces++;
                        }
                        if (version.IdlClassInfo.HasProtectedMembers)
                        {
                            countInterfaces++;
                        }
                        countInterfaces += version.ExplicitlyImplementedInterfaces.Count();
                    }
                    countInterfaces += BaseClassesVirtualMemberInterfaces.Count();

                    _implementOverridesExplicitly = countInterfaces > maxInterfaces;
                }
                return _implementOverridesExplicitly.Value;
            }
        }

        public DependencyPropertyDefinition ContentProperty
        {
            get;
            set;
        }

        public DependencyPropertyDefinition InputProperty
        {
            get;
            set;
        }

        public DependencyPropertyDefinition ContentPropertyInTypeTable
        {
            get
            {
                if (ContentProperty != null)
                {
                    return ContentProperty;
                }

                if (BaseClassInTypeTable != ClassDefinition.UnknownType)
                {
                    return BaseClassInTypeTable.ContentPropertyInTypeTable;
                }
                return PropertyDefinition.UnknownProperty;
            }
        }

        public string CoreCreationMethodName
        {
            get;
            set;
        }

        /// <summary>
        /// Example> Button.g.cpp
        /// </summary>
        public string CppCoreGeneratedBodyFileName
        {
            get
            {
                return CoreName + ".g.cpp";
            }
        }

        /// <summary>
        /// Example: Button.g.h
        /// </summary>
        ///         
        public string CppCoreGeneratedHeaderFileName
        {
            get
            {
                return CoreName + ".g.h";
            }
        }

        // Our file naming convention is inconsistent.  This maps class name to file where it's declared.

        private static Dictionary<string, string> ClassToHeaderMap = new Dictionary<string, string>()
        {
            { "CAutomationPeer",                    "AutomationPeer.h" },
            { "CButton",                            "Button.h" },
            { "CCanvas",                            "Canvas.h" },
            { "CContentControl",                    "ContentControl.h" },
            { "CContentPresenter",                  "ContentPresenter.h" },
            { "CControl",                           "CControl.h" },
            { "CDependencyObject",                  "CDependencyObject.h" },
            { "CDynamicTimeline",                   "DynamicTimeline.h" },
            { "CFrameworkElementAutomationPeer",    "FrameworkElementAutomationPeer.h" },
            { "CGrid",                              "Grid.h" },
            { "CItemsControl",                      "ItemsControl.h" },
            { "CListViewBaseItem",                  "ListViewBaseItem.h" },
            { "CPanel",                             "Panel.h" },
            { "CRangeBase",                         "RangeBase.h" },
            { "CScrollViewer",                      "ScrollViewer.h" },
            { "CStaggerFunctionBase",               "StaggerFunctions.h" },
            { "CTransition",                        "LayoutTransition.h" },
            { "CUserControl",                       "CControl.h" }
        };

        public string CppHeaderFileName
        {
            get
            {
                string result;

                if (!ClassToHeaderMap.TryGetValue(CoreName, out result))
                {
                    if (!GenerateInCore)
                    {
                        throw new Exception($"Don't know the name of header for {CoreName}, add it to ClassToHeaderMap.");
                    }

                    result = CppCoreGeneratedHeaderFileName;
                }

                return result;
            }
        }

        /// <summary>
        /// Example: Button_Partial.cpp
        /// </summary>
        public string CppFrameworkBodyFileName
        {
            get
            {
                return Name + "_Partial.cpp";
            }
        }

        /// <summary>
        /// Example: Button_Partial.h
        /// </summary>
        public string CppFrameworkHeaderFileName
        {
            get
            {
                return Name + "_Partial.h";
            }
        }


        /// <summary>
        /// Example> Button.g.cpp
        /// </summary>
        public string CppFrameworkGeneratedBodyFileName
        {
            get
            {
                return Name + ".g.cpp";
            }
        }

        /// <summary>
        /// Example: Button.g.h
        /// </summary>
        public string CppFrameworkGeneratedHeaderFileName
        {
            get
            {
                return Name + ".g.h";
            }
        }

        /// <summary>
        /// Gets the attached properties declared by this class. This doesn't mean these properties 
        /// can be set on an instance of this class.
        /// To get a list of properties that were attached to this class, EffectiveAttachedProperties.
        /// </summary>
        public IEnumerable<AttachedPropertyDefinition> DeclaredAttachedProperties
        {
            get
            {
                return _allProperties.OfType<AttachedPropertyDefinition>().Where(p => p.DeclaringType == ProjectionSource);
            }
        }

        /// <summary>
        /// Gets the attached properties targetting this class.
        /// </summary>
        public IEnumerable<AttachedPropertyDefinition> EffectiveAttachedProperties
        {
            get
            {
                return _allProperties.OfType<AttachedPropertyDefinition>().Where(p => p.TargetType.Type == ProjectionSource);
            }
        }

        /// <summary>
        /// Gets the effective type table properties. This includes properties attached by other types, and 
        /// excludes attached properties declared by this type.
        /// </summary>
        public IEnumerable<DependencyPropertyDefinition> EffectiveTypeTableProperties
        {
            get
            {
                // Include type table properties.
                IEnumerable<DependencyPropertyDefinition> result = TypeTableProperties;

                // Filter out all attached properties (declared + effective).
                result = result.Where(p => !(p is AttachedPropertyDefinition));

                // Add back effective attached properties that generate a field 
                // on the current class, unless they inherit their value and are part 
                // of a storage group.
                result = result.Concat(EffectiveAttachedProperties.Where(p => (p.HasAttachedField || p.HasPropertyMethod) && !(p.XamlPropertyFlags.IsValueInherited && p.IsInStorageGroup)));

                return result;
            }
        }

        public IEnumerable<DependencyPropertyDefinition> DependencyProperties
        {
            get
            {
                return Properties.OfType<DependencyPropertyDefinition>().Where(p => !p.IsSimpleProperty);
            }
        }

        public IEnumerable<EventDefinition> Events
        {
            get
            {
                return _allEvents;
            }
        }

        public IEnumerable<TypeReference> ExplicitlyImplementedInterfaces
        {
            get
            {
                return _implementedInterfaces.Where(i => !i.IsImplicitInterface);
            }
        }

        public string FactoryConstructor
        {
            get
            {
                return "CreateActivationFactory_" + Name;
            }
        }

        public string FactoryConstructorCallback
        {
            get
            {
                return string.Format("&{0}::{1}", OMContext.DefaultImplementationNamespace, FactoryConstructor);
            }
        }

        /// <summary>
        /// Example: ButtonFactory_Partial.h
        /// </summary>
        public string FactoryCppHeaderFileName
        {
            get
            {
                return FactoryTypeName + "_Partial.h";
            }
        }

        public IEnumerable<TypeReference> FactoryExplicitlyImplementedInterfaces
        {
            get
            {
                return _factoryImplementedInterfaces.Where(i => !i.IsImplicitInterface);
            }
        }

        public List<TypeReference> FactoryImplementedInterfaces
        {
            get
            {
                return _factoryImplementedInterfaces;
            }
        }

        public string FactoryTypeName
        {
            get;
            set;
        }

        public IEnumerable<PropertyDefinition> FieldBackedDependencyProperties
        {
            get
            {
                return DependencyProperties.Where(p => p.HasField);
            }
        }

        public DependencyPropertyDefinition FirstEnterPropertyInTypeTable
        {
            get
            {
                DependencyPropertyDefinition firstProperty = EffectiveTypeTableProperties.FirstOrDefault(p => p.IsEnterProperty && p.AllowEnumeration);
                if (firstProperty == null)
                {
                    if (BaseClassInTypeTable != UnknownType)
                    {
                        return BaseClassInTypeTable.FirstEnterPropertyInTypeTable;
                    }
                    return PropertyDefinition.UnknownProperty;
                }
                return firstProperty;
            }
        }

        public DependencyPropertyDefinition FirstObjectPropertyInTypeTable
        {
            get
            {
                DependencyPropertyDefinition firstProperty = EffectiveTypeTableProperties.FirstOrDefault(p => p.IsObjectProperty && p.AllowEnumeration);
                if (firstProperty == null)
                {
                    if (BaseClassInTypeTable != UnknownType)
                    {
                        return BaseClassInTypeTable.FirstObjectPropertyInTypeTable;
                    }
                    return PropertyDefinition.UnknownProperty;
                }
                return firstProperty;
            }
        }

        public DependencyPropertyDefinition FirstPropertyInTypeTable
        {
            get
            {
                DependencyPropertyDefinition firstProperty = TypeTableProperties.FirstOrDefault((p) => p.AllowEnumeration);
                if (firstProperty == null)
                {
                    if (BaseClassInTypeTable != UnknownType)
                    {
                        return BaseClassInTypeTable.FirstPropertyInTypeTable;
                    }
                    return PropertyDefinition.UnknownProperty;
                }
                return firstProperty;
            }
        }

        public DependencyPropertyDefinition FirstRenderPropertyInTypeTable
        {
            get
            {
                DependencyPropertyDefinition firstProperty = EffectiveTypeTableProperties.FirstOrDefault(p => p.IsRenderProperty && p.AllowEnumeration);
                if (firstProperty == null)
                {
                    if (BaseClassInTypeTable != UnknownType)
                    {
                        return BaseClassInTypeTable.FirstRenderPropertyInTypeTable;
                    }
                    return PropertyDefinition.UnknownProperty;
                }
                return firstProperty;
            }
        }

        public string EventArgsFrameworkDefaultConstructor
        {
            get
            {
                if (!IsActivatable)
                {
                    throw new InvalidOperationException();
                }

                return "OnFrameworkCreate" + Name;
            }
        }

        public string FrameworkDefaultConstructor
        {
            get
            {
                if (!IsActivatable)
                {
                    throw new InvalidOperationException();
                }

                return "ctl::CreateComObjectInstanceNoInit<" + AbiImplementationFullName.Replace(".", "::") + ">";
            }
        }

        public string FrameworkDefaultConstructorPointer
        {
            get
            {
                if (IsActivatable && !IsAEventArgs)
                {
                    return "&" + FrameworkDefaultConstructor;
                }
                return string.Empty;
            }
        }

        public string GeneratedClassName
        {
            get
            {
                if (GeneratePartialClass)
                {
                    return Name + "Generated";
                }
                return Name;
            }
        }

        public string GeneratedClassFullName
        {
            get
            {
                return OMContext.DefaultImplementationNamespace + "." + GeneratedClassName;
            }
        }

        public string GeneratedFactoryName
        {
            get
            {
                if (GeneratePartialFactory)
                {
                    return FactoryTypeName + "Generated";
                }
                return FactoryTypeName;
            }
        }

        public string GeneratedFactoryFullName
        {
            get
            {
                return OMContext.DefaultImplementationNamespace + "." + GeneratedFactoryName;
            }
        }

        public bool GenerateCoreFieldInitializer
        {
            get
            {
                return CoreInstanceFields.Any() || XamlClassFlags.ForceCoreFieldInitializer;
            }
        }

        public bool GenerateFrameworkHFile
        {
            get
            {
                return GenerateInFramework &&
                    (!IsValueType || HasStructHelperClass) &&
                    !IsImported &&
                    !IsGenericType &&
                    !IsXbfType &&
                    (!IsInterface || IdlClassInfo.IsExcluded);
            }
        }

        public bool GenerateFrameworkCppFile
        {
            get
            {
                return GenerateInFramework &&
                    !IsInterface &&
                    (!IsValueType || HasStructHelperClass) &&
                    !IsImported &&
                    !IsGenericType &&
                    !IsXbfType &&
                    (!IsInterface || IdlClassInfo.IsExcluded);
            }
        }

        public bool GenerateInCore
        {
            get;
            set;
        }

        public bool GenerateInFramework
        {
            get;
            set;
        }

        public bool GeneratePartialClass
        {
            get;
            set;
        }

        public bool GeneratePartialFactory
        {
            get;
            set;
        }

        public bool GenerateTypeCheckInfo { get; set; } = true;

        public string GuidMacroName
        {
            get
            {
                return string.Format("__{0}_GUID", Name);
            }
        }

        public bool HasCustomConstructors
        {
            get
            {
                return CustomConstructors.Any();
            }
        }

        public bool HasCustomFactory
        {
            get
            {
                return
                    !IsInterface &&
                    !IsGenericType &&
                    (!IsValueType || HasStructHelperClass) &&
                    ((Modifier == OM.Modifier.Public && !XamlClassFlags.IsHiddenFromIdl) && (!IsSealed || HasCustomConstructors || HasStaticMembers)) ||
                    XamlClassFlags.IsFreeThreaded;
            }
        }

        public bool HasFactory
        {
            get
            {
                return HasCustomFactory || IdlClassInfo.HasRuntimeClass || XamlClassFlags.ForceIncludeInManifest;
            }
        }

        public bool HasParameterlessConstructor
        {
            get
            {
                return _constructors.Where(c => c.IsParameterless).Any();
            }
        }

        public bool HasPeerStateAndLogic
        {
            get;
            set;
        }

        public bool HasPropertyChangeCallback
        {
            get;
            set;
        }

        public bool HasStaticMembers
        {
            get
            {
                return !IsInterface && (StaticPMEs.Any() || DependencyProperties.Any() || DeclaredAttachedProperties.Any());
            }
        }


        public bool HasStructHelperClass
        {
            get
            {
                return IsValueType && (IdlClassInfo.StaticPMEs.Any() || IdlClassInfo.InstanceMethods.Any());
            }
        }

        public List<TypeReference> ImplementedInterfaces
        {
            get
            {
                return _implementedInterfaces;
            }
        }

        public string InspectableClassMacroName
        {
            get
            {
                return ImplementOverridesExplicitly ? "WuxpInspectableClassNoGetIids" : "WuxpInspectableClass";
            }
        }

        public IEnumerable<ClassDefinition> OverrideInterfaces
        {
            get
            {
                if (_overrideInterfaces == null)
                {
                    _overrideInterfaces = new List<ClassDefinition>();
                    if (BaseClass != null)
                    {
                        if (BaseClass.IdlClassInfo.HasVirtualMembers)
                        {
                            foreach (var version in BaseClass.VersionProjections.Where(v => v.IdlClassInfo.HasVirtualMembers))
                            {
                                _overrideInterfaces.Add(version);
                            }
                        }
                        _overrideInterfaces.AddRange(BaseClass.OverrideInterfaces);
                    }
                }
                return _overrideInterfaces;
            }
        }

        public IEnumerable<EventDefinition> InstanceEvents
        {
            get
            {
                return Events.Where(e => !e.IsStatic);
            }
        }

        public IEnumerable<PropertyDefinition> InstanceFields
        {
            get
            {
                return InstanceProperties.Where(p => p.HasBackingFieldInFramework && !(p is DependencyPropertyDefinition) && !(p.IsAbstract && p.IsReadOnly));
            }
        }

        public IEnumerable<PropertyDefinition> InstanceProperties
        {
            get
            {
                return Properties.Where(p => !p.IsStatic);
            }
        }

        public IEnumerable<MemberDefinition> InstancePMEs
        {
            get
            {
                return InstanceProperties.Cast<MemberDefinition>().Concat(InstanceEvents.Cast<MemberDefinition>()).Concat(InstanceMethods.Cast<MemberDefinition>());
            }
        }

        public IEnumerable<MethodDefinition> InstanceMethods
        {
            get
            {
                return Methods.Where(m => !m.IsStatic);
            }
        }

        public Idl.IdlClassInfo IdlClassInfo
        {
            get;
            private set;
        }

        public override Idl.IdlTypeInfo IdlTypeInfo
        {
            get
            {
                return IdlClassInfo;
            }
        }

        public bool IsAbstract
        {
            get;
            set;
        }

        public bool IsActivatable
        {
            get
            {
                // If this is an abstract type, but there are no visible constructors externally, then don't consider this 
                // an activatable type.
                if (IsAbstract && ((Modifier < Modifier.Public) || !_constructors.Any(c => c.Modifier > OM.Modifier.Internal)))
                {
                    return false;
                }

                // For now, we only support framework activation of DOs.
                if ((!IsADependencyObject && !IsAEventArgs) || IsStatic || IsGenericType || IsActivationDisabledInTypeTable)
                {
                    return false;
                }

                if (IsImported)
                {
                    // For now, we don't support activation of imported types.
                    return false;
                }
                return _constructors.Any();
            }
        }

        public bool IsActivatableInCore
        {
            get
            {
                if (XamlClassFlags.HasTypeConverter)
                {
                    return true;
                }

                if (IsGenericType || IsAEventArgs)
                {
                    return false;
                }

                // If this is an abstract type, but there are no visible constructors externally, then don't consider this 
                // an activatable core type.
                if (IsAbstract && ((Modifier < Modifier.Public) || !_constructors.Any(c => c.Modifier > OM.Modifier.Internal)))
                {
                    return false;
                }

                return IsADependencyObject && !IsInterface && !IsStatic && !string.IsNullOrEmpty(CoreCreationMethodName);
            }
        }

        public bool ContainsChildrenInLogicalTree
        {
            get
            {
                return _allProperties.Any(dp => dp.XamlPropertyFlags.IsForcedIntoVisualTree);
            }
        }

        /// <summary>
        /// Gets or sets whether this class sends instance count telemetry
        /// </summary>
        public bool InstanceCountTelemetry
        {
            get;
            set;
        }

        /// <summary>
        /// TODO: This property exists temporarily to let us incrementally generate more 
        /// code as we move to a new code-gen tool. Eventually we should remove this property.
        /// </summary>
        public bool IsActivationDisabledInTypeTable
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether this is an AutomationPeer class.
        /// </summary>
        public bool IsAnAutomationPeer
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whetherh this is an EventArgs class, such as 
        /// Windows.UI.Xaml.RoutedEventArgs.
        /// </summary>
        public bool IsAEventArgs
        {
            get;
            set;
        }

        /// <summary>
        /// Used for e.g. VisualStateCollection, to describe that references to this class should generate 
        /// a different class (e.g. IVector&lt;VisualState&gt;).
        /// NOTE: Eventually once the old code-gen model is deprecated, we should consider making sure that e.g. 
        /// VisualStateGroup.States is of type IVector&lt;VisualState&gt;, and have an annotation on that 
        /// to describe what the default implementation class is.
        /// </summary>
        public bool IsCollectionImplementationClass
        {
            get;
            set;
        }

        public bool IsCreateableFromXAML
        {
            get;
            set;
        }

        public bool IsCustomControl
        {
            get;
            set;
        }

        public bool IsEventArgsWithCorePeer
        {
            get
            {
                return IsAEventArgs && !IsFrameworkEventArgs;
            }
        }

        /// <summary>
        /// Gets or sets whether this class is an EventArgs class that only exists in the framework.
        /// </summary>
        public bool IsFrameworkEventArgs
        {
            get;
            set;
        }

        public bool IsInterface
        {
            get;
            set;
        }

        public bool IsISupportInitialize
        {
            get
            {
                if (this == UnknownType)
                {
                    return false;
                }

                return XamlClassFlags.IsISupportInitialize || BaseClassInTypeTable.IsISupportInitialize;
            }
        }

        public bool IsPrivateProjection
        {
            get;
            internal set;
        }

        public bool IsSealed
        {
            get;
            set;
        }

        public bool IsStatic
        {
            get;
            set;
        }

        public bool IsVersionProjection
        {
            get;
            internal set;
        }

        public bool IsCollection => (IsVector || IsMap);
        public bool IsVector => (VectorGenericArgument != null);
        public bool IsMap => (MapGenericArguments != null);

        public TypeReference VectorGenericArgument { get; set; }
        public KeyValuePair<TypeReference, TypeReference>? MapGenericArguments { get; set; }

        public IEnumerable<MethodDefinition> Methods
        {
            get
            {
                return _allMethods;
            }
        }

        /// <summary>
        /// Returns properties that:
        /// 1. Are not attached properties.
        /// 2. Are not content properties (unless this is a collection implementation class).
        /// </summary>
        public IEnumerable<PropertyDefinition> Properties
        {
            get
            {
                return _allProperties.Where(p => (!p.IsContentProperty || !IsCollectionImplementationClass) && !(p is AttachedPropertyDefinition));
            }
        }

        public IEnumerable<MemberDefinition> PMEs
        {
            get
            {
                return Properties.Cast<MemberDefinition>().Concat(Events.Cast<MemberDefinition>()).Concat(Methods.Cast<MemberDefinition>());
            }
        }

        private int InitialVersion
        {
            get
            {
                return 1;
            }
        }

        public Guid RuntimeClassGuid
        {
            get
            {
                return Guids.ClassGuid;
            }
        }

        public bool ClassGuidExplicitlySet
        {
            get
            {
                return RuntimeClassGuid.CompareTo(Guid.Empty) != 0;
            }
        }

        /// <summary>
        /// Gets the class a version projection was created from. If we're not projecting anything, 
        /// this will return itself.
        /// </summary>
        internal ClassDefinition ProjectionSource
        {
            get;
            set;
        }

        public bool RequiresPeerActivation
        {
            get
            {
                return
                    IsActivatable &&
                    HasPeerStateAndLogic;
            }
        }

        public bool ForcePrimaryInterfaceGeneration
        {
            get;
            set;
        }

        public IEnumerable<PropertyDefinition> StaticProperties
        {
            get
            {
                return Properties.Where(p => p.IsStatic && !(p is AttachedPropertyDefinition));
            }
        }

        public IEnumerable<MemberDefinition> StaticPMEs
        {
            get
            {
                return StaticProperties.Cast<MemberDefinition>().Concat(StaticEvents.Cast<MemberDefinition>()).Concat(StaticMethods.Cast<MemberDefinition>());
            }
        }

        public IEnumerable<EventDefinition> StaticEvents
        {
            get
            {
                return Events.Where(e => e.IsStatic);
            }
        }

        public IEnumerable<MethodDefinition> StaticMethods
        {
            get
            {
                return Methods.Where(m => m.IsStatic);
            }
        }

        public string StructHelperFullName
        {
            get
            {
                return DeclaringNamespace.Name + "." + StructHelperName;
            }
        }

        public string StructHelperName
        {
            get;
            set;
        }

        public List<TemplatePartDefinition> TemplateParts
        {
            get
            {
                return _templateParts;
            }
        }

        /// <summary>
        /// Specifies the threading model for this class.
        /// </summary>
        public ThreadingModelKind ThreadingModel
        {
            get;
            set;
        }

        public ulong TypeHandle
        {
            get;
            set;
        }

        public ulong TypeHandleMask
        {
            get;
            set;
        }

        public IEnumerable<DependencyPropertyDefinition> SimpleProperties
        {
            get
            {
                return Properties.OfType<DependencyPropertyDefinition>().Where(p => p.IsSimpleProperty);
            }
        }

        public IEnumerable<DependencyPropertyDefinition> TypeTableProperties
        {
            get
            {
                if (IsInterface || IsAEventArgs)
                {
                    return Enumerable.Empty<DependencyPropertyDefinition>();
                }

                return _allProperties.OfType<DependencyPropertyDefinition>().Where(p => p.UnderlyingDependencyProperty == null && p.DeclaringType == this).OrderBy(p => p.TypeTableIndex);
            }
        }

        public IEnumerable<ClassDefinition> VersionProjections
        {
            get
            {
                return Versions.OrderBy(v => v.Version).Select(v => v.GetProjection());
            }
        }

        /// <summary>
        /// Gets the version this class represents. If IsVersionProjection is false, this will return 0.
        /// </summary>
        public int Version
        {
            get;
            set;
        }
        /// <summary>
        /// Gets or sets the velocity version this class represents. If this value is nonzero then the entire class is
        /// part of a velocity feature.
        /// </summary>
        public int VelocityVersion
        {
            get;
            set;
        }

        public List<ClassVersion> Versions
        {
            get
            {
                return _versions;
            }
        }

        internal Func<int, bool> VersionSelector
        {
            get;
            set;
        }

        public XamlClassFlags XamlClassFlags
        {
            get;
            set;
        }

        public Dictionary<int, bool> InterfaceForwardedVersions
        {
            get
            {
                return _interfaceForwardedVersions;
            }
        }

        public bool IsVersionInterfaceForwarded(int version)
        {
            if (InterfaceForwardedVersions != null && InterfaceForwardedVersions.ContainsKey(version))
            {
                return InterfaceForwardedVersions[version];
            }
            return false;
        }

        public bool IsVersionInterfaceForwarded()
        {
            if (IsVersionProjection)
            {
                return ProjectionSource.IsVersionInterfaceForwarded(Version);
            }
            return IsVersionInterfaceForwarded(Version);
        }

        public IEnumerable<MethodDefinition> GetInterfaceForwardedMethods()
        {
            if (!IsVersionInterfaceForwarded()) { throw new InvalidOperationException("Can't obtain interface forwarded methods if class isn't forwarding an interface"); }
            foreach (var prop in InstanceProperties.OrderBy(property => property.Name))
            {
                yield return prop.GetGetMethod();
                if (!prop.IsReadOnly)
                {
                    yield return prop.GetSetMethod();
                }
            }
            foreach (var method in InstanceMethods.OrderBy(method => method.Name))
            {
                yield return method;
            }
        }

        // If the entire type should be considered strict, we can skip generating strict-ness checks to
        // its members.
        public bool IsStrict
        {
            get; set;
        }

        static ClassDefinition()
        {
            UnknownType = new ClassDefinition()
            {
                GenericClrName = "UnknownType",
                Name = "UnknownObject",
                TypeTableName = "",
                DeclaringNamespace = NamespaceDefinition.UnknownNamespace
            };
            UnknownType.IdlTypeInfo.IsExcluded = true;
        }

        public ClassDefinition()
        {
            CoreCreationMethodName = "Create";
            GenerateInCore = true;
            GenerateInFramework = true;
            HasPropertyChangeCallback = false;
            IdlClassInfo = new Idl.IdlClassInfo(this);
            IsCreateableFromXAML = true;
            ProjectionSource = this;
            ThreadingModel = ThreadingModelKind.STA;
            XamlClassFlags = new XamlClassFlags();
        }

        public PropertyDefinition GetProperty(string propertyName)
        {
            PropertyDefinition property = Properties.SingleOrDefault(p => p.Name == propertyName);
            if (property != null)
            {
                return property;
            }

            if (BaseClass == null)
            {
                throw new InvalidOperationException(string.Format("Unable to find property '{0}'.", propertyName));
            }

            return BaseClass.GetProperty(propertyName);
        }

        /// <summary>
        /// Returns the properties, including attached properties which are targetting this class and all parent slotted properties. Slotted 
        /// properties are properties with a unique index (slot) per type. This allows the runtime to e.g. use bitfields to efficiently 
        /// describe the state of a property.
        /// </summary>
        /// <returns></returns>
        public IEnumerable<DependencyPropertyDefinition> GetSlottedProperties()
        {
            if (this == UnknownType)
            {
                return Enumerable.Empty<DependencyPropertyDefinition>();
            }

            return GetSlottedPropertiesRecursive();
        }

        private IEnumerable<DependencyPropertyDefinition> GetSlottedPropertiesRecursive()
        {
            if (_cachedSlottedProperties != null)
            {
                return _cachedSlottedProperties;
            }

            if (this == UnknownType)
            {
                return Enumerable.Empty<DependencyPropertyDefinition>();
            }

            IEnumerable<DependencyPropertyDefinition> result = BaseClassInTypeTable.GetSlottedPropertiesRecursive().Concat(EffectiveTypeTableProperties);

            // Cache and return information.
            _cachedSlottedProperties = result.ToArray();
            return _cachedSlottedProperties;
        }

        public ClassVersion GetVersion(int version)
        {
            var result = Versions.SingleOrDefault(v => v.Version == version);
            if (result == null)
            {
                if (VelocityFeatures.IsVelocityVersion(version))
                {
                    throw new InvalidOperationException(string.Format("Velocity feature {0} not found for type {1}", VelocityFeatures.GetFeatureName(version), this.Name));
                }
                else
                {
                    throw new InvalidOperationException(string.Format("Version {0} not found for type {1}", version, this.Name));
                }
            }
            return result;
        }

        public ClassDefinition InitialVersionProjection
        {
            get 
            {
                return Versions.Single(cv => cv.Version == InitialVersion).GetProjection();
            }
        }
    }
}
