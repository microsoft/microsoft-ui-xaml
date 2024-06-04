// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;

namespace OM
{
    public sealed class ContractReference
    {
        public override string ToString()
        {
            //return $"[contract({Helper.EnsureCorrectNamespace(FullName)}, {Version})]";
            return $"[contract({FullName}, {Version})]";
        }
        private ContractDefinition Definition
        {
            get;
            set;
        }

        public int Version
        {
            get;
            private set;
        }

        public string FullName
        {
            get
            {
                //return Helper.EnsureTypeModelNamespace(Definition.FullName);
                return Definition.FullName;
            }
        }

        public bool IsPrivateApiContract
        {
            get
            {
                return Definition.IdlTypeInfo.IsPrivateIdlOnly;
            }
        }

        public ContractReference(ContractDefinition definition, int version)
        {
            if (!definition.ContainsVersion(version))
            {
                throw new InvalidOperationException(string.Format("Invalid reference to contract {0}, version {1} is not defined", definition.FullName, version));
            }

            Definition = definition;
            Version = version;
        }

        // Is this an RS4+ contract?  If so, we can fix some bugs in code gen (for example, marker interfaces on static
        // classes aren't required).
        public bool SupportsV2CodeGen
        {
            get
            {
                int value;
                if (_codeGenV1Contracts.TryGetValue(FullName, out value))
                {
                    if (Version <= value)
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        public bool SupportsModernIdl
        {
            get
            {
                int value;
                if (_preModernIdlContracts.TryGetValue(FullName, out value))
                {
                    if (Version <= value)
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        // These are all the contracts (up to the given version) produced by code gen as of RS3
        static Dictionary<string, int> _codeGenV1Contracts = new Dictionary<string, int>
        {
            { "Windows.Foundation.UniversalApiContract", 5 },
            { "Microsoft.UI.Xaml.Hosting.HostingContract", 2 },
            { "Windows.Services.Maps.LocalSearchContract", 4 },
            { "Microsoft.UI.Xaml.WinUIContract", 1 },
        };

        static Dictionary<string, int> _preModernIdlContracts = new Dictionary<string, int>
        {
            { "Windows.Foundation.UniversalApiContract", 6 },
            { "Microsoft.UI.Xaml.Hosting.HostingContract", 2 },
            { "Microsoft.UI.Xaml.WinUIContract", 1 },
        };
    }
}
