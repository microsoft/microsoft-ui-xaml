// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Win8Xaml.CompilerProxies
{
    public class XbfMetadataProvider
    {
        static ProxyHelper _xbfMetadataProviderType;

        static XbfMetadataProvider()
        {
            _xbfMetadataProviderType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XBF.XbfMetadataProvider");
        }

        object _instance;

        public XbfMetadataProvider(DirectUISchemaContext schema)
        {
            // support proxy test initialization with null args.
            object schemaInstance = (schema == null) ? null : schema.Instance;

            object[] args = new object[] { schemaInstance };
            _instance = _xbfMetadataProviderType.CreateInstance(args);
        }

        public Object Instance
        {
            get { return _instance; }
        }

    }
}
