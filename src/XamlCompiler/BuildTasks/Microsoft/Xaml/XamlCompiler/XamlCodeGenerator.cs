// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal class XamlCodeGenerator
    {
        private Language _language;
        private bool _isPass1;
        XamlProjectInfo _projectInfo;
        XamlSchemaCodeInfo _schemaInfo;

        public XamlCodeGenerator(Language language, bool isPass1, XamlProjectInfo projectInfo, XamlSchemaCodeInfo schemaInfo)
        {
            _language = language;
            _isPass1 = isPass1;
            _projectInfo = projectInfo;
            _schemaInfo = schemaInfo;
        }

        public List<FileNameAndContentPair> GenerateCodeBehind(XamlClassCodeInfo codeInfo, out IEnumerable<FileNameAndChecksumPair> xamlFilesChecksumPairs)
        {
            CodeGeneratorDelegate codeGenDelegate;
            T4Base codeGenerator;
            xamlFilesChecksumPairs = null;
            if (codeInfo.IsApplication)
            {
                codeGenDelegate = _isPass1 ? _language.AppPass1CodeGenerator : _language.AppPass2CodeGenerator;
            }
            else
            {
                codeGenDelegate = _isPass1 ? _language.PagePass1CodeGenerator : _language.PagePass2CodeGenerator;
            }
            if (codeGenDelegate == null)
            {
                return null;
            }
            codeGenerator = codeGenDelegate();
            var model = new PageDefinition(_projectInfo, _schemaInfo) { CodeInfo = codeInfo };
            codeGenerator.SetModel(_projectInfo, _schemaInfo, model);
          
            String code = codeGenerator.TransformText();
            xamlFilesChecksumPairs = model.XamlFileFullPathAndCheckSums;

            Debug.Assert(!String.IsNullOrEmpty(codeInfo.BaseFileName));
            string codeFileName = codeInfo.BaseFileName + (_isPass1 ? _language.Pass1Extension : _language.Pass2Extension);
            var retList = new List<FileNameAndContentPair>();
            retList.Add(new FileNameAndContentPair(codeFileName, code));
            return retList;
        }


        public List<FileNameAndContentPair> GenerateTypeInfo(ClassName appXamlInfo)
        {
            CodeGeneratorDelegate codeGenDelegate = _isPass1 ? 
                _language.TypeInfoPass1CodeGenerator : 
                _language.TypeInfoPass2CodeGenerator;

            String code = GenerateTypeInfoCode(codeGenDelegate, appXamlInfo);
            if (code == null)
            {
                return null;
            }

            var retList = new List<FileNameAndContentPair>();
            string fileName = "XamlTypeInfo" + (_isPass1 ? _language.Pass1Extension : _language.Pass2Extension);

            // C++ language extension is .g.hpp rather than CPP for all except XamlTypeInfo
            if (!_isPass1 && fileName.EndsWith(".g.hpp"))
            {
                fileName = "XamlTypeInfo.g.cpp";
            }

            retList.Add(new FileNameAndContentPair(fileName, code));

            // Generate the extra pass1 file
            if (_isPass1)
            {
                code = GenerateTypeInfoCode(_language.XamlMetaDataProviderPass1, appXamlInfo);
                if (code != null)
                {
                    fileName = "XamlMetaDataProvider.h";
                    retList.Add(new FileNameAndContentPair(fileName, code));
                }

                code = GenerateTypeInfoCode(_language.XamlMetaDataProviderPass2, appXamlInfo);
                if (code != null)
                {
                    fileName = "XamlLibMetadataProvider.g.cpp";
                    retList.Add(new FileNameAndContentPair(fileName, code));
                }

                code = GenerateTypeInfoCode(_language.TypeInfoPass1ImplCodeGenerator, appXamlInfo);
                if (code != null)
                {
                    fileName = "XamlTypeInfo.Impl.g.cpp";
                    retList.Add(new FileNameAndContentPair(fileName, code));
                }
            }

            return retList;
        }

        public List<FileNameAndContentPair> GenerateBindingInfo(
                Dictionary<string, XamlType> observableVectorTypes,
                Dictionary<string, XamlType> observableMapTypes,
                Dictionary<string, XamlMember> bindingSetters,
                bool eventBindingUsed)
        {
            var retList = new List<FileNameAndContentPair>();
            string filename = KnownStrings.XamlBindingInfo + (_isPass1 ? _language.Pass1Extension : _language.Pass2Extension);

            Debug.Assert(_language.IsNative, "Binding infos are only supposed to be generated for native");

            T4Base codeGenerator = _isPass1 ? _language.BindingInfoPass1CodeGenerator() : _language.BindingInfoPass2CodeGenerator();
            codeGenerator.SetModel(
                _projectInfo, 
                _schemaInfo, 
                new BindingInfoDefinition(_projectInfo, _schemaInfo)
                {
                    ObservableVectorTypes = observableVectorTypes,
                    ObservableMapTypes = observableMapTypes,
                    BindingSetters = bindingSetters,
                    EventBindingUsed = eventBindingUsed,
                });
            

            String code = codeGenerator.TransformText();
            Debug.Assert(code != null);

            if (code != null)
            {
                retList.Add(new FileNameAndContentPair(filename, code));
            }
            return retList;
        }

        string GenerateTypeInfoCode(
                CodeGeneratorDelegate codeGenDelegate,
                ClassName appXamlInfo)
        {
            if (codeGenDelegate == null)
            {
                return null;
            }
            T4Base codeGenerator;
            codeGenerator = codeGenDelegate();
            codeGenerator.SetModel(_projectInfo, _schemaInfo,
                new TypeInfoDefinition(_projectInfo, _schemaInfo) {
                    AppXamlInfo = appXamlInfo
                });
            return codeGenerator.TransformText();
        }
    }
}
