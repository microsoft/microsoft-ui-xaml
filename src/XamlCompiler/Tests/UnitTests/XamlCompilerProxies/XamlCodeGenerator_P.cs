// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.CodeDom.Compiler;

namespace Win8Xaml.CompilerProxies
{
    public class XamlCodeGenerator
    {
        static ProxyHelper _xamlCodeGeneratorType;
        static MethodInfo _generateCodeBehindMethod;
        static MethodInfo _generateTypeInfoMethod;

        object _instance;

        static XamlCodeGenerator()
        {
            _xamlCodeGeneratorType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CodeGen.XamlCodeGenerator");
            _generateCodeBehindMethod = _xamlCodeGeneratorType.GetMethod("GenerateCodeBehind");
            _generateTypeInfoMethod = _xamlCodeGeneratorType.GetMethod("GenerateTypeInfo");
        }

        public XamlCodeGenerator(Language language, bool isPass1, XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
        {
            object[] args = new object[] { language?.Instance, isPass1, projectInfo?.Instance, schemaInfo?.Instance };
            _instance = _xamlCodeGeneratorType.CreateInstance(args);
        }

        public List<FileNameAndContentPair> GenerateCodeBehind(XamlClassCodeInfo codeInfo)
        {
            try
            {
                List<object> checksumsList = null;
                Object[] args = new Object[] { codeInfo.Instance, checksumsList };

                IEnumerable objectList = _generateCodeBehindMethod.Invoke(_instance, args) as IEnumerable;
                List<FileNameAndContentPair> fileNameAndContentPairList = new List<FileNameAndContentPair>();
                foreach (Object obj in objectList)
                {
                    FileNameAndContentPair fileNameAndContentPair = new FileNameAndContentPair(obj);
                    fileNameAndContentPairList.Add(fileNameAndContentPair);
                }
                return fileNameAndContentPairList;
            }
            catch (Exception e)
            {
                throw e.InnerException ?? e;
            }
        }

        public List<FileNameAndContentPair> GenerateTypeInfo(ClassName appXamlInfo)
        {
            Object[] args = new Object[] { appXamlInfo.Instance };
            try
            {
                IEnumerable objectList = _generateTypeInfoMethod.Invoke(_instance, args) as IEnumerable;
                List<FileNameAndContentPair> fileNameAndContentPairList = new List<FileNameAndContentPair>();
                foreach (Object obj in objectList)
                {
                    FileNameAndContentPair fileNameAndContentPair = new FileNameAndContentPair(obj);
                    fileNameAndContentPairList.Add(fileNameAndContentPair);
                }
                return fileNameAndContentPairList;
            }
            catch (Exception e)
            {
                throw e.InnerException ?? e;
            }
        }
    }
}
