// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [Flags]
    enum SchemaMode
    {
        // Load the Native OR Managed runtime.  Also controls managed projection mode.
        ManagedRuntime = 0,
        NativeRuntime = 0x01,

        // Load the usre types from the Managed DLL or WinMD (cannot do both)
        LoadUserDll = 0x02,
        LoadUserWinmd = 0x04,

        // Set the user type dll/winmd to be the "local assembly".
        SetAsLocal = 0x08
    }

    class TestHelper
    {
        public TestHelper()
        {
            // Use .NET Framework 4.7.2 Facades which has System.Runtime.dll and other required facades.
            // Falls back to .NETCore\v4.5 if Facades path doesn't exist.
            string facadesPath = null;
            try
            {
                facadesPath = ProxyHelper.FindProgramFilesDir(@"Reference Assemblies\Microsoft\Framework\.NETFramework\v4.7.2\Facades");
            }
            catch
            {
                try
                {
                    facadesPath = ProxyHelper.FindProgramFilesDir(@"Reference Assemblies\Microsoft\Framework\.NETCore\v4.5");
                }
                catch { }
            }

            if (String.IsNullOrEmpty(facadesPath) || !Directory.Exists(facadesPath))
            {
                throw new DirectoryNotFoundException(
                    "Could not locate reference assemblies. Expected either " +
                    @"'Reference Assemblies\Microsoft\Framework\.NETFramework\v4.7.2\Facades' or " +
                    @"'Reference Assemblies\Microsoft\Framework\.NETCore\v4.5' under Program Files.");
            }

            FrameworkSDKPath = facadesPath;
            LibManagedDllName = @"LibManagedDll.dll";
            LibManagedWinmdName = @"LibManagedWinmd.winmd";
        }

        public String FrameworkSDKPath { get; set; }
        public String LibManagedDllName { get; set; }
        public String LibManagedWinmdName { get; set; }

        // This creates an instance of every proxied type
        // This checks that all the proxy types are correct WRT proeprty names.
        public void TestProxies()
        {
            ConnectionIdElement codeBehindElement = new ConnectionIdElement();

            CompilerDomRootToken compilerDomRootToken = new CompilerDomRootToken(null);

            Assembly[] currentAssemblies = AppDomain.CurrentDomain.GetAssemblies();
            DirectUISchemaContext directUISchemaContext = new DirectUISchemaContext(currentAssemblies, null, null, null, true);

            CompileXaml compileXaml = new CompileXaml();
            EventAssignment eventAssignment = new EventAssignment(null);
            FieldDefinition fieldDefinition = new FieldDefinition(null);
            InternalTypeEntry internalTypeEntry = new InternalTypeEntry(null);
            InternalXamlUserMemberInfo internalXamlMember = new InternalXamlUserMemberInfo(null);
            LineNumberInfo lineNumberInfo = new LineNumberInfo(null);
            TypeInfoCollector typeInfoCollector = new TypeInfoCollector(directUISchemaContext);
            XamlClassCodeInfo xamlClassCodeInfo = new XamlClassCodeInfo(null);
            XamlCodeGenerator xamlCodeGenerator = new XamlCodeGenerator(null, false, null, null);
            XamlConnectionIdRewriter xamlConnectionIdRewriter = new XamlConnectionIdRewriter();
            XamlDomValidator xamlDomValidator = new XamlDomValidator();
            XamlHarvester xamlHarvester = new XamlHarvester(Path.GetTempPath(), true);
            XamlSchemaCodeInfo xamlSchemaCodeInfo = new XamlSchemaCodeInfo(null);
            XamlFileCodeInfo xamlFileCodeInfo = new XamlFileCodeInfo(null);
            XamlCompileError xamlCompileError = new XamlCompileError(null);
            XamlProjectInfo xamlProjectInfo = new XamlProjectInfo();
            XamlNodeStreamHelper xamlNodStreamHelper = new XamlNodeStreamHelper();
            ClassName xamlAppInfo = new ClassName("App1.MainPage");
            DirectUIAssembly duiAssembly = new DirectUIAssembly(null);
            DirectUISystem duiSystem = new DirectUISystem(null);
            XbfMetadataProvider xbfMetadata = new XbfMetadataProvider(null);
            XbfGenerator xbfGenerator = new XbfGenerator(null, null);
        }

        public CompilerDomRootToken LoadXamlDom(string xaml, SchemaMode systemMode)
        {
            var schema = LoadSchema(systemMode);
            return LoadXamlDom(xaml, schema);
        }

        public CompilerDomRootToken LoadXamlDom(string xaml, DirectUISchemaContext schema)
        {
            var settings = new XamlXmlReaderSettings();
            settings.ProvideLineInfo = true;
            var textReader = new StringReader(xaml);
            var xamlReader = new XamlXmlReader(textReader, schema, settings);
            var compilerHelper = new XamlCompilerReflectionHelper();
            var domRootToken = compilerHelper.CreateCompilerDomRoot(xamlReader.Instance);
            domRootToken.Schema = schema;
            return domRootToken;
        }

        public XamlDomValidator ValidateXAML(string xaml,
                                            SchemaMode schemaMode = SchemaMode.ManagedRuntime,
                                            bool isPass1 = false,
                                            string targetPlatformMinVersionOverride=null)
        {
            return ValidateXAML(xaml, null, schemaMode, isPass1, targetPlatformMinVersionOverride);
        }

        public XamlDomValidator ValidateXAML(string xaml, DirectUISchemaContext schema, bool isPass1=false, string targetPlatformMinVersionOverride=null)
        {
            return ValidateXAML(xaml, schema, /*ignored*/SchemaMode.ManagedRuntime, isPass1, targetPlatformMinVersionOverride);
        }

        private XamlDomValidator ValidateXAML(string xaml, DirectUISchemaContext schema, SchemaMode schemaMode, bool isPass1, string targetPlatformMinVersionOverride)
        {
            CompilerDomRootToken domRootToken = null;
            try
            {
                if (schema != null)
                {
                    domRootToken = LoadXamlDom(xaml, schema);
                }
                else
                {
                    domRootToken = LoadXamlDom(xaml, schemaMode);
                }
            }
            catch (Exception ex)
            {
                if (ex.InnerException != null)
                {
                    Assert.Fail(ex.InnerException.Message);
                }
                Assert.Fail(ex.Message);
                return null;
            }
            return ValidateXamlDom(domRootToken, isPass1, targetPlatformMinVersionOverride);
        }

        public XamlDomValidator ValidateXamlDom(CompilerDomRootToken domRootToken, bool isPass1, string targetPlatformMinVersionOverride=null)
        {
            var validator = new XamlDomValidator();
            validator.IsPass1 = isPass1;
            if (targetPlatformMinVersionOverride != null)
            {
                validator.TargetPlatformMinVersion = new Version(targetPlatformMinVersionOverride);
            }
            validator.Validate(domRootToken);
            return validator;
        }

        public TypeInfoCollector CollectTypes(string xaml, DirectUISchemaContext schema)
        {
            CompilerDomRootToken domRoot = this.LoadXamlDom(xaml, schema);
            TypeInfoCollector collector = new TypeInfoCollector(schema);
            collector.Collect(domRoot);
            return collector;
        }

        public XbfGenerator GenerateXbf(Version tpmv, string xaml)
        {
            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(tpmv);
            context.IsPass1 = true;

            string file = Path.GetTempFileName();
            string xamlfile = file + ".xaml";
            string editedfile = file + "_edited.xaml";
            string xbffile = file + ".xbf";

            File.WriteAllText(xamlfile, xaml);

            CompilerDomRootToken domTree = this.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            XamlClassCodeInfo classInfo = this.HarvestClassCodeInfo(".", domTree, false, false);
            XamlFileCodeInfo codeInfo = this.Harvest(".", domTree, false, false);
            XamlConnectionIdRewriter xamlEditor = new XamlConnectionIdRewriter();
            string newXaml = xamlEditor.Parse(xaml, classInfo, codeInfo);

            File.WriteAllText(editedfile, newXaml);

            var schema = this.LoadSchema(SchemaMode.ManagedRuntime);
            XbfMetadataProvider metadataProvider = new XbfMetadataProvider(schema);
            
            var compilerHelper = new XamlCompilerReflectionHelper();
            string[] filenameArray = new string[] {xamlfile, editedfile, xbffile};
            var filenameArrayToken = compilerHelper.CreateXbfFilenameInfoArray(filenameArray);
            XbfGenerator xbfGen = new XbfGenerator(context.ProjectInfo, metadataProvider);
            xbfGen.SetXamlInputFiles(filenameArrayToken);
            xbfGen.GenerateXbfFiles();
            return xbfGen;
        }

        public void AssertContainsString(string largeString, string subString, string prefix = "")
        {
            string sub = Regex.Replace(subString, @"\s+", " ");
            string large = Regex.Replace(largeString, @"\s+", " ");

            Assert.AreEqual(true, large.Contains(sub), prefix + subString);
        }

        public void AssertContainsStringLists(string largeString, string prefix, params string[][] lists)
        {
            foreach (string[] strList in lists)
            {
                foreach (string subString in strList)
                {
                    AssertContainsString(largeString, subString, prefix);
                }
            }
        }

        public String MatchErrors(XamlDomValidator validator, IEnumerable<String> expectedErrorCodes, IEnumerable<String> expectedWarningCodes)
        {
            if (validator == null)
            {
                return "Validation Failed";
            }
            return MatchErrors(validator.Errors, validator.Warnings, expectedErrorCodes, expectedWarningCodes);
        }

        public string MatchErrors(List<XamlCompileError> errors, List<XamlCompileError> warnings,
                                  IEnumerable<String> expectedErrorCodes, IEnumerable<String> expectedWarningCodes)
        {
            string result = MatchErrorHelper("Error", errors, expectedErrorCodes);
            result += MatchErrorHelper("Warnings", warnings, expectedWarningCodes);
            return String.IsNullOrWhiteSpace(result) ? null : result;
        }

        public void AssertListsAreEqual(string elementName, List<string> actual, IEnumerable<string> shouldBe)
        {
            List<string> added, missing;
            if (MatchStringLists(shouldBe, actual, out added, out missing))
                return;

            if (missing != null && missing.Count > 0)
            {
                string names = " ";
                foreach (string s in missing)
                {
                    names += s + ", ";
                }
                Assert.Fail("Missing {0}(s): {1}", elementName, names);
            }

            if (added != null && added.Count > 0)
            {
                string names = String.Empty;
                foreach (string s in added)
                {
                    names += " " + s;
                }
                Assert.Fail("Unexpected {0}(s) {1}", elementName, names);
            }
        }

        private String MatchErrorHelper(string listName, List<XamlCompileError> actualCodes, IEnumerable<String> expectedStrings)
        {
            string result = String.Empty;
            int count = (actualCodes == null) ? 0 : actualCodes.Count;
            String[] actualStrings = new String[count];
            for (int i = 0; i < count; i++)
            {
                actualStrings[i] = actualCodes[i].ErrorCode;
            }
            List<String> addedStrings, missingStrings;
            if (MatchStringLists(expectedStrings, actualStrings, out addedStrings, out missingStrings))
            {
                return null;
            }
            if (missingStrings != null)
            {
                foreach (string m in missingStrings)
                {
                    result += String.Format("Expected {0} {1}", listName, m);
                }
            }
            if (addedStrings != null)
            {
                foreach (string a in addedStrings)
                {
                    foreach (XamlCompileError additionalError in actualCodes)
                    {
                        if (additionalError.ErrorCode == a)
                        {
                            result += String.Format("Unexpected {0} {1}: {2}", listName, additionalError.ErrorCode, additionalError.Message);
                        }
                    }
                }
            }
            return result;
        }

        // non-order dependent string list matching helper function
        private bool MatchStringLists(IEnumerable<String> before, IEnumerable<String> after, out List<String> added, out List<String> missing)
        {
            added = null;
            missing = null;
            if (after != null)
            {
                added = new List<string>(after);
                if (before != null)
                {
                    foreach (string b in before)
                    {
                        added.Remove(b);
                    }
                }
            }

            if (before != null)
            {
                missing = new List<string>(before);
                if (after != null)
                {
                    foreach (string a in after)
                    {
                        missing.Remove(a);
                    }
                }
            }
            return ((added == null || added.Count == 0) && (missing == null || missing.Count == 0));
        }

        // System Assembly AKA MsCorLib
        public Assembly GetSystemAssembly(DirectUISchemaContext schema)
        {
            DirectUISystem diSystem = schema.DirectUISystem;
            XamlTypeUniverse xamlTypeUniverse = diSystem.XamlTypeUniverse;
            Assembly asm = xamlTypeUniverse.GetSystemAssembly();
            return asm;
        }

        public Assembly GetWindowsWinmdAssembly(DirectUISchemaContext schema)
        {
            DirectUISystem diSystem = schema.DirectUISystem;
            DirectUIAssembly asm = diSystem.WindowsWinmd;
            return asm.WrappedAssembly;
        }

        public Assembly GetUserTypesAssembly(DirectUISchemaContext schema)
        {
            return (schema.UserAssemblies.Count > 0) ? schema.UserAssemblies[0] : null;
        }

        public DirectUISchemaContext LoadSchema(SchemaMode schemaMode)
        {
            // Find the Run Time assemblies.
            bool loadNativeRuntime = (schemaMode & SchemaMode.NativeRuntime) == SchemaMode.NativeRuntime;
            List<String> runtimeAssembyPaths = GetRuntimeAssemblyPaths(loadNativeRuntime);

            // Load the Run Time assemblies.
            bool useManagedProjections = !loadNativeRuntime;
            XamlTypeUniverse typeUniverse = new XamlTypeUniverse(useManagedProjections);
            List<Assembly> assemblies = new List<Assembly>();
            foreach (string path in runtimeAssembyPaths)
            {
                Assembly asm = typeUniverse.LoadAssemblyFromFile(path);
                assemblies.Add(asm);
            }

            // Load the User types
            string userTypeAssemblyPath = null;
            bool loadUserDll = (schemaMode & SchemaMode.LoadUserDll) == SchemaMode.LoadUserDll;
            bool loadUserWinmd = (schemaMode & SchemaMode.LoadUserWinmd) == SchemaMode.LoadUserWinmd;
            if (loadUserDll && loadUserWinmd)
            {
                throw new ArgumentOutOfRangeException("Both LoadUserDll & LoadUserWinmd cannot be set");
            }

            Assembly localAsm = null;
            Assembly userTypeAssembly = null;

            if (loadUserDll || loadUserWinmd)
            {
                userTypeAssemblyPath = GetUserAssemblyPath(loadUserWinmd);

                // Load the User types assembly.
                Assembly asm = typeUniverse.LoadAssemblyFromFile(userTypeAssemblyPath);
                userTypeAssembly = asm;
                assemblies.Add(asm);

                // Set to the Schema's "local" assembly.
                if ((schemaMode & SchemaMode.SetAsLocal) == SchemaMode.SetAsLocal)
                {
                    localAsm = asm;
                }
            }

            // Make sure there is an "mscorlib" around.
            if (!typeUniverse.IsSystemAssemblyLoaded)
            {
                Assembly mscorlib = typeUniverse.GetSystemAssembly();
                assemblies.Add(mscorlib);
            }

            var schema = new DirectUISchemaContext(assemblies, null, null, null, true);

            TypeResolver typeResolver = new TypeResolver(typeUniverse);
            typeResolver.InitializeTypeNameMap();
            schema.TypeResolver = typeResolver;

            if (localAsm != null)
            {
                schema.LocalAssembly = localAsm;
            }
            if (userTypeAssembly != null)
            {
                schema.UserAssemblies.Add(userTypeAssembly);
            }

            return schema;
        }

        public void XamlRewrite(string xamlFileName, XamlClassCodeInfo classInfo, XamlFileCodeInfo fileInfo)
        {
            XamlConnectionIdRewriter rewriter = new XamlConnectionIdRewriter();
            rewriter.Edit(xamlFileName, classInfo, fileInfo);
        }

        public XamlFileCodeInfo Harvest(string projectPath, CompilerDomRootToken domRootToken, bool isPass1, bool isApplication)
        {
            XamlClassCodeInfo classCodeInfo = this.HarvestClassCodeInfo(projectPath, domRootToken, isPass1, isApplication);

            return this.HarvestFileCodeInfo(projectPath, isPass1, classCodeInfo, domRootToken);
        }

        public XamlClassCodeInfo HarvestClassCodeInfo(string projectPath, CompilerDomRootToken domRootToken,
                                         bool isPass1, bool isApplication)
        {
            var harvester = new XamlHarvester(projectPath, isPass1);
            var classFullName = harvester.GetClassFullName(domRootToken);
            return harvester.HarvestClassInfo(classFullName, domRootToken, isPass1, isApplication);
        }

        public XamlFileCodeInfo HarvestFileCodeInfo(string projectPath, bool isPass1, XamlClassCodeInfo classCodeInfo, CompilerDomRootToken domRootToken)
        {
            var harvester = new XamlHarvester(projectPath, isPass1);
            return harvester.HarvestXamlFileInfo(classCodeInfo, domRootToken);
        }

        public XamlConnectionIdRewriter RewriteXaml(string projectPath, List<string> xamlStrings, DirectUISchemaContext schema, bool isPass1, bool isApplication, List<string> newXamlStrings, out XamlClassCodeInfo classCodeInfo)
        {
            classCodeInfo = null;
            List<XamlFileCodeInfo> xamlFileCodeInfos = new List<XamlFileCodeInfo>();

            foreach (string xamlString in xamlStrings)
            {
                CompilerDomRootToken domRoot = this.LoadXamlDom(xamlString, schema);
                XamlDomValidator validator = this.ValidateXamlDom(domRoot, isPass1);
                Assert.AreEqual(validator.Errors.Count, 0);
                XamlClassCodeInfo codeInfo = this.HarvestClassCodeInfo(projectPath, domRoot, isPass1, isApplication);

                if (classCodeInfo == null)
                {
                    classCodeInfo = codeInfo;
                }
                else
                {
                    Assert.AreEqual(classCodeInfo.ClassName.FullName, codeInfo.ClassName.FullName);
                }

                xamlFileCodeInfos.Add(this.HarvestFileCodeInfo(projectPath, isPass1, classCodeInfo, domRoot));
            }

            XamlConnectionIdRewriter xamlEditor = new XamlConnectionIdRewriter();

            for (int i = 0; i < xamlStrings.Count; i++)
            {
                xamlFileCodeInfos[i].ApparentRelativePath = String.Format("Dummy.{0}.xaml", i);
                classCodeInfo.AddXamlFileInfo(xamlFileCodeInfos[i]);
                newXamlStrings.Add(xamlEditor.Parse(xamlStrings[i], classCodeInfo, xamlFileCodeInfos[i]));
            }

            return xamlEditor;
        }

        public List<FileNameAndContentPair> GenerateCodeBehind(CodeGeneratorProjectContext cpx, List<string> xamlStrings, DirectUISchemaContext schema, CodeGenLanguage lang)
        {
            XamlProjectInfo projectInfo = cpx.ProjectInfo;
            XamlClassCodeInfo sharedCodeInfo = null;

            string dummyFileName = "DummyFile.xaml";
            string dummyFilePath = Path.Combine(cpx.ProjectPath, dummyFileName);
            File.WriteAllText(dummyFilePath, "");

            foreach (string xamlString in xamlStrings)
            {
                CompilerDomRootToken domRoot = this.LoadXamlDom(xamlString, schema);
                XamlDomValidator validator = this.ValidateXamlDom(domRoot, cpx.IsPass1);
                Assert.AreEqual(validator.Errors.Count, 0);
                XamlClassCodeInfo codeInfo = this.HarvestClassCodeInfo(cpx.ProjectPath, domRoot, cpx.IsPass1, cpx.IsApplication);

                if (sharedCodeInfo == null)
                {
                    sharedCodeInfo = codeInfo;
                }
                else
                {
                    Assert.AreEqual(sharedCodeInfo.ClassName.FullName, codeInfo.ClassName.FullName);
                }

                XamlFileCodeInfo fileInfo = this.HarvestFileCodeInfo(cpx.ProjectPath, cpx.IsPass1, sharedCodeInfo, domRoot);
                fileInfo.ApparentRelativePath = dummyFileName;
                fileInfo.FullPathToXamlFile = dummyFilePath;
                fileInfo.RelativePathFromGeneratedCodeToXamlFile = dummyFileName;
                fileInfo.SourceXamlGivenPath = dummyFilePath;
                sharedCodeInfo.AddXamlFileInfo(fileInfo);
            }

            if (!cpx.IsPass1)
            {
                // Mirror CompileXamlInternal, which parses the bind universes once every file has
                // been harvested and before any code is generated. Without this the x:Binds have
                // no path steps and no binding code is emitted at all.
                foreach (BindUniverse bindUniverse in sharedCodeInfo.BindUniverses)
                {
                    IEnumerable<XamlCompileError> errors = bindUniverse.Parse(sharedCodeInfo);
                    Assert.AreEqual(0, errors.Count(),
                        String.Join(", ", errors.Select(error => error.ErrorCode + ": " + error.Message)));
                }
            }

            if (!cpx.IsPass1 && lang == CodeGenLanguage.Cpp)
            {
                cpx.ProjectInfo.ClassToHeaderFileMap = new Dictionary<string, string>();
                cpx.ProjectInfo.ClassToHeaderFileMap.Add(sharedCodeInfo.ClassName.FullName, sharedCodeInfo.ClassName.ShortName + ".h");

                // It's hard to get the real type into the ClassType field - just use Page
                XamlTypeName xamlTypeName = new XamlTypeName("http://schemas.microsoft.com/winfx/2006/xaml/presentation", "Page");
                sharedCodeInfo.ClassType = new TypeForCodeGen(schema.GetXamlType(xamlTypeName));
            }

            var codeLang = GetCodeLanguage(cpx.IsPass1, lang);
            XamlCodeGenerator codeGenerator = new XamlCodeGenerator(codeLang, cpx.IsPass1, projectInfo, null);
            return codeGenerator.GenerateCodeBehind(sharedCodeInfo);
        }

        public List<FileNameAndContentPair> GenerateTypeInfo(bool isPass1, XamlSchemaCodeInfo schemaInfo, XamlProjectInfo projectInfo,
                                                    ClassName appXamlInfo, CodeGenLanguage lang)
        {
            var codeLang = GetCodeLanguage(isPass1, lang);
            XamlCodeGenerator codeGenerator = new XamlCodeGenerator(codeLang, isPass1, projectInfo, schemaInfo);
            return codeGenerator.GenerateTypeInfo(appXamlInfo);
        }

        // ====  private stuff =====

        private Language GetCodeLanguage(bool isPass1, CodeGenLanguage lang)
        {
            switch (lang)
            {
                case CodeGenLanguage.CSharp:
                    return Language.Parse("C#");

                case CodeGenLanguage.VisualBasic:
                    return Language.Parse("VB");

                case CodeGenLanguage.Cpp:
                    return Language.Parse("C++");

                default:
                    throw new ArgumentOutOfRangeException("Bad Code Language");
            }
        }

        private List<String> GetRuntimeAssemblyPaths(bool loadNativeRuntime)
        {
            string referencesPath = $"Windows Kits\\10\\References\\{KnownVersions.Latest}\\";

            List<String> paths = new List<string>();
            paths.Add(ProxyHelper.FindProgramFilesFile(
                referencesPath + @"Windows.Foundation.FoundationContract\{0}\Windows.Foundation.FoundationContract.winmd",
                KnownVersions.FoundationContractVersion));
            paths.Add(ProxyHelper.FindProgramFilesFile(
                referencesPath + @"Windows.Foundation.UniversalApiContract\{0}\Windows.Foundation.UniversalApiContract.winmd",
                KnownVersions.UniversalApiContractVersion));
            // Load Microsoft.UI.winmd before Microsoft.UI.Xaml.winmd (dependency order)
            string winuiDir = Path.GetDirectoryName(ProxyHelper.WinUIWinmdPath);
            if (!String.IsNullOrEmpty(winuiDir))
            {
                string muiWinmdPath = Path.Combine(winuiDir, "Microsoft.UI.winmd");
                if (File.Exists(muiWinmdPath))
                {
                    paths.Add(muiWinmdPath);
                }
            }
            paths.Add(ProxyHelper.WinUIWinmdPath);

            if (loadNativeRuntime)
            {
                paths.AddRange(NativeWinmdFilePaths);
            }
            else
            {
                // Load Managed runtime.
                paths.AddRange(FrameworkAssemblyFilePaths);
            }
            return paths;
        }

        private String GetUserAssemblyPath(bool useWinmd)
        {
            string userAssemblyName = (useWinmd) ? LibManagedWinmdName : LibManagedDllName;
            string executingAssembly = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            string fullpath = Path.Combine(executingAssembly, userAssemblyName);
            if (String.IsNullOrWhiteSpace(fullpath) || !File.Exists(fullpath))
            {
                throw new ArgumentException("Local Assembly path is not found", "GetUserAssemblyPath()");
            }
            return fullpath;
        }

        List<String> _frameworkAssemblyFilePaths;
        private List<String> FrameworkAssemblyFilePaths
        {
            get
            {
                if (_frameworkAssemblyFilePaths == null)
                {
                    _frameworkAssemblyFilePaths = new List<string>();
                    _frameworkAssemblyFilePaths.AddRange(Directory.EnumerateFiles(FrameworkSDKPath, "*.dll"));

                    // The .NETFramework v4.7.2\Facades folder does NOT ship
                    // System.Runtime.WindowsRuntime.dll or System.Runtime.WindowsRuntime.UI.Xaml.dll.
                    // Those assemblies are required by the LMR type universe to resolve WinRT
                    // projection types referenced transitively from Microsoft.UI.Xaml.winmd /
                    // Microsoft.UI.winmd. The real implementation DLLs live under
                    // %WinDir%\Microsoft.NET\Framework64 (or Framework for x86). Add them to the
                    // universe so the schema loader can resolve System.Runtime.WindowsRuntime{,.UI.Xaml}.
                    // No files are copied into the repo - we only point the universe at the
                    // existing system path.
                    string windir = Environment.GetEnvironmentVariable("WINDIR");
                    if (string.IsNullOrEmpty(windir)) { windir = @"C:\Windows"; }
                    string[] candidateRuntimeDirs = new[]
                    {
                        Path.Combine(windir, @"Microsoft.NET\Framework64\v4.0.30319"),
                        Path.Combine(windir, @"Microsoft.NET\Framework\v4.0.30319"),
                    };
                    string[] runtimeAssemblyNames = new[]
                    {
                        "System.Runtime.WindowsRuntime.dll",
                        "System.Runtime.WindowsRuntime.UI.Xaml.dll",
                    };
                    foreach (string asmName in runtimeAssemblyNames)
                    {
                        foreach (string dir in candidateRuntimeDirs)
                        {
                            string full = Path.Combine(dir, asmName);
                            if (File.Exists(full))
                            {
                                _frameworkAssemblyFilePaths.Add(full);
                                break;
                            }
                        }
                    }
                }
                return _frameworkAssemblyFilePaths;
            }
        }

        List<String> _nativeWinmdFilePaths;
        private List<String> NativeWinmdFilePaths
        {
            get
            {
                if (_nativeWinmdFilePaths == null)
                {
                    _nativeWinmdFilePaths = new List<string>();
                    // Nothing for now.  (windows.winmd is added later)
                    // Perhaps add the C++ Platform assembly.
                }
                return _nativeWinmdFilePaths;
            }
        }

    }
}
