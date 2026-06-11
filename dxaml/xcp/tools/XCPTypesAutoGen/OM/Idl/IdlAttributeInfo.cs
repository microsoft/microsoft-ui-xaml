// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlAttributeInfo : IdlTypeInfo
    {
        private AttributeDefinition _owner;

        public override bool HasRuntimeClass
        {
            get
            {
                return false;
            }
        }

        internal IdlAttributeInfo(AttributeDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
