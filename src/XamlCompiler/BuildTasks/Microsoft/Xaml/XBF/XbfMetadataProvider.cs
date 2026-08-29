// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Collections.Generic;
    using DirectUI;

    internal class XbfMetadataProvider : IXbfMetadataProvider, IXmpXamlType
    {
        private DirectUISchemaContext _schema;
        private Dictionary<string, IXbfType> _standardNames = new Dictionary<string, IXbfType>();
        private Dictionary<DirectUIXamlType, IXbfType> _typeMap = new Dictionary<DirectUIXamlType, IXbfType>();

        public XbfMetadataProvider(DirectUISchemaContext schema)
        {
            this._schema = schema;
        }

        public IXbfType GetXamlType(string fullName)
        {
            IXbfType xmpXamlType = null;

            if (!this._standardNames.TryGetValue(fullName, out xmpXamlType))
            {
                DirectUIXamlType xamlType = (DirectUIXamlType)this._schema.GetXamlType(fullName);
                xmpXamlType = this.GetXmpXamlType(xamlType);
            }
            return xmpXamlType;
        }

        public IXbfType GetXamlType(Type type)
        {
            DirectUIXamlType xamlType = (DirectUIXamlType)this._schema.GetXamlType(type);
            return this.GetXmpXamlType(xamlType);
        }

        public IXbfType GetXmpXamlType(DirectUIXamlType xamlType)
        {
            if (xamlType == null)
            {
                return null;
            }

            IXbfType xmpXamlType;
            if (!this._typeMap.TryGetValue(xamlType, out xmpXamlType))
            {
                xmpXamlType = new XbfXamlType(xamlType, this);
                this._typeMap.Add(xamlType, xmpXamlType);
                if (xmpXamlType.FullName != xamlType.UnderlyingType.FullName)
                {
                    this._standardNames.Add(xmpXamlType.FullName, xmpXamlType);
                }
            }
            return xmpXamlType;
        }

        public object[] GetXmlnsDefinitions()
        {
            throw new NotImplementedException();
        }
    }
}