// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;

namespace OM.Idl
{
    public class IdlClassInfo : IdlTypeInfo
    {
        private ClassDefinition _owner;

        public string AbiInterfaceFullName
        {
            get
            {
                string effectiveInterfaceName = AbiInterfaceName;
                if (string.IsNullOrEmpty(effectiveInterfaceName))
                {
                    effectiveInterfaceName = InterfaceName;
                }

                if (IsInterfaceNameFinal)
                {
                    return effectiveInterfaceName;
                }

                return GetDeclaringNamespace() + "." + effectiveInterfaceName;
            }
        }

        public string AbiInterfaceName
        {
            get;
            set;
        }

        public string AbiReferenceFullName
        {
            get
            {
                if (IsSurrogate)
                {
                    return SurrogateFor.AbiReferenceFullName;
                }
                return _owner.AbiReferenceFullName;
            }
        }

        public ClassDefinition BaseClass
        {
            get;
            set;
        }

        public IEnumerable<ConstructorDefinition> CustomConstructors
        {
            get
            {
                return _owner.Constructors.Where(c => !c.IdlMemberInfo.IsExcluded && (!c.IsParameterless || !IsSealed));
            }
        }

        public ConstructorDefinition DefaultConstructor
        {
            get
            {
                return _owner.Constructors.SingleOrDefault(c => !c.IdlMemberInfo.IsExcluded && c.IsParameterless && IsSealed);
            }
        }


        public IEnumerable<AttachedPropertyDefinition> DeclaredAttachedProperties
        {
            get
            {
                return _owner.DeclaredAttachedProperties.Where(dp => !dp.IdlMemberInfo.IsExcluded);
            }
        }

        public IEnumerable<DependencyPropertyDefinition> DependencyProperties
        {
            get
            {
                return _owner.DependencyProperties.Where(dp => !dp.IdlMemberInfo.IsExcluded && dp.IsHandlePublic);
            }
        }
        public Guid FactoryInterfaceGuid
        {
            get
            {
                return _owner.Guids.FactoryGuids.GetGuid(_owner, "FactoryGuids", GetEffectiveVersion());
            }
        }

        public string FactoryInterfaceName
        {
            get;
            set;
        }

        /// <summary>
        /// Forces inclusion of the [constructor_name] attribute in IDL. Our interfaces are versioned with the version
        /// of the class, not after the previous interface name. For example, if V1 of a class had a custom constructor, but
        /// we didn't add a second constructor until v3, the v3 interface name will be IFooFactory3 instead of IFooFactory2.
        /// We will continue doing this to maintain consistency, and so we don't ship an IFooFactory2 that was introduced
        /// after IFooFactory3
        /// </summary>
        public bool ForceIncludeFactoryInterfaceName
        {
            get;
            set;
        }

        public bool RequiresConstructorNameAttribute
        {
            get
            {
                return HasFactoryMethods;
            }
        }

        /// <summary>
        /// Forces inclusion of the [ContentProperty] attribute in IDL. This only exists right now
        /// for compat reasons, because we shipped Windows Blue with a [ContentProperty("Template")] attribute
        /// on FrameworkTemplate, while Template is an internal property.
        /// </summary>
        public bool ForceIncludeContentProperty
        {
            get;
            set;
        }

        public string FullFactoryInterfaceName
        {
            get
            {
                return GetDeclaringNamespace() + "." + FactoryInterfaceName;
            }
        }

        public string FullInterfaceName
        {
            get
            {
                var result = string.Empty;

                if (_owner.IdlClassInfo.IsInterfaceNameFinal)
                {
                    result = InterfaceName;
                }
                else
                {
                    result = GetDeclaringNamespace() + "." + InterfaceName;
                }

                return result;
            }
        }

        public override string FullName
        {
            get
            {
                if (IsSurrogate)
                {
                    return SurrogateFor.FullName;
                }

                return base.FullName;
            }
        }

        public override string Name
        {
            get
            {
                if (IsSurrogate)
                {
                    return SurrogateFor.Name;
                }

                return base.Name;
            }
            set
            {
                base.Name = value;
            }
        }


        public string FullProtectedMembersInterfaceName
        {
            get
            {
                return GetDeclaringNamespace() + "." + ProtectedMembersInterfaceName;
            }
        }

        public string FullStaticMembersInterfaceName
        {
            get
            {
                return GetDeclaringNamespace() + "." + StaticMembersInterfaceName;
            }
        }

        public string FullVirtualMembersInterfaceName
        {
            get
            {
                return GetDeclaringNamespace() + "." + VirtualMembersInterfaceName;
            }
        }

        /// <summary>
        /// Determines whether the class factory implements any interfaces.
        /// </summary>
        public bool HasAnyFactoryInterfaces
        {
            get
            {
                return HasFactoryMethods || HasStaticMembers;
            }
        }

        /// <summary>
        /// Determines whether the runtime class implements any interfaces.
        /// </summary>
        public bool HasAnyInstanceInterfaces
        {
            get
            {
                return HasPrimaryInterface
                    || HasProtectedMembers
                    || HasVirtualMembers
                    || ExplicitlyImplementedInterfaces.Any();
            }
        }

        public bool HasBaseClass
        {
            get
            {
                return BaseClass != null && !BaseClass.IsObjectType;
            }
        }

        public bool HasContentProperty
        {
            get
            {
                return _owner.ContentProperty != null && (!_owner.ContentProperty.IdlMemberInfo.IsExcluded || ForceIncludeContentProperty);
            }
        }

        public bool HasInputProperty
        {
            get
            {
                return _owner.InputProperty != null;
            }
        }

        public bool HasConstructors
        {
            get
            {
                return _owner.Constructors.Any(c => !c.IdlMemberInfo.IsExcluded);
            }
        }

        public bool HasPublicConstructors
        {
            get
            {
                return _owner.Constructors.Any(c => !c.IdlMemberInfo.IsExcluded && c.Modifier == Modifier.Public);
            }
        }

        public bool HasCustomConstructors
        {
            get
            {
                return CustomConstructors.Any();
            }
        }

        public bool HasDefaultConstructor
        {
            get
            {
                return DefaultConstructor != null;
            }
        }

        public bool HasFactoryMethods
        {
            get
            {
                return HasCustomConstructors || IsComposable;
            }
        }

        public override bool HasRuntimeClass
        {
            get
            {
                return (!_owner.IsValueType || _owner.HasStructHelperClass)
                    && !_owner.IsInterface
                    && !IsExcluded;
            }
        }

        public bool HasPrimaryInterface
        {
            get
            {
                if (IsExcluded)
                {
                    return false;
                }

                if (_owner.IsVersionProjection)
                {
                    if (_owner.SupportsV2CodeGen)
                    {
                        // Static classes don't need a primary interface. Activatable and composable classes
                        // do, and derived classes do, even if empty
                        return (HasPublicInstanceMembers
                                || _owner.ForcePrimaryInterfaceGeneration
                                || IsComposable
                                || IsActivatable
                                // WinRT classes have to implement something.  So if it doesn't have a constructor, or instance members
                                // or static members, we'll have to create an empty instance interface.
                                || !HasStaticMembers && _owner.Version <= 1
                                // Need an interface for V1 of a derived class (note: base of a static class is Windows.Foundation.Object)
                                || BaseClass != null && BaseClass.GenericClrFullName != "Windows.Foundation.Object"
                                   && BaseClass.IdlClassInfo.IsComposable && _owner.Version <= 1
                                || _owner.Version <= 1 && _owner.IsInterface);
                    }
                    else
                    {
                        return ((_owner.Version <= 1 && !_owner.IsCollectionImplementationClass) || HasPublicInstanceMembers || _owner.ForcePrimaryInterfaceGeneration);
                    }
                }

                return _owner.InitialVersionProjection.IdlClassInfo.HasPrimaryInterface;
            }
        }

        public bool HasRequiredInterfaces
        {
            get
            {
                // A runtime class needs an interface if it has its own instance members, or if it implements
                // IVector/IIterable/etc
                return HasPrimaryInterface || _owner.IsCollectionImplementationClass;
            }
        }

        public bool HasProtectedMembers
        {
            get
            {
                return ProtectedPMEs.Any();
            }
        }

        public bool HasPublicInstanceMembers
        {
            get
            {
                return PublicInstancePMEs.Any();
            }
        }

        public bool HasStaticMembers
        {
            get
            {
                return !_owner.IsInterface && (StaticPMEs.Any() || DependencyProperties.Where(p => !p.IsSimpleProperty).Any() || DeclaredAttachedProperties.Any());
            }
        }

        public bool HasPublicStaticMembers
        {
            get
            {
                return !_owner.IsInterface && (StaticPMEs.Any(pme => pme.Modifier > Modifier.Internal) || DependencyProperties.Any(dp => dp.Modifier == Modifier.Public) || DeclaredAttachedProperties.Any(dp => dp.Modifier == Modifier.Public));
            }
        }

        public bool HasStructHelperClass
        {
            get
            {
                return _owner.IsValueType && (_owner.Properties.Any(m => !m.IdlMemberInfo.IsExcluded && m.IsStructHelper) || _owner.Methods.Any(m => !m.IdlMemberInfo.IsExcluded));
            }
        }

        public bool HasVirtualMembers
        {
            get
            {
                return VirtualPMEs.Any();
            }
        }

        public bool IncludeInManifest
        {
            get
            {
                return _owner.XamlClassFlags.ForceIncludeInManifest || (!IsImported && HasRuntimeClass && (IsActivatable || HasFactoryMethods || HasStaticMembers)) || HasStructHelperClass;
            }
        }

        public bool IsActivatable
        {
            get
            {
                return HasRuntimeClass && HasConstructors;
            }
        }

        public bool IsComposable
        {
            get
            {
                bool canBeComposable = (!_owner.IsVersionProjection || (_owner.Version == InitialVersion));
                return HasRuntimeClass && !IsSealed && (canBeComposable || HasCustomConstructors);
            }
        }

        /// <summary>
        /// Gets or sets whether InterfaceName is a final name that doesn't need a namespace prefix anymore.
        /// </summary>
        public bool IsInterfaceNameFinal
        {
            get;
            set;
        }

        public bool IsSealed
        {
            get;
            set;
        }

        /// <summary>
        /// Example: Rect/Point/Size.
        /// </summary>
        public bool IsSurrogate
        {
            get;
            set;
        }

        public IEnumerable<TypeReference> ExplicitlyImplementedInterfaces
        {
            get
            {
                return _owner.ImplementedInterfaces.Where(i => !i.IsImplicitInterface && !i.IdlInfo.IsExcluded && !i.IdlInfo.Type.IdlTypeInfo.IsExcluded && !Helper.IsLessVisible(_owner, i.IdlInfo.Type)).OrderBy(i => i.IdlInfo.Type.IdlTypeInfo.GenericFullName);
            }
        }

        public IEnumerable<TypeReference> ImplementedInterfaces
        {
            get
            {
                return _owner.ImplementedInterfaces.Where(i => !i.IdlInfo.IsExcluded && !i.IdlInfo.Type.IdlTypeInfo.IsExcluded && !Helper.IsLessVisible(_owner, i.IdlInfo.Type));
            }
        }

        public IEnumerable<EventDefinition> InstanceEvents
        {
            get
            {
                return _owner.InstanceEvents.Where(e => !e.IdlMemberInfo.IsExcluded && e.InterfaceMember == null);
            }
        }

        public IEnumerable<PropertyDefinition> InstanceProperties
        {
            get
            {
                return _owner.InstanceProperties.Where(p => !p.IdlMemberInfo.IsExcluded && p.InterfaceMember == null);
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
                return _owner.InstanceMethods.Where(m => !m.IdlMemberInfo.IsExcluded && m.InterfaceMember == null);
            }
        }

        public Guid InterfaceGuid
        {
            get
            {
                return _owner.Guids.InterfaceGuids.GetGuid(_owner, "InterfaceGuids", GetEffectiveVersion());
            }
        }

        public string InterfaceName
        {
            get;
            set;
        }

        /// <summary>
        /// Forces inclusion of the [interface_name] attribute in IDL. Our interfaces are versioned with the version
        /// of the class, not after the previous interface name. For example, if V1 of a class had instance PMEs, but
        /// we didn't add any others until v3, the v3 interface name will be IFoo3 instead of IFoo2.
        /// We will continue doing this to maintain consistency, and so we don't ship an IFoo2 that was introduced
        /// after IFoo3
        /// </summary>
        public bool ForceIncludeInterfaceName
        {
            get;
            set;
        }

        /// <summary>
        /// Only set when IsInterfaceNameFinal is set to true. This allows us to still infer
        /// a meaningful interface name for versioned types.
        /// Example: UIElementCollection
        /// </summary>
        public string NonFinalInterfaceName
        {
            get;
            set;
        }

        public IEnumerable<EventDefinition> ProtectedEvents
        {
            get
            {
                return InstanceEvents.Where(m => !m.IsVirtual && m.Modifier == Modifier.Protected);
            }
        }

        public Guid ProtectedMembersInterfaceGuid
        {
            get
            {
                return _owner.Guids.ProtectedGuids.GetGuid(_owner, "ProtectedGuids", GetEffectiveVersion());
            }
        }

        public string ProtectedMembersInterfaceName
        {
            get;
            set;
        }

        /// <summary>
        /// Forces inclusion of the [protected_name] attribute in IDL. Our interfaces are versioned with the version
        /// of the class, not after the previous interface name. For example, if V1 of a class had a custom constructor, but
        /// we didn't add a second constructor until v3, the v3 interface name will be IFooProtected3 instead of IFooProtected2.
        /// We will continue doing this to maintain consistency, and so we don't ship an IFooProtected2 that was introduced
        /// after IFooProtected3
        /// </summary>
        public bool ForceIncludeProtectedMembersInterfaceName
        {
            get;
            set;
        }

        public IEnumerable<PropertyDefinition> ProtectedProperties
        {
            get
            {
                return InstanceProperties.Where(m => !m.IsVirtual && m.Modifier == Modifier.Protected);
            }
        }

        public IEnumerable<MemberDefinition> ProtectedPMEs
        {
            get
            {
                return ProtectedProperties.Cast<MemberDefinition>().Concat(ProtectedEvents.Cast<MemberDefinition>()).Concat(ProtectedMethods.Cast<MemberDefinition>());
            }
        }

        public IEnumerable<MethodDefinition> ProtectedMethods
        {
            get
            {
                return InstanceMethods.Where(m => !m.IsVirtual && m.Modifier == Modifier.Protected);
            }
        }

        public IEnumerable<EventDefinition> PublicInstanceEvents
        {
            get
            {
                return InstanceEvents.Where(m => m.Modifier == Modifier.Public);
            }
        }

        public IEnumerable<PropertyDefinition> PublicInstanceProperties
        {
            get
            {
                return InstanceProperties.Where(m => m.Modifier == Modifier.Public);
            }
        }

        public IEnumerable<MemberDefinition> PublicInstancePMEs
        {
            get
            {
                return PublicInstanceProperties.Cast<MemberDefinition>().Concat(PublicInstanceEvents.Cast<MemberDefinition>()).Concat(PublicInstanceMethods.Cast<MemberDefinition>());
            }
        }

        public IEnumerable<MethodDefinition> PublicInstanceMethods
        {
            get
            {
                return InstanceMethods.Where(m => m.Modifier == Modifier.Public);
            }
        }

        public string RuntimeClassFullName
        {
            get
            {
                if (HasStructHelperClass)
                {
                    return _owner.StructHelperFullName;
                }

                return FullName;
            }
        }

        public string RuntimeClassName
        {
            get
            {
                if (HasStructHelperClass)
                {
                    return _owner.StructHelperName;
                }

                return FullName;
            }
        }

        public string RuntimeClassFullNameOverride
        {
            get;
            set;
        }

        public string RuntimeClassString
        {
            get
            {
                return "RuntimeClass_" + RuntimeClassFullName.Replace('.', '_'); ;
            }
        }

        public IEnumerable<EventDefinition> StaticEvents
        {
            get
            {
                return _owner.StaticEvents.Where(e => !e.IdlMemberInfo.IsExcluded);
            }
        }

        public IEnumerable<PropertyDefinition> StaticProperties
        {
            get
            {
                return _owner.StaticProperties.Where(p => !p.IdlMemberInfo.IsExcluded);
            }
        }

        public IEnumerable<MemberDefinition> StaticPMEs
        {
            get
            {
                return StaticProperties.Cast<MemberDefinition>().Concat(StaticEvents.Cast<MemberDefinition>()).Concat(StaticMethods.Cast<MemberDefinition>());
            }
        }

        public IEnumerable<MethodDefinition> StaticMethods
        {
            get
            {
                return _owner.StaticMethods.Where(m => !m.IdlMemberInfo.IsExcluded);
            }
        }

        public IEnumerable<PropertyDefinition> StructFields
        {
            get
            {
                return _owner.InstanceProperties.Where(p => !p.IdlMemberInfo.IsExcluded && !p.IsStructHelper);
            }
        }

        public Guid StaticMembersInterfaceGuid
        {
            get
            {
                return _owner.Guids.StaticsGuids.GetGuid(_owner, "StaticsGuids", GetEffectiveVersion());
            }
        }

        public string StaticMembersInterfaceName
        {
            get;
            set;
        }


        /// <summary>
        /// Forces inclusion of the [statics_name] attribute in IDL. Our interfaces are versioned with the version
        /// of the class, not after the previous interface name. For example, if V1 of a class had a custom constructor, but
        /// we didn't add a second constructor until v3, the v3 interface name will be IFooStatics3 instead of IFooStatics2.
        /// We will continue doing this to maintain consistency, and so we don't ship an IFooStatics2 that was introduced
        /// after IFooStatics3
        /// </summary>
        public bool ForceIncludeStaticMembersInterfaceName
        {
            get;
            set;
        }

        public ClassDefinition SurrogateFor
        {
            get;
            set;
        }

        public override int Version
        {
            get
            {
                return _owner.Version;
            }
        }

        protected int InitialVersion
        {
            get
            {
                return 1;
            }
        }

        public IEnumerable<EventDefinition> VirtualEvents
        {
            get
            {
                return InstanceEvents.Where(m => m.IsVirtual);
            }
        }

        public IEnumerable<PropertyDefinition> VirtualProperties
        {
            get
            {
                return InstanceProperties.Where(m => m.IsVirtual);
            }
        }

        public IEnumerable<MemberDefinition> VirtualPMEs
        {
            get
            {
                return VirtualProperties.Cast<MemberDefinition>().Concat(VirtualEvents.Cast<MemberDefinition>()).Concat(VirtualMethods.Cast<MemberDefinition>());
            }
        }

        public Guid VirtualMembersInterfaceGuid
        {
            get
            {
                return _owner.Guids.OverrideGuids.GetGuid(_owner, "OverrideGuids", GetEffectiveVersion());
            }
        }

        public string VirtualMembersInterfaceName
        {
            get;
            set;
        }

        /// <summary>
        /// Forces inclusion of the [overridable_name] attribute in IDL. Our interfaces are versioned with the version
        /// of the class, not after the previous interface name. For example, if V1 of a class had a custom constructor, but
        /// we didn't add a second constructor until v3, the v3 interface name will be IFooOverrides3 instead of IFooOverrides2.
        /// We will continue doing this to maintain consistency, and so we don't ship an IFooOverrides2 that was introduced
        /// after IFooOverrides3
        /// </summary>
        public bool ForceIncludeVirtualMembersInterfaceName
        {
            get;
            set;
        }

        public IEnumerable<MethodDefinition> VirtualMethods
        {
            get
            {
                return InstanceMethods.Where(m => m.IsVirtual);
            }
        }

        internal IdlClassInfo(ClassDefinition owner)
            : base(owner)
        {
            FactoryInterfaceName = string.Empty;
            ProtectedMembersInterfaceName = string.Empty;
            StaticMembersInterfaceName = string.Empty;
            VirtualMembersInterfaceName = string.Empty;
            _owner = owner;
        }

        private int GetEffectiveVersion()
        {
            if (_owner.IsVersionProjection)
            {
                return _owner.Version;
            }

            return InitialVersion;
        }

        private string GetDeclaringNamespace()
        {
            if (string.IsNullOrEmpty(_owner.CorrectedTypeName))
            {
                return _owner.DeclaringNamespace.Name;
            }
            else
            {
                // If the TypeDefinition corresponding to this IdlClassInfo has a CorrectedTypeName,
                // then that, rather than the TypeDefinition's own name and namespace, is the name that 
				// will be used in the generated IDL. We need to extract the namespace from the
				// CorrectedTypeName for our own use in order to be consistent with the IDL.
                return _owner.CorrectedTypeName.Substring(0, _owner.CorrectedTypeName.LastIndexOf('.'));
            }
        }
    }
}
