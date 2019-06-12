// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using CustomTasks;
using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;

namespace CustomTasksTool
{
    class Program
    {
        static void Main(string[] args)
        {
            // This tool is here just to let you F5 the DependencyPropertyCodeGen task and debug it more easily than through MSBuild.
            // Pass command line arguments of the WinMDInput, then references.
            var test = new DependencyPropertyCodeGen();
            test.WinMDInput = new string[] { args[0] };
            test.References = args.Skip(1).ToArray();
            test.OutputDirectory = "..\\..\\..\\..\\dev\\Generated";
            test.LogToConsole = true;
            test.Execute();
        }
    }
}
