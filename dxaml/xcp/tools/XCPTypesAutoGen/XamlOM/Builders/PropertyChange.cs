// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the type of property change handler to create.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Property, Inherited = false)]
    public class PropertyChangeAttribute : Attribute, NewBuilders.IClassBuilder, NewBuilders.IPropertyBuilder
    {
        public PropertyChangeCallbackType PropertyChangeCallbackType
        {
            get;
            set;
        }

        public PropertyChangeAttribute(PropertyChangeCallbackType propertyChangeCallbackType)
        {
            PropertyChangeCallbackType = propertyChangeCallbackType;
        }

        void NewBuilders.IClassBuilder.BuildNewClass(ClassDefinition definition, Type source)
        {
            definition.HasPropertyChangeCallback = (PropertyChangeCallbackType == PropertyChangeCallbackType.OnPropertyChangeCallback);
        }

        void NewBuilders.IPropertyBuilder.BuildNewProperty(PropertyDefinition definition, PropertyInfo source)
        {
            definition.HasPropertyChangeCallback = PropertyChangeCallbackType != PropertyChangeCallbackType.NoCallback;
        }
    }
}
