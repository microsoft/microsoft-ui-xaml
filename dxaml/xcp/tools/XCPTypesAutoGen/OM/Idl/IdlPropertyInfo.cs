// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlPropertyInfo : IdlMemberInfo
    {
        private PropertyDefinition _owner;

        public bool IsReadOnly
        {
            get;
            set;
        }

        public virtual string ProtectedGetterName
        {
            get
            {
                return "get_" + ProtectedName;
            }
        }

        public virtual string ProtectedSetterName
        {
            get
            {
                return "put_" + ProtectedName;
            }
        }

        public virtual string VirtualGetterName
        {
            get
            {
                return "get_" + VirtualName;
            }
        }

        public virtual string VirtualSetterName
        {
            get
            {
                return "put_" + VirtualName;
            }
        }

        internal IdlPropertyInfo(PropertyDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
