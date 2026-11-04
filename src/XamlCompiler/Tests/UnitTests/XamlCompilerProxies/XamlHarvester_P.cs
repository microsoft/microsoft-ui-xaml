// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;

namespace Win8Xaml.CompilerProxies
{
    public class XamlHarvester
    {
        static ProxyHelper _xamlHarvesterType;
        static MethodInfo _getClassFullNameMethod;
        static MethodInfo _harvestClassInfoMethod;
        static MethodInfo _harvestXamlFileInfoMethod;

        object _instance;

        static XamlHarvester()
        {
            _xamlHarvesterType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.XamlHarvester");
            _getClassFullNameMethod = _xamlHarvesterType.GetStaticMethod("GetClassFullName");
            _harvestClassInfoMethod = _xamlHarvesterType.GetMethod("HarvestClassInfo");
            _harvestXamlFileInfoMethod = _xamlHarvesterType.GetMethod("HarvestXamlFileInfo");
        }

        public XamlHarvester(String projectPath, bool isPass1 )
        {
            var targetPlatformType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Platform");
            Object[] args = new Object[] { projectPath, isPass1, targetPlatformType.CreateInstance() };
            _instance = _xamlHarvesterType.CreateInstance(args);
        }

        public string GetClassFullName(CompilerDomRootToken domRootToken)
        {
            Object[] args = new Object[] { domRootToken.Instance };
            Object result = _getClassFullNameMethod.Invoke(_instance, args);
            return (string)result;
        }

        public XamlClassCodeInfo HarvestClassInfo(string classFullName, CompilerDomRootToken domRootToken, bool isPass1, bool isApplication)
        {
            Object[] args = new Object[] { classFullName, domRootToken.Instance, isApplication };
            Object result = _harvestClassInfoMethod.Invoke(_instance, args);
            return new XamlClassCodeInfo(result);
        }

        public XamlFileCodeInfo HarvestXamlFileInfo(XamlClassCodeInfo classCodeInfo, CompilerDomRootToken domRootToken)
        {
            Object[] args = new Object[] { classCodeInfo.Instance, domRootToken.Instance };
            Object result = _harvestXamlFileInfoMethod.Invoke(_instance, args);
            return new XamlFileCodeInfo(result);
        }
    }
}
