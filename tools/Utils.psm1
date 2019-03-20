# There's a bug in Powershell where it doesn't properly handle bad paths in the LIB environment variable
# when loading C# reference assemblies.  In order to work around that bug, we'll manually test the paths in LIB
# and remove ones that don't actually exist.
if ($env:LIB -ne $null -and $env:LIB.Length -gt 0)
{
    [System.Collections.Generic.List[string]]$libPaths = @()

    foreach ($libPath in $env:LIB.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries))
    {
        if (Test-Path $libPath)
        {
            $libPaths.Add($libPath)
        }
    }

    $env:LIB = $libPaths -join ";"
}

Add-Type -Language CSharp @"
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;

namespace WinmdHelper
{
    public class LoadWinmdTypes
    {
        public static List<Type> FromFile(string assemblyPath, string referenceWinmds)
        {
            List<string> references = new List<string>(referenceWinmds.Replace("\\\\", "\\").Split(';'));

            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += (sender, eventArgs) =>
            {
                Assembly assembly = null;
                foreach (var reference in references)
                {
                    string name = eventArgs.Name.Split(',')[0];
                    if (reference.Contains(name))
                    {
                        assembly = Assembly.ReflectionOnlyLoadFrom(reference);
                        break;
                    }
                }

                if (assembly == null)
                {
                    string fullName = eventArgs.Name;
                    assembly = Assembly.ReflectionOnlyLoad(fullName);
                }

                return assembly;
            };

            WindowsRuntimeMetadata.ReflectionOnlyNamespaceResolve += (sender, eventArgs) =>
            {
                foreach (var refer in references)
                {
                    eventArgs.ResolvedAssemblies.Add(Assembly.ReflectionOnlyLoadFrom(refer));
                }

                return;
            };

            string fullAssemblyPath = Path.GetFullPath(assemblyPath);
            Assembly loadFrom = null;
            foreach (var loadedAssembly in AppDomain.CurrentDomain.ReflectionOnlyGetAssemblies())
            {
                if (String.Equals(loadedAssembly.Location, fullAssemblyPath, StringComparison.OrdinalIgnoreCase))
                {
                    loadFrom = loadedAssembly;
                    break;
                }
            }

            if (loadFrom == null)
            {
                loadFrom = Assembly.ReflectionOnlyLoadFrom(fullAssemblyPath);
            }

            return loadFrom.GetExportedTypes().ToList();
        }

        public static List<Type> GetActivatableTypesFromFile(string assemblyPath, string referenceWinmds)
        {
            var exportedTypes = FromFile(assemblyPath, referenceWinmds);
            List<Type> activatableTypes = new List<Type>();
            foreach(var type in exportedTypes)
            {
                if(!type.IsEnum && 
                   !type.IsValueType &&
                   !type.IsAbstract)
                   {
                     activatableTypes.Add(type);
                   }
            }

            return activatableTypes;
            
        }
    }
}
"@

function Get-ActivatableTypes
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Paths to the WinMD files whose types need to be added to the manifest.")]
        [string]$WinmdPaths,

        [Parameter(Mandatory=$true, HelpMessage="Semicolon-delimited list of reference WinMD files.")]
        [string]$ReferenceWinmds
    )

    $winmdPathList = $WinmdPaths.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries)
    [System.Collections.Generic.List[System.Type]]$typeList = @()

    foreach ($winmdPath in $winmdPathList)
    {
        $typeList.AddRange([WinmdHelper.LoadWinmdTypes]::GetActivatableTypesFromFile($winmdPath, $ReferenceWinmds))
    }

    $typeList
}

function Get-WinmdTypes
{
    param(
        [Parameter(Mandatory=$true, HelpMessage="Paths to the WinMD files whose types need to be added to the manifest.")]
        [string]$WinmdPaths,

        [Parameter(Mandatory=$true, HelpMessage="Semicolon-delimited list of reference WinMD files.")]
        [string]$ReferenceWinmds
    )

    $winmdPathList = $WinmdPaths.Split(";", [System.StringSplitOptions]::RemoveEmptyEntries)
    [System.Collections.Generic.List[System.Type]]$typeList = @()

    foreach ($winmdPath in $winmdPathList)
    {
        $typeList.AddRange([WinmdHelper.LoadWinmdTypes]::FromFile($winmdPath, $ReferenceWinmds))
    }

    $typeList
}

function Get-WindowsNamespace
{
    param(
        [Parameter(Mandatory=$true)]
        [string]$Namespace
    )

    $Namespace.Replace("Microsoft.UI.Xaml", "Windows.UI.Xaml").Replace("Microsoft.UI.Private", "MUXControls")
}

function Write-ErrorInFile
{
    param(
        [Parameter(Mandatory=$true)]
        [string]$Message,

        [Parameter(Mandatory=$true)]
        [string]$FilePath,

        [Parameter(Mandatory=$false)]
        [int]$LineNumber = 0,

        [Parameter(Mandatory=$false)]
        [int]$ColumnNumber = 0
    )

    Write-Information "{ERROR:$message|$FilePath|$LineNumber|$ColumnNumber}"
}