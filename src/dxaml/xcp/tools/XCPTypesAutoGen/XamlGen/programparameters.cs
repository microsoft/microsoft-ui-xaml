// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;

namespace XamlGen
{
    public class InvalidParametersException : Exception
    {
        private static string GetMessage(string programName)
        {
            var sb = new StringBuilder();
            sb.AppendFormat("{0} syntax:", programName);
            sb.AppendLine();
            sb.AppendFormat("    {0}.exe [-XbfGeneration <partial/full>] [-i <input_directory>] [-o <output_directory>] [-e <extension_name>] [-a <AssemblyPath>]*", programName);
            sb.AppendLine();
            return sb.ToString();
        }

        public InvalidParametersException(string programName)
            : base(GetMessage(programName))
        {
        }

    }

    public sealed class ProgramParameters
    {
        public string InputDirectory  { get; private set; }
        public string OutputDirectory { get; private set; }
        public string ExtensionName { get; private set; }
        public bool   PartialXbfGen   { get; private set; }
        private readonly List<Assembly> assemblies;
        public IEnumerable<Assembly> Assemblies
        {
            get
            {
                return this.assemblies;
            }
        }

        public bool IsExtension 
        { 
            get
            {
                return ExtensionName != null;
            }
        }

        private ProgramParameters(String[] args)
        {
            this.assemblies = new List<Assembly>();

            this.InputDirectory = "." + Path.DirectorySeparatorChar;
            this.OutputDirectory = "." + Path.DirectorySeparatorChar;
            this.ExtensionName = null;

            if (args.Length % 2 == 1)
            {
                throw new InvalidParametersException(args[0]);
            }

            if (args.Length > 0)
            {
                var basePath = Path.GetDirectoryName(new Uri(typeof(ProgramParameters).Assembly.Location).LocalPath);
                for (Int16 iArg = 0; iArg < args.Length; iArg += 2)
                {
                    if (String.Compare(args[iArg], "-i", ignoreCase: true) == 0)
                    {
                        this.InputDirectory = args[iArg + 1];
                    }
                    else if (String.Compare(args[iArg], "-o", ignoreCase: true) == 0)
                    {
                        this.OutputDirectory = args[iArg + 1];
                    }
                    else if (String.Compare(args[iArg], "-e", ignoreCase: true) == 0)
                    {
                        this.ExtensionName = args[iArg + 1];
                    }
                    else if (String.Compare(args[iArg], "-a", ignoreCase: true) == 0)
                    {
                        var assemblyName = args[iArg + 1];
                        var path = Path.Combine(basePath, assemblyName);
                        var assembly = Assembly.LoadFrom(File.Exists(path) ? path : assemblyName);
                        this.assemblies.Add(assembly);
                    }
                    else if (String.Compare(args[iArg], "-XbfGeneration", ignoreCase: true) == 0)
                    {
                        this.PartialXbfGen = (String.Compare(args[iArg+1], "partial", ignoreCase: true) == 0);
                    }
                    else
                    {
                        throw new InvalidParametersException(args[0]);
                    }
                }

                if (!this.InputDirectory.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.InputDirectory += Path.DirectorySeparatorChar;
                }
                if (!this.OutputDirectory.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.OutputDirectory += Path.DirectorySeparatorChar;
                }
            }
        }

        public static ProgramParameters Create(String[] args)
        {
            return new ProgramParameters(args);
        }

        public OM.OMContext CreateOMContext()
        {
            foreach (var assembly in Assemblies)
            {
                Type modelFactoryType = assembly.GetType("XamlOM.NewBuilders.ModelFactory", false, true);
                if (modelFactoryType != null)
                {
                    MethodInfo createMethod = modelFactoryType.GetMethod("Create");
                    if (createMethod != null)
                    {
                        return createMethod.Invoke(null, new object[] { Assemblies }) as OM.OMContext;
                    }
                }
            }
            throw new ArgumentException("None of the assemblies passed in contains a ModelFactory.Create method");
        }
    }
}
