// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.Linq;

namespace OM
{
    public abstract class VersionedElement
    {
        private List<ContractReference> _supportedContracts = new List<ContractReference>();
        private int _version;

        public int Version
        {
            get
            {
                return _version;
            }
            set
            {
                _version = value;
                GetterVersion = value;
                SetterVersion = value;
            }
        }

        public int GetterVersion
        {
            get;
            set;
        }

        public int SetterVersion
        {
            get;
            set;
        }

        public bool VersionedAccessors
        {
            get
            {
                return GetterVersion != Version || SetterVersion != Version;
            }
        }

        public List<ContractReference> SupportedContracts
        {
            get
            {
                return _supportedContracts;
            }
        }
    }

    public class ClassVersion : VersionedElement
    {
        private ClassDefinition _cachedProjection;
        private string _postfix;

        public ClassDefinition ActualClass
        {
            get;
            private set;
        }

        public bool ForcePrimaryInterfaceGeneration
        {
            get;
            private set;
        }

        public ClassVersion(ClassDefinition actualClass, int version, string postfix)
            : this(actualClass, version, postfix, false)
        {
        }

        public ClassVersion(ClassDefinition actualClass, int version, string postfix, bool forcePrimaryInterfaceGeneration)
        {
            ActualClass = actualClass;
            Version = version;
            _postfix = postfix;
            ForcePrimaryInterfaceGeneration = forcePrimaryInterfaceGeneration;
        }

        public ClassDefinition GetProjection()
        {
            if (_cachedProjection != null)
            {
                return _cachedProjection;
            }

            ClassDefinition projection = new ClassDefinition();
            Helper.ShallowCopyProperties(ActualClass, projection);
            Helper.ShallowCopyProperties(ActualClass.IdlClassInfo, projection.IdlClassInfo);
            projection.ProjectionSource = ActualClass;
            projection.Deprecations.AddRange(ActualClass.Deprecations);
            projection.IsVersionProjection = true;
            projection.Version = Version;
            projection.Attributes.AddRange(ActualClass.Attributes);
            projection.Constructors.AddRange(ActualClass.Constructors.Where(m => m.Version == Version));
            projection.AllEvents.AddRange(ActualClass.AllEvents.Where(m => m.Version == Version));
            projection.GenericArguments.AddRange(ActualClass.GenericArguments);
            projection.FactoryImplementedInterfaces.AddRange(ActualClass.FactoryImplementedInterfaces.Where(i => i.Version == Version));
            projection.ImplementedInterfaces.AddRange(ActualClass.ImplementedInterfaces.Where(i => i.Version == Version));
            projection.AllMethods.AddRange(ActualClass.AllMethods.Where(m => m.Version == Version));
            projection.AllProperties.AddRange(ActualClass.AllProperties.Where(m => m.Version == Version || m.GetterVersion == Version || m.SetterVersion == Version));
            projection.SupportedContracts.AddRange(SupportedContracts);
            projection.ForcePrimaryInterfaceGeneration = ForcePrimaryInterfaceGeneration;

            if (ActualClass.VelocityVersion != 0)
            {
                projection.VelocityVersion = ActualClass.VelocityVersion;
            }
            else if (VelocityFeatures.IsVelocityVersion(Version))
            {
                projection.VelocityVersion = Version;
            }

            projection.IdlClassInfo.FactoryInterfaceName = ActualClass.IdlClassInfo.FactoryInterfaceName + _postfix;
            if (ActualClass.IdlClassInfo.IsInterfaceNameFinal)
            {
                if (Version > 1)
                {
                    projection.IdlClassInfo.InterfaceName = ActualClass.IdlClassInfo.NonFinalInterfaceName + _postfix;
                    projection.IdlClassInfo.IsInterfaceNameFinal = false;
                }
            }
            else
            {
                projection.IdlClassInfo.InterfaceName = ActualClass.IdlClassInfo.InterfaceName + _postfix;
            }
            projection.IdlClassInfo.ProtectedMembersInterfaceName = ActualClass.IdlClassInfo.ProtectedMembersInterfaceName + _postfix;
            projection.IdlClassInfo.StaticMembersInterfaceName = ActualClass.IdlClassInfo.StaticMembersInterfaceName + _postfix;
            projection.IdlClassInfo.VirtualMembersInterfaceName = ActualClass.IdlClassInfo.VirtualMembersInterfaceName + _postfix;

            projection.IdlClassInfo.ForceIncludeFactoryInterfaceName = ShouldForceIncludeInterfaceName(projection, def => def.IdlClassInfo.RequiresConstructorNameAttribute, true);
            projection.IdlClassInfo.ForceIncludeInterfaceName = ShouldForceIncludeInterfaceName(projection, def => {
                if (def.IdlClassInfo.HasPublicInstanceMembers)
                {
                    return true;
                }

                // It's unclear if this is something that is forced by MIDL3, or just needed for compat reasons so that
                // we continue to generate the same interfaces we had before. Prior to the switch to MIDL3, codegen would generate
                // a lot of empty interfaces that didn't always seem necessary.
                return def.IdlClassInfo.HasPrimaryInterface;

            });
            projection.IdlClassInfo.ForceIncludeStaticMembersInterfaceName = ShouldForceIncludeInterfaceName(projection, def => def.IdlClassInfo.HasPublicStaticMembers);
            projection.IdlClassInfo.ForceIncludeProtectedMembersInterfaceName = ShouldForceIncludeInterfaceName(projection, def => def.IdlClassInfo.HasProtectedMembers);
            projection.IdlClassInfo.ForceIncludeVirtualMembersInterfaceName = ShouldForceIncludeInterfaceName(projection, def => def.IdlClassInfo.HasVirtualMembers);

            _cachedProjection = projection;

            return projection;
        }

        bool ShouldForceIncludeInterfaceName(ClassDefinition currentProjection, System.Func<ClassDefinition, bool> func, bool isFactory = false)
        {
            int version = 0;
            bool currentVersionHasMembers = func(currentProjection);
            if (!int.TryParse(_postfix, out version))
            {
                // If someone wants to postfix this with a non-integer variable, then let them, they shouldn't have to fight code-gen. This will mean
                // that the interface_name attribute has to appear on the interface if it has members. If the postfix is null or empty, then this is v1 and we
                // don't need to include it, unless it's a factory for an abstract class, because we don't use standard Modern IDL factory name conventions which
                // would be "IFooProtectedFactory" instead of just "IFooFactory"
                return currentVersionHasMembers && (!System.String.IsNullOrEmpty(_postfix) || (isFactory && currentProjection.IsAbstract));
            }

            var currentContract = currentProjection.SupportedContracts.GetConcreteContractReference();

            // Just get the cached projection, we explicitly don't want to create a projection that doesn't exist (see below for more details)
            foreach (var classVersion in ActualClass.Versions.OrderBy(v=>v.Version).Where(v => v.Version < Version).Select(v => v._cachedProjection))
            {
                if (classVersion == null)
                {
                    // We only select versions that are lower than the currently being created one. If the cached projection is null at this point, then the previous
                    // versions haven't been created, which means we can't properly know whether or not we need to provide the interface name for the MIDL compiler. 
                    // It's possible that these ForceInclude*Name properties could be set at a later point and we could remove this dependency, since it seems
                    // arbitrary, but there may have been a good reason why this was done here and not later, so I'm leaving as-is for now.

                    // This exception can be hit if doing something like: ActualClass.Versions.Select(v => v.GetProjection()).OrderBy(v => v.Version)
                    // The "correct" way would be: ActualClass.Versions.OrderBy(v => v.Version).Select(v => v.GetProjection()). This is done for you already by 
                    // the ClassDefinition.VersionProjections property, so tell people to just use that instead. Only someone trying to make somewhat substantial
                    // changes to code-gen would ever hit this.
                    throw new System.NotSupportedException($"{nameof(ClassVersion)}.{nameof(ClassVersion.GetProjection)}() called out of order. " +
                                                           $"Use {nameof(ClassDefinition)}.{nameof(ClassDefinition.VersionProjections)} to properly access different version projections.");
                }

                bool previousVersionHasMembers = func(classVersion);

                if (currentVersionHasMembers && !previousVersionHasMembers)
                {
                    // The previous version doesn't have any members, which means there is a gap in the interface versions. Normally,
                    // modern IDL just increments the version by 1, but our practice is to put the version of the class with each interface.
                    // This way we know IFoo4, IFooFactory4, IFooStatics4 all correlate to the same class version (v4 of Foo).
                    // Without this, we could have a scenario where IFoo4, IFooFactory3, and IFooStatics2 correlate to the same class version.
                    return true;
                }

                // Previous version has members, but make sure they are in different contracts. If in the same contract, then
                // these would be flattened into the same interface, so we'll force include the interface name here
                var versionContract = classVersion.SupportedContracts.GetConcreteContractReference();
                if (currentContract.Version == versionContract.Version && currentVersionHasMembers)
                {
                    return true;
                }

                // v2 of the Window class was removed for DCPP. This is a temporary workaround until the interfaces are all flattened.
                if (currentVersionHasMembers && currentProjection.FullName == "Microsoft.UI.Xaml.Window" && currentProjection.Version > 2)
                {
                    return true;
                }
            }

            return false;
        }

        public bool IsInterfaceForwarded()
        {
            return ActualClass.IsVersionInterfaceForwarded(Version);
        }
    }
}
