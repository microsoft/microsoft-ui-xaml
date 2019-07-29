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
    std::string str;
    MathToken();
    MathToken(TokenType t, std::string s);
};

// Handles tokenizing strings
class MathTokenizer
{
    private:
        std::string inputString;
        int inputLength;
        int index;
        bool IsNumeric(std::string in);
        bool IsOperator(std::string ss);

    public:
        MathTokenizer(std::string input);
        MathToken GetToken();
};


class NumberBoxParser
{
    private:
        static int NumberBoxParser::CmpPrecedence(char op1, char op2);
        static std::string ConvertInfixToPostFix(const std::string& infix);
        static double ComputeRpn(const std::string& expr);

    public:
        static double Compute(const winrt::hstring& expr);
        static double Compute(const std::string&& expr);
        struct MalformedExpressionException : public std::exception
        {
            virtual const char* what() const throw()
            {
                return "Mathematical Expression Malformed. No Evaluation Performed";
            }
        };
};
