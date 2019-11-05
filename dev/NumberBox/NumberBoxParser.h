#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <regex>
#include <stack>
#include <optional>

// Represents a token in a mathematic expression. This can be a simple operator (+-*/^), a decimal number, or parentheses. 
class MathToken
{
public:
    enum TokenType
    {
        Invalid,
        Numeric,
        Operator,
        ParenLeft,
        ParenRight,
        EOFToken
    };
    MathToken(TokenType t, std::wstring s);
    MathToken(TokenType t, std::wstring& s);
    MathToken() = default;
    TokenType type{};
    std::wstring str;

};

// Handles tokenizing strings
class MathTokenizer
{
    public:
        enum ExpressionType
        {
            Infix,
            Postfix
        };
        MathTokenizer(std::wstring_view input);
        MathToken GetToken(ExpressionType type);
        MathToken PeekNextToken(ExpressionType type);
        std::wstring m_inputString;

    private:
        bool IsNumeric(std::wstring_view in);
        bool IsOperator(std::wstring_view in);
        void SkipWhiteSpace();
        int m_index{0};
        int m_inputLength{};
        bool m_negVal{ false };
        MathToken m_lastToken{};

};

// Handles parsing and evaluating mathematical strings
class NumberBoxParser
{
    public:
        static std::optional<double> Compute(const std::wstring_view expr);

    private:
        enum OperatorPrecedence
        {
            Error = -2,
            Lower = -1,
            Equal = 0,
            Higher = 1
        };
        static OperatorPrecedence CmpPrecedence(wchar_t op1, wchar_t op2);
        static std::wstring ConvertInfixToPostFix(const std::wstring& infix);
        static std::optional<double> ComputeRpn(const std::wstring& expr);
};
