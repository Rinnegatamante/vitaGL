#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <string>
#include <stack>
#include <limits.h>

#include "const.h"
#include "expression.h"

namespace expression
{
    struct Node
    {
        Node() : child(NULL), child2(NULL)
        {
        }

        ~Node()
        {
            if (child) delete child;
            if (child2) delete child2;
        }

        int type;
        std::string id;
        Node* child;
        Node* child2;
    };

    std::list<Token>::iterator ptok, pEnd;
    long long int eax, ecx;
    std::stack<long long int> stack;
    int g_lineno;
    std::string g_fname;

    Node* parse_exp();

    // ######################################################################

    void advance()
    {
        if (ptok == pEnd)
        {
            throw std::string("Unexpected end of line when processing IF or ELIF");
        }
        ptok++;
    }

    // ######################################################################

    int getType()
    {
        if (ptok == pEnd)
        {
            throw std::string("Unexpected end of line when processing IF or ELIF");
        }
        return ptok->type;
    }

    // ######################################################################

    void fail(const char* err)
    {
        throw std::string("Error: ") + err + " got " + std::to_string(getType()) + ": " + g_fname + " @ " + std::to_string(g_lineno);
    }

    // ######################################################################
    // 2 or -2
    Node* parse_factor()
    {
        Node* exp = new Node; //( Node*)malloc(sizeof( Node));
        int type = getType();

        if (type == OPEN_BRACKET) {
            advance();
            exp = parse_exp();
            if (getType() != CLOSE_BRACKET) fail("Expected )");
            advance();
        }
        else if (type == NUMBER) {
            exp->type = NUMBER;
            exp->id = ptok->id; //newStr(tokenHead->id);
            exp->child = NULL;
            advance();
        }
        else if (type == CHAR) {
            exp->type = CHAR;
            exp->id = ptok->id; //newStr(tokenHead->id);
            exp->child = NULL;
            advance();
        }
        else if (type == MINUS) {
            exp->type = UNARY_MINUS;
            advance();
            exp->child = parse_factor();
        }
        else if (type == PLUS) {
            exp->type = UNARY_PLUS;
            advance();
            exp->child = parse_factor();
        }
        else if (type == TILDE) {
            exp->type = UNARY_COMPLEMENT;
            advance();
            exp->child = parse_factor();
        }
        else if (type == EXCLAM) {
            exp->type = UNARY_NOT;
            advance();
            exp->child = parse_factor();
        }
        else
            fail("Expected literal or unary operator");

        return exp;
    }

    // ######################################################################

    //3*5*7
    Node* parse_term()
    {
        Node* factor = parse_factor();
        int nextType = getType();
        while (nextType == ASTERISK || nextType == SLASH) {
            advance();
            Node* next_factor = parse_factor();
            Node* new_factor = new Node; //(Node*)malloc(sizeof(Node));

            if (nextType == ASTERISK)
                new_factor->type = BINARY_TIMES;
            else
                new_factor->type = BINARY_DIVIDE;

            new_factor->child = factor;
            new_factor->child2 = next_factor;
            factor = new_factor;
            nextType = getType();
        }

        return factor;
    }

    // ######################################################################

    //1+2+3
    Node* parse_additive_exp()
    {
        Node* term = parse_term();
        int nextType = getType();
        while (nextType == PLUS || nextType == MINUS) {
            advance();
            Node* next_term = parse_term();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            if (nextType == PLUS)
                new_term->type = BINARY_PLUS;
            else
                new_term->type = BINARY_MINUS;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1<<2
    Node* parse_shift_exp()
    {
        Node* term = parse_additive_exp();
        int nextType = getType();
        while (nextType == LESSTHAN2 || nextType == GREATERTHAN2) {
            advance();
            Node* next_term = parse_additive_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            if (nextType == LESSTHAN2)
                new_term->type = BINARY_LEFT_SHIFT;
            else
                new_term->type = BINARY_RIGHT_SHIFT;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1<2<=3
    Node* parse_relational_exp()
    {
        Node* term = parse_shift_exp();
        int nextType = getType();
        while (nextType == GREATERTHAN || nextType == LESSTHAN ||
            nextType == GREATERTHAN_EQUAL || nextType == LESSTHAN_EQUAL) {
            advance();
            Node* next_term = parse_shift_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            if (nextType == GREATERTHAN) {
                new_term->type = BINARY_GREATER_THAN;
            }
            else if (nextType == LESSTHAN) {
                new_term->type = BINARY_LESS_THAN;
            }
            else if (nextType == GREATERTHAN_EQUAL) {
                new_term->type = BINARY_GREATER_THAN_OR_EQUAL;
            }
            else {
                new_term->type = BINARY_LESS_THAN_OR_EQUAL;
            }

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 != 2 == 3
    Node* parse_equality_exp()
    {
        Node* term = parse_relational_exp();
        int nextType = getType();
        while (nextType == EQUALS2 || nextType == EXCLAM_EQUAL) {
            advance();
            Node* next_term = parse_relational_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            if (nextType == EQUALS2)
                new_term->type = BINARY_EQUAL;
            else
                new_term->type = BINARY_NOT_EQUAL;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 | 2 
    Node* parse_bitwise_and_exp()
    {
        Node* term = parse_equality_exp();
        int nextType = getType();
        while (nextType == AMP) {
            advance();
            Node* next_term = parse_equality_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            new_term->type = BINARY_BITWISE_AND;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 ^ 2 
    Node* parse_bitwise_xor_exp()
    {
        Node* term = parse_bitwise_and_exp();
        int nextType = getType();
        while (nextType == HAT) {
            advance();
            Node* next_term = parse_bitwise_and_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            new_term->type = BINARY_BITWISE_XOR;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 | 2 
    Node* parse_bitwise_or_exp()
    {
        Node* term = parse_bitwise_xor_exp();
        int nextType = getType();
        while (nextType == PIPE) {
            advance();
            Node* next_term = parse_bitwise_xor_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            new_term->type = BINARY_BITWISE_OR;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 && 2 && 3
    Node* parse_and_exp()
    {
        Node* term = parse_bitwise_or_exp();
        int nextType = getType();
        while (nextType == AMP2) {
            advance();
            Node* next_term = parse_bitwise_or_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            new_term->type = BINARY_AND;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    //1 || 2 || 3
    Node* parse_or_exp()
    {
        Node* term = parse_and_exp();
        int nextType = getType();
        while (nextType == PIPE2) {
            advance();
            Node* next_term = parse_and_exp();
            Node* new_term = new Node; // (Node*)malloc(sizeof(Node));

            new_term->type = BINARY_OR;

            new_term->child = term;
            new_term->child2 = next_term;
            term = new_term;
            nextType = getType();
        }

        return term;
    }

    // ######################################################################

    Node* parse_exp()
    {
        Node* exp = parse_or_exp();
        return exp;
    }

    // ######################################################################

    void processTree(Node* node)
    {
        if (node->type == UNARY_MINUS)
        {
            processTree(node->child);
            eax = -eax;
        }
        else if (node->type == UNARY_PLUS)
        {
            processTree(node->child);
            eax = +eax;
        }
        else if (node->type == UNARY_NOT)
        {
            processTree(node->child);
            eax = !eax;
        }
        else if (node->type == UNARY_COMPLEMENT)
        {
            processTree(node->child);
            eax = ~eax;
        }
        else if (node->type == NUMBER)
        {
            char* endptr = NULL;
            errno = 0;

            eax = strtoll(node->id.c_str(), &endptr, 0);

            if (errno == ERANGE && eax == LONG_MIN)
                throw "Number invalid (underflow occurred): " + node->id;
            else if (errno == ERANGE && eax == LONG_MAX)
                throw "Number invalid (overflow occurred): " + node->id;

            bool ok = true;
            if (errno == 0)
            {
                while (*endptr != '\0')
                {
                    if (toupper(*endptr) != 'U' && toupper(*endptr) != 'L')
                    {
                        ok = false;
                        break;
                    }
                    endptr++;
                }
            }

            if (!ok || errno != 0 || *endptr != '\0')
            {
                throw "Error: failed to make sense of number. Integers must be used in IF or ELIF statements: " + node->id;
            }
        }
        else if (node->type == CHAR)
        {
            if (node->id == "\\0")
                eax = 0;
            else if (node->id == "\\n")
                eax = '\n';
            else if (node->id == "\\r")
                eax = '\r';
            else if (node->id == "\\a")
                eax = '\a';
            else if (node->id == "\\b")
                eax = '\b';
            else if (node->id == "\\f")
                eax = '\f';
            else if (node->id == "\\t")
                eax = '\t';
            else if (node->id == "\\v")
                eax = '\v';
            else if (node->id == "\\'")
                eax = '\'';
            else if (node->id == "\\\\")
                eax = '\\';
            else
                eax = (long long int)node->id[0];
        }
        else if (node->type == BINARY_PLUS)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax += ecx;
        }
        else if (node->type == BINARY_MINUS)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax -= ecx;
        }
        else if (node->type == BINARY_TIMES)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax *= ecx;
        }
        else if (node->type == BINARY_DIVIDE)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax /= ecx;
        }
        else if (node->type == BINARY_EQUAL)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax == ecx);
        }
        else if (node->type == BINARY_NOT_EQUAL)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax != ecx);
        }
        else if (node->type == BINARY_GREATER_THAN)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax > ecx);
        }
        else if (node->type == BINARY_LESS_THAN)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax < ecx);
        }
        else if (node->type == BINARY_GREATER_THAN_OR_EQUAL)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax >= ecx);
        }
        else if (node->type == BINARY_LESS_THAN_OR_EQUAL)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax <= ecx);
        }
        else if (node->type == BINARY_AND)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax && ecx);
        }
        else if (node->type == BINARY_OR)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax = (eax || ecx);
        }
        else if (node->type == BINARY_BITWISE_OR)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax |= ecx;
        }
        else if (node->type == BINARY_BITWISE_AND)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax &= ecx;
        }
        else if (node->type == BINARY_BITWISE_XOR)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax ^= ecx;
        }
        else if (node->type == BINARY_LEFT_SHIFT)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax <<= ecx;
        }
        else if (node->type == BINARY_RIGHT_SHIFT)
        {
            processTree(node->child2);
            stack.push(eax);
            processTree(node->child);
            ecx = stack.top();
            stack.pop();
            eax >>= ecx;
        }
        else
        {
            throw std::string("Unknown or illegal node type found in processTree: ") + std::to_string(node->type);
        }
    }

    // ######################################################################

    long long int evaluate(std::list<Token> line, std::string fname, int lineno)
    {
        g_fname = fname;
        g_lineno = lineno;

        Token semi;
        semi.type = SEMICOLON;
        line.push_back(semi);

        ptok = line.begin();
        pEnd = line.end();

        ptok++;
        Node* tree = parse_exp();   // uses and advances ptok
        processTree(tree);          // result stored in eax
        delete tree;                // deletes entire tree
        return eax;
    }
}
