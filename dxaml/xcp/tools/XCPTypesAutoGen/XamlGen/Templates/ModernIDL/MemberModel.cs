// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.ModernIDL
{
    public abstract class MemberModel<T> where T : MemberDefinition
    {
        public string IdlName
        {
            get
            {
                if (RequestedInterface == RequestedInterface.VirtualMembers && Member.Modifier == Modifier.Public)
                {
                    return Member.IdlMemberInfo.VirtualName;
                }
                return Member.IdlMemberInfo.Name;
            }
        }

        public T Member
        {
            get;
            protected set;
        }
        public RequestedInterface RequestedInterface
        {
            get;
            set;
        }

        protected MemberModel()
        {
        }
    }
}
