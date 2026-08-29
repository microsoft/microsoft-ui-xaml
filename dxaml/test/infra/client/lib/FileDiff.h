// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This diff implementation was adopted from a tool that ships with iDNA tracing found in
// the windows source depots here:
// It has been lightly simplified and adopted to work with WEX::Logger. It performed the
// classic Myers diff algorithm based on finding the edit distance from the LCS.

namespace FileDiff
{
    class FileLine;
    class HashTable;
    typedef UINT HashKey;
    struct IngorePairsArray;

    class HashLine
    {

    public:
        static void Hash (
            const char* textStart,
            const char* textEnd,
            FileLine* hashedFileLine,
            HashTable*  hashTable,
            int fileFlag,
            bool fIgnoreSpace
        );

        static bool Compare(HashLine* hl1, HashLine* hl2);

        const char* Text;
        size_t Length;
        HashLine* Next; // hash bucket link
        HashKey Key;
        int fileFlag; // only need two bits; can steal from length if is an issue
        bool fTextOwned;

    private:
        static char* ProcessToIgnoreList(const char* textStart, const char* textEnd);
        static bool ProcessToIgnorePair(char** processingStr, char* leftStr, char* rightStr, size_t startindex);

        static char* RemoveSpaces (const char* textStart, const char* textEnd);
        static char* RemoveCallCount (const char* textStart, const char* textEnd);
    };

    struct HashLineAllocList
    {
        HashLineAllocList* next;
        HashLine* alloc;
    };

    class HashTable
    {
    public:
        HashTable(UINT);
        ~HashTable();

        HashLine* LookUp(HashLine* object);
        HashLine* LookUpOrInsert(HashLine* object);

        HashLine* NewHashLine();
        void FreeHashLine(HashLine* hl);

        UINT NumBuckets;
        HashLine** Buckets;

    private:
        HashLine* GetBucket(HashKey key)
        {
            key %= this->NumBuckets;
            return this->Buckets[key];
        }

        HashLine* AddToBucket(HashLine* object);

        enum Constants
        {
            HashLineAllocSize = 4096 / sizeof(HashLine)
        };

        HashLine* HashLineFreeList;
        HashLine* HashLineAllocArray;
        HashLineAllocList* HashLineAllocArrayList;

        UINT HashLineAllocCount;
    };

    class TextLine
    {
    public:
        TextLine() {}
        const char * Text;
        size_t Length;
    };

    class FileLine
    {
    public:
        FileLine() {}
        HashLine * hashLine; // Hashed text (intraline spacing compressed)
        int LineNumber;
    };

    class FileDiff;
    class File
    {
    public:
        struct SFlags
        {
            UINT IsDifferent : 1;
            UINT IsIgnored : 1;
            UINT WasIgnored : 1;
        };

        File(const wchar_t* fileName);
        ~File();

        void ParseFile (FileDiff* fileDiff, int fileFlag);
        void RemoveUnique(int fileFlag);

        FileLine* HashedLineArray;
        TextLine* TextLineArray;
        UINT HashedLineCount;
        UINT TextLineCount;
        SFlags* Flags; // should be fixed bit vectors
        bool fIgnoredAny;
        char* buf;
        char* bufEnd;
        UINT lineCount;
    private:
        bool ReadFromResource(const wchar_t* name);
        void ReadFromFile(const wchar_t* fileName);
    };

    class FileDiff
    {
        friend class File;
    public:
        FileDiff();
        ~FileDiff();
        void Diff(const wchar_t* file1, const wchar_t* file2);

    private:
        HashTable* hashTable;

        bool fIgnoreIntralineSpace;
        bool fIgnoreInterlineSpace;
        bool fElide;
        bool fRemoveUnique; // TEMPORARY!!

        void PrintAdd(int xs, int ys, int ye);
        void PrintDelete(int xs, int xe, int ys);
        void PrintChange (int xs, int xe, int ys, int ye);

        File* file[2];
        FileLine* lineArray[2];
        int xIndex;

        void Diff();
        void PrintDiffs();
        void PrintDiff(int xs, int xe, int ys, int ye);
        void MarkHashDiff (int xs, int xe, int ys, int ye);
        void MarkOneHashDiff(int index, int start, int end);
        void MarkTextDiff( int xs, int xe, int ys, int ye);
        void MarkOneTextDiff(int index, int start, int end);

        void PrintCached();
        bool CanElide(int start, int end, int index);

        __forceinline int Snake(int k, int ys, int * len);

        void MiddleSnake(int xs, int xe, int ys, int ye, int parentSnakeLen);

        int GetFileSize(const char* file);
        void PostProcessIgnored();

        void HandleOneIgnore(int index, unsigned start, unsigned end, bool fTouchingDiff);

    private:
        int* fpBuf;  // furthest point buffer
        int bestSnakePos[2];
        int bestSnakeLen;
        double bestSnakeDensity;

        enum
        {
            BigSnake = 200
        };

        int cachedKind; // -1 delete, 0, change, 1 add
        int cachedStart[2];
        int cachedEnd[2];
    };
}
