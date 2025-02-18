#pragma once

#include <string>

struct Token
{
    int type;
    std::string id;
};

namespace expression
{
    long long int evaluate(std::list<Token> line, std::string fname, int lineno);
}
