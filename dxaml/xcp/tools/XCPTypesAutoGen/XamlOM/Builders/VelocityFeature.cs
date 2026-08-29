// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    public enum VelocityOptions
    {
        None,
        SkipRuntimeChecks
    }

    /// <summary>
    /// Specifies the velocity feature of a class, member or enum.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public class VelocityFeatureAttribute : Attribute, NewBuilders.IClassBuilder, NewBuilders.IMemberBuilder, NewBuilders.IEnumBuilder
    {
        public string Name
        {
            get;
            private set;
        }



        public VelocityFeatureAttribute(string name)
        {
            Name = name;
        }

        public VelocityFeatureAttribute(string name, VelocityOptions options)
        {
            Name = name;
            int id = VelocityFeatures.GetVersion(Name);
            if (options == VelocityOptions.SkipRuntimeChecks)
            {
                VelocityFeatures.SetSkipRuntimeChecks(id, true);
            }
        }

        void NewBuilders.IClassBuilder.BuildNewClass(ClassDefinition definition, Type source)
        {
            definition.VelocityVersion = VelocityFeatures.GetVersion(Name);
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            if (Attribute.IsDefined(source, typeof(VersionAttribute)))
            {
                throw new InvalidOperationException(string.Format("VelocityFeature and Version both defined for method {0} on type {1}", definition.Name, definition.DeclaringType.Name));
            }

            definition.VelocityFeatureName = Name;
            definition.Version = VelocityFeatures.GetVersion(Name);

            var enumValueDef = definition as EnumValueDefinition;
            if (enumValueDef != null)
            {
                var enumType = (EnumDefinition)enumValueDef.DeclaringType;
                if (enumType.Versions.Find(v => v.Version == definition.Version) == null)
                {
                    throw new InvalidOperationException(string.Format("Platform entry for Velocity feature {0} (used on enum value {1}) not found on enum {2}", Name, definition.Name, definition.DeclaringType.Name));
                }
                var contracts = enumType.GetVersion(definition.Version).SupportedContracts;
                definition.SupportedContracts.AddRange(contracts);
            }
            else if (definition.DeclaringClass.Versions.Find(v => v.Version == definition.Version) == null)
            {
                throw new InvalidOperationException(string.Format("Platform entry for Velocity feature {0} (used on method {1}) not found on type {2}", Name, definition.Name, definition.DeclaringType.Name));
            }

            OM.ClassDefinition declaringClass = definition.DeclaringType as OM.ClassDefinition;
            if (declaringClass != null)
            {
                var contracts = declaringClass.GetVersion(definition.Version).SupportedContracts;
                definition.SupportedContracts.AddRange(contracts);
            }
        }

        void NewBuilders.IEnumBuilder.BuildNewEnum(EnumDefinition definition, Type source)
        {
            definition.VelocityVersion = VelocityFeatures.GetVersion(Name);
        }
    }
}
