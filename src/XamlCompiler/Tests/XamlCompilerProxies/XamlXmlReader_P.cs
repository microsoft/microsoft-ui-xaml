// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.IO;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlXmlReader
    {
        static ProxyHelper _xamlXmlReaderProxy;

        object _instance;

        static XamlXmlReader()
        {
            _xamlXmlReaderProxy = new ProxyHelper("System.Xaml.XamlXmlReader");
        }

        public XamlXmlReader(
            StringReader reader,
            DirectUISchemaContext context,
            XamlXmlReaderSettings settings
            )
        {
            try
            {
                object[] args = new object[] { reader, context.Instance, settings.Instance };
                _instance = _xamlXmlReaderProxy.CreateInstance(args);
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public object Instance { get { return _instance; } }
    }

    public class XamlXmlReaderSettings
    {
        static ProxyHelper _xamlXmlReaderSettingsProxy;
        static PropertyInfo _provideLineInfoProperty;

        object _instance;

        static XamlXmlReaderSettings()
        {
            _xamlXmlReaderSettingsProxy = new ProxyHelper("System.Xaml.XamlXmlReaderSettings");
            _provideLineInfoProperty = _xamlXmlReaderSettingsProxy.GetProperty("ProvideLineInfo");
        }

        public XamlXmlReaderSettings()
        {
            try
            {
                _instance = _xamlXmlReaderSettingsProxy.CreateInstance();
            }
            catch (Exception e)
            {
                throw e.InnerException;
            }
        }

        public object Instance { get { return _instance; } }

        public bool ProvideLineInfo
        {
            get
            {
                object provide = _provideLineInfoProperty.GetValue(_instance, null);
                return (bool)provide;
            }
            set
            {
                _provideLineInfoProperty.SetValue(_instance, value);
            }
        }
    }
}