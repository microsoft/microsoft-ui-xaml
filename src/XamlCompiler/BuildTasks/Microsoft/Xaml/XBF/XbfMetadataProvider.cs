// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices.Marshalling;
    using DirectUI;

    /// <summary>
    /// The XAML type information GenXbf.dll needs to generate XBF, which it gets as a COM object (see XbfComInterfaces.cs).
    /// The objects themselves are the COM servers (rather than separate wrapper objects), so that every managed instance maps to a
    /// single, stable COM identity. The members GenXbf.dll doesn't use throw NotImplementedException (E_NOTIMPL).
    /// </summary>
    [GeneratedComClass]
    internal sealed partial class XbfMetadataProvider : IXbfMetadataProvider
    {
        private DirectUISchemaContext _schema;
        private Dictionary<string, XbfXamlType> _standardNames = new Dictionary<string, XbfXamlType>();
        private Dictionary<DirectUIXamlType, XbfXamlType> _typeMap = new Dictionary<DirectUIXamlType, XbfXamlType>();

        public XbfMetadataProvider(DirectUISchemaContext schema)
        {
            this._schema = schema;
        }

        public XbfXamlType GetXamlType(string fullName)
        {
            XbfXamlType xmpXamlType = null;

            if (!this._standardNames.TryGetValue(fullName, out xmpXamlType))
            {
                DirectUIXamlType xamlType = (DirectUIXamlType)this._schema.GetXamlType(fullName);
                xmpXamlType = this.GetXmpXamlType(xamlType);
            }
            return xmpXamlType;
        }

        public XbfXamlType GetXmpXamlType(DirectUIXamlType xamlType)
        {
            if (xamlType == null)
            {
                return null;
            }

            XbfXamlType xmpXamlType;
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

        IXbfType IXbfMetadataProvider.GetXamlType(XbfTypeName type) => throw new NotImplementedException();

        IXbfType IXbfMetadataProvider.GetXamlTypeByFullName(string fullName) => GetXamlType(fullName);

        unsafe void IXbfMetadataProvider.GetXmlnsDefinitions(uint* length, void** definitions) => throw new NotImplementedException();
    }
}