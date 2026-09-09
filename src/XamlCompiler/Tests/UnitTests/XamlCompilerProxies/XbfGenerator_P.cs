// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------
namespace Win8Xaml.CompilerProxies
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Reflection;

    public class XbfGenerator
    {
        private static ProxyHelper _xbfGenerator;
        private static MethodInfo _xbfSetXamlInputFilesMethod;
        private static MethodInfo _xbfGenerateXbfFilesMethod;
        private static PropertyInfo _xbfErrorsProperty;

        private object _instance;

        static XbfGenerator()
        {
            XbfGenerator._xbfGenerator = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XBF.XbfGenerator");
            XbfGenerator._xbfSetXamlInputFilesMethod = XbfGenerator._xbfGenerator.GetMethod("SetXamlInputFiles");
            XbfGenerator._xbfGenerateXbfFilesMethod = XbfGenerator._xbfGenerator.GetMethod("GenerateXbfFiles");
            XbfGenerator._xbfErrorsProperty = XbfGenerator._xbfGenerator.GetProperty("XbfErrors");
        }

        public XbfGenerator(XamlProjectInfo projectInfo, XbfMetadataProvider xbfMetadataProvider)
        {
            object[] args = new object[] { projectInfo?.Instance, xbfMetadataProvider?.Instance};
            this._instance = XbfGenerator._xbfGenerator.CreateInstance(args);
        }

        public void SetXamlInputFiles(XbfFilenameInfoArrayToken filenames)
        {
            object[] args = new object[] { filenames.Instance };
            XbfGenerator._xbfSetXamlInputFilesMethod.Invoke(_instance, args);
        }

        public bool GenerateXbfFiles(uint xbfGenerationFlags = 0, bool v80Compat = false)
        {
            object[] args = new object[] { xbfGenerationFlags, v80Compat };
            object ret = XbfGenerator._xbfGenerateXbfFilesMethod.Invoke(_instance, args);
            return (bool)ret;
        }

        public List<XamlCompileError> XbfErrors
        {
            get
            {
                IEnumerable listVal = (IEnumerable)XbfGenerator._xbfErrorsProperty.GetValue(this._instance, null);
                List<XamlCompileError> errorList = new List<XamlCompileError>();
                XamlDomValidator.ConvertListOfErrors(errorList, listVal);
                return errorList;
            }
        }
    }
}
