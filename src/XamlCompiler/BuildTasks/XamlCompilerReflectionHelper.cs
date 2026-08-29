// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Core
{
    using System;
    using System.IO;
    using System.Reflection;
    using System.Xaml;
    using System.Xml;
    using System.Collections.Generic;
    using XamlDom;
    using XBF;

    // This class is being used by unit tests.
    internal class XamlCompilerReflectionHelper
    {
        static string testSourceFile = string.Empty;

        public XamlDomObject CreateCompilerDomRoot(XamlReader xamlReader)
        {
            XamlDomWriter domWriter = new XamlDomWriter(xamlReader.SchemaContext, testSourceFile);
            XamlServices.Transform(xamlReader, domWriter, true);

            XamlDomObject domRoot = domWriter.RootNode as XamlDomObject;
            return domRoot;
        }

        public XamlDomObject CreateDomRoot(String xamlString, XamlSchemaContext schema, Assembly localAssembly)
        {
            // Create an XML reader from the string
            //
            TextReader textReader = new StringReader(xamlString);
            XmlReader xmlReader = XmlReader.Create(textReader);

            // Create XAML reader setting that are good for harvesting.
            //
            XamlXmlReaderSettings settings = new XamlXmlReaderSettings();
            settings.LocalAssembly = localAssembly;
            settings.AllowProtectedMembersOnRoot = true;
            settings.ProvideLineInfo = true;

            // Create the XAML reader with the Schema and the Settings.
            //
            XamlXmlReader xamlReader = new XamlXmlReader(xmlReader, schema, settings);

            // Create a XAML DOM builder and load the XAML into it.
            //
            XamlDomWriter domWriter = new XamlDomWriter(schema, testSourceFile);
            XamlServices.Transform(xamlReader, domWriter, true);

            XamlDomObject domRoot = domWriter.RootNode as XamlDomObject;
            return domRoot;
        }

        public static IEnumerable<IXbfFileNameInfo> CreateXbfFilenameInfoArray(string[] filenames)
        {
            if (filenames.Length % 3 != 0)
            {
                throw new ArgumentException("Array of filenames must be a multiple of 3 in length");
            }
            int n = filenames.Length / 3;
            var a = new List<IXbfFileNameInfo>();
            for (int i = 0; i < n; i++)
            {
                int idx = i * 3;
                a.Add(new XbfFileNameInfo(filenames[idx], filenames[idx], filenames[idx + 1], filenames[idx + 2]));
            }
            return a;
        }
    }
}