// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class MethodDefinition : MethodBaseDefinition
    {
        public static readonly MethodDefinition UnknownMethod;

        /// <summary>
        /// Gets or sets whether to generate code in the framework that delegates the call to the core.
        /// </summary>
        public bool DelegateToCore
        {
            get;
            set;
        }

        public bool HasReturnType
        {
            get
            {
                return ReturnType != null && !ReturnType.IsVoid;
            }
        }

        public Idl.IdlMethodInfo IdlMethodInfo
        {
            get;
            private set;
        }

        public override Idl.IdlMemberInfo IdlMemberInfo
        {
            get
            {
                return IdlMethodInfo;
            }
        }

        public bool IsDefaultOverload
        {
            get;
            set;
        }

        public bool IsOverloaded
        {
            get
            {
                return !string.IsNullOrEmpty(OverloadName);
            }
        }
        
        /// <summary>
        /// Gets or sets whether this method mutates a struct.
        /// </summary>
        public bool IsMutator
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the name of the method that can be used to disambiguate between method overloads in IDL.
        /// </summary>
        public string OverloadName
        {
            get;
            set;
        }

        public string PInvokeName
        {
            get
            {
                return string.Format("::{0}_{1}", DeclaringClass.CoreName, Name);
            }
        }

        public TypeReference ReturnType
        {
            get;
            set;
        }

        public string VirtualOverloadName
        {
            get
            {
                if (!IsVirtual)
                {
                    throw new InvalidOperationException("Member is not virtual");
                }

                if (Modifier == Modifier.Public)
                {
                    return OverloadName + "Core";
                }
                return OverloadName;
            }
        }

        public string IndexName
        {
            get;
            set;
        }

        public string IndexNameWithoutPrefix
        {
            get;
            set;
        }

        static MethodDefinition()
        {
            UnknownMethod = new MethodDefinition()
            {
                Name = "UnknownMethod",
                DeclaringType = ClassDefinition.UnknownType,
            };
            UnknownMethod.IdlMemberInfo.IsExcluded = true;
        }

        public bool IsImplVirtual
        {
            get;
            set;
        }

        public MethodDefinition()
        {
            IdlMethodInfo = new Idl.IdlMethodInfo(this);
        }
    }
}
