// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.IO;
using System.Xaml;
using System.Xml;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    class XamlNodeStreamHelper
    {
        public static string ReadXClassFromXamlFileStream(TextReader fileStream, XamlSchemaContext schemaContext)
        {
            string className = null;
            using (XmlReader xmlReader = XmlReader.Create(fileStream))
            {
                var xamlReader = new XamlXmlReader(xmlReader, schemaContext);
                className = ReadXClassFromXamlReader(xamlReader);
            }
            return className;
        }

        public static string ReadXClassFromXamlReader(XamlXmlReader xamlReader)
        {
            int startObjectCount = 0;
            bool inClassMember = false;
            while (xamlReader.Read())
            {
                switch (xamlReader.NodeType)
                {
                    // Don't read too far down.  The x:Class must be on the first object.
                    // ME's are objects in the node stream so skip them.
                    case XamlNodeType.StartObject:
                        if (!xamlReader.Type.IsMarkupExtension)
                        {
                            startObjectCount += 1;
                            if (startObjectCount > 1)
                            {
                                return null;
                            }
                        }
                        break;
                       
                    case XamlNodeType.StartMember:
                        if (xamlReader.Member == XamlLanguage.Class)
                        {
                            inClassMember = true;
                        }
                        break;

                    case XamlNodeType.Value:
                        if (inClassMember)
                        {
                            return (string)xamlReader.Value;
                        }
                        break;
                }
            }
            return null;
        }
    }
}
