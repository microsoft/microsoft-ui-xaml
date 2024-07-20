// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;

namespace OM
{
    public sealed class ContractDefinition : TypeDefinition
    {
        private readonly Lazy<Idl.IdlTypeInfo> idlTypeInfo;

        // The versions list defines the versions that are available for a contract definition.
        // The index corresponds to the version while the value may be one of:
        //
        //   -1: Version is not valid for this contract defintion
        //    0: Version is valid but does not correspond to a Xaml Direct Version
        //   >0: Version is valid and corresponds to the specified Xaml Direct Version.
        private List<int> versions;

        public ContractDefinition()
        {
            idlTypeInfo = new Lazy<Idl.IdlTypeInfo>(() => new IdlContractInfo(this));
            versions = new List<int>();
            IsWebHostHidden = false;
        }

        public void AddVersion(int version, int xamlDirectVersion)
        {
            if (version <= 0 || xamlDirectVersion < 0) throw new InvalidOperationException("Invalid version or xamlDirectVersion for contract");
            while (versions.Count <= version)
            {
                versions.Add(-1);
            }
            versions[version] = xamlDirectVersion;


        }

        public int MaxVersion
            {
            get
            {
                return versions.Count - 1;
            }
        }

        public bool ContainsVersion(int version)
        {
            return version < versions.Count && versions[version] >= 0;
        }

        public int GetXamlDirectVersion(int version)
        {
            return version < versions.Count ? versions[version] : 0;
        }

        public override Idl.IdlTypeInfo IdlTypeInfo
        {
            get
            {
                return idlTypeInfo.Value;
            }
        }

        private sealed class IdlContractInfo : Idl.IdlTypeInfo
        {
            public IdlContractInfo(ContractDefinition owner)
                : base(owner)
            {
            }

            public override bool HasRuntimeClass
            {
                get { return false; }
            }
        }
    }
}
