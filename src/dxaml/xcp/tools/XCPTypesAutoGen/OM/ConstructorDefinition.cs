// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class ConstructorDefinition : MethodBaseDefinition
    {
        public Idl.IdlConstructorInfo IdlConstructorInfo
        {
            get;
            private set;
        }


        public override Idl.IdlMemberInfo IdlMemberInfo
        {
            get
            {
                return IdlConstructorInfo;
            }
        }

        public bool IsParameterless
        {
            get
            {
                return !Parameters.Any();
            }
        }

        public string TypeName
        {
            get
            {
                return IdlMemberInfo.OwnerType;
            }
        }

        public string FactoryMethodName
        {
            get;
            set;
        }

        public string FactoryMethodImplName
        {
            get
            {
                return IdlConstructorInfo.FactoryMethodName + "Impl";
            }
        }

        public ConstructorDefinition()
        {
            IdlConstructorInfo = new Idl.IdlConstructorInfo(this);
        }
    }
}
