// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


// this is a helper class with all static function. This takes a plainTextPosition as input, and moving to a specific position.
class TextNavigationHelper
{
private:
    static _Check_return_ HRESULT SkipHiddenCharacters(
        _In_ bool isMoveForward,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

public:
    static _Check_return_ HRESULT MoveByCharacter(
        _In_ int count,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

    static _Check_return_ HRESULT MoveByWord(
        _In_ int count,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

    static _Check_return_ HRESULT MoveToStartOrEndOfLine(
        _In_ bool isMoveToEnd,
        _In_ CDependencyObject* textOwnerObject,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

    static _Check_return_ HRESULT MoveToStartOrEndOfContent(
        _In_ bool isMoveToEnd,
        _In_ CDependencyObject* textOwnerObject,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

    static _Check_return_ HRESULT MoveToLineUpOrDownPosition(
        _In_ bool isMoveDown,
        _In_ CDependencyObject* textOwnerObject,
        _Inout_ CPlainTextPosition* plainTextPosition
        );

    static _Ret_maybenull_ CDependencyObject* FindOwnerForPosition(
        _In_ CDependencyObject* textOwnerCandidate,
        _In_ CPlainTextPosition* plainTextPosition
        );
};
