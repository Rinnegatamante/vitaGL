#ifdef HAVE_GLSL_PREPROCESSOR
// Credits: https://github.com/john-blackburn/preprocessor
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <list>
#include <string>
#include <assert.h>
#include <algorithm>
#include <set>
#include <map>
#include <stack>
#include <time.h>
#include "const.h"
#include "expression.h"

using namespace std;

namespace preprocessor
{

const char* tokNames[] = { "<<=", ">>=", "->*",

          "&&", "||", "==", "!=", "<=", ">=", "<<", ">>",
          "++", "--", "->",
          "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ".*",

          "<", ">", "!", "=", ",", ";", "...", ".", "(",  ")", "{", "}", "[", "]",
          "+", "-", "*", "/", "%", "&", "|", "^", "~", "?", "::", ":", "##",
          "@", "\\",        // not standard but used in Microsoft headers

          "#define", "#undef", "#include", "#if", "#ifdef", "#ifndef", "#elif", "#else", "#endif", "#line", "#error", "#warning", "#pragma" };

const char* names[] = {
  "LESSTHAN2_EQUAL",
  "GREATERTHAN2_EQUAL",
  "MINUS_GREATERTHAN_ASTERISK",
  "AMP2",
  "PIPE2",
  "EQUALS2",
  "EXCLAM_EQUAL",
  "LESSTHAN_EQUAL",
  "GREATERTHAN_EQUAL",
  "LESSTHAN2",
  "GREATERTHAN2",
  "PLUS2",
  "MINUS2",
  "MINUS_GREATERTHAN",
  "PLUS_EQUALS",
  "MINUS_EQUALS",
  "ASTERISK_EQUALS",
  "SLASH_EQUALS",
  "PERCENT_EQUALS",
  "AMP_EQUALS",
  "PIPE_EQUALS",
  "HAT_EQUALS",
  "DOT_ASTERISK",
  "LESSTHAN",
  "GREATERTHAN",
  "EXCLAM",
  "EQUALS",
  "COMMA",
  "SEMICOLON",
  "DOT3",
  "DOT",
  "OPEN_BRACKET",
  "CLOSE_BRACKET",
  "OPEN_BRACE",
  "CLOSE_BRACE",
  "OPEN_SQUARE",
  "CLOSE_SQUARE",
  "PLUS",
  "MINUS",
  "ASTERISK",
  "SLASH",
  "PERCENT",
  "AMP",
  "PIPE",
  "HAT",
  "TILDE",
  "QUESTION",
  "COLON2",
  "COLON",
  "HASH2",
  "AT",
  "BACKSLASH",

  "DEFINE",
  "UNDEF",
  "INCLUDE",
  "IF",
  "IFDEF",
  "IFNDEF",
  "ELIF",
  "ELSE",
  "ENDIF",
  "LINE",
  "ERROR",
  "WARNING",
  "PRAGMA",
  "HASH",

  "NUMBER",           // eg 123
  "IDENTIFIER",       // eg main
  "STRING",           // "hello, world!"
  "CHAR",             // 'a'
  "WSTRING",          // L"hello, world!"
  "WCHAR",            // L'a'

  "UNARY_PLUS",
  "UNARY_MINUS",
  "UNARY_COMPLEMENT",
  "UNARY_NOT",
  "BINARY_PLUS",
  "BINARY_MINUS",
  "BINARY_TIMES",
  "BINARY_DIVIDE",
  "BINARY_AND",
  "BINARY_OR",
  "BINARY_EQUAL",
  "BINARY_NOT_EQUAL",
  "BINARY_LESS_THAN_OR_EQUAL",
  "BINARY_LESS_THAN",
  "BINARY_GREATER_THAN_OR_EQUAL",
  "BINARY_GREATER_THAN",
  "BINARY_BITWISE_AND",
  "BINARY_BITWISE_OR",
  "BINARY_BITWISE_XOR",
  "BINARY_LEFT_SHIFT",
  "BINARY_RIGHT_SHIFT",
  "DEFINE_FUNC"
};

struct Var
{
    string name;
    list<string> args;
    list<Token> val;
    bool functionLike;
};

const bool g_debug=false;
string g_mode;
string g_outstring;
FILE* g_outfile;
int g_lineno;
string g_fname;
bool g_hasIncludeSupported;

list<string> g_blacklist;
map<string, stack<Var>> g_stacks;      // for push/pop_macro
map<string, string> g_attributeMap;    // for __has_cpp_attribute

// forward declaration
void processFile(string fname, map<string,Var>& vars, const list<string>& includePaths, list<string>& included);

// ######################################################################

void wrtError(string msg)
{
    throw "ERROR, line " + to_string(g_lineno) + ", " + g_fname + ": " + msg;
}

// ######################################################################

// ----------------------------------------------------------------------
// Replace tokens in list of tokens "mylist" (in situ). Tokens in range [st,ed) are replaced by "replacement"
// ----------------------------------------------------------------------

void replace(list<Token> &mylist, list<Token>::iterator st, list<Token>::iterator ed, list<Token> replacement)
{
    mylist.erase(st, ed);

    if (ed == mylist.end())
    {
        for (Token tok : replacement)
            mylist.push_back(tok);
    }
    else
    {
        for (Token tok : replacement)
            mylist.insert(ed, tok);
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// As above but replace with a single token
// ----------------------------------------------------------------------

void replace(list<Token>& mylist, list<Token>::iterator st, list<Token>::iterator ed, Token replacement)
{
    mylist.erase(st, ed);

    if (ed == mylist.end())
    {
        mylist.push_back(replacement);
    }
    else
    {
        mylist.insert(ed, replacement);
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// Starting at st, read past white space and comments get next token.
// Return a Token struct. Return ed, a pointer to where we got to
// Return token with type=-1 if nothing left (end of line)
// Return skipped=true if we had to skip whitespace to get to the returned token
// (this is how we can distinguish between object and function-like macros)
// set incomment=true (and return -1) if we couldn't find end of comment (so in multi-line comment block)
// ----------------------------------------------------------------------

bool incomment = false;

Token getTok(char* st, char** ed, bool *skipped)
{
    int i, j;
    Token tok;
    *skipped = false;

    // ----------------------------------------------------------------------
    // skip past any number of whitespace blocks and /*comments*/
    // ----------------------------------------------------------------------

    while (true)
    {
        bool done = false;

        if (*st == '/' && *(st + 1) == '*')
        {
            incomment = true;
            st += 2;
        }

        while (incomment)
        {
            if (*st == '\0')
            {
                tok.type = -1;
                return tok;
            }
            if (*st == '*' && *(st + 1) == '/')
            {
                incomment = false;
                st += 2;
                break;
            }
            st++;
            done = true;
        }

        while (isspace(*st))
        {
            *skipped = true;
            st++;
            done = true;
        }
        if (!done) break;
    }

    // ----------------------------------------------------------------------
    // Are we now at the end of the string, or at inline comment start? Give up!
    // ----------------------------------------------------------------------

    if (*st == '\0') // end of the string, give up
    {
        tok.type = -1;
        return tok;
    }

    if (*st == '/' && *(st + 1) == '/')   // inline comment (like this one). Give up
    {
        tok.type = -1;
        return tok;
    }

    int numToks = sizeof(tokNames) / sizeof(char*);

    // ----------------------------------------------------------------------
    // Consider all predefined tokens in tokNames
    // The first group (up to DEFINE) are all punctuation and can be idenfied easily
    // remaining tokens like #if must be whole words with space or punctuation after them
    // so #iffy would not be identified as #if + fy
    // ----------------------------------------------------------------------

    for (j = 0; j < numToks; j++) 
    {
        size_t lentok = strlen(tokNames[j]);
        int puncTok = j < DEFINE;          // all previous are punctuation
        bool good = true;
        for (i = 0; i < lentok; i++) 
        {
            if (*(st + i) != tokNames[j][i]) 
            {
                good = false;
                break;
            }
        }
        if (good && (puncTok || !(isalnum(*(st + lentok)) || *(st + lentok) == '_')))  // ie followed by space, punctuation etc
        {
            *ed = st + lentok;
            tok.type = j;
            return tok;
        }
    }

    // ----------------------------------------------------------------------
    // Do the single hash separately else it prevents us identifying #define etc
    // ----------------------------------------------------------------------

    if (*st == '#')
    {
        *ed = st + 1;
        tok.type = HASH;
        return tok;
    }

    // ----------------------------------------------------------------------
    // It's not a predefined token so it must be one of the following
    // (w)string literal
    // (w)char literal
    // identifier (eg int, foo, for, return etc: NB we don't care about C keywords!)
    // number (int, float, double: we don't distinguish)
    // ----------------------------------------------------------------------

    if (*st == 'L' && *(st + 1) == '"')   // wstring literal
    {
        st += 2;
        char* p = st;
        while (*p != '\0')
        {
            if (*p == '"' && !(*(p - 1) == '\\' && *(p - 2) != '\\'))
            {
                size_t sz = p - st;
                tok.id = string(st, sz);
                tok.type = WSTRING;
                *ed = p + 1;
                return tok;
            }
            p++;
        }
        wrtError("Unexpected EOF while scanning string const");
    }
    else if (*st == 'L' && *(st + 1) == '\'')  // wchar literal
    {
        st += 2;
        char* p = st;
        while (*p != '\0')
        {
            if (*p == '\'' && !(*(p - 1) == '\\' && *(p - 2) != '\\'))
            {
                size_t sz = p - st;
                tok.id = string(st, sz);
                tok.type = WCHAR;
                *ed = p + 1;
                return tok;
            }
            p++;
        }
        wrtError("Unexpected EOF while scanning char const");
    }
    else if (*st == '"')   // string literal
    {
        st++;
        char* p = st;
        while (*p != '\0')
        {
            if (*p == '"' && !(*(p - 1) == '\\' && *(p - 2) != '\\'))
            {
                size_t sz = p - st;
                tok.id = string(st, sz);
                tok.type = STRING;
                *ed = p + 1;
                return tok;
            }
            p++;
        }
        wrtError("Unexpected EOF while scanning string const");
    }
    else if (*st == '\'')  // char literal
    {
        st++;
        char* p = st;
        while (*p != '\0') 
        {
            if (*p == '\'' && !(*(p - 1) == '\\' && *(p - 2) != '\\'))
            {
                size_t sz = p - st;
                tok.id = string(st, sz);
                tok.type = CHAR;
                *ed = p + 1;
                return tok;
            }
            p++;
        }
        wrtError("Unexpected EOF while scanning char const");
    }
    else if (isalpha(*st) || *st == '_' || *st == '$')    // identifier, var name etc (some compilers allow $ in var names)
    {
        // ----------------------------------------------------------------------
        // Check for identifier:
        // bar, _foo123, $myvar (extension), if, for, int etc
        // white space, punctuation or end of string will terminate eg _foo123+
        // ----------------------------------------------------------------------

        char* p = st;

        while (isalnum(*p) || *p == '_' || *p == '$')  // ie not (punctuation, space, \0)
        {
            p++;
        }
        *ed = p;
        tok.type = IDENTIFIER;
        size_t sz = *ed - st;
        tok.id = string(st, sz);
        return tok;
    }
    else if (isdigit(*st))  // number
    {
        // ----------------------------------------------------------------------
        // Check for number including int, float or double
        // white space, punctuation (but not dot or (+/- if following an e or E)) or end of string will terminate eg 123; 123+ but not 123e+
        // The number (float or int) is stored as a string and will later be converted to an int via strtoll if used in an #if or #elif
        // (we don't actually care about floats in the preprocessor)
        // The following numbers would be accepted:
        // 789, 123e+7, 0xFUL, 0xBAADF00D, 123.456e-78, 0233, 0b1111'0000
        // And also these incorrect numbers would be accepted (we don't care)
        // 7.8.9, 123e+7e-5, 123typewriter, ff.abe-3, 1UUU, 2eee, 3'''''
        // ----------------------------------------------------------------------

        char* p = st;

        char prev=' ';
        while (isalnum(*p) || *p == '.' || *p=='\'' || (prev=='E' && (*p == '+' || *p == '-')))  // ie not (punctuation, space, \0) but dot +- OK
        {
            prev = toupper(*p);
            p++;
        }
        *ed = p;
        tok.type = NUMBER;
        size_t sz = *ed - st;
        tok.id = string(st, sz);
        return tok;
    }
    else
    {
        throw string("Error could not parse token, ") + st + " (line " + to_string(g_lineno) + ", file " + g_fname + ")";
    }
	
	return tok;
}

// ######################################################################

// ----------------------------------------------------------------------
// Given a char array "buff", corresponding to a source line, get a list of Tokens
// ----------------------------------------------------------------------

list<Token> getTokLine(char* buff)
{
    char* st = buff;
    char* ed;

    // ----------------------------------------------------------------------
    // If there is a space between # and directive, move it forward, eg
    // #  define, becomes:
    //   #define
    // (but only if # is first non-blank char in line)
    // ----------------------------------------------------------------------

    for (st = buff; *st != '\0'; st++)
    {
        if (*st == '#')
        {
            char* p = st;
            p++;
            bool foundSpace = false;
            while (isspace(*p))
            {
                foundSpace = true;
                p++;
            }
            if (foundSpace)
            {
                *st = ' ';
                *(p - 1) = '#';
            }
            break;
        }
        else if (!isspace(*st)) 
            break;
    }

    st = buff;
    list<Token> line;

    // ----------------------------------------------------------------------
    // Get tokens from this line until none are left. Distinguish between
    // #define foo() bar   function-like: DEFINE_FUNC
    // #define foo () bar  object-like with value "() bar": DEFINE
    // ----------------------------------------------------------------------

    bool skipped;
    while (true) 
    {
        Token tok = getTok(st, &ed, &skipped);
        if (tok.type == -1) break;
        line.push_back(tok);
        st = ed;
        if (line.size() == 3 && line.begin()->type == DEFINE && tok.type == OPEN_BRACKET && !skipped)
            line.begin()->type = DEFINE_FUNC;
    }

    return line;
}

// ######################################################################

// ----------------------------------------------------------------------
// Write out a token depending on its type. Write a space after the token
// ----------------------------------------------------------------------

void writeTok(Token tok)
{
    if (tok.type == IDENTIFIER || tok.type==NUMBER)
    {
        fprintf(g_outfile, "%s", tok.id.c_str());
    }
    else if (tok.type == STRING)
    {
        fprintf(g_outfile, "\"%s\"", tok.id.c_str());
    }
    else if (tok.type == CHAR)
    {
        fprintf(g_outfile, "'%s'", tok.id.c_str());
    }
    else if (tok.type == WSTRING)
    {
        fprintf(g_outfile, "L\"%s\"", tok.id.c_str());
    }
    else if (tok.type == WCHAR)
    {
        fprintf(g_outfile, "L'%s'", tok.id.c_str());
    }
    else if (tok.type == HASH)
    {
        fprintf(g_outfile, "#");
    }
    else if (tok.type == DEFINE_FUNC)
    {
        fprintf(g_outfile, "#define");
    }
    else
    {
        fprintf(g_outfile, "%s", tokNames[tok.type]);
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// As writeTok but create a string from token
// ----------------------------------------------------------------------

string tok2Str(Token tok)
{
    if (tok.type == IDENTIFIER || tok.type == NUMBER)
    {
        return tok.id;
    }
    else if (tok.type == STRING)
    {
        return "\"" + tok.id + "\"";
    }
    else if (tok.type == CHAR)
    {
        return "'" + tok.id + "'";
    }
    else if (tok.type == WSTRING)
    {
        return "L\"" + tok.id + "\"";
    }
    else if (tok.type == WCHAR)
    {
        return "L'" + tok.id + "'";
    }
    else if (tok.type == HASH)
    {
        return "#";
    }
    else if (tok.type == DEFINE_FUNC)
    {
        return "#define";
    }
    else
    {
        return tokNames[tok.type];
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// turn tok into string, including when token is an arg with actuals given as tokens of type STRING
// eg with args "x","y"
// and stringActuals "1","2", 
// x is converted to "1"
// If the token is not an arg, use tok2Str to convert
// ----------------------------------------------------------------------

string tok2StrArgs(Token tok, list<Token> stringActuals, list<string> args)
{
    bool found = false;
    string str;
    if (tok.type == IDENTIFIER)
    {
        auto itStringActuals = stringActuals.begin();
        for (string arg : args)
        {
            if (tok.id == arg)
            {
                str = itStringActuals->id;
                found = true;
                break;
            }
            itStringActuals++;
        }
    }
    if (!found)
    {
        str = tok2Str(tok);
    }
    return str;
}

// ######################################################################

// ----------------------------------------------------------------------
// Either write a line of tokens out to g_outfile or concat them to string g_outstring
// ----------------------------------------------------------------------

void writeLine(list<Token> line)
{

    string str = "";

    if (line.empty())
    { 
    }
    else if (line.front().type == DEFINE_FUNC)   // #define foo( x , y ) x + y
    {
        auto it = line.begin();

        str += "#define ";
        it++;

        str += tok2Str(*it); // func name
        it++;

        str += tok2Str(*it); // open bracket
        it++;

        for (; it != line.end(); it++)
        {
            str += (" " + tok2Str(*it));
        }
    }
    else
    {
        Token prev;
        prev.type = -1;
        for (Token tok : line)
        {
            if (!(tok.type == 30 && tok2Str(tok).c_str()[0] == '.')) {
                if (prev.type != -1 && (!(prev.type == 30 && tok2Str(prev).c_str()[0] == '.')))
                    str += " ";
            }

            str += tok2Str(tok);
            prev = tok;
        }
    }
    str += '\n';

    if (g_outfile == NULL)
    {
        g_outstring += str;
    }
    else
    {
        fprintf(g_outfile, "%s", str.c_str());
    }

    /*
    if (g_outfile == NULL)
    {
        Token prev;
        prev.type = -1;
        for (Token tok : line)
        {
            if (prev.type != -1)
                g_outstring += " ";

            g_outstring += tok2Str(tok);
            prev = tok;
        }
        g_outstring += '\n';
    }
    else
    {
        Token prev;
        prev.type = -1;
        for (Token tok : line)
        {
            if (prev.type != -1)
                fprintf(g_outfile, " ");

            if (tok.type == DEFINE_FUNC)
                fprintf(g_outfile, "#define");
            else
                writeTok(tok);
            prev = tok;
        }
        fprintf(g_outfile, "\n");
    }
    */
}

// ######################################################################

// ----------------------------------------------------------------------
// Expand "line" (a list of tokens) in situ based on "vars", the list of macros (function and object-like) currently defined
// Eg a line might be:
// int fib(int n) { qprint("%d%d%d",1,2,3); return min(n,1); }
// and our vars list might contain:
// fib := foo*foo
// foo := [(1 + 2 - 3)]
// min(x,y) := (x)<(y) ? (x):(y)
// qprint(format, __VA_ARGS__) := printf(format, __VA_ARGS__)
// used is a set of macros which have already been expanded so can't be expanded again
// (to avoid runaway recursion when this function calls itself)
// return true if expansion succeeded or false if mismatched brackets when expanding a function-like macro
// ----------------------------------------------------------------------

bool expand(list<Token>& line, map<string,Var>& vars, set<string> used)
{
    set<string> used2;
    auto it_tok = line.begin();
    while (it_tok != line.end())
    {
        used2 = used;   // we will append on to the used list which was given to us

        // Get a tok out of the line (eg fib)
        Token tok = *it_tok;

        bool found = false;

        // Is this tok an identifier? If so we might be able to replace it
        if (tok.type == IDENTIFIER && used.count(tok.id)==0)
        {
            // ----------------------------------------------------------------------
            // Search the vars list to find a macro (object or function) with the same name as the token
            // ----------------------------------------------------------------------

            list<Token> lineReplace;
            Var varToReplace;

            auto pvar = vars.find(tok.id);
            if (pvar != vars.end())
            {
                varToReplace = pvar->second;
                lineReplace = varToReplace.val;
                found = true;
            }

            if (found)
            {
                auto st = it_tok;
                auto ed = it_tok;
                ed++;               // for object-like ed is just st+1. For function-like, ed could be higher

                // ----------------------------------------------------------------------
                // Is the macro a function-like macro? Is it being called with an arg list in brackets eg fib(1,2,3)?
                // If yes, then lineReplace should have arguments replaced with actual values before substitution
                // ----------------------------------------------------------------------

                bool functionLikeExpanded = false;
                if (varToReplace.functionLike && ed != line.end() && ed->type == OPEN_BRACKET)
                {
                    // ----------------------------------------------------------------------
                    // Prepare a list of actual arguments being used
                    // min(abc,1*(2+3))  actuals are abc and 1*(2+3)
                    // min(a)            actuals has one member: a
                    // min()             actuals is empty list
                    // ----------------------------------------------------------------------

                    list<list<Token>> actuals;
                    list<Token> actual;
                    ed++;
                    if (ed == line.end()) return false;  // mismatched

                    if (ed->type == CLOSE_BRACKET)  // special case, no actual args, actuals is empty
                    {
                        ed++;
                    }
                    else  // at least one actual arg
                    {
                        int level = 0;
                        bool vararg = (varToReplace.args.back() == "__VA_ARGS__");
                        size_t nargs = varToReplace.args.size();
                        int iarg = 0;

                        while (!(ed->type == CLOSE_BRACKET && level == 0))
                        {
                            if (ed->type == OPEN_BRACKET) level++;
                            if (ed->type == CLOSE_BRACKET) level--;

                            if (ed->type == COMMA && level == 0 && !(vararg && iarg == nargs - 1))
                            {
                                actuals.push_back(actual);
                                actual.clear();
                                iarg++;
                            }
                            else
                            {
                                actual.push_back(*ed);
                            }
                            ed++;
                            if (ed == line.end()) return false;
                        }
                        actuals.push_back(actual);
                        ed++;
                    }

                    if (actuals.size() != varToReplace.args.size())
                    {
                        wrtError("Error: wrong number of arguments for function-like macro invocation");
                    }

                    // ----------------------------------------------------------------------
                    // create a parallel list of expanded actual args. Call this function recursively to expand them
                    // eg min(min(1,2),3), first arg is min(1,2) which we will expand to (1)<(2) ? (1):(2)
                    // We will use expanded arguments except for stringization # or concatenation ##
                    // ----------------------------------------------------------------------

                    list<list<Token>> expandedActuals;
                    for (auto actual : actuals)
                    {
                        expandedActuals.push_back(actual);
                    }

                    for (auto& expandedActual : expandedActuals)
                        expand(expandedActual, vars, used2);

                    list<Token> stringActuals;
                    for (auto actual : actuals)
                    {
                        string str;
                        Token prev; prev.type = -1;
                        for (Token tok : actual)
                        {
                            if (prev.type != -1)
                                str += " ";
                            str += tok2Str(tok);
                            prev = tok;
                        }
                        Token tok;
                        tok.type = STRING;
                        tok.id = str;
                        stringActuals.push_back(tok);
                    }

                    // ----------------------------------------------------------------------
                    // go over lineReplace, eg (x)<(y) ? (x):(y) and replace args (eg x,y) with actual values
                    // ----------------------------------------------------------------------

                    auto it = lineReplace.begin();
                    while (it != lineReplace.end())
                    {
                        auto it1 = next(it);
                        if (it1 != lineReplace.end() && it1->type == HASH2)
                        {
                            // min(x,y) x##y##z  1##2 X_ ## x
                            // min(1,2)

                            string str = tok2StrArgs(*it, stringActuals, varToReplace.args);
                            do
                            {
                                it1++;
                                if (it1 == lineReplace.end()) wrtError("## cannot be at end of line");
                                str += tok2StrArgs(*it1, stringActuals, varToReplace.args);
                                it1++;
                            } while (it1 != lineReplace.end() && it1->type == HASH2);

                            char* ed;
                            bool skipped;
                            Token tok = getTok(const_cast<char*>(str.c_str()), &ed, &skipped);  // TODO: create multiple tokens for concated string?
                            replace(lineReplace, it, it1, tok);
                        }
                        else if (it->type == IDENTIFIER)   // eg it points to the first x
                        {
                            auto itExpandedActuals = expandedActuals.begin();
                            for (auto arg : varToReplace.args)    // is x one of the args of function min?
                            {
                                if (it->id == arg)
                                {
                                    replace(lineReplace, it, it1, *itExpandedActuals);
                                    break;
                                }
                                itExpandedActuals++;
                            }
                        }
                        else if (it->type == HASH)
                        {
                            if (it1->type != IDENTIFIER) wrtError("# must be followed by variable");

                            string name = it1->id;
                            it1++;
                            auto itStringActuals = stringActuals.begin();
                            bool found = false;
                            for (auto arg : varToReplace.args)    // is x one of the args of function min?
                            {
                                if (name == arg)
                                {
                                    replace(lineReplace, it, it1, *itStringActuals);
                                    found = true;
                                    break;
                                }
                                itStringActuals++;
                            }
                            if (!found) wrtError("# must be followed by argument");
                        }
                        it = it1;
                    }
                    functionLikeExpanded = true;  // We successfully expanded the function value, substituting args
                }

                // ----------------------------------------------------------------------
                // In line, replace token(s) with lineReplace, eg replace fib with foo*foo or min(2,3) with (2)<(3)?(2):(3)
                // But don't replace if functionLike macro being referenced without an argument list specified
                // ----------------------------------------------------------------------

                if (!(varToReplace.functionLike && !functionLikeExpanded))
                {
                    used2.insert(varToReplace.name);    // we're about to use this var so don't use it again when calling recursively
                    expand(lineReplace, vars, used2);   // recursive: get lineReplace fully expanded
                    replace(line, st, ed, lineReplace); // macro replaced with its value
                }
                it_tok = ed;  // continue from just above where we inserted stuff

                if (ed!=line.end() && ed->type == OPEN_BRACKET)  // ... there's one exception: If we paste in a function-like macro name
                {                                                // and it's followed by an open bracket, then, expand as macro call
                    auto prev = ed;
                    prev--;
                    if (prev->type == IDENTIFIER)
                    {
                        auto p=vars.find(prev->id);
                        if (p != vars.end() && p->second.functionLike) it_tok = prev;
                    }
                }
            }
        }
        if (!found) it_tok++;
    }
    return true;
}

// ######################################################################

// ----------------------------------------------------------------------
// given #include "include" in "file", find a file with that name rel to "file" and then include paths
// For non-quoted, just consider include paths. Throw if include not found
// ----------------------------------------------------------------------

string findIncludeFile(string file, string include, const list<string>& includePaths, bool quoted)
{
    if (quoted)
    {
        // file=path/to/file.c
        // include=bar.h
        string base = "";
        for (int i = file.length(); i >= 0; i--)
        {
            if (file[i] == '/')
            {
                base = file.substr(0, i + 1);
                break;
            }
        }

        string relPath = base + include;   // path/to/bar.h
        FILE* fp = fopen(relPath.c_str(), "r");
        if (fp != NULL)
        {
            fclose(fp);
            return relPath;
        }
    }

    for (string path : includePaths)
    {
        string fname = path + include;
        FILE* fp = fopen(fname.c_str(), "r");
        if (fp != NULL)
        {
            fclose(fp);
            return fname;
        }
    }

    throw "Could not open include file " + include;
}

// ######################################################################

// ----------------------------------------------------------------------
// evaluate "defined", "__has_cpp_attribute" and "__has_include" to numbers 
// (0, 1 or version) in an if expression
// ----------------------------------------------------------------------

void expandIfFuncs(list<Token>& line, map<string, Var>& vars, const list<string>& includePaths)
{
    Token tok;
    tok.type = NUMBER;

    for (auto it = line.begin(); it != line.end();)
    {
        if (it->type == IDENTIFIER && it->id == "defined")
        {
            auto ed = it;
            ed++;

            if (ed == line.end())
            {
                wrtError("unexpected end of line when processing defined");
            }

            string name;
            if (ed->type == IDENTIFIER || ed->type==NOEXPAND)
            {
                name = ed->id;
                ed++;
            }
            else if (ed->type == OPEN_BRACKET)
            {
                ed++;
                if (ed->type != IDENTIFIER && ed->type!=NOEXPAND) wrtError("defined bad, expecting identifier");
                name = ed->id;
                ed++;
                if (ed->type != CLOSE_BRACKET) wrtError("expected ) on defined");
                ed++;
            }
            else
                wrtError("defined must specify a macro");

            tok.id = "0";

            // __has_cpp_attribute and __has_include flagged as defined if set
            if ((name == "__has_cpp_attribute" && !g_attributeMap.empty()) || (name == "__has_include" && g_hasIncludeSupported) || vars.count(name) > 0)
            {
                tok.id = "1";
            }

            replace(line, it, ed, tok);
            it = ed;
        }
        else if (it->type == IDENTIFIER && it->id == "__has_include" && g_hasIncludeSupported)
        {
            auto ed = it;
            ed++;

            if (ed == line.end() || ed->type != OPEN_BRACKET)
            {
                wrtError("Malformed __has_include");
            }
            ed++;

            if (ed->type != LESSTHAN)
            {
                throw string("Weird __has_include, expected <");
            }
            ed++;

            string include;
            while (ed->type != GREATERTHAN)
            {
                include += tok2Str(*ed);
                ed++;
            }
            ed++;

            if (ed->type != CLOSE_BRACKET)
                throw string("Weird __has_include, expected )");
            ed++;

            tok.id = "1";
            try
            {
                findIncludeFile("", include, includePaths, false);  // throws if include file not found
            }
            catch (string)
            {
                tok.id = "0";   // include file not found
            }

            replace(line, it, ed, tok);
            it = ed;
        }
        else if (it->type == IDENTIFIER && it->id == "__has_cpp_attribute" && !g_attributeMap.empty())
        {
            auto ed = it;
            ed++;

            if (ed == line.end() || ed->type != OPEN_BRACKET)
            {
                wrtError("Malformed __has_cpp_attribute");
            }
            ed++;

            if (ed == line.end() || ed->type != IDENTIFIER)
            {
                wrtError("Malformed __has_cpp_attribute, expected identifier");
            }
            string id = ed->id;
            ed++;

            if (ed == line.end() || ed->type != CLOSE_BRACKET)
            {
                wrtError("Malformed __has_cpp_attribute, expected )");
            }
            ed++;

            tok.id = "0";
            if (g_attributeMap.find(id) != g_attributeMap.end())
                tok.id = g_attributeMap[id];

            replace(line, it, ed, tok);
            it = ed;
        }
        else
            it++;
    }

}

// ----------------------------------------------------------------------
// Expand #if or #elif statement
// 1) evaluate "defined", "__has_cpp_attribute" and "__has_include" to numbers
// 2) Use expand (above) to evaluate macros
// 3) Repeat 1 in case any defined(x) inserted through expansion
// 4) Any remaining, undefined, identifiers are set to 0
// Once this function has run we can use expression::evaluate to see if condition is true
// ----------------------------------------------------------------------

void expandIf(list<Token>& line, map<string,Var>& vars, const list<string>& includePaths)
{
    expandIfFuncs(line, vars, includePaths);
    expand(line, vars, set<string>());
    expandIfFuncs(line, vars, includePaths);

    for (Token& tok : line)
    {
        if (tok.type == IDENTIFIER)   // not a macro, replace with 0
        {
            tok.type = NUMBER;
            tok.id = "0";
        }
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// Discover if a line of code is mismatched, in the sense that a function-like
// macro is being called but brackets are mismatched eg
// add+add(
// would be mismatched if add is a function-like macro (the first add is OK)
// ----------------------------------------------------------------------

bool mismatched(list<Token> &line, map<string,Var> &vars)
{
    if (line.empty()) return false;

    auto it = line.begin();
    while (it != line.end())
    {
        auto it2 = it;
        it2++;

        if (it2 == line.end()) break;

        if (it2->type == OPEN_BRACKET && it->type == IDENTIFIER && vars.count(it->id) > 0 && vars[it->id].functionLike)
        {
            it2++;
            if (it2 == line.end()) return true;

            int level = 0;
            while (!(it2->type == CLOSE_BRACKET && level==0))
            {
                if (it2->type == OPEN_BRACKET) level++;
                if (it2->type == CLOSE_BRACKET) level--;
                it2++;
                if (it2 == line.end()) return true;
            }
            it2++;
        }
        it = it2;
    }
    return false;
}

// ######################################################################

// ----------------------------------------------------------------------
// set __FILE__ and __LINE__ macros (and g_fname and g_lineno)
// ----------------------------------------------------------------------

void setLineFile(int lineno, string fname, map<string, Var>& vars)
{
    vars["__FILE__"].val.front().id = fname;
    vars["__LINE__"].val.front().id = to_string(lineno);

    g_lineno = lineno;
    g_fname = fname;
}

// ######################################################################

// ----------------------------------------------------------------------
// If a macro value contains the "defined" function, set its argument
// to be type NOEXPAND. Eg 
// #define foo defined(bar) && version>0
// #if foo...
// When we expand the if, the "bar" argument of defined should be left alone
// expandIfFuncs will then set the defined to 1 or 0
// ----------------------------------------------------------------------

void preventDefinedExpand(list<Token> &val)
{
    for (auto it = val.begin(); it != val.end(); it++)
    {
        if (it->type == IDENTIFIER && it->id == "defined")
        {
            auto it2 = it;
            it2++;
            if (it2 == val.end()) return;
            if (it2->type == IDENTIFIER)
            {
                it2->type = NOEXPAND;
            }
            it2++;
            if (it2 == val.end()) return;
            if (it2->type == IDENTIFIER)
            {
                it2->type = NOEXPAND;
            }
        }
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// Process a line of tokens and write the (expanded) line to g_outfile (or add to g_outstring if g_outfile="")
// Note that a "line" might consist of an entire #if..#endif block (which is made of lines, hence recursive)
// it_line points to the line to be written and we will increase this, normally by 1, but perhaps more in the case of if block
// vars is the macro list which we might alter (define, undef). Wrt tells us whether to actually write the line
// (it will be false if we are in an inactive branch of an if)
// This function also indirectly calls itself via processFile (in the case of include)
// ----------------------------------------------------------------------

void processLine(list<list<Token>>::iterator &it_line, map<string,Var> &vars, const list<string> &includePaths, list<string> &included, string fname, int &lineno, bool wrt)
{
    list<Token> line = *it_line;
    setLineFile(lineno, fname, vars);  // set __FILE__ and __LINE__

    // ----------------------------------------------------------------------
    // print a blank for empty lines
    // ----------------------------------------------------------------------

    if (line.size() == 0)
    {
        it_line++;
        lineno++;
        return;
    }

    // ----------------------------------------------------------------------
    // Sanity check: #define etc must be the first token of the line
    // ----------------------------------------------------------------------

    int ind = 0;
    for (Token tok : line)
    {
        if (ind > 0 && tok.type >= DEFINE && tok.type <= PRAGMA)
        {
            wrtError("Preprocessor directive must be first token in line");
        }
        ind++;
    }

    int type = line.front().type;
    if (type == ENDIF || type == ELSE || type == ELIF)
    {
        wrtError("Malformed if block");
    }

    // ----------------------------------------------------------------------
    // ERROR and WARNING
    // ----------------------------------------------------------------------

    if (type == ERROR)
    {
        if (wrt)
        {
            string err;
            for (Token tok : line)
            {
                err += tok2Str(tok);
            }
            wrtError("Preprocessor terminated with #error: " + err);
        }
        it_line++;
        lineno++;
    }

    else if (type == WARNING)
    {
        if (wrt)
        {
            string warn;
            for (Token tok : line)
            {
                warn += tok2Str(tok);
            }
            printf("Preprocessor WARNING: %s\n", warn.c_str());
        }
        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // PRAGMA. We support pragma once, push_macro and pop_macro
    // All others are written out for the compiler to deal with
    // see https://docs.microsoft.com/en-us/cpp/preprocessor/pragma-directives-and-the-pragma-keyword?view=vs-2019
    // ----------------------------------------------------------------------

    else if (type == PRAGMA)
    {
        if (wrt)
        {
            auto it = line.begin();
            it++;
            if (it->type == IDENTIFIER && it->id == "once")
            {
                g_blacklist.push_back(fname);
                if (g_mode=="full" || g_mode=="flatten") writeLine(line);
            }
            else if (it->type == IDENTIFIER && it->id == "push_macro")
            {
                it++;
                if (it->type != OPEN_BRACKET) wrtError("Expected (");

                it++;
                if (it->type != STRING) wrtError("Expected string");
                string name = it->id;

                it++;
                if (it->type != CLOSE_BRACKET) wrtError("Expected )");

                if (vars.count(name) > 0)
                    g_stacks[name].push(vars[name]);

                if (g_mode == "flatten") writeLine(line);
            }
            else if (it->type == IDENTIFIER && it->id == "pop_macro")
            {
                it++;
                if (it->type != OPEN_BRACKET) wrtError("Expected (");

                it++;
                if (it->type != STRING) wrtError("Expected string");
                string name = it->id;

                it++;
                if (it->type != CLOSE_BRACKET) wrtError("Expected )");

                auto pstack = g_stacks.find(name);

                if (pstack == g_stacks.end())
                {
                    auto pvar = vars.find(name);
                    if (pvar != vars.end()) vars.erase(pvar);
                }
                else
                {
                    vars[name] = g_stacks[name].top();
                    g_stacks[name].pop();
                }
                if (g_mode == "flatten") writeLine(line);
            }
            else
            {
                if (g_mode=="full") expand(line, vars, set<string>());
                if (g_mode == "full" || g_mode == "flatten") writeLine(line);
            }
        }
        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // LINE
    // ----------------------------------------------------------------------

    else if (type == LINE)
    {
        auto it = line.begin();
        it++;
        if (it->type == NUMBER)
        {
            char* endptr = NULL;
            errno = 0;

            lineno = strtol(it->id.c_str(), &endptr, 0);
            setLineFile(lineno, fname, vars);

            if (!(errno == 0 && *endptr == '\0'))
            {
                wrtError("Error: failed to make sense of number. Integers must be used in #line statements: " + it->id);
            }
        }
        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // For include, work out the file name and call processFile (which calls this function: recursive)
    // For quote-include, filename is just a string token, for <include> we must stringify several tokens
    // ----------------------------------------------------------------------

    else if (type == INCLUDE)
    {
        if (wrt)   // false if in an inactive block
        {
            auto it = line.begin();
            it++;

            if (it->type != LESSTHAN && it->type != STRING)
            {
                expand(line, vars, set<string>());  // computed include
                it = line.begin();
                it++;
            }

            string includeFile;
            if (it->type == STRING)   // #include "foo.h"
            {
                includeFile=findIncludeFile(fname, it->id, includePaths, true);
            }
            else                                // #include <foo.h>
            {
                if (it->type != LESSTHAN)
                {
                    wrtError("Weird INCLUDE, expected <");
                }
                it++;

                string include;
                while (it != line.end() && it->type != GREATERTHAN)
                {
                    Token tok = *it;
                    if (tok.type == STRING || tok.type == CHAR)
                    {
                        wrtError("Weird Include: string or char literal found in <filename>");
                    }

                    if (tok.type == IDENTIFIER || tok.type == NUMBER)
                    {
                        include += tok.id;
                    }
                    else
                    {
                        include += tokNames[tok.type];
                    }
                    it++;
                }
                includeFile = findIncludeFile(fname, include, includePaths, false);
            }

            if (find(g_blacklist.begin(), g_blacklist.end(), includeFile) == g_blacklist.end())
            {
                processFile(includeFile, vars, includePaths, included);
                if (g_debug) fprintf(g_outfile, "# %d \"%s\" 2\n", g_lineno, fname.c_str());
            }

            if (find(included.begin(), included.end(), includeFile) == included.end()) included.push_back(includeFile);
        }
        it_line++;  // advance even if we didn't write the line
        lineno++;
    }

    // ----------------------------------------------------------------------
    // Define object-like macro. Add it to the vars list
    // var.args will be empty
    // TODO: check macro either (1) doesn't exist or (2) already has this definition
    // ----------------------------------------------------------------------

    else if (type == DEFINE)
    {
        if (wrt)
        {
            auto it = line.begin();
            it++;

            Var var;
            var.name = it->id;
            var.functionLike = false;
            it++;

            if (it != line.end())
            {
                var.val = list<Token>(it, line.end());
            }

            preventDefinedExpand(var.val);

            if (vars.count(var.name) > 0)
            {
                printf("Warning: redefining macro %s\n", var.name.c_str());
            }

            vars[var.name] = var;

            if (g_mode == "flatten") writeLine(line);
        }

        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // Define function-like macro. Add it to the vars list
    // var.args will contain the list of arguments eg min(x,y)
    // ----------------------------------------------------------------------

    else if (type == DEFINE_FUNC)
    {
        if (wrt)
        {
            auto it = line.begin();
            it++;

            Var var;
            var.name = it->id;
            var.functionLike = true;
            it++;

            if (it == line.end() || it->type != OPEN_BRACKET) wrtError("Expected (");

            it++;
            while (it->type != CLOSE_BRACKET)
            {
                if (it->type == DOT3)
                    var.args.push_back("__VA_ARGS__");
                else if (it->type == IDENTIFIER)
                    var.args.push_back(it->id);
                else if (it->type != COMMA)
                    wrtError("Error in arg list");
                it++;
            }
            it++;

            int ind = 0;
            for (string arg : var.args)
            {
                if (ind < var.args.size() - 1 && arg == "__VA_ARGS__") wrtError("... must be last argument");
                ind++;
            }

            var.val = list<Token>(it, line.end());
            preventDefinedExpand(var.val);

            if (vars.count(var.name)>0)
                printf("Warning: redefining macro %s\n", var.name.c_str());

            vars[var.name] = var;

            if (g_mode == "flatten")
            {
                writeLine(line);
            }

        }

        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // Undefine a macro
    // ----------------------------------------------------------------------

    else if (type == UNDEF)
    {
        if (wrt)
        {
            auto itname = line.begin();
            itname++;
            string name = itname->id;

            auto pvar = vars.find(name);
            if (pvar == vars.end())
            {
                printf("Warning: #undef used but macro not defined: %s\n", name.c_str());
            }
            else
            {
                vars.erase(pvar);
            }
            if (g_mode == "flatten") writeLine(line);
        }

        it_line++;
        lineno++;
    }

    // ----------------------------------------------------------------------
    // if-like block
    // ----------------------------------------------------------------------

    else if (type == IF || type == IFDEF || type == IFNDEF)
    {
        bool shouldWrt;   // true if the if, ifdef or ifndef is true so we should write unless suppressed by "wrt" arg
        bool done = false; // true if we have already written a branch in if..elif..else..endif structure

        if (type == IF)
        {
            expandIf(line, vars, includePaths);
            if (expression::evaluate(line, fname, lineno))   // if condition is true
            {
                shouldWrt = true;   // so write all lines under it unless suppressed by "wrt"
                done = true;        // and make sure we don't write any elif or else clauses
            }
            else
            {
                shouldWrt = false;
            }
        }
        else if (type == IFDEF || type == IFNDEF)
        {
            auto it = line.begin();
            it++;

            string name = it->id;
            bool found = (name=="__has_include" && g_hasIncludeSupported) || (name=="__has_cpp_attribute" && !g_attributeMap.empty()) || vars.count(name) > 0;

            if (type == IFDEF && found) 
            {
                shouldWrt = true;     // write the block
                done = true;          // suppress further blocks being written
            }            
            else if (type==IFNDEF && !found)
            {
                shouldWrt = true;
                done = true;
            }
            else 
            {
                shouldWrt = false;
            }
        }

        it_line++;
        lineno++;

        while (it_line->size()==0 || it_line->front().type != ENDIF)
        {
            setLineFile(lineno, fname, vars);

            if (it_line->size() == 0)
            {
                it_line++;
                lineno++;
            }
            else if ((*it_line).front().type == ELSE)
            {
                it_line++;
                lineno++;
                if (!done)
                    shouldWrt = true;   // found else and no previous block written so write this
                else
                    shouldWrt = false;  // the if or an elif was written so don't write this
            }
            else if ((*it_line).front().type == ELIF)
            {
                list<Token> line2;
                if (!done)
                {
                    line2 = *it_line;
                    expandIf(line2, vars, includePaths);
                }

                if (!done && expression::evaluate(line2, fname, lineno))   // elif is true and we haven't done a previous elif or the if itself
                {
                    shouldWrt = true;  // so write it!
                    done = true;       // and don't write other elif or else
                }
                else
                    shouldWrt = false;

                it_line++;
                lineno++;
            }
            else
                processLine(it_line, vars, includePaths, included, fname, lineno, wrt && shouldWrt);  // recursive
        }
        it_line++;
        lineno++;
    }
    else  // ordinary line needs to be expanded
    {
        if (wrt)
        {
            if (g_mode == "full")
            {
                while (true)
                {
                    if (expand(line, vars, set<string>())) break;   // expanded successfully
                    while (mismatched(line, vars))
                    {
                        it_line++;
                        lineno++;
                        list<Token> line2 = *it_line;
                        for (Token tok : line2)
                        {
                            line.push_back(tok);
                        }
                    }
                }

                writeLine(line);
            }
            else if (g_mode == "flatten")
            {
                writeLine(line);
            }

            if (fname.rfind(".rc") == fname.size() - 3 && line.size() >= 3)
            {
                auto it = line.begin();
                it++;
                string id = it->id;
                if (it->type == IDENTIFIER && (id == "ICON" || id == "BITMAP" || id == "CURSOR" || id == "FONT" || id == "MESSAGETABLE"))
                {
                    it++;
                    if (it->type == STRING)
                    {
                        try
                        {
                            string file = findIncludeFile(fname, it->id, includePaths, true);
                            included.push_back(file);
                        }
                        catch (string) {}
                    }
                }
            }
        }

        it_line++;
        lineno++;
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// process file fname appending to macro list "vars" as needed
// Open the file, read in all the lines and tokenise them using getTokLine
// Then call processLine to process all such lines
// This function indirectly calls itself via processLine (in the case of include)
// ----------------------------------------------------------------------

void processFile(string fname, map<string,Var> &vars, const list<string> &includePaths, list<string> &included)
{
    // ----------------------------------------------------------------------
    // Open the file, read the whole contents into allocated char array "source" (NULL terminated)
    // For Unix lines will be terminated with \n, for Windows with \r\n
    // ----------------------------------------------------------------------

    char* source = (char *)fname.c_str();

    if (g_debug)
    {
        printf("###%s###\n", source);
        printf("source length=%llu\n", strlen(source));
    }

    // ----------------------------------------------------------------------
    // Process each line into a list of Tokens and add them to "lines"
    // Add in \0 to split the file into lines (account for Windows or Unix line endings)
    // When getTokLine is called:
    // st points to the beginning of the next line
    // ed points to the \0 which terminates this line
    // if it's a blank line, the created line is an empty list of Tokens
    // Also concatenate lines with backslash-newline, but add blank lines so number of lines is the same
    // ----------------------------------------------------------------------

    incomment = false;  // getTok (called by getTokLine will constantly change this but it must finish as false
    char* st = source;
    bool done = false;
    g_lineno = 1;
    list<list<Token> > lines;

    g_fname = fname;
    while (!done)
    {
        char* ed = st;
        int extra = 0;           // extra blank lines to add if continuation to keep line numbers correct
        while (*ed != '\r' && *ed != '\n' && *ed != '\0')
        {
            if (*ed == '\\' && *(ed + 1) == '\n')  // continuation (Unix line ending)
            {
                *ed = ' ';
                *(ed + 1) = ' ';
                extra++;
            }
            else if (*ed == '\\' && *(ed + 1) == '\r' && *(ed + 2) == '\n') // continuation (Windows line ending)
            {
                *ed = ' ';
                *(ed + 1) = ' ';
                *(ed + 2) = ' ';
                extra++;
            }
            ed++;
        }

        int skip;
        if (*ed == '\0') 
            done = true;
        else if (*ed == '\r' && *(ed + 1) == '\n') 
            skip = 2;
        else if (*ed == '\n') 
            skip = 1;

        *ed = '\0';  // insert end of line marker

        lines.push_back(getTokLine(st));
        g_lineno++;

        for (int i = 0; i < extra; i++)
        {
            lines.push_back(list<Token>());
            g_lineno++;
        }

        if (!done) st = ed + skip;
    }

    //free(source);

    if (incomment) wrtError("runaway multiline comment");

    if (g_mode=="lex")
    {
        for (auto line : lines)
        {
            writeLine(line);
        }
        return;
    }

    if (g_debug)
    {
        for (auto it_line = lines.begin(); it_line != lines.end(); it_line++)
        {
            list<Token> line = *it_line;

            for (auto it_tok = line.begin(); it_tok != line.end(); it_tok++)
            {
                Token tok = *it_tok;
                printf("%s", names[tok.type]);

                if (tok.type == IDENTIFIER || tok.type == NUMBER || tok.type == STRING || tok.type == CHAR)
                    printf(": '%s'\n", tok.id.c_str());
                else
                    printf("\n");
            }
            printf("--------------------\n");
        }
    }

    int lineno = 1;
    auto it_line = lines.begin();
    while (it_line != lines.end())
    {
        processLine(it_line, vars, includePaths, included, fname, lineno, true);  // will increase it_line by 1 or more. True means write all lines
    }
}

// ######################################################################

// ----------------------------------------------------------------------
// Preprocess file "infile". Write postprocessed output to file "outfile" or return as string if outname=""
// (write to stdout if outname="stdout")
// Also return "included": the list of all files that were included (recursively) via #includes
//
// * defines is a list of strings each of which is a regular #define statement
// * includePaths is a list of include paths (strings) either absolute path or relative to pwd
// * forceIncludes is a list of strings each of which is a regular #include statement
// * attributeMap is a string-string map for __has_cpp_attributes eg attributeMap["nodiscard"] = "201907L" 
//   (pass an empty map to turn off this feature, then __has_cpp_attribute will be undefined and the function can't be used)
// * hasIncludeSupported: set to true if __has_include should be supported. If false, __has_include is not defined and function can't be used
//
// Note that there are no preset includes or defines (except __LINE__, __FILE__, __DATE__, __TIME__): the preprocessor can be
// made to imitate that of any compiler by setting them appropriately.
//
// eg to achive the same as with this MSVC example:
// cl /E foo.c -DFoo=2 -DBAR -DFUNC(X,Y)=X+Y -Ipath/to/headers -FIbar.h
// you would specify:
// 
// defines[0,1,...,n] = << MSVC specific defines >>
// defines[n+1]="#define Foo 2"
// defines[n+1]="#define BAR 1" (in -D, value defaults to "1" but we must specify this)
// defines[n+2]="#define FUNC(X,Y) X+Y"
// includePaths[0,1,..,m]= << MSVC specific includes >> 
// includePaths[m+1]="path/to/headers" (forward or backslashes allowed)
// forceIncludes[0]="#include \"bar.h\""
//
// There are several modes:
// mode="full": full, normal preprocess.
// mode="flatten": preprocess includes and ifs but don't expand macros
// mode="dependencies": discover which files are included but don't write postprocessed output to file (or return as string)
// mode="lex" (debug) only lex infile
//
// The function throws std::string exceptions if an error occurs
// ----------------------------------------------------------------------

string preprocess(string mode, string infile, string outfile, list<string> defines, list<string> includePaths, list<string> forceIncludes, 
    list<string> &included, map<string, string> attributeMap, bool hasIncludeSupported)
{
    // ----------------------------------------------------------------------
    // Sanity check. Set g_mode etc
    // ----------------------------------------------------------------------

    if (mode != "full" && mode != "flatten" && mode != "dependencies" && mode!="lex")
    {
        throw string("Illegal preprocess mode: must be 'full', 'flatten', 'dependencies'");
    }

    g_mode = mode;
    g_attributeMap = attributeMap;
    g_hasIncludeSupported = hasIncludeSupported;

    // ----------------------------------------------------------------------
    // vars is the list of macros: add __LINE__ etc
    // ----------------------------------------------------------------------

    tm* newTime;
    time_t szClock;

    // Get time in seconds
    time(&szClock);

    // Convert time to struct tm form
    newTime = localtime(&szClock);

    map<string, Var> vars;    // global macros list

    Token tok;
    Var var;
    var.functionLike = false;

    tok.type = STRING;
    tok.id = "";
    var.name = "__FILE__";
    var.val.clear();
    var.val.push_back(tok);
    vars["__FILE__"] = var;

    tok.type = NUMBER;
    tok.id = "0";
    var.name = "__LINE__";
    var.val.clear();
    var.val.push_back(tok);
    vars["__LINE__"] = var;

    tok.type = STRING;
    tok.id = asctime(newTime);
    var.name = "__DATE__";
    var.val.clear();
    var.val.push_back(tok);
    vars["__DATE__"] = var;

    tok.type = STRING;
    tok.id = asctime(newTime);
    var.name = "__TIME__";
    var.val.clear();
    var.val.push_back(tok);
    vars["__TIME__"] = var;

    // ----------------------------------------------------------------------
    // Add user-specified defines provided into vars
    // ----------------------------------------------------------------------

    incomment = false;

    list<list<Token>> lines;
    for (string def : defines)
    {
        char* p = const_cast<char*>(def.c_str());
        list<Token> line = getTokLine(p);
        lines.push_back(line);
    }

    int lineno = 1;
    for (auto it = lines.begin(); it != lines.end();)
    {
        processLine(it, vars, includePaths, included, "", lineno, true);  // will append to vars and advance "it"
    }

    for (string &incl : includePaths)
    {
        for (char& c : incl)
        {
            if (c == '\\') c = '/';
        }

        if (incl.back() != '/')
            incl += '/';
    }

    // ----------------------------------------------------------------------
    // Open output file if needed. Clear g_outstring
    // ----------------------------------------------------------------------

    g_outfile = NULL;
    if (g_mode == "full" || g_mode == "flatten" || g_mode=="lex")
    {
        if (outfile == "stdout")
        {
            g_outfile = stdout;
        }
        else if (outfile == "")
        {
            g_outfile = NULL;
        }
        else
        {
            g_outfile = fopen(outfile.c_str(), "w");
        }
    }

    g_outstring.reserve(1024);
    g_outstring = "";

    // ----------------------------------------------------------------------
    // Execute force includes
    // ----------------------------------------------------------------------

    lines.clear();
    incomment = false;
    for (string fi : forceIncludes)
    {
        char* p = const_cast<char*>(fi.c_str());
        list<Token> line = getTokLine(p);
        lines.push_back(line);
    }

    lineno = 1;
    for (auto it = lines.begin(); it != lines.end();)
    {
        processLine(it, vars, includePaths, included, infile, lineno, true);
    }

    // ----------------------------------------------------------------------
    // Process the specified input file. Close output file if needed
    // output written to g_outstring if that was requested
    // ----------------------------------------------------------------------

    processFile(infile, vars, includePaths, included);

    if (g_mode == "full" || g_mode == "flatten" || g_mode == "lex")
    {
        if (outfile != "stdout" && outfile != "")
        {
            fclose(g_outfile);
        }
    }

    // ----------------------------------------------------------------------
    // DEBUG: Write the final list of macros defined
    // ----------------------------------------------------------------------

    if (g_debug)
    {
        printf("\nMacros defined:\n");
        for (pair<string,Var> varpair : vars)
        {
            Var var = varpair.second;

            printf("%s", var.name.c_str());
            size_t nargs = var.args.size();

            if (var.functionLike)
            {
                printf("(");
                int n = 0;
                for (string s : var.args)
                {
                    n++;
                    printf("%s%s", s.c_str(), n == nargs ? "" : ", ");
                }
                printf(")");
            }

            printf(" := ");

            writeLine(var.val);
        }
    }

    return g_outstring;
}
}

extern "C" {
void glsl_preprocess(char *mode, const char *infile, char *output) {
	std::list<std::string> dummy;
	std::list<std::string> defines;
	defines.push_back("#define GL_ES 1");
	std::map<std::string, std::string> hasCppAttributeMap;
	std::string out = preprocessor::preprocess(mode, infile, "", defines, dummy, dummy, dummy, hasCppAttributeMap, false);
	strcpy(output, out.c_str());
}
}
#endif
