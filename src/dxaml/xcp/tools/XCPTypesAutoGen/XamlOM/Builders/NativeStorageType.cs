// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Used for 'struct KnownProperty.m_type' in XcpTypes.g.h.
    /// </summary>
    [AttributeUsage(AttributeTargets.Event | AttributeTargets.Property, Inherited = false)]
    public class NativeStorageTypeAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public OM.ValueType NativeStorageType
        {
            get;
            private set;
        }

        public NativeStorageTypeAttribute(OM.ValueType valueType)
        {
            NativeStorageType = valueType;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, System.Reflection.PropertyInfo source)
        {
            definition.NativeStorageType = NativeStorageType;

            if (NativeStorageType == OM.ValueType.valueObject)
            {
                OM.ClassDefinition c = definition.PropertyType.Type as OM.ClassDefinition;
                if (c != null)
                {
                    if (c.IsStringType)
                    {
                        throw new InvalidOperationException(string.Format("Property '{0}.{1}' is of type string and cannot be backed by storage of type valueObject.", source.DeclaringType.FullName, source.Name));
                    }
                    else if (c == NewBuilders.ModelFactory.GetOrCreateType(typeof(Windows.Foundation.Uri)))
                    {
                        throw new InvalidOperationException(string.Format("Property '{0}.{1}' is of type Uri and cannot be backed by storage of type valueObject.", source.DeclaringType.FullName, source.Name));
                    }
                }
            }

            if (NativeStorageType == OM.ValueType.valueFloat)
            {
                definition.XamlPropertyFlags.StoreDoubleAsFloat = true;
            }
        }
    }
}
