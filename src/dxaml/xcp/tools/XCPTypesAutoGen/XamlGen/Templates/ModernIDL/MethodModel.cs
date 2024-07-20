// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.ModernIDL
{
    public class MethodModel : MemberModel<MethodDefinition>
    {
        public string Name
        {
            get
            {
                if (Member.IsOverloaded)
                {
                    return OverloadName;
                }

                if (RequestedInterface == RequestedInterface.VirtualMembers && Member.Modifier == OM.Modifier.Public)
                {
                    return Member.Name + "Core";
                }
                return Member.Name;
            }
        }

        public string IdlReturnTypeName
        {
            get
            {
                if (RequestedInterface == RequestedInterface.VirtualMembers)
                {
                    return "overridable " + Member.ReturnType.IdlTypeName;
                }
                else if (RequestedInterface == RequestedInterface.ProtectedMembers)
                {
                    return "protected " + Member.ReturnType.IdlTypeName;
                }
                else if (RequestedInterface == RequestedInterface.StaticMembers)
                {
                    return "static " + Member.ReturnType.IdlTypeName;
                }

                return Member.ReturnType.IdlTypeName;
            }

        }

        public string OverloadName
        {
            get
            {
                if (RequestedInterface == RequestedInterface.VirtualMembers && Member.Modifier == OM.Modifier.Public)
                {
                    return Member.VirtualOverloadName;
                }
                return Member.OverloadName;
            }
        }

        private MethodModel()
        {
        }

        public string Modifier
        {
            get
            {
                string memberModifer = Member.Modifier.ToString();
                if (RequestedInterface == RequestedInterface.VirtualMembers)
                {
                    memberModifer = "virtual " + memberModifer;
                }
                return memberModifer;
            }
        }

        /// <summary>
        /// Creates a model for the public members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static MethodModel Public(MethodDefinition method)
        {
            return new MethodModel()
            {
                Member = method,
                RequestedInterface = RequestedInterface.PublicMembers
            };
        }

        /// <summary>
        /// Creates a model for the protected members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static MethodModel Protected(MethodDefinition method)
        {
            return new MethodModel()
            {
                Member = method,
                RequestedInterface = RequestedInterface.ProtectedMembers
            };
        }

        /// <summary>
        /// Creates a model for the virtuals members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static MethodModel Virtual(MethodDefinition method)
        {
            return new MethodModel()
            {
                Member = method,
                RequestedInterface = RequestedInterface.VirtualMembers
            };
        }

        /// <summary>
        /// Creates a model for the static members interface.
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        public static MethodModel Static(MethodDefinition method)
        {
            return new MethodModel()
            {
                Member = method,
                RequestedInterface = RequestedInterface.StaticMembers
            };
        }
    }
}
