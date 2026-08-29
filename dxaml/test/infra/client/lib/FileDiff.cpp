// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FileDiff.h"

#define yIndex (1 - xIndex)
#define IsSpace(c) (((c) == ' ') || ((c) == '\t'))
#undef max
#undef min

namespace Private {
    namespace Infrastructure {
        HRESULT ReadTestResource(_In_ const wchar_t* resourceName, _Out_ const uint8_t** pData, _Out_ DWORD* pDataLength);
    }
}

namespace FileDiff
{
    void HashLine::Hash (
        const char* textStart, const char* textEnd,
        FileLine* hashedFileLine, HashTable* hashTable,
        int fileFlag, bool fIgnoreSpace)
    {
        char* compressedText = NULL;
        size_t realLen = textEnd - textStart + 1;
        HashKey realKey = 0;

        char* newText = NULL;

        if (fIgnoreSpace)
        {
            // We're ignoring space, so check if there are any spaces to compress.
            // We'll truncate at head and tail.
            compressedText = HashLine::RemoveSpaces(textStart, textEnd);
        }

        if (compressedText != NULL)
        {
            newText = compressedText;
            realLen = strlen(newText);
            compressedText = NULL;
        }

        if (compressedText != NULL)
        {
            if (newText != NULL)
            {
                delete[] newText;
            }
            newText = compressedText;
            realLen = strlen(newText);
        }

        if (newText != NULL)
        {
            realLen = strlen(newText);
        }
        if (newText != NULL)
        {
            for (size_t i = 0; i < realLen; i++)
            {
                realKey = realKey * 0x12345 + newText[i];
            }
        }
        else
        {
            for (const char* p = textStart; p <= textEnd; p++)
            {
                realKey = realKey * 0x12345 + *p;
            }
        }

        HashLine* hashLine = hashTable->NewHashLine();
        hashLine->Text = newText == NULL ? textStart : newText;
        hashLine->Length = realLen;
        hashLine->Key = realKey;

        HashLine* hashedLine = hashTable->LookUpOrInsert(hashLine);
        if (hashedLine != hashLine)
        {
            if (newText != NULL)
            {
                delete[] newText;
            }
            hashTable->FreeHashLine(hashLine);
            hashedLine->fileFlag |= fileFlag;
        }
        else
        {
            hashedLine->fTextOwned = newText != NULL;
            hashedLine->fileFlag = fileFlag;
        }

        hashedFileLine->hashLine = hashedLine;
    }

    char* HashLine::RemoveSpaces(const char* textStart, const char* textEnd)
    {
        size_t realLen = textEnd - textStart + 1;

        size_t compressedLen = 0;
        bool fLastSpace = false;

        for (const char* p = textStart; p <= textEnd; p++)
        {
            if (IsSpace(*p))
            {
                if (fLastSpace)
                {
                    continue;
                }
                fLastSpace = true;
            }
            else
            {
                fLastSpace = false;
            }

            compressedLen++;
        }

        // Truncate the last space if there is one

        if (fLastSpace)
        {
            compressedLen--;
        }

        if (realLen == compressedLen)
        {
            return NULL;
        }

        char* retVal = new char[compressedLen + 1];
        char* pCopy = retVal;

        fLastSpace = false;

        size_t copyLen = 0;
        for (const char* p = textStart; copyLen < compressedLen; p++)
        {
            if (IsSpace(*p))
            {
                if (fLastSpace)
                {
                    continue;
                }
                fLastSpace = true;
                *pCopy = ' ';
            }
            else
            {
                fLastSpace = false;
                *pCopy = *p;
            }
            pCopy++;
            copyLen++;
        }

        *pCopy = '\0';
        return retVal;
    }

    // At the end of the line there is the number in parenthesis that has the count
    // of calls within the function. If the proper variable is set, we want to
    // ignore that number for diffing. This function will return a string with
    // that piece of string removed.
    char* HashLine::RemoveCallCount(const char* textStart, const char* textEnd)
    {
        size_t len = textEnd - textStart + 1;
        bool openParen = false;
        size_t removeCount = 0;
        for (const char* p = textStart; p <= textEnd; p++)
        {
            if (*p == '(')
            {
                openParen = true;
            }
            else if (*p == ')')
            {
                removeCount += (openParen == true);
                openParen = false;
            }
            removeCount += (openParen == true);
        }

        if ((removeCount == 0) || (removeCount == len))
        {
            return NULL;
        }

        // In order to make room for the closure of the string, we need to add
        // one more space at the end

        char* retStr = new char[len - removeCount + 1];
        char* pCopy = retStr;
        openParen = false;
        for (const char* p = textStart; p <= textEnd; p++)
        {
            if (*p == '(')
            {
                openParen = true;
            }
            else if (*p == ')')
            {
                p++;
                openParen = false;
                continue;
            }
            if (!openParen)
            {
                *pCopy = *p;
                pCopy++;
            }
        }
        *pCopy = '\0';
        return retStr;
    }

    HashTable::HashTable(UINT size)
        : NumBuckets(size)
    {
        this->Buckets = new HashLine *[size]; // this->NumBuckets causes FE ICE

        for (UINT i = 0; i < this->NumBuckets; i++)
        {
            this->Buckets[i] = NULL;
        }

        this->HashLineFreeList = NULL;
        this->HashLineAllocArray = NULL;
        this->HashLineAllocArrayList = NULL;
        this->HashLineAllocCount = 0;
    }

    HashTable::~HashTable()
    {
        for (UINT i = 0; i < this->NumBuckets; i++)
        {
            HashLine* hashLine = this->Buckets[i];
            while (hashLine != NULL)
            {
                if (hashLine->fTextOwned)
                {
                    delete[] hashLine->Text;
                }

                hashLine = hashLine->Next;
            }
        }

        // Delete the hashline allocation arrays.

        HashLineAllocList* hlaal = this->HashLineAllocArrayList;

        while (hlaal != NULL)
        {
            HashLineAllocList* next = hlaal->next;

            delete[] hlaal->alloc;

            delete hlaal;

            hlaal = next;
        }

        delete[] this->Buckets;
    }

    HashLine* HashTable::NewHashLine()
    {
        HashLine* hl = this->HashLineFreeList;
        if (hl != NULL)
        {
            this->HashLineFreeList = hl->Next;
            return hl;
        }

        if (this->HashLineAllocCount == 0)
        {
            this->HashLineAllocArray = new HashLine[HashLineAllocSize];
            this->HashLineAllocCount = HashLineAllocSize;

            HashLineAllocList* hlaal = new HashLineAllocList;

            hlaal->alloc = this->HashLineAllocArray;
            hlaal->next = this->HashLineAllocArrayList;
            this->HashLineAllocArrayList = hlaal;
        }

        hl = &this->HashLineAllocArray[--this->HashLineAllocCount];
        return hl;
    }

    void HashTable::FreeHashLine
    (
        HashLine* hl
    )
    {
        hl->Next = this->HashLineFreeList;
        this->HashLineFreeList = hl;
    }

    HashLine* HashTable::AddToBucket
    (
        HashLine* object
    )
    {
        UINT key = object->Key % this->NumBuckets;

        object->Next = this->Buckets[key];
        this->Buckets[key] = object;

        return object;
    }

    HashLine* HashTable::LookUp(HashLine* object)
    {
        HashLine* ho;

        ho = this->GetBucket(object->Key);

        while (ho != NULL)
        {
            if ((ho->Key == object->Key)
                    && (ho->Length == object->Length)
                    && (memcmp(ho->Text, object->Text, ho->Length) == 0))
            {
                return ho;
            }

            ho = ho->Next;
        }

        return NULL;
    }

    HashLine* HashTable::LookUpOrInsert(HashLine* object)
    {
        HashLine* findObject = this->LookUp(object);
        if (findObject != NULL)
        {
            return findObject;
        }

        this->AddToBucket(object);

        return object;
    }

    bool File::ReadFromResource(const wchar_t* filePath)
    {
        std::wstring fileName(filePath);
        const static std::wstring mastersMarkerStr(L"\\test\\resources\\masters\\");
        const auto found = fileName.find(mastersMarkerStr);
        const bool isMaster = found != std::wstring::npos;
        if (isMaster)
        {
            std::wstring name = fileName.substr(found + mastersMarkerStr.size());
            const uint8_t* data = nullptr;
            DWORD dataLength = 0;
            if (FAILED(Private::Infrastructure::ReadTestResource(name.c_str(), &data, &dataLength)))
            {
                return false;
            }

            this->buf = new char[dataLength + 1]; // +1 because we'll stash a 0 at end
            this->bufEnd = this->buf + dataLength;
            memcpy_s(this->buf, dataLength, data, dataLength);
            this->buf[dataLength] = 0;

            // Count the number of lines in the file.
            char* p = this->buf;
            this->lineCount = 0;

            while (p < this->bufEnd)
            {
                while ((p < this->bufEnd)
                    && (*p != '\n'))
                {
                    p++;
                }
                p++;

                this->lineCount++;
            }
        }

        return isMaster;
    }

    void File::ReadFromFile(const wchar_t* fileName)
    {
        HashedLineArray = NULL;
        TextLineArray = NULL;
        Flags = NULL;
        // Read the entire file into memory.

        // Use NT functions rather than CRT.

        // ISSUE-REVIEW-Work around XP bug involving CreateProcess and handles
        // not being released soon enough.
        //
        // Note that there may be a real problem in some strange situation, so
        // give up after a little while.

        HANDLE hFile;
        int sleepTime = 15;

        for (;;)
        {
            hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                break;
            }

            Sleep(sleepTime);
            if (sleepTime < 5000)
            {
                sleepTime *= 2;
            }
        }

        if (hFile == INVALID_HANDLE_VALUE)
        {
            this->buf = NULL;
            this->bufEnd = NULL;
            this->lineCount = 0;

            return;
        }

        DWORD fileSize = GetFileSize(hFile, NULL);

        this->buf = new char[fileSize + 1]; // +1 because we'll stash a 0 at end
        this->bufEnd = this->buf + fileSize;

        DWORD bytesRead;
        const BOOL fRet = ReadFile(hFile, buf, fileSize, &bytesRead, NULL);
        if (!fRet || (bytesRead != fileSize))
        {
            __debugbreak();
        }

        CloseHandle(hFile);

        // Count the number of lines in the file.

        char* p = this->buf;
        this->lineCount = 0;

        while (p < this->bufEnd)
        {
            while ((p < this->bufEnd)
                    && (*p != '\n'))
            {
                p++;
            }
            p++;

            this->lineCount++;
        }
    }

    File::File(const wchar_t* fileName)
    {
        if (!ReadFromResource(fileName))
        {
            ReadFromFile(fileName);
        }
    }

    void File::ParseFile(FileDiff* fileDiff, int fileFlag)
    {
        // Allocate the line array.  We allocate one more than actually needed to
        // handle insertions at the end of the file which are emitted as
        // <line number> - 1.  In that special case, line number is not a real
        // line, so we need to fabricate it.

        this->HashedLineArray = new FileLine[lineCount + 1];
        this->TextLineArray = new TextLine[lineCount + 1];
        this->Flags = new SFlags[lineCount + 1];
        this->fIgnoredAny = false;

        // In case of 0 length file.

        this->TextLineArray[0].Text = NULL;

        ZeroMemory(this->Flags, sizeof(SFlags) * lineCount);

        // Parse the file into text lines.

        char* p = this->buf;

        int lineNum = -1;
        unsigned count = 0;

        while (p < this->bufEnd)
        {
            lineNum++;

            char* start = p;

            while ((p < this->bufEnd)
                    && (*p != '\n'))
            {
                p++;
            }

            char* end = p;

            // Skip line endings.

            if (*p == '\n')
            {
                p++;
            }

            // For traceDiff we'll probably want to hash these lines as well since we
            // won't be keeping the original files around.

            this->TextLineArray[lineNum].Text = start;
            this->TextLineArray[lineNum].Length = p - start;

            if (end == this->bufEnd)
            {
                // We reached the end of the buffer; back up.

                end--;
            }
            else if (fileDiff->fIgnoreIntralineSpace)
            {
                // We want to compare the line endings if we're ignoring intraline
                // spacing.

                if (*end == '\n')
                {
                    end--;
                    if ((end >= start) && (*end == '\r'))
                    {
                        end--;
                    }
                }
            }

            if (fileDiff->fIgnoreInterlineSpace)
            {
                bool fThisIsBlank = true;
                for (char* b = start; b <= end; b++)
                {
                    if (!IsSpace(*b) && (*b != '\r') && (*b != '\n'))
                    {
                        fThisIsBlank = false;
                        break;
                    }
                }

                if (fThisIsBlank)
                {
                    this->Flags[lineNum].IsIgnored = true;
                    this->fIgnoredAny = true;
                    continue;
                }
            }

            HashLine::Hash(start, end,
                           &this->HashedLineArray[count],
                           fileDiff->hashTable, fileFlag, fileDiff->fIgnoreIntralineSpace);

            this->HashedLineArray[count].LineNumber = lineNum;
            count++;
        }

        this->HashedLineCount = count;
        this->TextLineCount = lineNum + 1;
    }

    File::~File()
    {
        delete[] this->buf;
        delete[] this->HashedLineArray;
        delete[] this->TextLineArray;
        delete[] this->Flags;
    }

    void File::RemoveUnique(int fileFlag)
    {
        unsigned from;
        unsigned to = 0;

        for (from = 0; from < this->HashedLineCount; from++)
        {
            int lineNum = this->HashedLineArray[from].LineNumber;

            // Assert ->fileFlag & fileFlag != 0
            // (i.e. this line must have been hashed for this file)

            if (this->HashedLineArray[from].hashLine->fileFlag == fileFlag)
            {
                this->Flags[lineNum].IsDifferent = true;
                continue;
            }

            if (to < from)
            {
                this->HashedLineArray[to] = this->HashedLineArray[from];
            }

            to++;
        }

        this->HashedLineCount = to;
    }


    // Runs of ignored lines that exist between diffs should be considered
    // different.  Ignored lines adjacent to diffs are considered diffs if one
    // side is longer than the other.  In that case, only the unbalanced lines
    // are different.
    void FileDiff::PostProcessIgnored()
    {
        if (!this->file[0]->fIgnoredAny
                && !this->file[1]->fIgnoredAny)
        {
            return;
        }

        int xs = 0;
        unsigned xe = 0;
        int ys = 0;
        unsigned ye = 0;

        for (;;)
        {
            bool fEOX = (xs >= (int)this->file[xIndex]->TextLineCount);
            bool fEOY = (ys >= (int)this->file[yIndex]->TextLineCount);

            if (fEOX && fEOY)
            {
                break;
            }

            // Skip different lines.

            if (!fEOX && this->file[xIndex]->Flags[xs].IsDifferent)
            {
                do
                {
                    xs++;
                    fEOX = (xs >= (int)this->file[xIndex]->TextLineCount);
                }
                while (!fEOX && this->file[xIndex]->Flags[xs].IsDifferent);

                if (fEOX)
                {
                    continue;
                }
            }

            if (!fEOY && this->file[yIndex]->Flags[ys].IsDifferent)
            {
                do
                {
                    ys++;
                    fEOY = (ys >= (int)this->file[yIndex]->TextLineCount);
                }
                while (!fEOY && this->file[yIndex]->Flags[ys].IsDifferent);

                if (fEOY)
                {
                    continue;
                }
            }

            bool fXIsIgnored = !fEOX && this->file[xIndex]->Flags[xs].IsIgnored;
            bool fYIsIgnored = !fEOY && this->file[yIndex]->Flags[ys].IsIgnored;

            // Not ignored, iterate.

            if (!fXIsIgnored && !fYIsIgnored)
            {
                if (!fEOX)
                {
                    xs++;
                }

                if (!fEOY)
                {
                    ys++;
                }

                continue;
            }

            // Find the ignored extent and surrounding line status.

            bool fBeforeIsDiff[2];
            bool fAfterIsDiff[2];

            if (fXIsIgnored)
            {
                xe = xs;

                while ((xe < this->file[xIndex]->TextLineCount - 1)
                        && this->file[xIndex]->Flags[xe + 1].IsIgnored)
                {
                    xe++;
                }

                fBeforeIsDiff[xIndex] = (xs < 0)
                                        || this->file[xIndex]->Flags[xs - 1].IsDifferent;
                fAfterIsDiff[xIndex] = (xe >= this->file[xIndex]->TextLineCount)
                                       || this->file[xIndex]->Flags[xe + 1].IsDifferent;
            }
            else
            {
                xe = xs - 1;
            }

            if (fYIsIgnored)
            {
                ye = ys;

                while ((ye < this->file[yIndex]->TextLineCount - 1)
                        && this->file[yIndex]->Flags[ye + 1].IsIgnored)
                {
                    ye++;
                }

                fBeforeIsDiff[yIndex] = (ys < 0)
                                        || this->file[yIndex]->Flags[ys - 1].IsDifferent;
                fAfterIsDiff[yIndex] = (ye >= this->file[yIndex]->TextLineCount)
                                       || this->file[yIndex]->Flags[ye + 1].IsDifferent;
            }
            else
            {
                ye = ys - 1;
            }

            bool fTouchingDiff[2];

            fTouchingDiff[0] = fBeforeIsDiff[0] | fAfterIsDiff[0];
            fTouchingDiff[1] = fBeforeIsDiff[1] | fAfterIsDiff[1];

            // If only one side ignored, mark as different.

            if (fXIsIgnored && !fYIsIgnored)
            {
                HandleOneIgnore(xIndex, xs, xe, fTouchingDiff[xIndex]);
            }
            else if (fYIsIgnored && !fXIsIgnored)
            {
                HandleOneIgnore(yIndex, ys, ye, fTouchingDiff[yIndex]);
            }
            else
            {
                // Both sides ignored, check for surrounding status.

                bool fSurrounded[2];

                fSurrounded[0] = fBeforeIsDiff[0] && fAfterIsDiff[0];
                fSurrounded[1] = fBeforeIsDiff[1] && fAfterIsDiff[1];

                if (fSurrounded[xIndex] && fSurrounded[yIndex])
                {
                    MarkTextDiff(xs, xe, ys, ye);
                }
                else
                {
                    // Neither surrounded.  Mark the common lines as no longer ignored
                    // and restart the comparison leaving the rest unchecked.

                    int len[2];
                    int minLen;

                    len[xIndex] = xe - xs + 1;
                    len[yIndex] = ye - ys + 1;

                    minLen = __min(len[xIndex], len[yIndex]);

                    // Clear the ignored status.  Otherwise we can end up skipping over
                    // the line when there is a nearby diff on one side but not the
                    // other.

                    for (int i = 0; i < minLen; i++)
                    {
                        this->file[xIndex]->Flags[xs + i].IsIgnored = false;
                        this->file[xIndex]->Flags[xs + i].WasIgnored = true;
                        this->file[yIndex]->Flags[ys + i].IsIgnored = false;
                        this->file[yIndex]->Flags[ys + i].WasIgnored = true;
                    }

                    xs = xs + minLen;
                    ys = ys + minLen;

                    continue;
                }
            }

            xs = xe + 1;
            ys = ye + 1;
        }
    }

    // If the ignored lines are touching a diff, we mark them as different.  If
    // they are touching other ignored lines, we elide backwards looking for a
    // diff to become part of.
    void FileDiff::HandleOneIgnore
    (
        int index,
        unsigned start,
        unsigned end,
        bool fTouchingDiff
    )
    {
        if (fTouchingDiff)
        {
            MarkOneTextDiff(index, start, end);
            return;
        }

        unsigned newStart = start;
        unsigned newEnd = end;

        while ((newStart > 0)
                && this->file[index]->Flags[newStart - 1].WasIgnored)
        {
            newStart--;
            newEnd--;
        }

        if ((newStart > 0)
                && this->file[index]->Flags[newStart - 1].IsDifferent)
        {
            MarkOneTextDiff(index, newStart, newEnd);

            while (end != newEnd)
            {
                this->file[index]->Flags[end].IsIgnored = false;
                end--;
            }

            return;
        }
    }

    void FileDiff::PrintDiffs()
    {
        unsigned xs = 0;
        unsigned xe = 0;
        unsigned ys = 0;
        unsigned ye = 0;

        for (;;)
        {
            bool fEOX = (xs >= this->file[xIndex]->TextLineCount);
            bool fEOY = (ys >= this->file[yIndex]->TextLineCount);

            if (fEOX && fEOY)
            {
                break;
            }

            bool fXIsDifferent = !fEOX && this->file[xIndex]->Flags[xs].IsDifferent;
            bool fYIsDifferent = !fEOY && this->file[yIndex]->Flags[ys].IsDifferent;

            // Not different, iterate.

            if (!fXIsDifferent && !fYIsDifferent)
            {
                if (!fEOX)
                {
                    // Skip ignored, non-different lines.
                    // (We want to treat ignored lines that are "the same" as in the
                    //  other file as one unit because they can be arbitrarily
                    //  different length.)

                    do
                    {
                        xs++;
                    }
                    while ((xs < this->file[xIndex]->TextLineCount)
                            && this->file[xIndex]->Flags[xs - 1].IsIgnored);
                }

                if (!fEOY)
                {
                    // Skip ignored, non-different lines.
                    // (We want to treat ignored lines that are "the same" as in the
                    //  other file as one unit because they can be arbitrarily
                    //  different length.)

                    do
                    {
                        ys++;
                    }
                    while ((ys < this->file[yIndex]->TextLineCount)
                            && this->file[yIndex]->Flags[ys - 1].IsIgnored);
                }

                continue;
            }

            // Find the diff extent.

            if (fXIsDifferent)
            {
                xe = xs;

                while ((xe < this->file[xIndex]->TextLineCount - 1)
                        && this->file[xIndex]->Flags[xe + 1].IsDifferent)
                {
                    xe++;
                }
            }
            else
            {
                xe = xs - 1;
            }

            if (fYIsDifferent)
            {
                ye = ys;

                while ((ye < this->file[yIndex]->TextLineCount - 1)
                        && this->file[yIndex]->Flags[ye + 1].IsDifferent)
                {
                    ye++;
                }
            }
            else
            {
                ye = ys - 1;
            }

            // Print the diff.

            PrintDiff(xs, xe, ys, ye);

            xs = xe + 1;
            ys = ye + 1;
        }

        this->PrintCached();
    }

    void FileDiff::PrintDiff(int xs, int xe, int ys, int ye)
    {
        int kind;

        if (xe < xs)
        {
            if (ye < ys)
            {
                return;
            }

            kind = 1;
        }
        else if (ye < ys)
        {
            kind = -1;
        }
        else
        {
            kind = 0;
        }

        if (this->fElide
                && ((this->cachedKind == 0) || (this->cachedKind == kind)))
        {
            bool fSuccess = true;

            // Try to elide this diff with the previous one.

            // Left half...

            if (kind <= 0)
            {
                fSuccess &= this->CanElide(xs, xe, 0);
            }

            // Right half...

            if (fSuccess && (kind >= 0))
            {
                fSuccess &= this->CanElide(ys, ye, 1);
            }

            if (fSuccess)
            {
                if (kind <= 0)
                {
                    this->cachedEnd[0] = xe - (xs - this->cachedEnd[0]) + 1;
                }

                if (kind >= 0)
                {
                    this->cachedEnd[1] = ye - (ys - this->cachedEnd[1]) + 1;
                }

                if (this->cachedKind != kind)
                {
                    this->cachedKind = 0;
                }

                return;
            }
        }

        this->PrintCached();

        this->cachedKind = kind;
        this->cachedStart[0] = xs;
        this->cachedEnd[0] = xe;
        this->cachedStart[1] = ys;
        this->cachedEnd[1] = ye;
    }

    bool FileDiff::CanElide(int start, int end, int index)
    {
        int cachedEnd = this->cachedEnd[index];
        int common = start - 1;

        // Compare the tail of the diff with the common text before it.
        // ISSUE-TODO We are dealing with diff line numbers here, so eliding uses
        // the real text rather than hashed text.  This is only an issue when
        // intraline space is compressed.

        TextLine* lineArray = this->file[index]->TextLineArray;
        File::SFlags* flagArray = this->file[index]->Flags;

        while ((end > start) && (common > cachedEnd))
        {
            if ((flagArray[end].WasIgnored && flagArray[common].IsIgnored)
                    || ((lineArray[end].Length == lineArray[common].Length)
                        && (memcmp(lineArray[end].Text, lineArray[common].Text,
                                   lineArray[end].Length) == 0)))
            {
                end--;
                common--;
            }
            else
            {
                break;
            }
        }

        if (common == cachedEnd)
        {
            return true;
        }

        return false;
    }

    void FileDiff::PrintCached()
    {
        switch (this->cachedKind)
        {
        case 1:

            this->PrintAdd(this->cachedStart[0],
                           this->cachedStart[1], this->cachedEnd[1]);
            break;

        case -1:

            this->PrintDelete(this->cachedStart[0], this->cachedEnd[0],
                              this->cachedStart[1]);
            break;

        case 0:

            this->PrintChange(this->cachedStart[0], this->cachedEnd[0],
                              this->cachedStart[1], this->cachedEnd[1]);
            break;
        }

        this->cachedKind = -2;
    }

    void FileDiff::MarkHashDiff(int xs, int xe, int ys, int ye)
    {
        this->MarkOneHashDiff(xIndex, xs, xe);
        this->MarkOneHashDiff(yIndex, ys, ye);
    }

    void FileDiff::MarkOneHashDiff
    (
        int index,
        int start,
        int end
    )
    {
        while (start <= end)
        {
            int lineNum = this->lineArray[index][start].LineNumber;

            this->file[index]->Flags[lineNum].IsDifferent = true;
            start++;
        }
    }

    // We mark the differing lines rather than the common lines under the
    // assumption that there will be less diffs.
    void FileDiff::MarkTextDiff(int xs, int xe, int ys, int ye)
    {
        this->MarkOneTextDiff(xIndex, xs, xe);
        this->MarkOneTextDiff(yIndex, ys, ye);
    }

    void FileDiff::MarkOneTextDiff(int index, int start, int end)
    {
        while (start <= end)
        {
            this->file[index]->Flags[start].IsDifferent = true;
            this->file[index]->Flags[start].IsIgnored = false;
            start++;
        }
    }

    __forceinline int FileDiff::Snake(int k, int ys, int* len)
    {
        int x = ys - k;
        int y = ys;

        while ((x < len[xIndex])
                && (y < len[yIndex])
                && (lineArray[xIndex][x].hashLine == lineArray[yIndex][y].hashLine))
        {
            x++;
            y++;
        }

        int snakeLen = y - ys;

        if ((snakeLen > 0)
                && (snakeLen >= bestSnakeLen))
        {
            int lineNumRangeX = this->lineArray[xIndex][x - 1].LineNumber -
                                this->lineArray[xIndex][ys - k].LineNumber + 1;
            int lineNumRangeY = this->lineArray[yIndex][y - 1].LineNumber -
                                this->lineArray[yIndex][ys].LineNumber + 1;
            double density;

            if (lineNumRangeY > lineNumRangeX)
            {
                density = (double)lineNumRangeY / (double)snakeLen;
            }
            else
            {
                density = (double)lineNumRangeX / (double)snakeLen;
            }

            if ((snakeLen > bestSnakeLen)
                    || (density < bestSnakeDensity))
            {
                bestSnakeLen = snakeLen;
                bestSnakeDensity = density;
                bestSnakePos[xIndex] = ys - k;
                bestSnakePos[yIndex] = ys;
            }
        }

        return y;
    }

    void FileDiff::MiddleSnake(int xs, int xe, int ys, int ye, int parentSnakeLen)
    {
        int len[2];

        len[xIndex] = xe - xs + 1;
        len[yIndex] = ye - ys + 1;

        if ((len[xIndex] < 1) ||
                (len[yIndex] < 1))
        {
            MarkHashDiff(xs, xe, ys, ye);
            return;
        }

        // Not sure this is necessary, but I don't understand the P-Band algorithm
        // well enough yet, so being conservative and making everything 0-based.

        lineArray[xIndex] += xs;
        lineArray[yIndex] += ys;

        bestSnakeLen = 0;

        //--------------------------------------------------------------------------
        //
        // The Longest Common Sequence algorithm is from
        //    An O(NP) Sequence Comparison Algorithm
        //       Wu, Manber, Myers 1989
        //
        // I have made an alteration since we aren't trying to find the edit
        // distance, but just the LCS: Stop when the biggest possible snake has
        // been found.
        //
        // We also can stop if we find a "big enough" snake to minimize how much
        // work is going on but this code is currently disabled.
        //
        // Start of the algorithm

        int delta = len[yIndex] - len[xIndex];
        bool fSwap = delta < 0;
        if (fSwap)
        {
            // Algorithm requires |Y| > |X| to be efficient.

            xIndex = yIndex;
            delta = -delta;
        }

        int* fp = &fpBuf[file[xIndex]->HashedLineCount + 1];
        for (int i = -(len[xIndex] + 1); i <= len[yIndex] + 1; i++)
        {
            fp[i] = -1;
        }

        int p = -1;

        do
        {
            p++;

            for (int k = -p; k <= delta - 1; k++)
            {
                fp[k] = Snake(k, __max(fp[k - 1] + 1, fp[k + 1]), len);
            }

            for (int k = delta + p; k >= delta + 1; k--)
            {
                fp[k] = Snake(k, __max(fp[k - 1] + 1, fp[k + 1]), len);
            }

            fp[delta] = Snake(delta, __max(fp[delta - 1] + 1, fp[delta + 1]), len);

            // If we have a snake as big as our parent's, stop since there can be
            // none larger... but make sure we have good density.

            if ((bestSnakeLen == parentSnakeLen)
                    && (bestSnakeDensity == 1.0))
            {
                break;
            }
        }
        // ORIG: while (fp[delta] < len[yIndex]);
        while (len[yIndex] - fp[delta] > bestSnakeLen);

        if (fSwap)
        {
            xIndex = yIndex;
        }

        // End of the algorithm
        //
        //--------------------------------------------------------------------------

        lineArray[xIndex] -= xs;
        lineArray[yIndex] -= ys;

        int snakePos[2];

        snakePos[xIndex] = bestSnakePos[xIndex] + xs;
        snakePos[yIndex] = bestSnakePos[yIndex] + ys;
        int snakeLen = bestSnakeLen;

        if (snakeLen > 0)
        {
            // If we have more than one snake there is enough commonality to require
            // divide and conquer.

            // Recurse the part before the best snake.

            MiddleSnake(xs, snakePos[xIndex] - 1,
                        ys, snakePos[yIndex] - 1, snakeLen);

            // Recurse the part after the best snake.

            MiddleSnake(snakePos[xIndex] + snakeLen, xe,
                        snakePos[yIndex] + snakeLen, ye, snakeLen);
        }
        else
        {
            // No snake; both parts completely different so emit both as a change.

            MarkHashDiff(xs, xe, ys, ye);
        }
    }

    FileDiff::FileDiff()
    {
        hashTable = NULL;
        this->cachedKind = -2;
    }

    FileDiff::~FileDiff()
    {
        delete this->file[0];
        delete this->file[1];

        delete this->hashTable;
    }

    int FileDiff::GetFileSize(const char* fileName)
    {
        HANDLE hFile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        // DWORD, but whatever.

        int fileSize = ::GetFileSize(hFile, NULL);

        CloseHandle(hFile);

        return fileSize;
    }

    void FileDiff::Diff(const wchar_t* file1, const wchar_t* file2)
    {
        // Get the file sizes so we can size the hash table appropriately.

        this->file[0] = new File(file1);
        this->file[1] = new File(file2);
        this->Diff();
    }

    void FileDiff::Diff()
    {
        int maxSize = __max(this->file[0]->lineCount, this->file[1]->lineCount);
        int hashSize = __max(maxSize / 8, 128);

        this->hashTable = new HashTable(hashSize);

        this->file[0]->ParseFile(this, 1);
        this->file[1]->ParseFile(this, 2);

        if (this->fRemoveUnique)
        {
            // Remove unique lines from the line arrays.

            this->file[0]->RemoveUnique(1);
            this->file[1]->RemoveUnique(2);
        }

        int bufSize = this->file[0]->HashedLineCount +
                      this->file[1]->HashedLineCount + 2;
        this->fpBuf = new int[bufSize + 1];

        this->lineArray[0] = this->file[0]->HashedLineArray;
        this->lineArray[1] = this->file[1]->HashedLineArray;

        this->xIndex = 0;

        this->MiddleSnake(0, this->file[0]->HashedLineCount - 1,
                          0, this->file[1]->HashedLineCount - 1, -1);

        // Post-process ignored lines.

        this->PostProcessIgnored();

        this->PrintDiffs();

        delete this->fpBuf;
    }

    void FileDiff::PrintAdd(int xs, int ys, int ye)
    {
        LOG_OUTPUT(L"===== Add @ %d =====", xs);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        for (int i = std::max(xs - 3, 0); i < xs; i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = ys; i <= ye; i++)
        {
            auto& textLine = this->file[1]->TextLineArray[i];
            LOG_OUTPUT(L"+++%s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = xs; i < std::min(xs + 3, static_cast<int>(this->file[0]->TextLineCount)); i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }
    }

    void FileDiff::PrintDelete(int xs, int xe, int /*ys*/)
    {
        LOG_OUTPUT(L"===== Delete @ %d =====", xs);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        for (int i = std::max(xs - 3, 0); i < xs; i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = xs; i <= xe; i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"---%s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = std::min(xe + 1, static_cast<int>(this->file[0]->TextLineCount - 1));
            i < std::min(xe + 4, static_cast<int>(this->file[0]->TextLineCount)); i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }
    }

    void FileDiff::PrintChange(int xs, int xe, int ys, int ye)
    {
        LOG_OUTPUT(L"===== Edit @ %d =====", xs);

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        for (int i = std::max(xs - 3, 0); i < xs; i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = xs; i <= xe; i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"---%s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = ys; i <= ye; i++)
        {
            auto& textLine = this->file[1]->TextLineArray[i];
            LOG_OUTPUT(L"+++%s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }

        for (int i = std::min(xe + 1, static_cast<int>(this->file[0]->TextLineCount - 1));
            i < std::min(xe + 4, static_cast<int>(this->file[0]->TextLineCount)); i++)
        {
            auto& textLine = this->file[0]->TextLineArray[i];
            LOG_OUTPUT(L"   %s", converter.from_bytes(textLine.Text, std::find(textLine.Text, textLine.Text + textLine.Length, '\r')).c_str());
        }
    }

} // namespace FileDiff
