// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;
using System.Xml;

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    internal static class XamlDomServices
    {
        public static XamlDomNode Load(XamlReader xamlReader, string sourceFilePath)
        {
            IXamlLineInfo ixli = xamlReader as IXamlLineInfo;

            XamlDomWriter dw = new XamlDomWriter(xamlReader.SchemaContext, sourceFilePath);
            XamlServices.Transform(xamlReader, dw);
            return dw.RootNode;
        }

        public static void Save(XamlDomObject rootObjectNode, string fileName)
        {
            XamlSchemaContext schemaContext = rootObjectNode.Type.SchemaContext;
            XamlDomReader dr = new XamlDomReader(rootObjectNode, schemaContext);
            XmlWriterSettings xws = new XmlWriterSettings();
            xws.Indent = true;
            using (XmlWriter xw = XmlWriter.Create(fileName, xws))
            {
                XamlXmlWriter xxw = new XamlXmlWriter(xw, schemaContext);
                XamlServices.Transform(dr, xxw);
            }
        }
    }
}
