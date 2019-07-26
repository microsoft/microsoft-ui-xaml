#include "pch.h"
#include "NumberBoxParser.h"




MathToken::MathToken(TokenType t, std::string s)
{
    this->type = t;
    this->str = s;
}

MathToken::MathToken()
{
    this->type = MathToken::TokenType::EOFToken;
    this->str = "";
}

MathTokenizer::MathTokenizer(std::string input)
{
    inputString = input;
    inputLength = (int) inputString.size();
    index = 0;
}

MathToken MathTokenizer::GetToken()
{
    // Skip Whitespaces
    while (inputString[index] == ' ' && index <= inputLength)
    {
        index++;
    }

    if (index >= inputLength)
    {
        return MathToken(MathToken::TokenType::EOFToken, "");
    }

    std::stringstream ss;
    ss << inputString[index];

    // Return token that is +,-,*,^,/
    if (IsOperator(ss.str()))
    {
        index++;
        return MathToken(MathToken::TokenType::Operator, ss.str());
    }
    // Return parenth token
    else if (inputString[index] == '(')
    {
        index++;
        return MathToken(MathToken::TokenType::ParenLeft, ss.str());
    }
    else if (inputString[index] == ')')
    {
        index++;
        return MathToken(MathToken::TokenType::ParenRight, ss.str());
    }
    // Numeric Tokens
    else if (IsNumeric(ss.str()))
    {
        // Keep Reading until all of number is parsed
        while (IsNumeric(ss.str()) && index <= inputLength)
        {
            ss << inputString[++index];
        }
        return MathToken(MathToken::TokenType::Numeric, ss.str().substr(0, ss.str().size() - 1));
    }
    else
    {
        index++;
        return MathToken(MathToken::TokenType::Invalid, ss.str());
    }
}


bool MathTokenizer::IsNumeric(std::string in)
{
    std::regex r("^-?(\\d?)+(\\.)?(\\d?)+?$");
    return (std::regex_match(in, r));
}

bool MathTokenizer::IsOperator(std::string strin)
{
    std::regex r("^\\+|-|\\*|\\/|\\^$");
    return (std::regex_match(strin, r));
}


int NumberBoxParser::CmpPrecedence(char op1, char op2)
{
    const std::string ops = "-+/*^";
    int op1prec = (int) ops.find(op1) / 2;
    int op2prec = (int) ops.find(op2) / 2;
    if (op1prec == std::string::npos || op2prec == std::string::npos)
    {
        return -2;
    }
    if (op1prec > op2prec)
    {
        return 1;
    }
    else if (op2prec > op1prec)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

// Converts an infix mathematical expression into a postfix one, whitespace separation.
std::string NumberBoxParser::ConvertInfixToPostFix(const std::string& infix)
{
    const std::string ops = "-+/*^";

    std::stringstream out;
    std::stack<char> opstack;
    MathTokenizer input(infix);

    MathToken token;
    while ((token = input.GetToken()).type != MathToken::TokenType::EOFToken)
    {
        // Numeric Tokens push directly to output queue
        if (token.type == MathToken::TokenType::Numeric)
        {
            out << token.str << ' ';
        }
        // Operator Token
        else if (token.type == MathToken::TokenType::Operator)
        {
            while (!opstack.empty())
            {
                int prec = CmpPrecedence(opstack.top(), token.str[0]);
                // operator at top of opstack with greater precendence or equal precedence and left associative should be popped
                if ( (prec > 0 || (prec == 0 && opstack.top() != '^')) && opstack.top() != '(' )
                {
                    out << opstack.top() << ' ';
                    opstack.pop();
                }
                else {
                    break;
                }
            }
            opstack.push(token.str[0]);
        }
        // Left paren goes to operator stack
        else if (token.type == MathToken::TokenType::ParenLeft)
        {
            opstack.push('(');
        }
        // Right Paren - Pop operators 
        else if (token.type == MathToken::TokenType::ParenRight)
        {
            while ( !opstack.empty() && opstack.top() != '(')
            {
                // Pop non leftParen operators onto output
                out << opstack.top() << ' ';
                opstack.pop();
            }
            // Broken Parentheses. Don't Resolve
            if (opstack.empty())
                throw MalformedExpressionException();
            // Pop LeftParen and discard
            opstack.pop();
        }
    }

    // Pop everything to output queue
    while (!opstack.empty())
    {
        if (opstack.top() == '(' || opstack.top() == ')')
        {
           throw MalformedExpressionException();
        }
        out << opstack.top() << ' ';
        opstack.pop();
    }

    return out.str();
}

// Evaluates a postfix expression
double NumberBoxParser::ComputeRpn(const std::string& expr)
{
    std::stack<double> stack;
    MathTokenizer input(expr);
    MathToken token;
    while ((token = input.GetToken()).type != MathToken::TokenType::EOFToken)
    {
        if (token.type == MathToken::TokenType::Operator)
        {
            if (stack.empty())
                throw MalformedExpressionException();
            double op1 = stack.top();
            stack.pop();

            if (stack.empty())
                throw MalformedExpressionException();
            double op2 = stack.top();
            stack.pop();

            double res = 0;

            switch (token.str[0])
            {
                case '-':
                    res = op2 - op1;
                    break;
                case '+':
                    res = op1 + op2;
                    break;
                case '*':
                    res = op1 * op2;
                    break;
                case '/':
                    if (op1 == 0)
                    {
                        return NAN;
                    }
                    res = op2 / op1;
                    break;
                case '^':
                    res = std::pow(op2, op1);
                    break;
                default: //uh-oh
                    throw MalformedExpressionException();
            }
            stack.push(res);
        }
        else if (token.type == MathToken::TokenType::Numeric) {
            stack.push(atof(token.str.c_str()));
        }
    }
    return stack.top();
}


double NumberBoxParser::Compute(const winrt::hstring& expr)
{
    std::string InputAsString = winrt::to_string(expr);
    return ComputeRpn(ConvertInfixToPostFix(InputAsString));
}

double NumberBoxParser::Compute(const std::string&& expr)
{
    return ComputeRpn(ConvertInfixToPostFix(expr));
}
