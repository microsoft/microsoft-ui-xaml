// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.Coverage.Analysis;
//using Microsoft.VisualStudio.Coverage;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections.ObjectModel;
using System.Data;
using System.IO;

namespace CodeCoverageReport
{
    class Program
    {
        static void Main(string[] args)
        {
            string coverageFile = @"UnitTest.coverage";
            List<String> compilerPath = new List<string>();
            compilerPath.Add(@"C:\Program Files (x86)\MSBuild\Microsoft\WindowsXaml\v12.0\8.1");

            List<String> excludedNamespaces = new List<string>();
            excludedNamespaces.Add("System.Reflection.Adds");
            excludedNamespaces.Add("Microsoft.MetadataReader");
            excludedNamespaces.Add("Microsoft.MetadataReader.Internal");
            excludedNamespaces.Add("Microsoft.Xaml.Tools.XamlDom");
            excludedNamespaces.Add("Microsoft.Windows.UI.Xaml.Build.Tasks.Utilities");
            excludedNamespaces.Add("Microsoft.Windows.UI.Xaml.Build.Tasks");
            excludedNamespaces.Add("Microsoft.Xaml.Tools.DirectUI.ProxyTypes");

            if (!File.Exists(coverageFile))
            {
                Console.Error.WriteLine("File not found: {0}", coverageFile);
                Environment.Exit(1);
            }

            bool showClassBreakdown = false;

            CoverageInfo ci = null;
            try
            {
                ci = CoverageInfo.CreateFromFile(coverageFile, compilerPath, compilerPath);
            }
            catch (ImageNotFoundException ex)
            {
                Console.Error.WriteLine("Error:");
                Console.Error.WriteLine("The installed XAML compiler is not instrumented for Code Coverage. Path={0}", compilerPath[0]);
                Console.Error.WriteLine();
                Console.Error.WriteLine(ex.Message);
                Environment.Exit(1);
            }
            // If you get an exception here, copy the VsBinary/ files in to the bin directory, beside the Exe.
            var dataset = ci.BuildDataSet();

            CoverageDSPriv.NamespaceTableDataTable nsTable = dataset.NamespaceTable;
            CoverageDS.ClassDataTable classTable = dataset.Class;

            uint sumCovered = 0;
            uint sumNotCovered = 0;
            uint codeGenCovered = 0;
            uint codeGenNotCovered = 0;

            foreach (CoverageDSPriv.NamespaceTableRow ns in nsTable)
            {
                if (!excludedNamespaces.Contains(ns.NamespaceName))
                {
                    PrintRow(String.Empty, ns.NamespaceName, ns.BlocksCovered, ns.BlocksNotCovered);
                    sumCovered += ns.BlocksCovered;
                    sumNotCovered += ns.BlocksNotCovered;
                    if (ns.NamespaceName == "Microsoft.Xaml.Tools.XamlCompiler.CodeGenerators")
                    {
                        codeGenCovered = ns.BlocksCovered;
                        codeGenNotCovered = ns.BlocksNotCovered;
                    }
                    if (showClassBreakdown)
                    {
                        foreach (Microsoft.VisualStudio.Coverage.Analysis.CoverageDSPriv.ClassRow row in classTable.Rows)
                        {
                            if (row.NamespaceKeyName == ns.NamespaceKeyName)
                            {
                                PrintRow("    ", row.ClassName, row.BlocksCovered, row.BlocksNotCovered);
                            }
                        }
                    }
                }
            }
            if (sumCovered == 0)
            {
                Console.Error.WriteLine("No Code Coverage Data collected");
                Environment.Exit(1);
            }
            Console.WriteLine("=================");
            PrintRow(String.Empty, "Total", sumCovered, sumNotCovered);
            PrintRow(String.Empty, "Total (Excluding Code Generators)", sumCovered - codeGenCovered, sumNotCovered - codeGenNotCovered);
        }

        static void PrintRow(string prefix, string name, uint covered, uint notCovered)
        {
            uint totalBlocks = covered + notCovered;
            double percent = (double)(covered * 100) / totalBlocks;
            Console.WriteLine("{0}{1,3:0.00}% == {2,4}/{3,4}: {4}", prefix, percent, covered, totalBlocks, name);
        }
    }
}
