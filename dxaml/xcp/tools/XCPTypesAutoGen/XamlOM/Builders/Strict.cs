// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;
using OM;

namespace XamlOM
{
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public class StrictTypeAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public StrictTypeAttribute()
        {
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.IsStrict = true;
        }
    }

    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Method, Inherited = false)]
    public class StrictnessAttribute : Attribute, NewBuilders.IMemberBuilder
    {
        private Strictness _strictness = Strictness.Agnostic;

        public StrictnessAttribute(Strictness strictness)
        {
            _strictness = strictness;
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.Strictness = _strictness;
        }
    }
}
