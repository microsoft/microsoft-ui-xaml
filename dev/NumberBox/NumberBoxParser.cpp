#include "pch.h"
#include "common.h"
#include "NumberBoxParser.h"
#include "Utils.h"

static constexpr wstring_view c_numberBoxOperators{ L"+-*/^"sv };

// Returns list of MathTokens from expression input string. If there are any parsing errors, it returns an empty vector.
std::vector<MathToken> NumberBoxParser::GetTokens(const wchar_t* input, winrt::INumberParser numberParser)
{
    auto tokens = std::vector<MathToken>();

    size_t index = 0;
    bool error = false;
    bool expectNumber = true;
    while (input[0] != '\0' && !error)
    {
        // Skip spaces
        auto nextChar = input[0];
        if (nextChar != L' ')
        {
            if (expectNumber)
            {
                if (nextChar == '(')
                {
                    // Open parens are also acceptable, but don't change the next expected token type.
                    tokens.push_back(MathToken(MathTokenType::Parenthesis, nextChar));
                }
                else
                {
                    double value = NAN;
                    const auto charLength = GetNextNumber(input, numberParser, value);

                    if (charLength > 0)
                    {
                        tokens.push_back(MathToken(MathTokenType::Numeric, value));
                        input += charLength - 1; // advance the end of the token
                        expectNumber = false; // next token should be an operator
                    }
                    else
                    {
                        // Error case -- next token is not a number
                        error = true;
                    }
                }
            }
            else
            {
                if (c_numberBoxOperators.find(nextChar) != std::wstring::npos)
                {
                    tokens.push_back(MathToken(MathTokenType::Operator, nextChar));
                    expectNumber = true; // next token should be a number
                }
                else if (nextChar == ')')
                {
                    // Closed parens are also acceptable, but don't change the next expected token type.
                    tokens.push_back(MathToken(MathTokenType::Parenthesis, nextChar));
                }
                else
                {
                    // Error case -- could not evaluate part of the expression
                    error = true;
                }
            }
        }

        input++;
    }

    if (error)
    {
        return {};
    }
    return tokens;
}

size_t NumberBoxParser::GetNextNumber(std::wstring input, winrt::INumberParser numberParser, double& outValue)
{
    size_t charLength = 0;

    // Attempt to parse anything before an operator or space as a number
    std::wregex regex(L"^-?([^-+/*\\(\\)\\^\\s]+)");
    std::wsmatch match;
    if (std::regex_search(input.cbegin(), input.cend(), match, regex))
    {
        // Might be a number
        const auto matchLength = match[0].length();
        const auto parsedNum = numberParser.ParseDouble(input.substr(0, matchLength));

        if (parsedNum)
        {
            // Parsing was successful
            outValue = parsedNum.Value();
            charLength = matchLength;
        }
    }

    return charLength;
}

int NumberBoxParser::GetPrecedenceValue(wchar_t c)
{
    int opPrecedence = 0;
    if (c == L'*' || c == L'/')
    {
        opPrecedence = 1;
    }
    else if (c == L'^')
    {
        opPrecedence = 2;
    }

    return opPrecedence;
}

// Converts a list of tokens from infix format (e.g. "3 + 5") to postfix (e.g. "3 5 +")
std::vector<MathToken> NumberBoxParser::ConvertInfixToPostfix(std::vector<MathToken> infixTokens)
{
    std::vector<MathToken> postfixTokens;
    std::stack<MathToken> operatorStack;

    bool error = false;

    for (auto const token : infixTokens)
    {
        if (token.Type == MathTokenType::Numeric)
        {
            postfixTokens.push_back(token);
        }
        else if (token.Type == MathTokenType::Operator)
        {
            while (!operatorStack.empty())
            {
                const auto top = operatorStack.top();
                if (top.Type != MathTokenType::Parenthesis && (GetPrecedenceValue(top.Char) >= GetPrecedenceValue(token.Char)))
                {
                    postfixTokens.push_back(operatorStack.top());
                    operatorStack.pop();
                }
                else
                {
                    break;
                }
            }
            operatorStack.push(token);
        }
        else if (token.Type == MathTokenType::Parenthesis)
        {
            if (token.Char == L'(')
            {
                operatorStack.push(token);
            }
            else
            {
                while (!operatorStack.empty() && operatorStack.top().Char != L'(')
                {
                    // Pop operators onto output until we reach a left paren
                    postfixTokens.push_back(operatorStack.top());
                    operatorStack.pop();
                }

                if (operatorStack.empty())
                {
                    // Broken parenthesis
                    error = true;
                    break;
                }

                // Pop left paren and discard
                operatorStack.pop();
            }
        }
    }

    // Pop all remaining operators.
    while (!operatorStack.empty())
    {
        if (operatorStack.top().Type == MathTokenType::Parenthesis)
        {
            // Broken parenthesis
            error = true;
            break;
        }

        postfixTokens.push_back(operatorStack.top());
        operatorStack.pop();
    }

    if (error)
    {
        return {};
    }
    return postfixTokens;
}

std::optional<double> NumberBoxParser::ComputePostfixExpression(std::vector<MathToken> tokens)
{
    std::optional<double> value = {};
    std::stack<double> stack;
    bool error = false;

    for (auto const token : tokens)
    {
        if (error)
        {
            break;
        }

        if (token.Type == MathTokenType::Operator)
        {
            // There has to be at least two values on the stack to apply
            if (stack.size() < 2)
            {
                error = true;
                break;
            }

            const auto op1 = stack.top();
            stack.pop();

            const auto op2 = stack.top();
            stack.pop();

            double result = NAN;

            switch (token.Char)
            {
                case L'-':
                    result = op2 - op1;
                    break;

                case L'+':
                    result = op1 + op2;
                    break;

                case L'*':
                    result = op1 * op2;
                    break;

                case L'/':
                    if (op1 == 0)
                    {
                        error = true;
                        value = NAN;
                    }
                    else
                    {
                        result = op2 / op1;
                    }
                    break;

                case L'^':
                    result = std::pow(op2, op1);
                    break;

                default:
                    error = true;
                    break;
            }

            stack.push(result);
        }
        else if (token.Type == MathTokenType::Numeric)
        {
            stack.push(token.Value);
        }
    }

    // If there is more than one number on the stack, we didn't have enough operations, which is also an error.
    if (!error && stack.size() == 1)
    {
        value = stack.top();
    }

    return value;
}

std::optional<double> NumberBoxParser::Compute(const std::wstring_view expr, winrt::INumberParser numberParser)
{
    std::optional<double> answer = {};
    auto input = expr.data();

    // Tokenize the input string
    auto tokens = GetTokens(input, numberParser);
    if (tokens.size() > 0)
    {
        // Rearrange to postfix notation
        auto postfixTokens = ConvertInfixToPostfix(tokens);
        if (postfixTokens.size() > 0)
        {
            // Compute expression
            answer = ComputePostfixExpression(postfixTokens);
        }
    }

    return answer;
}
