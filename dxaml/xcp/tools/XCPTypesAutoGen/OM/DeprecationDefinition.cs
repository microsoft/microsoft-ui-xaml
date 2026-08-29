// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;

namespace OM
{
    /// <summary>
    /// Specifies deprecation information for a type or member.
    /// </summary>
    public class DeprecationDefinition
    {
        public DeprecationDefinition()
        {
            Contracts = new List<ContractReference>();
        }

        public string Comment
        {
            get;
            set;
        }

        public List<ContractReference> Contracts
        {
            get;
            set;
        }

        public DeprecationType Type
        {
            get;
            set;
        }
    }
}
