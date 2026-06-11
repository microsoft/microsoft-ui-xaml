// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace OM.Idl
{
    public abstract class IdlMemberInfo : IIdlSelectorInfo
    {
        private MemberDefinition _owner;

        public bool IsExcluded
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public string OwnerType
        {
            get
            {
                return _owner.DeclaringClass.IdlClassInfo.Name;
            }
        }

        public string ProtectedName
        {
            get
            {
                if (_owner.Modifier != Modifier.Protected)
                {
                    throw new InvalidOperationException("Member is not protected");
                }
                return Name + "Protected";
            }
        }

        public int Version
        {
            get
            {
                return _owner.Version;
            }
        }

        public string VirtualName
        {
            get
            {
                if (!_owner.IsVirtual)
                {
                    throw new InvalidOperationException("Member is not virtual");
                }

                if (_owner.Modifier == Modifier.Public)
                {
                    return Name + "Core";
                }
                return Name;
            }
        }

        protected IdlMemberInfo(MemberDefinition owner)
        {
            _owner = owner;
        }
    }
}
