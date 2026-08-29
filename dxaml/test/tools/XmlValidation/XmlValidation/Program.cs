// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.IO;

namespace XmlValidation
{
    public class UserArgumentException : Exception
    {
        public UserArgumentException(string msg) : base(msg) { }
    }

    public class Program
    {
        public enum ExitCode
        {
            Success = 0,
            ArgumentError = 1,
            OtherError = 2,
            VerificationFailed = 27182
        }

        class CommandLineArgs
        {
            public ReferenceFile referenceFile;
            public ValidatedFile validatedFile;
            public string rulesFile;
            public string rulesXml;
        }

        static CommandLineArgs ParseCommandLine(string[] args)
        {
            CommandLineArgs parsedArgs = new CommandLineArgs();

            for (int position = 0; position < args.Length; ++position)
            {
                // Mandatory args
                if (position == 0)
                {
                    parsedArgs.referenceFile = new ReferenceFile(args[position].Trim());
                }
                else if (position == 1)
                {
                    parsedArgs.validatedFile = new ValidatedFile(args[position].Trim());
                }
                else
                {
                    switch (args[position])
                    {
                        case "-f": // rules are in file
                            {
                                if (++position < args.Length)
                                {
                                    parsedArgs.rulesFile = args[position].Trim();
                                }
                            }
                            break;

                        case "-i": // rules are inline
                            {
                                if (++position < args.Length)
                                {
                                    parsedArgs.rulesXml = args[position].Trim();
                                }
                            }
                            break;

                        default:
                            throw new UserArgumentException(string.Format("Unrecognized option: ", args[position]));
                    }
                }
            }

            return parsedArgs;
        }

        private static void ValidateFileExists(string fileType, string fileName)
        {
            if (!File.Exists(fileName))
            {
                throw new UserArgumentException(string.Format("{0} does not exist", fileType));
            }
        }

        static void ValidateCommandLineArgs(CommandLineArgs args)
        {
            args.referenceFile.ValidateFileSpecified();
            args.validatedFile.ValidateFileSpecified();
            args.referenceFile.ValidateFileExists();
            args.validatedFile.ValidateFileExists();

            if (args.rulesFile != null)
            {
                ValidateFileExists("rules_file", args.rulesFile);
            }
        }

        static bool RunValidation(CommandLineArgs args)
        {
            Validator v = new Validator();

            if (args.rulesFile != null)
            {
                v.LoadRulesFromFile(args.rulesFile);
            }
            else if (args.rulesXml != null)
            {
                v.LoadRulesFromXml(args.rulesXml);
            }

            v.LoadReference(args.referenceFile);
            v.LoadValidated(args.validatedFile);
            return v.Validate();
        }

        static void DumpArgsToOutput(CommandLineArgs args)
        {
            string rules = "<< DEFAULT >>";

            if (args.rulesFile != null)
            {
                rules = Path.GetFullPath(args.rulesFile);
            }
            else if (args.rulesXml != null)
            {
                rules = "<< INLINE >>";
            }

            System.Console.Out.WriteLine(
                "reference_file: {0}\n" +
                "validated_file: {1}\n" +
                "rules:          {2}",
                args.referenceFile,
                args.validatedFile,
                rules);
        }

        static void PrintUsage()
        {
            Console.Out.WriteLine("Usage: XmlValidation reference_file validated_file [-f rules_file] [-i rules_xml]");
        }

        static void PrintStartBanner()
        {
            Console.Out.WriteLine("\n<< UIElement tree validation tool STARTED >>");
        }

        static void PrintEndBanner()
        {
            Console.Out.WriteLine("\n<< UIElement tree validation tool DONE >>");
        }

        static int Main(string[] args)
        {
            ExitCode result;

            try
            {
                PrintStartBanner();
                CommandLineArgs parsedArgs = ParseCommandLine(args);
                DumpArgsToOutput(parsedArgs);
                ValidateCommandLineArgs(parsedArgs);
                result = (RunValidation(parsedArgs)) ? ExitCode.Success : ExitCode.VerificationFailed;
            }
            catch (UserArgumentException e)
            {
                Console.Out.WriteLine(e.Message);
                PrintUsage();
                result = ExitCode.ArgumentError;
            }
            catch (Exception e)
            {
                Console.Out.WriteLine(e);
                result = ExitCode.OtherError;
            }

            PrintEndBanner();

            return (int)result;
        }
    }
}