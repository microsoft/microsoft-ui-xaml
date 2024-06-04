// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies what target an attribute is valid on.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class IdlAttributeTargetAttribute : Attribute, NewBuilders.IAttributeBuilder
    {
        public AttributeTargets ValidOn
        {
            get;
            private set;
        }

        public IdlAttributeTargetAttribute(AttributeTargets validOn)
        {
            ValidOn = validOn;
        }

        public void BuildNewAttribute(OM.AttributeDefinition definition, Type source)
        {
            definition.ValidOn = ValidOn;
        }
    }
}
