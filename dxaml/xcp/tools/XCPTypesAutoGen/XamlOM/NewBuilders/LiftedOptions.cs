// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type has special behavior in lifted codegen.  This attribute should eventually be removed.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false)]
    public class LiftedOptionsAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        public bool ExcludeFromLiftedCodegen
        {
            get;
            set;
        }

        public LiftedOptionsAttribute()
        {
        }

        public LiftedOptionsAttribute(bool excludeFromLiftedCodegen)
        {
            ExcludeFromLiftedCodegen = excludeFromLiftedCodegen;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IdlTypeInfo.ExcludeFromLiftedCodegen = ExcludeFromLiftedCodegen;
        }
    }
}
