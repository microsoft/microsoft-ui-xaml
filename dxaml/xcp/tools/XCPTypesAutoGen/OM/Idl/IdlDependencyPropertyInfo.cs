// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlDependencyPropertyInfo : IdlPropertyInfo
    {
        private DependencyPropertyDefinition _owner;

        public string DPName
        {
            get
            {
                return Name + "Property";
            }
        }

        internal IdlDependencyPropertyInfo(DependencyPropertyDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
