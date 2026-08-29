// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace XamlGen
{
    public class StableIndexEntry
    {
        public string stableIndexName;
        public int stableIndex;
        public string knownIndexName;
        public bool mappedToKnownType;

        public StableIndexEntry(string[] row)
        {
            if (row.Length == 3)
            {
                stableIndexName = row[0];
                knownIndexName = row[1];
                stableIndex = Convert.ToInt32(row[2], System.Globalization.CultureInfo.InvariantCulture);
                mappedToKnownType = false;
            }
            else
            {
                throw new ArgumentException("A stable xbf index entry must have 3 elements");
            }
        }

        public StableIndexEntry(string knownTypeName, int stableTypeIndex)
        {
            stableIndexName = knownTypeName;
            knownIndexName = knownTypeName;
            stableIndex = stableTypeIndex;
            mappedToKnownType = true;
        }

        public string Write()
        {
            string result = stableIndexName + "," + knownIndexName + "," + Convert.ToString(stableIndex, System.Globalization.CultureInfo.InvariantCulture);
            return result;
        }
    }

    public class StableXbfIndexGenerator
    {
        internal const string StableXbfTypeIndexEnumName = "StableXbfTypeIndex";
        internal const string StableXbfPropertyIndexEnumName = "StableXbfPropertyIndex";
        internal const string StableXbfMethodIndexEnumName = "StableEtwMethodIndex";
        internal const string StableEventIndexEnumName = "StableEventIndex";

        private const string TypeCsvFilename = "StableXbfTypeIndexes.g.csv";
        private const string PropertyCsvFilename = "StableXbfPropertyIndexes.g.csv";
        private const string MethodCsvFilename = "StableEtwMethodIndexes.g.csv";
        private const string EventCsvFilename = "StableEventIndexes.g.csv";

        private OMContextView context;
        private string inputDirectory;
        private string outputDirectory;
        private bool partialXbfGeneration;

        private List<StableIndexEntry> typeEntriesByIndex;
        private List<StableIndexEntry> propertyEntriesByIndex;
        private List<StableIndexEntry> methodEntriesByIndex;
        private List<StableIndexEntry> eventEntriesByIndex;
        Dictionary<string, StableIndexEntry> typeEntriesByName;
        Dictionary<string, StableIndexEntry> propertyEntriesByName;
        Dictionary<string, StableIndexEntry> methodEntriesByName;
        Dictionary<string, StableIndexEntry> eventEntriesByName;

        internal readonly string UnknownTypeName;
        internal readonly string UnknownPropertyName;
        internal readonly string UnknownMethodName;
        internal readonly string UnknownEventName;

        public StableXbfIndexGenerator(OMContextView contextArg, string inputDirectoryArg, string outputDirectoryArg, bool partialXbfGenerationArg)
        {
            context = contextArg;
            inputDirectory = inputDirectoryArg;
            outputDirectory = outputDirectoryArg;
            partialXbfGeneration = partialXbfGenerationArg;
            UnknownTypeName = ClassDefinition.UnknownType.IndexNameWithoutPrefix;
            UnknownPropertyName = PropertyDefinition.UnknownProperty.IndexNameWithoutPrefix;
            UnknownMethodName = MethodDefinition.UnknownMethod.IndexNameWithoutPrefix;
            UnknownEventName = EventDefinition.UnknownEvent.IndexNameWithoutPrefix;
        }

        public void Run()
        {
            Func<ClassDefinition, bool> classSelector = c => !c.IsInterface && !c.IsAEventArgs;
            RunGroup(
                TypeCsvFilename,
                out typeEntriesByIndex,
                out typeEntriesByName,
                context.GetAllTypeTableTypes().Where(t => !(t is ClassDefinition) || classSelector(t as ClassDefinition) && t.GenerateStableIndex)
                    .Select(c => c.IndexNameWithoutPrefix),
                UnknownTypeName
                );

            RunGroup(
                PropertyCsvFilename,
                out propertyEntriesByIndex,
                out propertyEntriesByName,
                context.GetAllTypeTableProperties().Where(p => classSelector(p.DeclaringClass) && p.GenerateStableIndex).
                    Select(p => p.IndexNameWithoutPrefix),
                UnknownPropertyName
                );

            RunGroup(
                MethodCsvFilename,
                out methodEntriesByIndex,
                out methodEntriesByName,
                context.GetAllMethodIndexNamesNoPrefix(),
                UnknownMethodName
                );

            RunGroup(
                EventCsvFilename,
                out eventEntriesByIndex,
                out eventEntriesByName,
                context.GetAllEvents().Where(e => e.GenerateStableIndex).
                    Select(e => e.IndexNameWithoutPrefix),
                UnknownEventName
                );
        }

        public static bool ExposeToPublicStableIndexes(TypeDefinition definition, bool allowAbstract = false, bool requirePublicConstructor = true, bool requireDO = true)
        {
            bool safeType = null != definition
                && definition.Modifier == Modifier.Public // Hide all non-public
                && !definition.IsValueType // Hide all structs
                && !definition.IdlTypeInfo.IsExcluded // Hide all excluded idl types
                && !definition.IdlTypeInfo.IsImported // Hide all imported idl types
                && !definition.IdlTypeInfo.IsPrivateIdlOnly // Hide all private idl types
                && !definition.IsExcludedFromTypeTable // Hide types excluded from type table
                && !definition.IsImported // Hide all imported types
            ;

            ClassDefinition classDefinition = definition as ClassDefinition;
            if (null != classDefinition)
            {
                safeType = safeType
                    && (allowAbstract || (allowAbstract == classDefinition.IsAbstract)) // Hide abstract classes if allowAbstract is false
                    && (!requireDO || (requireDO && classDefinition.IsADependencyObject && classDefinition.XamlClassFlags.HasBaseTypeInDXamlInterface)) // Hide non-DO classes if requireDO is true
                    && (!requirePublicConstructor || (requirePublicConstructor && (classDefinition.IsValueType || classDefinition.Constructors.Any(c => c.Modifier == Modifier.Public && c.IsParameterless)))) // Hide classes without public parameterless constrctors if requirePublicConstructor is true
                    && classDefinition.VelocityVersion == 0 // Hide all classes that have a velocity feature version
                    && !classDefinition.XamlClassFlags.IsHiddenFromIdl // Hide all classes that are hidden from idl
                ;
            }

            safeType = safeType && !(definition is EnumDefinition); // Hide all enums

            return safeType;
        }

        public static bool ExposeToPublicStableIndexes(DependencyPropertyDefinition definition)
        {
            return null != definition
                && definition.Modifier == Modifier.Public // Hide all non-public
                && !definition.IdlDPInfo.IsExcluded // Hide all excluded idl dependency properties
                && !definition.IdlMemberInfo.IsExcluded // Hide all excluded idl members
                && !definition.IdlPropertyInfo.IsExcluded // Hide all excluded idl properties
                && string.IsNullOrWhiteSpace(definition.VelocityFeatureName) // Hide all velocity properties
                && ExposeToPublicStableIndexes(definition.DeclaringType, true, false, false) // Safe if the declaring type is safe
                ;
        }

        public static bool ExposeToPublicStableIndexes(EventDefinition definition)
        {
            return null != definition
                && definition.IdlEventInfo.ForwardDeclareIReference
                && ExposeToPublicStableIndexes(definition.DeclaringType, true, false, false) // Safe if the declaring type is safe
                ;
        }

        private void RunGroup(
            string csvFilename,
            out List<StableIndexEntry> entriesByIndex,
            out Dictionary<string, StableIndexEntry> entriesByName,
            IEnumerable<string> knownSourceEntries,
            string unknownName)
        {
            // In order to guarantee a stable ordering, even when types are removed from the table,
            // we need to store the stable index mapping in a separate file that can be compared each time this tool is run
            string inputFilename = Path.Combine(inputDirectory, csvFilename);
            string inputFileContents = "";

            // Only uncomment this try/catch block temporarily if we are adding a new stable xbf table. In normal use, the input file should always exist
            //try
            //{
            inputFileContents = File.ReadAllText(inputFilename);
            //}
            //catch (FileNotFoundException)
            //{
            //}

            List<string[]> inputValues = CSVHelper.Read(inputFileContents);

            entriesByIndex = new List<StableIndexEntry>();
            entriesByName = new Dictionary<string, StableIndexEntry>();

            for (int row = 0; row < inputValues.Count; ++row)
            {
                StableIndexEntry newEntry = new StableIndexEntry(inputValues[row]);
                entriesByIndex.Add(newEntry);
                entriesByName.Add(newEntry.stableIndexName, newEntry);
            }

            // This code should only execute if we are adding a new table!
            if (entriesByIndex.Count == 0)
            {
                StableIndexEntry newEntry = new StableIndexEntry(unknownName, 0);
                entriesByName.Add(unknownName, newEntry);
                entriesByIndex.Add(newEntry);
            }

            // Now that we've read in the stored CSV, try to match all the known types
            foreach (string knownName in knownSourceEntries)
            {
                if (entriesByName.ContainsKey(knownName))
                {
                    entriesByName[knownName].mappedToKnownType = true;
                    int stableIndex = entriesByName[knownName].stableIndex;
                    entriesByIndex[stableIndex].mappedToKnownType = true;
                }
                else if (!partialXbfGeneration)
                {
                    int newIndex = GetNextAvailableIndex(entriesByIndex);
                    StableIndexEntry newEntry = new StableIndexEntry(knownName, newIndex);
                    entriesByName.Add(knownName, newEntry);
                    entriesByIndex.Add(newEntry);
                }
            }

            // Now that we've mapped every known type to a stable xbf index
            // look for any existing stable xbf indexes that didn't get mapped to
            // and flag them as unknown
            for (int i = 0; i < entriesByIndex.Count; ++i)
            {
                if (!entriesByIndex[i].mappedToKnownType &&
                    entriesByIndex[i].knownIndexName != unknownName)
                {
                    entriesByIndex[i].stableIndexName = GetDeletedName(entriesByIndex[i].stableIndexName, entriesByName);
                    entriesByIndex[i].knownIndexName = unknownName;
                }
            }

            if (!partialXbfGeneration)
            {
                // Validate indexes and write out new CSV
                string[] lines = new string[entriesByIndex.Count];
                for (int i = 0; i < entriesByIndex.Count; ++i)
                {
                    StableIndexEntry entry = entriesByIndex[i];
                    lines[i] = entry.Write();
                }
                System.IO.File.WriteAllLines(Path.Combine(outputDirectory, csvFilename), lines);
            }
        }

        private string GetDeletedName(string stableIndexName, Dictionary<string, StableIndexEntry> entriesByName)
        {
            string root = stableIndexName + "_Deleted";
            int count = 0;
            string candidate = root + Convert.ToString(count, System.Globalization.CultureInfo.InvariantCulture);
            while (entriesByName.ContainsKey(candidate))
            {
                ++count;
                candidate = root + Convert.ToString(count, System.Globalization.CultureInfo.InvariantCulture);
            }
            return candidate;
        }

        private int GetNextAvailableIndex(List<StableIndexEntry> entriesByIndex)
        {
            return entriesByIndex.Count;
        }

        public IEnumerable<StableIndexEntry> GetStableTypeIndexes()
        {
            return typeEntriesByIndex.AsEnumerable();
        }
        public IEnumerable<StableIndexEntry> GetStablePropertyIndexes()
        {
            return propertyEntriesByIndex.AsEnumerable();
        }

        public IEnumerable<StableIndexEntry> GetStableEventIndexes()
        {
            return eventEntriesByIndex.AsEnumerable();
        }

        private string GetStableIndexNameFromKnownIndexName(string knownIndexName, Dictionary<string, StableIndexEntry> entriesByName, string unknownName)
        {
            if (entriesByName.ContainsKey(knownIndexName))
            {
                return entriesByName[knownIndexName].stableIndexName;
            }
            else
            {
                return unknownName;
            }
        }

        public string GetStableTypeIndexNameFromKnownTypeIndexName(string knownTypeIndexName)
        {
            return GetStableIndexNameFromKnownIndexName(knownTypeIndexName, typeEntriesByName, UnknownTypeName);
        }
        public string GetStablePropertyIndexNameFromKnownPropertyIndexName(string knownPropertyIndexName)
        {
            return GetStableIndexNameFromKnownIndexName(knownPropertyIndexName, propertyEntriesByName, UnknownPropertyName);
        }

        public string GetStableEventIndexNameFromKnownEventIndexName(string knownEventIndexName)
        {
            return GetStableIndexNameFromKnownIndexName(knownEventIndexName, eventEntriesByName, UnknownEventName);
        }
    }
}
