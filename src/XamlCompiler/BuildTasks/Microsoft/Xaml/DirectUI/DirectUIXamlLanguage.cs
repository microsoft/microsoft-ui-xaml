// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System;
    using System.Xaml;

    internal class DirectUIXamlLanguage : IDirectUIXamlLanguage
    {
        private DirectUISchemaContext schema;
        private Lazy<XamlType> objectXamlType;
        private Lazy<XamlType> stringXamlType;
        private Lazy<XamlType> doubleXamlType;
        private Lazy<XamlType> int32XamlType;
        private Lazy<XamlType> booleanXamlType;
        private Lazy<XamlType> uiElementXamlType;
        private Lazy<XamlType> nullExtensionXamlType;
        private Lazy<XamlType> staticResourceExtensionXamlType;
        private Lazy<XamlType> customResourceExtensionXamlType;
        private Lazy<XamlType> bindExtensionXamlType;
        private Lazy<XamlType> propertiesXamlType;
        private Lazy<XamlType> propertyXamlType;

        public DirectUIXamlLanguage(DirectUISchemaContext schema, bool isStringNullable)
        {
            this.schema = schema;
            this.objectXamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.Object));
            this.stringXamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.String));
            this.doubleXamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.Double));
            this.int32XamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.Int32));
            this.booleanXamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.Boolean));
            this.uiElementXamlType = new Lazy<XamlType>(() => this.GetDirectUIXamlType(this.schema.DirectUISystem.UIElement));
            this.nullExtensionXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.NullExtension.Name));
            this.staticResourceExtensionXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.StaticResourceExtension.Name));
            this.customResourceExtensionXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.CustomResourceExtension.Name));
            this.bindExtensionXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.BindExtension.Name));
            this.propertiesXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.Properties.Name));
            this.propertyXamlType = new Lazy<XamlType>(() => this.GetDirectUIProxyXamlType(TypeProxyMetadata.Property.Name));
            this.IsStringNullable = isStringNullable;
        }

        public XamlType Object { get { return this.objectXamlType.Value; } }
        public XamlType String { get { return this.stringXamlType.Value; } }
        public XamlType Double { get { return this.doubleXamlType.Value; } }
        public XamlType Int32 { get { return this.int32XamlType.Value; } }
        public XamlType Boolean { get { return this.booleanXamlType.Value; } }
        public XamlType NullExtension { get { return this.nullExtensionXamlType.Value; } }
        public XamlType StaticResourceExtension { get { return this.staticResourceExtensionXamlType.Value; } }
        public XamlType CustomResourceExtension { get { return this.customResourceExtensionXamlType.Value; } }
        public XamlType BindExtension { get { return this.bindExtensionXamlType.Value; } }
        public XamlType UIElement { get { return this.uiElementXamlType.Value; } }
        public XamlType Properties { get { return this.propertiesXamlType.Value; } }
        public XamlType Property { get { return this.propertyXamlType.Value; } }
        public bool IsStringNullable { get; }

        // This implements the x: namespace for objects
        //  <x:String/>  Foo={x:Null}  etc
        public XamlType LookupXamlObjects(string name)
        {
            switch (name)
            {
                case "Null":
                case "NullExtension":
                    return this.NullExtension;
                case "String":
                    return this.String;
                case "Double":
                    return this.Double;
                case "Int32":
                    return this.Int32;
                case "Boolean":
                    return this.Boolean;
                case "Bind":
                    return this.BindExtension;
                case "Object":
                    return this.Object;
                case "Properties":
                    return this.Properties;
                case "Property":
                    return this.Property;
                default:
                    return null;
            }
        }

        private XamlType GetDirectUIXamlType(Type type)
        {
            return this.schema.GetXamlType(type);
        }

        private XamlType GetDirectUIProxyXamlType(string name)
        {
            return  this.schema.GetProxyType(KnownNamespaceUris.Wpf2006, name);
        }
    }
}