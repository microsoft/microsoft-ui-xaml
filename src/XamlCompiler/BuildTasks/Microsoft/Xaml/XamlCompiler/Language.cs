// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;
    using Properties;
    using Utilities;

    internal delegate CodeGen.T4Base CodeGeneratorDelegate();
    
    internal static class ProgrammingLanguage
    {
        public const string CSharp = "C#";
        public const string VB = "VB";

        // C++ with CX extensions (and hats)
        public const string CppCX = "C++";

        // C++ without CX extensions
        public const string CppWinRT = "CppWinRT";

        // Types as expressed in Idl (WinRT)
        public const string IdlWinRT = "WinRT";
    }

    internal class Language
    {
        public string Name { get; }
        public bool IsExperimental { get; }
        public bool IsManaged { get; }
        public bool IsNative { get; }
        public bool IsStringNullable { get; }
        public string Pass1Extension { get; }
        public string Pass2Extension { get; }

        // Codegen Delegates
        public CodeGeneratorDelegate AppPass1CodeGenerator { get; }
        public CodeGeneratorDelegate AppPass2CodeGenerator { get; }
        public CodeGeneratorDelegate PagePass1CodeGenerator { get; }
        public CodeGeneratorDelegate PagePass2CodeGenerator { get; }
        public CodeGeneratorDelegate XamlMetaDataProviderPass1 { get; }
        public CodeGeneratorDelegate XamlMetaDataProviderPass2 { get; }
        public CodeGeneratorDelegate TypeInfoPass1CodeGenerator { get; }
        public CodeGeneratorDelegate TypeInfoPass1ImplCodeGenerator { get; }
        public CodeGeneratorDelegate TypeInfoPass2CodeGenerator { get; }
        public CodeGeneratorDelegate BindingInfoPass1CodeGenerator { get; }
        public CodeGeneratorDelegate BindingInfoPass2CodeGenerator { get; }

        public static Language Parse(string name)
        {
            var language = languages.Where(l => l.Name == name).FirstOrDefault();
            if (language == null)
            {
                throw new ArgumentOutOfRangeException(ResourceUtilities.FormatString(
                    XamlCompilerResources.XamlCompiler_LanguageUnsupported, name));
            }
            return language;
        }

        private Language(
            string name,
            string pass1Extension,
            string pass2Extension,
            bool isManaged,
            bool isStringNullable,
            bool isExperimental,
            CodeGeneratorDelegate appPass1CodeGenerator,
            CodeGeneratorDelegate appPass2CodeGenerator,
            CodeGeneratorDelegate pagePass1CodeGenerator,
            CodeGeneratorDelegate pagePass2CodeGenerator,
            CodeGeneratorDelegate xamlMetaDataProviderPass1,
            CodeGeneratorDelegate xamlMetaDataProviderPass2,
            CodeGeneratorDelegate typeInfoPass1CodeGenerator,
            CodeGeneratorDelegate typeInfoPass1ImplCodeGenerator,
            CodeGeneratorDelegate typeInfoPass2CodeGenerator,
            CodeGeneratorDelegate bindingInfoPass1CodeGenerator,
            CodeGeneratorDelegate bindingInfoPass2CodeGenerator)
        {
            this.Name = name;
            this.IsExperimental = isExperimental;
            this.IsManaged = isManaged;
            this.IsNative = !isManaged;
            this.IsStringNullable = isStringNullable;
            this.Pass1Extension = pass1Extension;
            this.Pass2Extension = pass2Extension;

            this.AppPass1CodeGenerator = appPass1CodeGenerator;
            this.AppPass2CodeGenerator = appPass2CodeGenerator;
            this.PagePass1CodeGenerator = pagePass1CodeGenerator;
            this.PagePass2CodeGenerator = pagePass2CodeGenerator;
            this.XamlMetaDataProviderPass1 = xamlMetaDataProviderPass1;
            this.XamlMetaDataProviderPass2 = xamlMetaDataProviderPass2;
            this.TypeInfoPass1CodeGenerator = typeInfoPass1CodeGenerator;
            this.TypeInfoPass1ImplCodeGenerator = typeInfoPass1ImplCodeGenerator;
            this.TypeInfoPass2CodeGenerator = typeInfoPass2CodeGenerator;
            this.BindingInfoPass1CodeGenerator = bindingInfoPass1CodeGenerator;
            this.BindingInfoPass2CodeGenerator = bindingInfoPass2CodeGenerator;
        }

        private static Language[] languages = {
            new Language(
                ProgrammingLanguage.CSharp,
                ".g.i.cs", ".g.cs", true, true, false,
                () => new CSharpAppPass1(),
                () => new CSharpPagePass2(),
                () => new CSharpPagePass1(),
                () => new CSharpPagePass2(),
                null,
                null,
                null,
                null,
                () => new CSharpTypeInfoPass2(),
                null,
                null),
            new Language(
                ProgrammingLanguage.VB,
                ".g.i.vb", ".g.vb", true, true, false,
                () => new VisualBasicAppPass1(),
                () => new VisualBasicPagePass2(),
                () => new VisualBasicPagePass1(),
                () => new VisualBasicPagePass2(),
                null,
                null,
                null,
                null,
                () => new VisualBasicTypeInfoPass2(),
                null,
                null),
            new Language(
                ProgrammingLanguage.CppCX,
                ".g.h", ".g.hpp", false, true, false,
                () => new MoComCppAppPass1(),
                () => new MoComCppAppPass2(),
                () => new MoComCppPagePass1(),
                () => new MoComCppPagePass2(),
                null,
                () => new CppWinRT_XamlMetaDataProviderPass2(),
                () => new MoComCppTypeInfoPass1(),
                () => new MoComCppTypeInfoPass1Impl(),
                () => new MoComCppTypeInfoPass2(),
                () => new MoComCppBindingInfoPass1(),
                () => new MoComCppBindingInfoPass2()),
            new Language(
                ProgrammingLanguage.CppWinRT,
                ".xaml.g.h", ".xaml.g.hpp", false, false, false,
                () => new CppWinRT_AppPass1(),
                () => new CppWinRT_AppPass2(),
                () => new CppWinRT_PagePass1(),
                () => new CppWinRT_PagePass2(),
                () => new CppWinRT_XamlMetaDataProviderPass1(),
                () => new CppWinRT_XamlMetaDataProviderPass2(),
                () => new CppWinRT_TypeInfoPass1(),
                () => new CppWinRT_TypeInfoPass1Impl(),
                () => new CppWinRT_TypeInfoPass2(),
                () => new CppWinRT_BindingInfoPass1(),
                () => new CppWinRT_BindingInfoPass2()),
            };
    }
}
