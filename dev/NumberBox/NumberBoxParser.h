// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include <regex>
#include <stack>

enum MathTokenType
{
    Numeric,
    Operator,
    Parenthesis,
};

struct MathToken
{
    MathToken(MathTokenType t, wchar_t c) : Type(t), Char(c), Value(std::numeric_limits<double>::quiet_NaN()) {}
    MathToken(MathTokenType t, double d) : Type(t), Value(d), Char(0) {}

    MathTokenType Type;
    wchar_t Char;
    double Value;
};

class NumberBoxParser
{
    public:
        static winrt::IReference<double> Compute(const std::wstring_view expr, winrt::INumberParser numberParser);

    private:
        static std::vector<MathToken> GetTokens(const wchar_t *input, winrt::INumberParser numberParser);

        static size_t GetNextNumber(std::wstring input, winrt::INumberParser numberParser, double& outValue);
        static int GetPrecedenceValue(wchar_t c);

        static std::vector<MathToken> ConvertInfixToPostfix(std::vector<MathToken> tokens);

        static winrt::IReference<double> ComputePostfixExpression(std::vector<MathToken> tokens);
};
