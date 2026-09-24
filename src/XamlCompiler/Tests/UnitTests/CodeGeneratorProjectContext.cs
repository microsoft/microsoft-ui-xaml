// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    enum CodeGenLanguage { CSharp, Cpp, VisualBasic };

    class CodeGeneratorProjectContext
    {
        private string _projectPath = null;
        private XamlProjectInfo _projectInfo;
        private Version _tpmv;

        public CodeGeneratorProjectContext(Version tpmv, string projectName = "ContextDefault_Name")
        {
            // these are usable defaults for an: EXE, Page of XAML.
            IsApplication = false;
            IsPass1 = false;
            IsLibrary = false;
            ProjectName = projectName;
            RootNamespace = "ContextDefault_Name";
            this._tpmv = tpmv;
        }

        public bool IsApplication { get; set; }
        public bool IsPass1 { get; set; }
        public string ProjectName { get; private set; }  // caller can't change projectName because we make a /temp folder.

        private bool _isLibrary;
        private string _rootNamespace;

        public bool IsLibrary
        {
            get { return _isLibrary; }
            set
            {
                if (_projectInfo != null)
                {
                    throw new InvalidOperationException("Can't set IsLibrary, call ClearProjectInfo() first");
                }
                _isLibrary = value;
            }
        }

        public string RootNamespace
        {
            get { return _rootNamespace; }
            set
            {
                if (_projectInfo != null)
                {
                    throw new InvalidOperationException("Can't set RootNamespace, call ClearProjectInfo() first");
                }
                _rootNamespace = value;
            }
        }

        // The caller must set these (they don't default)
        public void AddXamlFile(string filePathRelativeToProjectDir, string text)
        {
            XamlFilePathRelativeToProjectDir = filePathRelativeToProjectDir;
            XamlText = text;
            WriteXamlFileToDisk();
        }

        public string XamlFilePathRelativeToProjectDir { get; private set; }
        public string XamlText { get; private set; }

        public XamlProjectInfo ProjectInfo
        {
            get
            {
                if (_projectInfo == null)
                {
                    _projectInfo = new XamlProjectInfo();
                    _projectInfo.RootNamespace = RootNamespace;
                    _projectInfo.ProjectName = ProjectName;
                    _projectInfo.IsLibrary = IsLibrary;
                    _projectInfo.TargetPlatformMinVersion = _tpmv;
                    
                    var testBinDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    _projectInfo.GenXbf32Path = _projectInfo.GenXbf64Path = _projectInfo.GenXbfArm64Path = Path.Combine(testBinDirectory, "genxbf.dll");
                }
                return _projectInfo;
            }
        }

        public void ClearProjectInfo()
        {
            _projectInfo = null;
        }

        // This step is needed only for C++ code gen.
        public void SetClassNameToHeaderFileMap(DirectUISchemaContext schema)
        {
            ProjectInfo.ClassToHeaderFileMap = GetClassNameToHeaderFileMap(schema);
        }

        // This can be anything really. For tests it doesn't matter.
        // But this is should match what CompileXaml does.
        public string LoadUri
        {
            get
            {
                ConfirmXamlPresent();
                string priPackage = (PriIndexName != null) ? (PriIndexName + "/") : String.Empty;
                string prefix = "ms-appx:///";
                return prefix + priPackage + XamlFilePathRelativeToProjectDir;
            }
        }

        // This creates the project directory in the filesystem
        // I consider this a defect that we need to do this here.
        // The XAML compiler in the  Harvester and the Code Gen (to
        // generate the Hash) touch the filesystem.
        //  They should operated only on passed data.
        // [Note: the code gen does need to know the path (it goes into the
        //   #Line pragmas, but it shouldn't actually open the file ]
        public string ProjectPath
        {
            get
            {
                if (_projectPath == null)
                {
                    _projectPath = Path.Combine(Path.GetTempPath(), ProjectName);
                    if (Directory.Exists(_projectPath))
                    {
                        Directory.Delete(_projectPath, true);
                    }
                    Directory.CreateDirectory(_projectPath);
                }
                return _projectPath;
            }
        }


        // ===== private stuff ======

        // Check that the caller set the above properties.
        private void ConfirmXamlPresent()
        {
            if (String.IsNullOrWhiteSpace(XamlFilePathRelativeToProjectDir) || String.IsNullOrWhiteSpace(XamlText))
                throw new ArgumentNullException("Missing XAML file for test");
        }

        private string PriIndexName
        {
            get { return IsLibrary ? RootNamespace : String.Empty; }
        }

        private void WriteXamlFileToDisk()
        {
            string fullXamlFilePath = Path.Combine(ProjectPath, XamlFilePathRelativeToProjectDir);
            string fullPath = Path.GetDirectoryName(fullXamlFilePath);
            if (!Directory.Exists(fullPath))
            {
                Directory.CreateDirectory(fullPath);
            }
            File.WriteAllText(fullXamlFilePath, XamlText);
        }

        public Dictionary<String, String> GetClassNameToHeaderFileMap(DirectUISchemaContext schema)
        {
            Stream str = new MemoryStream(Encoding.UTF8.GetBytes(XamlText));
            StreamReader rdr = new StreamReader(str);

            string className = XamlNodeStreamHelper.ReadXClassFromXamlFileStream(rdr, schema.Instance);

            string headerfileName = XamlFilePathRelativeToProjectDir + ".h";
            Dictionary<string, string> map = new Dictionary<string, string>();
            map.Add(className, headerfileName);
            return map;
        }

    }
}
