// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LineBreaking class provides line breaking classification based on Unicode.

#pragma once

namespace RichTextServices
{
    enum class SimpleBreakClass
    {
        NoBreak,
        BreakAllowed,
    };

    //---------------------------------------------------------------------------
    //
    //  LineBreaking
    //
    //  Provides line breaking classification based on Unicode.
    //
    //---------------------------------------------------------------------------
    class LineBreaking
    {
    public:
        //-----------------------------------------------------------------------
        //
        //  Member:
        //      GetDWriteBreakingInfo
        //
        //  Synopsis:
        //      Gets the breaking classes and units information.
        //
        //-----------------------------------------------------------------------
        static void GetDWriteBreakingInfo(
            _Out_ XUINT32 *pBreakUnitsCount,
                // Number of breaking units
            _Out_ XUINT32 *pBreakClassesCount,
                // Number of breaking classes
            _Outptr_result_buffer_(*pBreakUnitsCount) const Ptls6::LSBRK **ppBreakUnitsInfo,
                // Array of breaking unit information
            _Outptr_result_buffer_((*pBreakClassesCount) * (*pBreakClassesCount)) const XUINT8 **ppBreakClassesInfo
                // Square array mapping breaking class pairs to breaking units
            );


        //---------------------------------------------------------------------------
        //
        //  Member:
        //      DWriteBreakToSimpleBreakClass
        //
        //  Synopsis:
        //      Helper to translate a break condition from DWrite into a breaking
        //      class for LS.  See LineBreaking.cpp for more information.
        //
        //---------------------------------------------------------------------------

        static Ptls6::BRKCLS DWriteBreakToSimpleBreakClass(XUINT8 dwriteBreakCondition);


        //---------------------------------------------------------------------------
        //
        //  Member:
        //      SimpleBreakClassToLSBrkCond
        //
        //  Synopsis:
        //      Translate a simple break class to a break condition.
        //
        //---------------------------------------------------------------------------

        static Ptls6::BRKCOND SimpleBreakClassToLSBrkCond(Ptls6::BRKCLS brkcls);


        //---------------------------------------------------------------------------
        //
        //  Member:
        //      DWriteBreakToLSBrkCond
        //
        //  Synopsis:
        //      Helper to translate a break condition from DWrite into a break
        //      condition for LS.
        //
        //---------------------------------------------------------------------------

        static Ptls6::BRKCOND DWriteBreakToLSBrkCond(XUINT8 dwriteBreakCondition);

    private:
        static const Ptls6::LSBRK s_simpleBreakingInfo[2];
        static const XUINT8 s_simpleBreakTable[2][2];
    };
}

