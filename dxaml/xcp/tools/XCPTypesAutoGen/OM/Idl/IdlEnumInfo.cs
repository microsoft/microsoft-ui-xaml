// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlEnumInfo : IdlTypeInfo
    {
        private EnumDefinition _owner;

        public override bool HasRuntimeClass
        {
            get
            {
                return false;
            }
        }

        public IEnumerable<EnumValueDefinition> Values
        {
            get
            {
                return _owner.Values.Where(v => !v.IdlEnumValueInfo.IsExcluded);
            }
        }

        internal IdlEnumInfo(EnumDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
