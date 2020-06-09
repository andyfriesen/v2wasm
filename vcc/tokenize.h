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
	tokenize.h
	coded by aen
*/

#ifndef TOKENIZE_INC
#define TOKENIZE_INC

#include "str.h"

class zToken
{
public:
	zToken(string_t nid="", int nline=0)
		: id(nid), line(nline)
		{ }
	string_t id;
	int line;
};

#include "vector.h"

class zTokenizer
{
private:
	string_t filename;

// valid symbol groupings
	vector_t<string_t> symbol_key;

// source token accumulation
	vector_t<zToken> tokens;
	int curtoken;

// input stream traveller
	const char* source;

// > RECOGNIZERS <
	bool isWhite(char c);
	bool isAlpha(char c);
	bool isBinaryDigit(char c);
	bool IsOctalDigit(char c);
	bool isDigit(char c);
	bool isHexDigit(char c);
	bool isAlphaNumeric(char c);
	bool isQuote(char c);
	bool isValidIdentifier(string_t s);
	bool isValidInteger(string_t s);
	bool isValidNumber(string_t s);

// > EATERS <
	bool atC_Comment();
	bool atC_CommentEnd();
	bool atCPP_Comment();
	void skipC_Comment();
	void skipCPP_Comment();
	void skipWhite();

// > COLLECTORS <
	zToken collectIdentifier();
	zToken collectNumber();
	string_t accumulateStringLiteral();
	zToken collectStringLiteral();
	zToken collectSymbol();
	zToken collectToken();

	void error(string_t message)
	{
		cout << "zTokenizer: error: " << message.c_str() << endl;
		abort();
	}
public:
	zTokenizer()
		{ }
~zTokenizer()
	{ }

// generate symbol table from a NULL-terminated character string array
	void DefineSymbols(const char* nsymbol_key[])
	{
	int n;
	const char* e;

		e = nsymbol_key[n = 0];
		while (e)
		{
			symbol_key.push_top(e);
			e = nsymbol_key[++n];
		}
	}
// generate symbol table from a vector_t<string_t> object
	void DefineSymbols(vector_t<string_t>& nsymbol_key)
	{
		symbol_key = nsymbol_key;
	}

	void Tokenize(string_t nfilename)
	{
	// every time we tokenize a file, save the name
		filename = nfilename;

	memorystream_t in;

	// load input stream
		in.loadfromfile(filename.c_str());
		in.write("", 1);
	// start source off at beginnning
		source = (const char*)in.getdata();

	// clear token list
		tokens.clear();

	// collect the tokens!
		skipWhite();
		while (*source)
		{
			tokens.push_top(collectToken());
			skipWhite();
		}
	// we shouldn't access source anywhere outside this routine
		source = NULL;

		ReSet();
	}

// validators
	bool OK()
		{ return (curtoken >= 0 && curtoken < tokens.size()); }

// navigators
	void ReSet()
		{ curtoken = 0; }
	void Next()
		{ ++curtoken; }
	void Prev()
		{ --curtoken; }
	void Move(int count = 0)
		{ curtoken += count; }

// retrieval
	string_t Token()
		{ return OK() ? tokens[0+curtoken].id : ""; }
	string_t TokenInc()
		{ return OK() ? tokens[curtoken++].id : ""; }
	string_t TokenDec()
		{ return OK() ? tokens[curtoken--].id : ""; }

	void DumpTokens()
	{
	int n;
		for (n = 0; n < tokens.size(); n++)
			cout << tokens[n].id.c_str() << '\n';
	}

	int TokenCount()
		{ return tokens.size(); }
};

#endif // TOKENIZE_INC