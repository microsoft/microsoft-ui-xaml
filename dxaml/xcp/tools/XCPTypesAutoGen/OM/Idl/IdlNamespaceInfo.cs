// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlNamespaceInfo
    {
        private NamespaceDefinition _owner;

        public string Group
        {
            get;
            set;
        }

        internal IdlNamespaceInfo(NamespaceDefinition owner)
        {
            _owner = owner;
        }
    }
}
