// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlDelegateInfo : IdlTypeInfo
    {
        private DelegateDefinition _owner;

        public Guid DelegateGuid
        {
            get
            {
                return _owner.Guids.InterfaceGuids.GetGuid(_owner, "InterfaceGuids", this.Version);
            }
        }

        public string FullInterfaceName
        {
            get
            {
                return _owner.DeclaringNamespace.Name + "." + InterfaceName;
            }
        }

        public override bool HasRuntimeClass
        {
            get
            {
                return true;
            }
        }

        public string InterfaceName
        {
            get;
            set;
        }

        internal IdlDelegateInfo(DelegateDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
