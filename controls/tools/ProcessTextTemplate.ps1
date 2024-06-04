Param(
    [Parameter(Mandatory = $true)] 
    [string]$FilePath,

    [Parameter(Mandatory = $true)] 
    [string]$OutputPath,

    [string]$ParameterValuesString
)

if ("$env:SDXROOT".Length -eq 0)
{
    Write-Error "This script should only be run in a Razzle context."
}

$textTemplatingDllPath = "$env:SDXROOT\tools\managed\v2.0\Microsoft.VisualStudio.TextTemplating.dll"

[System.Reflection.Assembly]::LoadFrom($textTemplatingDllPath) 2>&1>> $null
Add-Type -Path $textTemplatingDllPath

Add-Type -Language CSharp -ReferencedAssemblies $textTemplatingDllPath @"
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;

using Microsoft.VisualStudio.TextTemplating;

namespace TextTemplateProcessing
{
    public sealed class TransformationHost : ITextTemplatingEngineHost
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
"@

[System.Collections.Generic.Dictionary[string, string]]$parameterValues = @{}

$ParameterValuesString.Split(',') | ForEach-Object {
    $components = $_.Split('=')
    $parameterValues.Add($components[0], $components[1])
}

$engine = New-Object -TypeName Microsoft.VisualStudio.TextTemplating.Engine
$transformationHost = New-Object -TypeName TextTemplateProcessing.TransformationHost -ArgumentList (Split-Path -Path $FilePath),$parameterValues
$transformedText = $engine.ProcessTemplate([System.IO.File]::ReadAllText($FilePath), $transformationHost)

# If we individually write out every error, then every single one of them will be annotated with the line number
# from this script.  In order to avoid that hindrance to readability, we'll collect all of the errors (if any)
# into a single string, and then write that out as an error.
$errors = ""

$transformationHost.Errors | ForEach-Object {
    $errors = "$errors$_$([Environment]::NewLine)"
}

if ($errors.Length -gt 0)
{
    Write-Error $errors
    exit 1
}
else
{
    [System.IO.File]::WriteAllText($OutputPath, $transformedText)
}