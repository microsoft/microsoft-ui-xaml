// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;

namespace XamlGen.Templates.Code.Framework.Headers
{
    public class ForwarderMethodModel : ForwarderMemberModel<MethodDefinition>
    {
        public string Name
        {
            get
            {
                if (ForwarderRequestedInterface == ForwarderRequestedInterface.VirtualMembers && Member.Modifier == Modifier.Public)
                {
                    return Member.IdlMethodInfo.VirtualName;
                }
                return Member.IdlMethodInfo.Name;
            }
        }

        private ForwarderMethodModel()
        {
        }

        /// <summary>
        /// Creates a model for the public members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static ForwarderMethodModel Public(MethodDefinition method)
        {
            return new ForwarderMethodModel()
            {
                Member = method,
                ForwarderRequestedInterface = ForwarderRequestedInterface.PublicMembers
            };
        }

        /// <summary>
        /// Creates a model for the protected members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static ForwarderMethodModel Protected(MethodDefinition method)
        {
            return new ForwarderMethodModel()
            {
                Member = method,
                ForwarderRequestedInterface = ForwarderRequestedInterface.ProtectedMembers
            };
        }

        /// <summary>
        /// Creates a model for the virtuals members interface.
        /// </summary>
        /// <param name="method"></param>
        /// <returns></returns>
        public static ForwarderMethodModel Virtual(MethodDefinition method)
        {
            return new ForwarderMethodModel()
            {
                Member = method,
                ForwarderRequestedInterface = ForwarderRequestedInterface.VirtualMembers
            };
        }
    }
}
