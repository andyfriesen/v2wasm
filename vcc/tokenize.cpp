/*
VERGE 2.5+j (AKA V2k+j) -  A video game creation engine
Copyright (C) 1998-2000  Benjamin Eirich (AKA vecna), et al
Please see authors.txt for a complete list of contributing authors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
        tokenize.cpp
        coded by aen
*/

#include "tokenize.h"

bool zTokenizer::isWhite(char c)
// whitespace characters
{
    return (' ' == c || '\t' == c || '\r' == c || '\n' == c);
}

bool zTokenizer::isAlpha(char c)
// lower & uppercase alpha
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool zTokenizer::isBinaryDigit(char c)
// base-2 digit
{
    return ('0' == c || '1' == c);
}

bool zTokenizer::IsOctalDigit(char c)
// base-8 digit
{
    return (c >= '0' && c <= '7');
}

bool zTokenizer::isDigit(char c)
// base-10 digit
{
    return (c >= '0' && c <= '9');
}

bool zTokenizer::isHexDigit(char c)
// base-16 digit
{
    return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool zTokenizer::isAlphaNumeric(char c)
// lower & uppercase alpha OR base-10 digit
{
    return isAlpha(c) || isDigit(c);
}

bool zTokenizer::isQuote(char c)
// single or double quote
{
    return ('\'' == c || '"' == c);
}

bool zTokenizer::isValidIdentifier(VCString s)
// returns true when:
//		first character is _ or alpha
//		AND remaining characters are _, alpha, or digit
{
    if (!isAlpha(s[0]))
        return false;

    int n;
    n = 0;
    while (n < s.length()) {
        if (isAlphaNumeric(s[n]) || '_' == s[n])
            ++n;
        else
            return false;
    }
    return true;
}

bool zTokenizer::isValidInteger(VCString s)
// returns true for non-floating-point numbers
{
    int n;
    n = 0;
    while (n < s.length()) {
        if (isDigit(s[n]) && '.' != s[n])
            ++n;
        else
            return false;
    }
    return true;
}

bool zTokenizer::isValidNumber(VCString s)
// returns true for either integer or floating-point numbers
{
    int n, dot;
    n = 0, dot = 0;
    while (n < s.length()) {
        if (isDigit(s[n]))
            ++n;
        else if ('.' == s[n])
            ++n, ++dot;
        else
            return false;
    }
    // only true if 1 decimal point
    return (dot < 2);
}

// > EATERS <

bool zTokenizer::atC_Comment()
// helper
{
    return ('/' == source[0] && '*' == source[1]);
}
bool zTokenizer::atC_CommentEnd()
// helper
{
    return ('*' == source[1] && '/' == source[1]);
}

bool zTokenizer::atCPP_Comment()
// helper
{
    return ('/' == source[0] && '/' == source[1]);
}

void zTokenizer::skipC_Comment()
// eats a C-style comment,
// assuming one exists at current source location
{
    // safeguard
    if (!atC_Comment())
        return;

    // opening slash/asterisk
    source += 2;
    // gobble the midsection
    while (*source) {
        // nested C comment?
        if (atC_Comment())
            skipC_Comment();
        // terminator?
        if (atC_CommentEnd())
            break;

        // still eating
        ++source;
    }
    // safeguard
    if ('*' != *source)
        error("skipC_Comment: unexpected eof");
    // closing asterisk/slash
    source += 2;
}

void zTokenizer::skipCPP_Comment()
// eats a C++-style comment,
// assuming one exists at current source location
{
    // safeguard
    if (!atCPP_Comment())
        return;

    // opening slash/slash
    source += 2;
    // gobble the midsection
    while (*source) {
        // terminator?
        if ('\n' == *source)
            break;

        // still eating
        ++source;
    }
    // safeguard
    if ('\n' != *source)
        error("skipCPP_Comment: unexpected eof");
    // closing newline
    ++source;
}

void zTokenizer::skipWhite()
// eat whitespace until non-whitespace is found
{
eat_white:
    // true whitespace
    while (*source && isWhite(*source))
        ++source;

    // comments
    if (atC_Comment())
        skipC_Comment();
    else if (atCPP_Comment())
        skipCPP_Comment();
    // only way to exit is if we fall on a non-comment here
    else
        return;

    // round and round she goes
    goto eat_white;
}

zToken zTokenizer::collectIdentifier() {
    // safeguard
    if (!isAlpha(*source))
        error("collectIdentifier: expected alpha character, got " +
              (VCString)*source);

    zToken token;

    token.id = "";
    while (*source && (isAlphaNumeric(*source) || '_' == *source))
        token.id += *source++;

    return token;
}

zToken zTokenizer::collectNumber()
// collect an integer or floating-point number
{
    // safeguard
    if (!isDigit(*source))
        error("collectNumber: expected digit, got " + (VCString)*source);

    zToken token;
    int dot;

    dot = 0;
    token.id = "";
    while (*source) {
        if ('.' == *source)
            token.id += *source++, ++dot;
        if (isDigit(*source))
            token.id += *source++;
        else
            break;
    }

    if (dot) {
        // safeguard
        if (dot > 1)
            error("collectNumber: more than one decimal in " + token.id);
    }

    return token;
}

VCString zTokenizer::accumulateStringLiteral()
// helper for collectStringLiteral()
{
    static int levels = 0;
    static char quote = 0;

    if (!isQuote(*source))
        error("accumulateStringLiteral: expected quote, got " +
              (VCString)*source);

    if (levels < 1)
        // terminator should be the same as initiator
        quote = *source;
    ++levels;

    VCString lit;
    // open quote
    ++source;
    // accumulate characters
    while (*source) {
        // newline shouldn't exist within a string literal
        if ('\n' == *source)
            error("accumulateStringLiteral: unexpected newline");
        else if (quote == *source) {
            // single initiator quote means close quotes
            if (quote != source[1])
                break;
            // otherwise, duplicate means insert quote character
            lit += quote;
            source += 2;
        }
        // embedded codes
        else if ('\\' == *source) {
            // backslash
            if ('\\' == source[1])
                lit += '\\';
            // newline
            else if ('n' == source[1])
                lit += '\n';
            // tab
            else if ('t' == source[1])
                lit += '\t';
            // double quotes
            else if ('\"' == source[1])
                lit += '\"';
            source += 2;
        } else
            lit += *source++;
    }
    // here we've either hit eof, or a close quote
    if (!*source)
        error("accumulateStringLiteral: unexpected eof");
    // close quote
    ++source;

    // skip to next non-whitespace character
    skipWhite();
    // if it's the start of a new string, join it to this one
    if (isQuote(*source)) {
        if (quote != *source)
            error("accumulateStringLiteral: quote encloser mismatch");
        lit += accumulateStringLiteral();
    }

    --levels;
    if (levels < 1)
        return quote + lit + quote;

    // otherwise, return the string we've accumulated
    return lit;
}

zToken zTokenizer::collectStringLiteral() {
    return zToken{accumulateStringLiteral(), 0};
}

zToken zTokenizer::collectSymbol() {
    zToken token;
    int n;

    for (n = 0; n < symbol_key.size(); n++) {
        if (!strncmp(source, symbol_key[n].c_str(), symbol_key[n].length())) {
            token.id = symbol_key[n];
            source += token.id.length();
            break;
        }
    }
    return token;
}

zToken zTokenizer::collectToken() {
    zToken token;

    // identifier?
    if (isAlpha(*source))
        token = collectIdentifier();
    // number?
    else if (isDigit(*source))
        token = collectNumber();
    // quoted string?
    else if (isQuote(*source))
        token = collectStringLiteral();
    // otherwise, must be a symbol; only if symbols exist o/c
    else if (symbol_key.size() > 0)
        token = collectSymbol();

    // nothing was accumulated into this token; die!
    if (token.id.length() < 1)
        error("unknown character: " + (VCString)*source);

    return token;
}