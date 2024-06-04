// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using XamlGen.Templates;

namespace XamlGen
{
    class Program
    {
        const string PublishedIdlFileName = "microsoft.ui.xaml.idl";
        const string MainIdlGroupName = "Main";
        const string CoreIdlGroupFileName = "microsoft.ui.xaml.coretypes.idl";
        const string Core2IdlGroupName = "coretypes2";
        const string Core2IdlGroupFileName = "microsoft.ui.xaml.coretypes2.idl";
        const string PrivateIdlGroupFileName = "microsoft.ui.xaml.private.idl";
        const string ControlsIdlGroupName = "Controls";
        const string ControlsIdlGroupFileName = "microsoft.ui.xaml.controls.controls.idl";
        const string Controls2IdlGroupName = "Controls2";
        const string Controls2IdlGroupFileName = "microsoft.ui.xaml.controls.controls2.idl";
        const string PhoneIdlGroupName = "Phone";
        const string ExtensionIdlGroupFileName = "microsoft.ui.xaml.{0}.idl";
        const string ExtensionPrivateIdlGroupFileName = "microsoft.ui.xaml.{0}-private.idl";
        const string IndexesFileName = "Indexes.g.h";
        const string StableXbfIndexesFileName = "StableXbfIndexes.g.h";
        const string TypeTableHeaderFileName = "TypeTable.g.h";
        const string StaticMetadataBodyFileName = "StaticMetadata.g.cpp";
        const string TypeCheckDataFileName = "TypeCheckData.g.h";
        const string DynamicMetadataBodyFileName = "DynamicMetadata.g.cpp";
        const string StableXbfIndexMappingFileName = "StableXbfIndexMapping.g.cpp";
        const string StableXbfIndexMetadataFileName = "StableXbfIndexMetadata.g.cs";
        const string ActivatorsHeaderFileName = "Activators.g.h";
        const string ActivatorsBodyFileName = "Activators.g.cpp";
        const string DependencyObjectTraitsFileName = "DependencyObjectTraits.g.h";
        const string FactoriesFileName = "Factories.g.cpp";
        const string UIElementControlDelegatesFileName = "UIElement.g.cpp";
        const string EnumValueTableFileName = "EnumValueTable.g.h";
        const string EnumsHeaderFileName = "Enums.g.h";
        const string EnumDefsFileName = "EnumDefs.g.h";
        const string EnumsBodyFileName = "Enums.g.cpp";
        const string BoxesFileName = "Boxes.g.cpp";
        const string ExtensionTypesHeaderFileName = "{0}Types.g.h";
        const string ExtensionTypesBodyFileName = "{0}Types.g.cpp";
        const string PhoneXamlTypeInfoBodyFileName = "XamlTypeInfo.g.cpp";
        const string PhoneXamlTypeInfoHeaderFileName = "XamlTypeInfo.g.h";
        const string PhoneXamlTypeInfoResourceFileName = "XamlTypeInfo.g.rc";
        const string UIACoreEnumsFileName = "UIACoreEnums.g.h";
        const string UIAEnumsFileName = "UIAEnums.g.h";
        const string XamlNativeRuntime_SimplePropertiesFileName = "XamlNativeRuntime_SimpleProperties.g.h";
        const string DiagnosticsInterop_SimplePropertiesFileName = "DiagnosticsInterop_SimpleProperties.g.h";
        const string SimplePropertiesCommonHeaderFileName = "SimplePropertiesCommon.g.h";
        const string SimplePropertiesCommonBodyFileName = "SimplePropertiesCommon.g.cpp";
        const string UIElementSimplePropertiesCallbacks = "UIElementSimplePropertiesCallbacks.g.cpp";
        const string XamlEtwManifestFileName = "EtwEvents.man";
        const string XamlEtwDiagnosticsManifestFileName = "EtwEvents-Diagnostics.man";
        const string XamlEtwLocalizationManifestFileName = "EtwEvents-Localization.man";
        const string SdkEtwManifestFileName = "Microsoft-Windows-XAML-ETW.man";
        const string XamlExtensionManifestFileName = "Microsoft-Windows-UI-Xaml-{0}.man";
        const string FrameworkSourcesAggregateFilename = "Aggregate.g.cpp";

        const string CoreClassesDirectoryName = "CoreClasses";
        const string CoreGeneratedHeadersFileName = "GeneratedClasses.g.h";
        const string CoreGeneratedBodiesFileName = "GeneratedClasses.g.cpp";

        const string FrameworkClassesDirectoryName = "FrameworkClasses";

        const string SimplePropertiesAdapterFilename = "SimplePropertiesAdapter.g.h";
        const string SimplePropertiesMetadataFilename = "SimplePropertiesMetadata.g.h";
        const string SimplePropertiesHelpersBodyFilename = "SimplePropertiesHelpers.g.cpp";
        static readonly string[] OutputDirectories = {
                                                         CoreClassesDirectoryName,
                                                         FrameworkClassesDirectoryName,
                                                     };

        private static ProgramParameters parameters;

        static int Main(String[] args)
        {
            try
            {
                parameters = ProgramParameters.Create(args);

                // Step 1: Create model.
                OMContext context = parameters.CreateOMContext();

                // Step 2: Validate the model.
                GuidCollisionDetector.Check(context);

                // Step 3: Run any last-minute computations, such as index and offset calculations.
                PrepareModel(context);

                // Step 4: Get a hash of the context so we can verify that the model was not modified during code-gen.
                byte[] hash = context.ComputeHash();

                // Step 5: Run code-gen.
                ExecuteGenerators(context);

                // Step 6: Verify the model was not modified. Modifications during code-gen lead to undesired complexity.
                byte[] newHash = context.ComputeHash();

                if (!hash.SequenceEqual(newHash))
                {
                    throw new InvalidOperationException("Model was modified during code-gen.");
                }
            }
            catch (InvalidParametersException ex)
            {
                Console.WriteLine(ex.Message);
                return 1;
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                return 1;
            }

            return 0;
        }

        private static void ExecuteGenerators(OMContext context)
        {
            // Make sure all directories exist and are empty.
            foreach (string outputDir in OutputDirectories)
            {
                string absoluteOutputDir = ResolveOutputPath(outputDir);
                if (Directory.Exists(absoluteOutputDir))
                {
                    foreach (string file in Directory.EnumerateFiles(absoluteOutputDir))
                    {
                        File.Delete(file);
                    }
                }
                else
                {
                    Directory.CreateDirectory(absoluteOutputDir);
                }
            }

            foreach (T4CodeGenerator generator in GetGenerators(context))
            {
                File.WriteAllText(generator.OutputPath, generator.TransformText(), generator.OutputEncoding);
            }
        }

        private static IEnumerable<T4CodeGenerator> GetGenerators(OMContext context)
        {
            // We create multiple IDL groups to work around IDL size limitations. Only select types 
            // to generate in each group.
            Func<NamespaceDefinition, bool> includeNamespaceInCoreTypesIdlGroup = ns => (ns.IdlInfo.Group == null || ns.IdlInfo.Group.Equals(MainIdlGroupName, StringComparison.OrdinalIgnoreCase));
            Func<NamespaceDefinition, bool> includeNamespaceInCoreTypes2IdlGroup = ns => (ns.IdlInfo?.Group?.Equals(Core2IdlGroupName, StringComparison.OrdinalIgnoreCase) ?? false);

            Func<TypeDefinition, bool> excludePhoneTypes = (type) => type.IdlTypeInfo.Group == null || !type.IdlTypeInfo.Group.Equals(PhoneIdlGroupName, StringComparison.OrdinalIgnoreCase);
            Func<TypeDefinition, bool> justPhoneTypes = (type) => !excludePhoneTypes(type);

            // Desktop IDL.
            Func<TypeDefinition, bool> generateInIdl = (type) => !type.IdlTypeInfo.IsExcluded && !type.IsGenericType && !type.IsImported && !type.IdlTypeInfo.IsImported && excludePhoneTypes(type);
            Func<TypeDefinition, bool> generateInPublicIdl = (type) => !type.IdlTypeInfo.IsPrivateIdlOnly && generateInIdl(type);
            Func<TypeDefinition, bool> generateInFactories = (type) => !type.IsImported && excludePhoneTypes(type);

            Func<TypeDefinition, bool> includeInCoreTypesIdlGroup = (type) => generateInPublicIdl(type) && (includeNamespaceInCoreTypesIdlGroup(type.DeclaringNamespace) && type.IdlTypeInfo.Group == null) || (type.IdlTypeInfo.Group != null && type.IdlTypeInfo.Group.Equals(MainIdlGroupName, StringComparison.OrdinalIgnoreCase));
            Func<TypeDefinition, bool> includeInCoreTypes2IdlGroup = (type) => generateInPublicIdl(type) && ((includeNamespaceInCoreTypes2IdlGroup(type.DeclaringNamespace) && type.IdlTypeInfo.Group == null) || (type.IdlTypeInfo.Group != null && type.IdlTypeInfo.Group.Equals(Core2IdlGroupName, StringComparison.OrdinalIgnoreCase)));
            Func<TypeDefinition, bool> includeInControls2IdlGroup = (type) => generateInPublicIdl(type) && (type.IdlTypeInfo.Group != null && type.IdlTypeInfo.Group.Equals(Controls2IdlGroupName, StringComparison.OrdinalIgnoreCase));
            Func<TypeDefinition, bool> includeInControlsIdlGroup = (type) => generateInPublicIdl(type) && !includeInCoreTypesIdlGroup(type) && !includeInControls2IdlGroup(type) && !includeInCoreTypes2IdlGroup(type);
            Func<TypeDefinition, bool> includeInPrivateIdlGroup = (type) => generateInIdl(type) && !generateInPublicIdl(type);

            // Phone IDL.
            Func<TypeDefinition, bool> generateInPhoneIdl = (type) => !type.IdlTypeInfo.IsExcluded && !type.IsGenericType && !type.IsImported && !type.IdlTypeInfo.IsImported && justPhoneTypes(type);
            Func<TypeDefinition, bool> generateInPublicPhoneIdl = (type) => !type.IdlTypeInfo.IsPrivateIdlOnly && generateInPhoneIdl(type);
            Func<TypeDefinition, bool> includeInPhoneIdlGroup = (type) => generateInPublicPhoneIdl(type);
            Func<TypeDefinition, bool> includeInPrivatePhoneIdlGroup = (type) => type.IdlTypeInfo.IsPrivateIdlOnly && generateInPhoneIdl(type);

            OMContextView defaultView = context.GetView(excludePhoneTypes);

            if (!parameters.IsExtension)
            {
                StableXbfIndexGenerator stableIndexes = new StableXbfIndexGenerator(defaultView, parameters.InputDirectory, parameters.OutputDirectory, parameters.PartialXbfGen);
                stableIndexes.Run();

                // microsoft.ui.xaml.coretypes.idl
                yield return new Templates.ModernIDL.Group()
                {
                    UndefGetCurrentTime = true,
                    Model = context.GetView(includeInCoreTypesIdlGroup),
                    OutputPath = ResolveOutputPath(CoreIdlGroupFileName),
                    StableIndexes = stableIndexes,
                };

                // microsoft.ui.xaml.coretypes2.idl
                yield return new Templates.ModernIDL.Group()
                {
                    Model = context.GetView(includeInCoreTypes2IdlGroup).WithCustomIdlFileNameImport(CoreIdlGroupFileName),
                    OutputPath = ResolveOutputPath(Core2IdlGroupFileName),
                    StableIndexes = stableIndexes,
                };

                // microsoft.ui.xaml.controls.controls.idl
                yield return new Templates.ModernIDL.Group()
                {
                    Model = context.GetView(includeInControlsIdlGroup).WithCustomIdlFileNameImport(Core2IdlGroupFileName),
                    OutputPath = ResolveOutputPath(ControlsIdlGroupFileName),
                    StableIndexes = stableIndexes,
                };

                // microsoft.ui.xaml.controls.controls2.idl
                yield return new Templates.ModernIDL.Group()
                {
                    Model = context.GetView(includeInControls2IdlGroup).WithCustomIdlFileNameImport(ControlsIdlGroupFileName),
                    OutputPath = ResolveOutputPath(Controls2IdlGroupFileName),
                    StableIndexes = stableIndexes,
                };

                // microsoft.ui.xaml.private.idl
                yield return new Templates.ModernIDL.Group()
                {
                    Model = context.GetView(includeInPrivateIdlGroup).WithCustomIdlFileNameImport(PublishedIdlFileName),
                    OutputPath = ResolveOutputPath(PrivateIdlGroupFileName),
                    StableIndexes = stableIndexes,
                };

                // Indexes.g.h
                yield return new Templates.Metadata.Indexes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(IndexesFileName)
                };

                // StableXbfIndexes.g.h
                yield return new Templates.Metadata.StableXbfIndexes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(StableXbfIndexesFileName),
                    StableIndexes = stableIndexes
                };

                // TypeTable.g.h
                yield return new Templates.Metadata.TypeTableHeader()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(TypeTableHeaderFileName)
                };

                // StaticMetadata.g.cpp
                yield return new Templates.Metadata.StaticMetadataBody()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(StaticMetadataBodyFileName)
                };

                // TypeCheckData.g.h
                yield return new Templates.Metadata.TypeCheckData()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(TypeCheckDataFileName)
                };

                // DynamicMetadata.g.cpp
                yield return new Templates.Metadata.DynamicMetadataBody()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(DynamicMetadataBodyFileName)
                };

                //StableXbfIndexMapping.g.cpp
                yield return new Templates.Metadata.StableXbfIndexMapping()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(StableXbfIndexMappingFileName),
                    StableIndexes = stableIndexes
                };

                // StableXbfIndexMetadata.g.cs
                yield return new Templates.Metadata.StableXbfIndexMetadata()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(StableXbfIndexMetadataFileName),
                    StableIndexes = stableIndexes
                };

                // DependencyObjectTraits.g.h
                yield return new Templates.Metadata.DependencyObjectTraits()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(DependencyObjectTraitsFileName)
                };

                // Activators.g.h
                yield return new Templates.Code.Core.Headers.Activators()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(ActivatorsHeaderFileName)
                };

                // Activators.g.cpp
                yield return new Templates.Code.Core.Bodies.Activators()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(ActivatorsBodyFileName)
                };

                // UIElement.g.cpp
                yield return new Templates.Code.Core.Bodies.UIElement()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(UIElementControlDelegatesFileName)
                };

                // Enums.g.h
                yield return new Templates.Code.Core.Headers.Enums()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(EnumsHeaderFileName)
                };

                // EnumDefs.g.h
                yield return new Templates.Code.Core.Headers.EnumDefs()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(EnumDefsFileName)
                };

                // Enums.g.cpp
                yield return new Templates.Code.Core.Bodies.Enums()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(EnumsBodyFileName)
                };

                // EnumValueTable.g.h
                yield return new Templates.Code.Core.Headers.EnumValueTables()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(EnumValueTableFileName)
                };

                yield return new Templates.Code.Framework.Headers.Synonyms()
                {
                    Model = context.GetView(t =>
                        includeInCoreTypesIdlGroup(t) ||
                        includeInCoreTypes2IdlGroup(t) ||
                        includeInControls2IdlGroup(t) ||
                        includeInControlsIdlGroup(t) ||
                        includeInPrivateIdlGroup(t)),
                    OutputPath = ResolveOutputPath("synonyms.g.h"),
                };

                // Core class *.g.h
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Core.Headers.Classes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(CoreClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Activators.g.cpp
                yield return new Templates.Code.Core.Bodies.Classes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(Path.Combine(CoreClassesDirectoryName, CoreGeneratedBodiesFileName))
                };

                /*// Core EventArgs class *.g.h
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Core.Headers.EventArgsClasses()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(CoreClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Core EventArgs class *.g.cpp
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Core.Bodies.EventArgsClasses()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(CoreClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }*/

                yield return new Templates.Code.Core.Headers.GeneratedClasses()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(Path.Combine(CoreClassesDirectoryName, CoreGeneratedHeadersFileName))
                };

                // Framework class *.g.h
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Framework.Headers.Classes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(FrameworkClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Framework class *.g.cpp
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Framework.Bodies.Classes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(FrameworkClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Framework EventArgs class *.g.h
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Framework.Headers.EventArgsClasses()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(FrameworkClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Framework EventArgs class *.g.cpp
                foreach (Templates.T4CodeGenerator generator in new Templates.Code.Framework.Bodies.EventArgsClasses()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(FrameworkClassesDirectoryName)
                }.GetGenerators())
                {
                    yield return generator;
                }

                // Boxes.g.cpp
                yield return new Templates.Code.Framework.Bodies.Boxes()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(Path.Combine(FrameworkClassesDirectoryName, BoxesFileName))
                };

                // sources
                yield return new Templates.Code.Framework.Bodies.AggregateSource()
                    {
                        Model = defaultView,
                        OutputPath = ResolveOutputPath(Path.Combine(FrameworkClassesDirectoryName, FrameworkSourcesAggregateFilename))
                    };

                // Factories.g.cpp.
                yield return new Templates.Code.Factories()
                {
                    Model = context.GetView(generateInFactories),
                    OutputPath = ResolveOutputPath(FactoriesFileName)
                };

                // UIACoreEnums.g.h
                yield return new Templates.Code.Core.Headers.UIAEnums()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(UIACoreEnumsFileName)
                };

                // UIAEnums.g.h
                yield return new Templates.Code.Framework.Headers.UIAEnums()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(UIAEnumsFileName)
                };

                // XamlNativeRuntime_SimpleProperties.g.h
                yield return new Templates.Code.Core.Headers.XamlNativeRuntime_SimpleProperties()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(XamlNativeRuntime_SimplePropertiesFileName)
                };

                // DiagnosticsInterop_SimpleProperties.g.h
                yield return new Templates.Code.Core.Headers.DiagnosticsInterop_SimpleProperties()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(DiagnosticsInterop_SimplePropertiesFileName)
                };

                // SimplePropertiesCommon.g.h
                yield return new Templates.Code.Core.Headers.SimplePropertiesCommon()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(SimplePropertiesCommonHeaderFileName)
                };

                // SimplePropertiesCommon.g.cpp
                yield return new Templates.Code.Core.Bodies.SimplePropertiesCommon()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(SimplePropertiesCommonBodyFileName)
                };

                // UIElementSimplePropertiesCallbacks.g.cpp
                yield return new Templates.Code.Core.Bodies.UIElementSimplePropertiesCallbacks()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(UIElementSimplePropertiesCallbacks)
                };

                // Microsoft-Windows-XAML-ETW.man.
                yield return new Templates.Manifests.Microsoft_Windows_Xaml_Etw()
                {
                    Model = context.GetView(excludePhoneTypes),
                    Arguments = new object[] {
                        Path.Combine(parameters.InputDirectory, XamlEtwManifestFileName),
                        Path.Combine(parameters.InputDirectory, XamlEtwDiagnosticsManifestFileName),
                        Path.Combine(parameters.InputDirectory, XamlEtwLocalizationManifestFileName)
                    },
                    OutputEncoding = Encoding.UTF8,
                    OutputPath = ResolveOutputPath(SdkEtwManifestFileName)
                };

                // SimplePropertiesAdapter.g.h
                yield return new Templates.Code.Core.Headers.SimplePropertiesAdapter()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(SimplePropertiesAdapterFilename)
                };

                // SimplePropertiesMetadata.g.h
                yield return new Templates.Code.Core.Headers.SimplePropertiesMetadata()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(SimplePropertiesMetadataFilename)
                };

                // SimplePropertiesHelpers.g.cpp
                yield return new Templates.Code.Core.Bodies.SimplePropertiesHelpersBody()
                {
                    Model = defaultView,
                    OutputPath = ResolveOutputPath(SimplePropertiesHelpersBodyFilename)
                };

            }
            else
            {
                // [Extension]Types.g.h
                yield return new Templates.Code.Phone.Headers.Types()
                {
                    Model = context.GetView(justPhoneTypes),
                    OutputPath = ResolveExtensionOutputPath(ExtensionTypesHeaderFileName)
                };

                // [Extension]Types.g.cpp
                yield return new Templates.Code.Phone.Bodies.Types()
                {
                    Model = context.GetView(justPhoneTypes),
                    OutputPath = ResolveExtensionOutputPath(ExtensionTypesBodyFileName)
                };

                // XamlTypeInfo.g.rc
                yield return new Templates.Code.Phone.Resources.XamlTypeInfo()
                {
                    Model = context.GetView(justPhoneTypes),
                    OutputPath = ResolveOutputPath(PhoneXamlTypeInfoResourceFileName)
                };

                // XamlTypeInfo.g.h
                yield return new Templates.Code.Phone.Headers.XamlTypeInfo()
                {
                    Model = context.GetView(justPhoneTypes),
                    OutputPath = ResolveOutputPath(PhoneXamlTypeInfoHeaderFileName)
                };

                // XamlTypeInfo.g.cpp
                yield return new Templates.Code.Phone.Bodies.XamlTypeInfo()
                {
                    Model = context.GetView(justPhoneTypes),
                    OutputPath = ResolveOutputPath(PhoneXamlTypeInfoBodyFileName)
                };

                if (context.Module != null)
                {
                    // microsoft.ui.xaml.[Extension].g.idl.
                    yield return new Templates.ModernIDL.Group()
                    {
                        Model = context.GetView(includeInPhoneIdlGroup, PhoneIdlGroupName).WithCustomIdlFileNameImport(PublishedIdlFileName),
                        OutputPath = ResolveExtensionOutputPath(ExtensionIdlGroupFileName),
                    };

                    // microsoft.ui.xaml.[Extension]-private.g.idl.
                    yield return new Templates.ModernIDL.Group()
                    {
                        Model = context.GetView(includeInPrivatePhoneIdlGroup).WithCustomIdlFileNameImport(PrivateIdlGroupFileName).WithCustomIdlFileNameImport(GetPhoneIdlGroupFileName(context)),
                        OutputPath = ResolveExtensionOutputPath(ExtensionPrivateIdlGroupFileName),
                    };
                }
            }
        }

        private static string GetPhoneIdlGroupFileName(OMContext context)
        {
            var module = context.Module;
            if (module != null)
            {
                return module.IdlGroupFileName;
            }

            return String.Format(ExtensionIdlGroupFileName, "Phone");
        }

        private static void PrepareModel(OMContext context)
        {
            TypeTableOptimizer.Optimize(context);
            IndexGenerator.GenerateIndexes(context);
            PhoneIndexGenerator.GenerateIndexes(context);
            TypeHandleGenerator.GenerateTypeHandles(context);
            DependencyGenerator.GenerateDependencies(context);
        }

        private static string ResolveExtensionOutputPath(string fileName)
        {
            return ResolveOutputPath(String.Format(fileName, parameters.ExtensionName ?? "Core"));
        }
        private static string ResolveOutputPath(string fileName)
        {
            return Path.Combine(parameters.OutputDirectory, fileName);
        }
    }
}
