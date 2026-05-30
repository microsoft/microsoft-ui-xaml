// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;

namespace XamlOM
{
    /// <summary>
    /// Specifies on which platform the specified type's version is supported. If no attribute is specified, 
    /// the type will by default support Win8.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false, AllowMultiple = true)]
    public class PlatformAttribute : Attribute, NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder, NewBuilders.IEnumBuilder
    {
        private readonly Type _contract;
        private readonly int _contractVersion;

        public int Version
        {
            get;
            private set;
        }

        public string VelocityFeature
        {
            get;
            private set;
        }

        public bool ForcePrimaryInterfaceGeneration
        {
            get;
            set;
        }

        public Type ContractType
        {
            get { return _contract; }
        }

        public int ContractVersion
        {
            get { return _contractVersion; }
        }

        public PlatformAttribute(Type contract, int contractVersion)
            : this(1, contract, contractVersion)
        {
        }

        public PlatformAttribute(string velocityFeature, Type contract, int contractVersion)
            : this(VelocityFeatures.GetVersion(velocityFeature), contract, contractVersion)
        {
            VelocityFeature = velocityFeature;
        }

        public PlatformAttribute(int version, Type contract, int contractVersion)
        {
            Version = version;

            _contract = contract;
            _contractVersion = contractVersion;
        }

        public void BuildNewType(TypeDefinition definition, Type source)
        {
            // Do not allow a Platform with Velocity if the entire class is under velocity
            if (VelocityFeatures.IsVelocityVersion(Version) && Attribute.IsDefined(source, typeof(VelocityFeatureAttribute)))
            {
                throw new InvalidOperationException(string.Format("Platform attribute with velocity feature not allow for type {0} because the entire type is under velocity", definition.Name));
            }

            AddContractReference(definition.SupportedContracts);

            if (definition.SupportedContracts.Where(c => c.IsPrivateApiContract).Any())
            {
                definition.IdlTypeInfo.IsPrivateIdlOnly = true;
            }
        }

        public void BuildNewClass(ClassDefinition definition, Type source)
        {
            // Create version objects that will hold projection information for just this specific version.
            ClassVersion version = definition.Versions.SingleOrDefault(v => v.Version == Version);
            if (version == null)
            {
                version = new ClassVersion(definition, Version, GetVersionPostfix(source), ForcePrimaryInterfaceGeneration);
                definition.Versions.Add(version);
            }

            // Don't forward statics or event args
            if (AddContractReference(version.SupportedContracts) && !definition.IsAEventArgs && !definition.IsStatic && !definition.IsInterface)
            {
                if (!definition.InterfaceForwardedVersions.ContainsKey(Version))
                {
                    definition.InterfaceForwardedVersions.Add(Version, true);
                }
            }
        }

        public void BuildNewEnum(EnumDefinition definition, Type source)
        {
            EnumVersion version = definition.Versions.SingleOrDefault(v => v.Version == Version);
            if (version == null)
            {
                version = new EnumVersion(definition, Version);
                definition.Versions.Add(version);
            }

            AddContractReference(version.SupportedContracts);
        }

        // Returns whether or not the contract is new and should auto interface forward
        private bool AddContractReference(List<ContractReference> contracts)
        {
            if (_contract != null)
            {
                var contractRef = XamlOM.NewBuilders.ModelFactory.CreateContractReference(_contract, _contractVersion);
                if (!contracts.Contains(contractRef))
                {
                    contracts.Add(contractRef);
                }
                return contractRef.SupportsModernIdl && !contractRef.IsPrivateApiContract;
            }
            return false;
        }

        private string GetVersionPostfix(Type source)
        {
            if (Version == 1)
            {
                return string.Empty;
            }

            VersionPostfixAttribute postfixAtt = NewBuilders.Helper.GetCustomAttributes<VersionPostfixAttribute>(source).Where(att => att.Version == Version).SingleOrDefault();
            if (postfixAtt != null)
            {
                return postfixAtt.Postfix;
            }

            if (VelocityFeatures.IsVelocityVersion(Version))
            {
                return VelocityFeatures.GetFeatureName(Version);
            }

            return Version.ToString();
        }
    }
}
