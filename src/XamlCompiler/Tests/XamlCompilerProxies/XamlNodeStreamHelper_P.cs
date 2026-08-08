// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.IO;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlNodeStreamHelper
    {
        static ProxyHelper _xamlProjectInfoType;
        static MethodInfo _readXClassFromXamlFileStreamMethod;

        static XamlNodeStreamHelper()
        {
            _xamlProjectInfoType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Utilities.XamlNodeStreamHelper");
            _readXClassFromXamlFileStreamMethod = _xamlProjectInfoType.GetMethod("ReadXClassFromXamlFileStream", 2, bflags: BindingFlags.Public | BindingFlags.Static);
        }

        public static string ReadXClassFromXamlFileStream(TextReader textReader, object schemaContext)
        {
            object[] args = new object[] { textReader, schemaContext };
            return (string) _readXClassFromXamlFileStreamMethod.Invoke(null, args);
        }
    }
}
