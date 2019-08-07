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
        Invalid = 0,
        Numeric = 1,
        Operator = 2,
        ParenLeft = 3,
        ParenRight = 4,
        EOFToken = 6
    };
    TokenType type;
    std::wstring str;
    MathToken();
    MathToken(TokenType t, std::wstring s);
    MathToken(TokenType t, std::wstring& s);

};

// Handles tokenizing strings
class MathTokenizer
{
    private:
        int m_inputLength{};
        int m_index{0};
        MathToken m_lastToken;
        bool IsNumeric(std::wstring_view in);
        bool IsOperator(std::wstring_view in);
        void SkipWhiteSpace();

    public:
        std::wstring m_inputString;
        MathTokenizer(std::wstring input);
        MathToken GetToken();
        MathToken MathTokenizer::PeekNextToken();
};

// Handles parsing and evaluating mathematical strings
class NumberBoxParser
{
    private:
        enum OperatorPrecedence
        {
            Error = -2,
            Lower = -1,
            Equal = 0,
            Higher = 1
        };
        static OperatorPrecedence NumberBoxParser::CmpPrecedence(wchar_t op1, wchar_t op2);
        static std::wstring ConvertInfixToPostFix(const std::wstring& infix);
        static std::optional<double> ComputeRpn(const std::wstring& expr);

    public:
        static std::optional<double> Compute(const winrt::hstring& expr);
        static std::optional<double> Compute(const std::wstring&& expr);
        struct MalformedExpressionException : public std::exception
        {
            virtual const char* what() const throw()
            {
                return "Mathematical Expression Malformed. No Evaluation Performed";
            }
        };
};
