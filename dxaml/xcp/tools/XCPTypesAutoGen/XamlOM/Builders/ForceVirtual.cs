// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Forces a member to be considered virtual.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class ForceVirtualAttribute : Attribute, NewBuilders.IMemberBuilder
    {
        public ForceVirtualAttribute()
        {
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.IsVirtual = true;
        }
    }
}
