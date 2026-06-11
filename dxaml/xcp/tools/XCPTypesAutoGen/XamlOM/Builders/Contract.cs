// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false, AllowMultiple = true)]
    public sealed class ContractAttribute : PlatformAttribute
    {
        public ContractAttribute(Type contract, int contractVersion)
            : base(contract, contractVersion)
        {
        }

        public ContractAttribute(int version, Type contract, int contractVersion)
            : base(version, contract, contractVersion)
        {
        }
    }
}
