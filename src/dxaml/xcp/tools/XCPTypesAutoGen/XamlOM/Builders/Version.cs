// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the version of a member.
    /// </summary>
    [AttributeUsage(AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class VersionAttribute :
        Attribute,
        NewBuilders.IMemberBuilder, NewBuilders.IEnumValueBuilder
    {
        public int Version
        {
            get;
            private set;
        }

        public VersionAttribute(int version)
        {
            this.Version = version;
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.Version = Version;

            OM.ClassDefinition declaringClass = definition.DeclaringType as OM.ClassDefinition;
            if (declaringClass != null)
            {
                var contracts = declaringClass.GetVersion(Version).SupportedContracts;
                definition.SupportedContracts.AddRange(contracts);
            }
        }

        public void BuildNewEnumValue(OM.EnumValueDefinition definition, FieldInfo source)
        {
            OM.EnumDefinition declaringEnum = (OM.EnumDefinition)definition.DeclaringType;

            var contracts = declaringEnum.GetVersion(Version).SupportedContracts;
            definition.SupportedContracts.AddRange(contracts);
            definition.Version = Version;
        }
    }
}
