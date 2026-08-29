// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM.Idl
{
    public class IdlConstructorInfo : IdlMemberInfo
    {
        private ConstructorDefinition _owner;

        public string FactoryMethodName
        {
            get
            {
                return _owner.FactoryMethodName ?? "CreateInstance";
            }
        }
        
        public bool IsCustomFactoryConstructor
        {
            get
            {
                return _owner.FactoryMethodName != null;
            }
        }
        internal IdlConstructorInfo(ConstructorDefinition owner)
            : base(owner)
        {
            _owner = owner;
        }
    }
}
