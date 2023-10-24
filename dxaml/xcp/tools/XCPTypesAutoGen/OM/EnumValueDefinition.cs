// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class EnumValueDefinition : MemberDefinition
    {
        public Idl.IdlEnumValueInfo IdlEnumValueInfo
        {
            get;
            private set;
        }

        public override Idl.IdlMemberInfo IdlMemberInfo
        {
            get
            {
                return IdlEnumValueInfo;
            }
        }

        public string UIAName
        {
            get;
            set;
        }

        public int Value
        {
            get;
            set;
        }

        public bool IsInOrder
        {
            get;
            set;
        }

        public EnumValueDefinition()
        {
            IdlEnumValueInfo = new Idl.IdlEnumValueInfo(this);
            Version = 1;
        }
    }
}
