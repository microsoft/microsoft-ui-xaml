// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class DelegateDefinition : TypeDefinition
    {
        private List<ParameterDefinition> _parameters = new List<ParameterDefinition>();

        public Idl.IdlDelegateInfo IdlDelegateInfo
        {
            get;
            private set;
        }

        public override Idl.IdlTypeInfo IdlTypeInfo
        {
            get
            {
                return IdlDelegateInfo;
            }
        }

        public List<ParameterDefinition> Parameters
        {
            get
            {
                return _parameters;
            }
        }

        public TypeReference ReturnType
        {
            get;
            set;
        }

        public DelegateDefinition()
        {
            IdlDelegateInfo = new Idl.IdlDelegateInfo(this);
        }
    }
}
