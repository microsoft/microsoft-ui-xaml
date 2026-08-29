// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace OM
{
    public static class ContractReferenceExtensions
    {
        private static ContractReference GetContract(this IEnumerable<ContractReference> supportedContracts, Func<IEnumerable<ContractReference>, ContractReference> sortFunc)
        {
            ContractReference selectedContract = null;
            if (supportedContracts.Any())
            {
                var orderedContracts = supportedContracts.OrderBy(c => c.Version).ThenBy(c => c.FullName);
                selectedContract = sortFunc(orderedContracts);
            }

            return selectedContract;
        }

        public static ContractReference GetConcreteContractReference(this IEnumerable<ContractReference> supportedContracts)
        {
            return supportedContracts.GetContract(Enumerable.First);
        }

        public static bool SupportsV2CodeGen(this IEnumerable<ContractReference> supportedContracts)
        {
            var contractsList = supportedContracts.ToList();
            var count = contractsList.Count;
            if (count == 0)
            {
                throw new Exception("Must have a contract");
            }

            // All the contracts need to support V2
            return contractsList.All(contract => contract.SupportsV2CodeGen);
        }

        public static bool SupportsModernIdl(this IEnumerable<ContractReference> supportedContracts)
        {
            var contractsList = supportedContracts.ToList();
            var count = contractsList.Count;
            if (count == 0)
            {
                // V1 versions and projections dont have a contract associated, so we'll assume not
                return false;
            }

            // All the contracts need to support V2
            return contractsList.All(contract => contract.SupportsModernIdl);
        }

    }
}
