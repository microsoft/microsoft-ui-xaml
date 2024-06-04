// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;
using OM;
using XamlOM.NewBuilders;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the property is an attached property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Event, Inherited = false)]
    public class AttachedAttribute : Attribute, NewBuilders.IAttachedPropertyBuilder, NewBuilders.IAttachedEventBuilder
    {
        /// <summary>
        /// Gets or sets the parameter name of the value parameter in the setter method.
        /// If no value is specified, the default name 'value' will be used.
        /// </summary>
        public string SetterParameterName
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the parameter name of the target parameter in the getter/setter or add/remove methods.
        /// If no value is specified, the default name 'element' will be used.
        /// </summary>
        public string TargetParameterName
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the type to which the property or event attaches.
        /// </summary>
        public Type TargetType
        {
            get;
            set;
        }

        public void BuildNewProperty(OM.AttachedPropertyDefinition definition, PropertyInfo source)
        {
            if (!string.IsNullOrEmpty(SetterParameterName))
            {
                definition.PropertyType.Name = SetterParameterName;
            }

            if (TargetType != null)
            {
                definition.TargetType = NewBuilders.Helper.GetDPTargetTypeRef(TargetType);
            }

            if (!string.IsNullOrEmpty(TargetParameterName))
            {
                definition.TargetType.Name = TargetParameterName;
            }
        }

        public void BuildNewEvent(AttachedEventDefinition definition, EventInfo source)
        {
            if (TargetType != null)
            {
                definition.TargetType = NewBuilders.Helper.GetDPTargetTypeRef(TargetType);
            }

            if (!string.IsNullOrEmpty(TargetParameterName))
            {
                definition.TargetType.Name = TargetParameterName;
            }
        }
    }
}
