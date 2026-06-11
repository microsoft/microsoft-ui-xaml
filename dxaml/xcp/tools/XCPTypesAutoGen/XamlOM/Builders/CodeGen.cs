// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    public enum CodeGenLevel
    {
        /// <summary>
        /// [Default] [Applies to types and members.] Generate IDL and a stub class/member.
        /// </summary>
        IdlAndStub,

        /// <summary>
        /// [Applies to types and members.] Generate code in the core layer only.
        /// </summary>
        CoreOnly,

        /// <summary>
        /// [Applies to types only.] Allow for lookup in the framework layer.
        /// </summary>
        LookupOnly,

        /// <summary>
        /// [Applies to types only.] Generate stub class only.
        /// </summary>
        Stub,

        /// <summary>
        /// [Applies to types and members.] Generate IDL and type-table entries.
        /// </summary>
        Idl,

        /// <summary>
        /// [Applies to types and members.] Generate IDL only.
        /// </summary>
        IdlAndNoTypeTableEntries,

        /// <summary>
        /// [Applies to members only.] Generate IDL and a stub with parameter validation.
        /// </summary>
        IdlAndPartialStub,

        /// <summary>
        /// [Applies to constructors only.] Don't generate anything.
        /// </summary>
        Excluded,
    }

    /// <summary>
    /// Specifies what the code-generator should do with this type or member.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Delegate | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.Field | AttributeTargets.Constructor, Inherited = false)]
    public class CodeGenAttribute : 
        Attribute,
        NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder, NewBuilders.IMemberBuilder, NewBuilders.IEnumValueBuilder
    {
        public CodeGenLevel Level
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets whether to generate a partial class.
        /// </summary>
        public bool Partial
        {
            get;
            set;
        }

        public CodeGenAttribute(CodeGenLevel level)
        {
            Level = level;
        }

        public CodeGenAttribute(bool partial)
        {
            Partial = partial;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.GeneratePartialClass = Partial;
            if (Level == CodeGenLevel.CoreOnly)
            {
                definition.IsActivationDisabledInTypeTable = true;
            }
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            if (Level == CodeGenLevel.CoreOnly)
            {
                definition.IdlTypeInfo.IsExcluded = true;
                if (definition.Modifier == OM.Modifier.Public)
                {
                    definition.Modifier = OM.Modifier.Internal;
                }
            }
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            if (Level == CodeGenLevel.CoreOnly)
            {
                definition.IdlMemberInfo.IsExcluded = true;
                definition.GenerateDefaultBody = true;
            }
            else if (Level == CodeGenLevel.IdlAndPartialStub)
            {
                definition.GenerateDefaultBody = false;
            }
            else if (Level == CodeGenLevel.Idl && !source.DeclaringType.IsValueType)
            {
                definition.GenerateStub = false;
                definition.GenerateDefaultBody = false;
            }
        }

        public void BuildNewEnumValue(OM.EnumValueDefinition definition, FieldInfo source)
        {
            switch (Level)
            {
                case CodeGenLevel.CoreOnly:
                    definition.IdlEnumValueInfo.IsExcluded = true;
                    break;
                default:
                    throw new NotSupportedException();
            }
        }
    }
}
