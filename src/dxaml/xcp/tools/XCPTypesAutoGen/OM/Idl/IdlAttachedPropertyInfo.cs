// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlAttachedPropertyInfo : IdlDependencyPropertyInfo
    {
        private AttachedPropertyDefinition _owner;

        public string GetterName
        {
            get
            {
                return "Get" + Name;
            }
        }

        public string SetterName
        {
            get
            {
                return "Set" + Name;
            }
        }

        internal IdlAttachedPropertyInfo(AttachedPropertyDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
