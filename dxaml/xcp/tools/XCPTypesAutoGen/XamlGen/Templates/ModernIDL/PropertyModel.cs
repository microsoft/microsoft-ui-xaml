// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.ModernIDL
{
    public class PropertyModel : MemberModel<PropertyDefinition>
    {
        public bool HasGet
        {
            get
            {
                switch (RequestedInterface)
                {
                    case RequestedInterface.PublicMembers:
                        return (Member.Modifier == Modifier.Public);
                    case RequestedInterface.ProtectedMembers:
                        return (Member.Modifier == Modifier.Protected);
                    case RequestedInterface.VirtualMembers:
                        return Member.IsVirtual;
                    case RequestedInterface.StaticMembers:
                        return (Member.SetterModifier == Modifier.Public);
                    default:
                        return false;
                }
            }
        }

        public bool HasSet
        {
            get
            {
                if (Member.IdlPropertyInfo.IsReadOnly)
                {
                    return false;
                }

                switch (RequestedInterface)
                {
                    case RequestedInterface.PublicMembers:
                        return (Member.SetterModifier == Modifier.Public);
                    case RequestedInterface.ProtectedMembers:
                        return (Member.SetterModifier == Modifier.Protected);
                    case RequestedInterface.VirtualMembers:
                        return Member.IsVirtual;
                    case RequestedInterface.StaticMembers:
                        return (Member.SetterModifier == Modifier.Public);
                    default:
                        return false;
                }
            }
        }

        private PropertyModel()
        {
        }

        /// <summary>
        /// Creates a model for the public members interface.
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        public static PropertyModel Public(PropertyDefinition property)
        {
            return new PropertyModel()
            {
                Member = property,
                RequestedInterface = RequestedInterface.PublicMembers
            };
        }

        /// <summary>
        /// Creates a model for the protected members interface.
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        public static PropertyModel Protected(PropertyDefinition property)
        {
            return new PropertyModel()
            {
                Member = property,
                RequestedInterface = RequestedInterface.ProtectedMembers
            };
        }

        /// <summary>
        /// Creates a model for the virtuals members interface.
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        public static PropertyModel Virtual(PropertyDefinition property)
        {
            return new PropertyModel()
            {
                Member = property,
                RequestedInterface = RequestedInterface.VirtualMembers
            };
        }

        /// <summary>
        /// Creates a model for the static members interface.
        /// </summary>
        /// <param name="property"></param>
        /// <returns></returns>
        public static PropertyModel Static(PropertyDefinition property)
        {
            return new PropertyModel()
            {
                Member = property,
                RequestedInterface = RequestedInterface.StaticMembers
            };
        }
    }
}
