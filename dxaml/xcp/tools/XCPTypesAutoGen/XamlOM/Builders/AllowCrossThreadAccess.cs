// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the member can be called from any thread.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event, Inherited = false)]
    public class AllowCrossThreadAccessAttribute : Attribute, NewBuilders.IMemberBuilder
    {
        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.AllowCrossThreadAccess = true;
        }
    }
}
