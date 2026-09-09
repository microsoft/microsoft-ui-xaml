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
    public class FileNameAndContentPair
    {
        static ProxyHelper _fileNameAndContentPairType;
        static PropertyInfo _fileNameProperty;
        static PropertyInfo _contentsProperty;

        object _instance;

        static FileNameAndContentPair()
        {
            _fileNameAndContentPairType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.FileNameAndContentPair");
            _fileNameProperty = _fileNameAndContentPairType.GetProperty("FileName");
            _contentsProperty = _fileNameAndContentPairType.GetProperty("Contents");
        }

        public FileNameAndContentPair() { }

        public FileNameAndContentPair(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public string FileName
        {
            get { return (string)_fileNameProperty.GetValue(_instance, null); }
        }

        public string Contents
        {
            get { return (string)_contentsProperty.GetValue(_instance, null); }
        }

    }
}
