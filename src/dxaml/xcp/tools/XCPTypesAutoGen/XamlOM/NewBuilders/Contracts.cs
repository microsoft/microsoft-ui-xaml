// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using XamlOM.NewBuilders;

namespace XamlOM
{
    [AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = true)]
    public sealed class ContractVersionAttribute : Attribute, IContractBuilder
    {
        public ContractVersionAttribute(int version, int xamlDirectVersion = 0)
        {
            Version = version;
            XamlDirectVersion = xamlDirectVersion;
        }

        public int Version
        {
            get;
            private set;
        }

        public int XamlDirectVersion
        {
            get;
            private set;
        }

        void IContractBuilder.BuildContract(OM.ContractDefinition definition, Type source)
        {
            definition.AddVersion(Version, XamlDirectVersion);
        }
    }

    public abstract class Contract
    {
    }
}
