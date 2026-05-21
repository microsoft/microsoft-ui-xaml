// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.IO;

using Codegen.Templates;

namespace Codegen
{
    class Program
    {
        static void Main(string[] args)
        {
            // We take one parameter, which is the output directory.
            if(args.Length != 1)
            {
                ShowUsage();
            }
            else
            {
                string outputDir = args[0];
                string filepath = Path.Combine(outputDir, "EventsListeners.g.cs");

                ManagedEventsListeners eventsTemplate = new ManagedEventsListeners();
                File.WriteAllText(filepath, eventsTemplate.TransformText());
            }
        }

        private static void ShowUsage()
        {
            Console.WriteLine("Usage: codegen <output_directory>");
        }
    }
}
