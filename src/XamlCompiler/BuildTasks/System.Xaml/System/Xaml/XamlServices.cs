// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.IO;
using System.Globalization;
using System.Xaml;

namespace System.Xaml
{
    public static class XamlServices
    {
        public static void Transform(XamlReader xamlReader, XamlWriter xamlWriter)
        {
            // arguments are validated by the callee here.
            Transform(xamlReader, xamlWriter, true);
        }

        public static void Transform(XamlReader xamlReader, XamlWriter xamlWriter, bool closeWriter)
        {
            if (xamlReader == null)
            {
                throw new ArgumentNullException("xamlReader");
            }

            if (xamlWriter == null)
            {
                throw new ArgumentNullException("xamlWriter");
            }

            IXamlLineInfo xamlLineInfo = xamlReader as IXamlLineInfo;
            IXamlLineInfoConsumer xamlLineInfoConsumer = xamlWriter as IXamlLineInfoConsumer;
            bool shouldPassLineNumberInfo = false;
            if ((xamlLineInfo != null && xamlLineInfo.HasLineInfo)
                && (xamlLineInfoConsumer != null && xamlLineInfoConsumer.ShouldProvideLineInfo))
            {
                shouldPassLineNumberInfo = true;
            }

            while (xamlReader.Read())
            {
                if (shouldPassLineNumberInfo)
                {
                    if (xamlLineInfo.LineNumber != 0)
                    {
                        xamlLineInfoConsumer.SetLineInfo(xamlLineInfo.LineNumber, xamlLineInfo.LinePosition);
                    }
                }
                xamlWriter.WriteNode(xamlReader);
            }

            if (closeWriter)
            {
                xamlWriter.Close();
            }
        }

    }
}
