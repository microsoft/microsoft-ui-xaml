using Microsoft.Build.Framework;
using Microsoft.VisualStudio.TextTemplating;
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

namespace CustomTasks
{
    public class TransformTemplate : IsolatedTask
    {
        [Required]
        public string Template
        {
            get;
            set;
        }

        [Required]
        public ITaskItem[] ParameterValues
        {
            get;
            set;
        }

        [Required]
        public string OutputFile
        {
            get;
            set;
        }

        public override bool ExecuteCore()
        {
            Dictionary<string, string> parameterValues = new Dictionary<string, string>();

            foreach (ITaskItem item in ParameterValues)
            {
                parameterValues.Add(item.GetMetadata("Identity"), item.GetMetadata("Value"));
            }

            LogMessage($"Transforming template {Template}...");

            Engine engine = new Engine();
            TransformationHost host = new TransformationHost(Path.GetDirectoryName(Template), parameterValues);
            string transformedText = engine.ProcessTemplate(File.ReadAllText(Template), host);

            foreach (CompilerError error in host.Errors)
            {
                if (error.IsWarning)
                {
                    LogWarning(string.Empty, string.Empty, string.Empty, error.FileName, error.Line, error.Column, error.ErrorText);
                }
                else
                {
                    LogError(string.Empty, string.Empty, string.Empty, error.FileName, error.Line, error.Column, error.ErrorText);
                }
            }

            bool success = host.Errors.Where(error => !error.IsWarning).Count() == 0;

            if (success)
            {
                File.WriteAllText(OutputFile, transformedText);
                LogMessage($"Output file {OutputFile} generated successfully.");
            }
            else
            {
                LogMessage($"Errors encountered. Failed to generate {OutputFile}.");
            }

            return success;
        }

        private sealed class TransformationHost : ITextTemplatingEngineHost
        {
            private readonly IList<CompilerError> _errors;
            private string _basePath;
            private IDictionary<string, string> _parameterValues;

            public TransformationHost(string basePath, IDictionary<string, string> parameterValues)
            {
                _errors = new List<CompilerError>();
                _basePath = basePath;
                _parameterValues = parameterValues;
            }

            public ICollection<CompilerError> Errors
            {
                get
                {
                    return _errors;
                }
            }

            public object GetHostOption(string optionName)
            {
                if (optionName == "isTransformAll")
                {
                    return true;
                }

                throw new NotImplementedException();
            }

            public bool LoadIncludeText(string requestFileName, out string content, out string location)
            {
                string resolvedLocation = ResolvePath(requestFileName);

                if (!string.IsNullOrEmpty(resolvedLocation))
                {
                    content = File.ReadAllText(resolvedLocation);
                    location = resolvedLocation;
                    return true;
                }
                else
                {
                    content = string.Empty;
                    location = string.Empty;
                    return false;
                }
            }

            public void LogErrors(CompilerErrorCollection errors)
            {
                foreach (CompilerError error in errors)
                {
                    _errors.Add(error);
                }
            }

            public AppDomain ProvideTemplatingAppDomain(string content)
            {
                return AppDomain.CurrentDomain;
            }

            public string ResolveAssemblyReference(string assemblyReference)
            {
                if (System.IO.File.Exists(assemblyReference))
                {
                    return assemblyReference;
                }

                var assembly = Assembly.Load(assemblyReference);
                if (assembly != null)
                {
                    return assembly.Location;
                }

                return string.Empty;
            }

            public Type ResolveDirectiveProcessor(string processorName)
            {
                throw new ArgumentException("Invalid directive processor.", "processorName");
            }

            public string ResolveParameterValue(string directiveId, string processorName, string parameterName)
            {
                if (directiveId == null)
                {
                    throw new ArgumentNullException("directiveId");
                }

                if (processorName == null)
                {
                    throw new ArgumentNullException("processorName");
                }

                if (parameterName == null)
                {
                    throw new ArgumentNullException("parameterName");
                }

                return _parameterValues[parameterName] as string;
            }

            public string ResolvePath(string path)
            {
                if (File.Exists(path))
                {
                    return path;
                }

                string pathFromBase = Path.Combine(_basePath, path);
                if (File.Exists(pathFromBase))
                {
                    return pathFromBase;
                }

                return string.Empty;
            }

            public void SetFileExtension(string extension)
            {
            }

            public void SetOutputEncoding(Encoding encoding, bool fromOutputDirective)
            {
            }

            public IList<string> StandardAssemblyReferences
            {
                get
                {
                    return new string[]
                    {
                        typeof(System.Uri).Assembly.Location
                    };
                }
            }

            public IList<string> StandardImports
            {
                get
                {
                    return new string[0];
                }
            }

            public string TemplateFile
            {
                get
                {
                    return string.Empty;
                }
            }
        }
    }
}
