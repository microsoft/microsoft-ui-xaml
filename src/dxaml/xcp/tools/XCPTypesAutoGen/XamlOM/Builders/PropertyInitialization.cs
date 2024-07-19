// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the type of property initialization type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class PropertyInitializationAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public PropertyInitializationType PropertyInitializationType
        {
            get;
            set;
        }

        public PropertyInitializationAttribute(PropertyInitializationType propertyInitializationType)
        {
            PropertyInitializationType = propertyInitializationType;
        }

        void NewBuilders.IPropertyBuilder.BuildNewProperty(PropertyDefinition definition, PropertyInfo source)
        {
            definition.HasCallbackRetrievedValue = (this.PropertyInitializationType == PropertyInitializationType.CallbackRetrievedValue);
        }
    }
}
