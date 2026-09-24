// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class FieldDefinition
    {
        static ProxyHelper _fieldDefinitionType;
        static PropertyInfo _fieldNameProperty;
        static PropertyInfo _fieldTypePathProperty;
        static PropertyInfo _fieldTypeShortNameProperty;
        static PropertyInfo _fieldTypeNameProperty;
        static PropertyInfo _fieldTypeProperty;
        static PropertyInfo _isValueTypeProperty;
        static PropertyInfo _isSystemTypeProperty;
        static PropertyInfo _isDeprecatedProperty;

        object _instance;

        static FieldDefinition()
        {
            _fieldDefinitionType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.FieldDefinition");
            _fieldNameProperty = _fieldDefinitionType.GetProperty("FieldName");
            _fieldTypePathProperty = _fieldDefinitionType.GetProperty("FieldTypePath");
            _fieldTypeShortNameProperty = _fieldDefinitionType.GetProperty("FieldTypeShortName");
            _fieldTypeNameProperty = _fieldDefinitionType.GetProperty("FieldTypeName");
            _fieldTypeProperty = _fieldDefinitionType.GetProperty("FieldType");
            _isValueTypeProperty = _fieldDefinitionType.GetProperty("IsValueType"); ;
            _isSystemTypeProperty = _fieldDefinitionType.GetProperty("IsSystemType"); ;
            _isDeprecatedProperty = _fieldDefinitionType.GetProperty("IsDeprecated"); ;
        }

        public FieldDefinition(object instance)
        {
            _instance = instance;
        }

        public String FieldName
        {
            get { return (string)_fieldNameProperty.GetValue(_instance, null); }
        }

        public String FieldTypeName
        {
            get { return (string)_fieldTypeNameProperty.GetValue(_instance, null); }
        }

        public String FieldTypePath
        {
            get { return (string)_fieldTypePathProperty.GetValue(_instance, null); }
        }

        public String FieldTypeShortName
        {
            get { return (string)_fieldTypeShortNameProperty.GetValue(_instance, null); }
        }

        public bool IsValueType
        {
            get { return (bool)_isValueTypeProperty.GetValue(_instance, null); }
        }

        public bool IsSystemType
        {
            get { return (bool)_isSystemTypeProperty.GetValue(_instance, null); }
        }

        public bool IsDeprecatedType
        {
            get { return (bool)_isDeprecatedProperty.GetValue(_instance, null); }
        }

    }
}
