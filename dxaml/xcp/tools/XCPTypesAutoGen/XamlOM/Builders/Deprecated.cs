// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the comment to generate.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Property | AttributeTargets.Method | AttributeTargets.Event | AttributeTargets.Field, Inherited = false, AllowMultiple = true)]
    public class DeprecatedAttribute :
        Attribute,
        NewBuilders.ITypeBuilder, NewBuilders.IMemberBuilder
    {
        public string Comment
        {
            get;
            private set;
        }

        public int ContractVersion
        {
            get;
            set;
        }

        public DeprecationType Type
        {
            get;
            private set;
        }


        public DeprecatedAttribute(string comment, DeprecationType type)
        {
            Comment = comment;
            Type = type;
            ContractVersion = OSVersions.InitialUAPVersion;
        }

        public DeprecatedAttribute(string comment) : this(comment, DeprecationType.Deprecate)
        {
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            var deprecation = new OM.DeprecationDefinition()
            {
                Comment = Comment,
                Type = (OM.DeprecationType)Type,
            };

            deprecation.Contracts.Add(XamlOM.NewBuilders.ModelFactory.CreateContractReference(typeof(Microsoft.UI.Xaml.WinUIContract), this.ContractVersion));
            definition.Deprecations.Add(deprecation);
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            var deprecation = new OM.DeprecationDefinition()
            {
                Comment = Comment,
                Type = (OM.DeprecationType)Type,
            };

            deprecation.Contracts.Add(XamlOM.NewBuilders.ModelFactory.CreateContractReference(typeof(Microsoft.UI.Xaml.WinUIContract), this.ContractVersion));
            definition.Deprecations.Add(deprecation);
        }
    }
}
