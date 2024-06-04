// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen
{
    /// <summary>
    /// Assigns type handles to classes that participate in the type table.
    /// </summary>
    class TypeHandleGenerator
    {
        private ClassDefinition[] _classes;

        private TypeHandleGenerator(ClassDefinition[] classes)
        {
            _classes = classes;
        }

        internal static void GenerateTypeHandles(OMContext context)
        {
            // Filter out interfaces, statics and value types, because they're not as common for type assignability checks.
            new TypeHandleGenerator(context.GetView().GetTypeTableClasses().Where(c => !c.IsInterface && !c.IsStatic && !c.IsAEventArgs && c.GenerateTypeCheckInfo).ToArray()).Run();
        }

        private void Run()
        {
            Dictionary<ClassDefinition, int> classDepth = new Dictionary<ClassDefinition, int>();
            HashSet<ClassDefinition> baseClasses = new HashSet<ClassDefinition>();
            Dictionary<uint, LevelData> levelsDictionary = new Dictionary<uint, LevelData>();

            const int TypeHandleBitLength = 48;
            const ulong LeafTypeMarker = (ulong)1 << (TypeHandleBitLength - 1);

            // Calculate class depth and the number of derived types per base class.
            foreach (ClassDefinition type in _classes)
            {
                if (type.BaseClassInTypeTable != ClassDefinition.UnknownType)
                {
                    baseClasses.Add(type.BaseClassInTypeTable);
                }

                classDepth.Add(type, GetClassDepth(type));
            }

            // For each depth where there are base classes, reserve bits.
            foreach (ClassDefinition baseClass in baseClasses)
            {
                LevelData level;
                uint depth = (uint)classDepth[baseClass];

                if (!levelsDictionary.TryGetValue(depth, out level))
                {
                    level = new LevelData(depth);
                    levelsDictionary.Add(depth, level);
                }

                level.BaseClasses.Add(baseClass);
            }

            LevelData[] levelsArray = levelsDictionary.OrderBy(p => p.Key).Select(p => p.Value).ToArray();

            // We use highest-order bit to mark a type as a leaf type (i.e. not a base class). Reserve 1 bit here.
            int requiredBitSize = 1;

            // Process each level and assign type handles to all the base classes.
            for (int i = 0; i < levelsArray.Length; i++)
            {
                LevelData currentLevel = levelsArray[i];

                // Calculate the bitsize for this level.
                requiredBitSize += currentLevel.NumberOfBits;

                ulong levelTypeId = 1;

                if (i == 0)
                {
                    // First level, no need to shift/combine.
                    foreach (ClassDefinition c in currentLevel.BaseClasses)
                    {
                        c.TypeHandle = levelTypeId++;
                        c.TypeHandleMask = currentLevel.LevelMask;
                    }
                }
                else
                {
                    LevelData previousLevel = levelsArray[i - 1];
                    currentLevel.Offset = (previousLevel.Offset + previousLevel.NumberOfBits);
                    foreach (ClassDefinition c in currentLevel.BaseClasses)
                    {
                        ulong typeId = levelTypeId++;

                        // Encode base type handle.
                        c.TypeHandle = c.BaseClassInTypeTable.TypeHandle;

                        // Write the type ID at the right offset.
                        c.TypeHandle |= (typeId << currentLevel.Offset);

                        // Write the type mask.
                        c.TypeHandleMask = currentLevel.LevelMask | previousLevel.LevelMask;
                    }
                }
            }

            // Validate that we haven't exceeded the maximum bit size we currently support. If we ever hit this exception, it means our type hierarchy 
            // has grown to a point where we need more bits to represent type handles.
            // This check was off by a factor of 2 before handle and handle mask were 32-bits each.  Now, that they are moved to 64-bits, it is correct.
            if (requiredBitSize > TypeHandleBitLength)
            {
                throw new InvalidOperationException(string.Format("TypeHandle bit size exceeds {0} bits. Update the size of the TypeHandle field in TypeTableStructs.h.", TypeHandleBitLength));
            }

            // Assign type handles to the remaining (leaf) types.
            foreach (ClassDefinition leafClass in _classes.Where(c => c.TypeHandle == 0))
            {
                // Encode base type handle.
                leafClass.TypeHandle = leafClass.BaseClassInTypeTable.TypeHandle;

                // Mark type as leaf type. Use the highest-order bit.
                leafClass.TypeHandle |= LeafTypeMarker;

                leafClass.TypeHandleMask = leafClass.TypeHandle;
            }
        }

        private static int GetClassDepth(ClassDefinition type)
        {
            int level = 0;
            for (ClassDefinition baseClass = type.BaseClassInTypeTable; baseClass != ClassDefinition.UnknownType; baseClass = baseClass.BaseClassInTypeTable)
            {
                level++;
            }
            return level;
        }

        private static int GetBitSize(int number)
        {
            if (number == 0) return 0;
            if (number < 2) return 1;
            if (number < 4) return 2;
            if (number < 8) return 3;
            if (number < 16) return 4;
            if (number < 32) return 5;
            if (number < 64) return 6;
            if (number < 128) return 7;
            if (number < 256) return 8;
            throw new ArgumentException("Too wide number");
        }

        private static ulong GetMask(int numberOfBits, int offset)
        {
            ulong mask = 0;
            ulong offsetBit = (ulong)1 << offset;

            for (int i = 0; i < numberOfBits; i++)
            {
                mask = (mask << 1) | offsetBit;
            }

            return mask;
        }

        class LevelData
        {
            public List<ClassDefinition> BaseClasses
            {
                get;
                private set;
            }

            public int MaxDescendents
            {
                get;
                set;
            }

            public uint LevelNumber
            {
                get;
                private set;
            }

            public ulong LevelMask
            {
                get
                {
                    return GetMask(NumberOfBits + Offset, 0);
                }
            }

            public int NumberOfBits
            {
                get
                {
                    return GetBitSize(BaseClasses.Count);
                }
            }

            public int Offset
            {
                get;
                set;
            }

            public LevelData(uint levelNumber)
            {
                BaseClasses = new List<ClassDefinition>();
                LevelNumber = levelNumber;
            }
        }
    }
}
