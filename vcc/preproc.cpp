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

// ���������������������������������������������������������������������Ŀ
// �                  The VergeC Compiler version 2.01                   �
// �              Copyright (C)1998 BJ Eirich (aka vecna)                �
// �                        Pre Processor module                         �
// �����������������������������������������������������������������������

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 14>
// + fixed incorrect file markers and last character omission.
// + one more incorrect file marker fix; was not restoring correctly after
//   #included files.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifdef __WATCOMC__
#include <io.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

#include "lexical.h"
#include "preproc.h"
#include "str.h"
#include "vcc.h"

#define SYM_MAX_LEN 39
#define RES_MAX_LEN 1023

int pp_is_punc(int ch);
int pp_is_letter(int ch);
int pp_is_digit(int ch);
int pp_is_ident(int ch);
int pp_is_white(int ch);
int pp_is_directive(int ch);
int pp_is_eof(int ch);

int pp_dump;
int pp_nomark;

void pp_error(const char* error, ...);

static int in_define = 0;

char pp_tok[1024] = "";
int pp_toktype = 0;

struct define_t {
    char* sym;
    int sym_len;

    char* resolve;
    int resolve_len;

    int end_resolve_type;

    int compare(void* o) {
        define_t* pdef = (define_t*)o;
        return stricmp(pdef->sym, sym);
    }

    define_t(char* s, char* r) : sym(0), resolve(0) {
        sym_len = strlen(s);
        sym = new char[sym_len + 1];
        if (!sym) {
            pp_error("define_t$ctor: memory exhausted on sym");
        }

        resolve_len = strlen(r);
        resolve = new char[resolve_len + 1];
        if (!resolve) {
            pp_error("define_t$ctor: memory exhausted on resolve");
        }

        strcpy(sym, s);
        strcpy(resolve, r);

        end_resolve_type = pp_toktype; //! pp_is_ident(resolve[strlen(r)-1]);
    }
};

struct include_t {
    std::string filename;
    std::vector<char> data;
    int line_count;
    char* src;

    int compare(void* o) {
        o = o;
        return 0;
    }

    include_t(const char* fname) {
        FILE* in;
        int z;

        in = fopen(fname, "rb");
        if (!in)
            pp_error("unable to open %s", fname);

        // filelength
        fseek(in, 0, SEEK_END);
        z = ftell(in);
        fseek(in, 0, SEEK_SET);

        // cache file
        data.resize(z + 2);
        fread(data.data(), 1, z, in);
        fclose(in);

        // two byte pad
        data[z] = 0;
        data[z + 1] = 0;
        src = data.data();

        filename = fname;
        line_count = 1;
    }
};

// char *pp_token(char *p);
void pp_get_token();

////////////////////////////////////////////////////////////////////////////
// DATA ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

std::vector<define_t*> defines;
// pp_def *pp_defarr               = NULL;

// int		pp_num_defs			= 0;
// int		pp_defs_allocated	= 0;
// int		PP_DEFS_BLOCK		= 25;

std::vector<include_t*> includes;
// pp_include	pp_incarr[100];
// pp_include *pp_icur       = NULL; // get rid of this
// char	*cur_filename		= 0;
// char	*last_filename		= 0;

// int		pp_num_includes		= 0;

int pp_total_lines = 0;

// const int	EOF_CHAR		= 0x00;

char pp_chr_table[256] = "";

enum {
    PP_ERROR,
    PP_WHITE,
    PP_PUNC,
    PP_DIGIT,
    PP_LETTER,
    PP_QUOTE,
    PP_DIRECTIVE,
    PP_EOF
};

FILE* pp_out = NULL;

int pp_last_delim = 1;

static include_t* pfile = 0;

////////////////////////////////////////////////////////////////////////////
// CODE ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

int logging = 1;

void pp_log(char* str) {
    FILE* fp = 0;

    if (!logging)
        return;

    fp = fopen("error.log", "a");
    if (!fp) {
        pp_error("pp_log: unable to open error.log");
    }
    fputs(str, fp);
    fclose(fp);
}

void init_pp_chr_table() {
    static char pp_initted = 0;
    int x = 0;

    if (pp_initted)
        return;

    memset(pp_chr_table, PP_ERROR, 256);

    pp_chr_table[0] = PP_EOF;

    for (x = 'A'; x <= 'Z'; x++)
        pp_chr_table[x] = PP_LETTER;
    for (x = 'a'; x <= 'z'; x++)
        pp_chr_table[x] = PP_LETTER;
    pp_chr_table['_'] = PP_LETTER;
    pp_chr_table['.'] = PP_LETTER;

    for (x = '0'; x <= '9'; x++)
        pp_chr_table[x] = PP_DIGIT;
    pp_chr_table['$'] = PP_DIGIT;
    pp_chr_table['\''] = PP_DIGIT;

    pp_chr_table['+'] = pp_chr_table['-'] = PP_PUNC;
    pp_chr_table['*'] = pp_chr_table['/'] = PP_PUNC;
    pp_chr_table['%'] = PP_PUNC;
    pp_chr_table['|'] = pp_chr_table['&'] = PP_PUNC;
    pp_chr_table['='] = pp_chr_table['^'] = PP_PUNC;
    pp_chr_table[','] = PP_PUNC;
    pp_chr_table['<'] = pp_chr_table['>'] = PP_PUNC;
    pp_chr_table['('] = pp_chr_table[')'] = PP_PUNC;
    pp_chr_table['['] = pp_chr_table[']'] = PP_PUNC;
    pp_chr_table['{'] = pp_chr_table['}'] = PP_PUNC;
    pp_chr_table[':'] = pp_chr_table[';'] = PP_PUNC;
    pp_chr_table['\\'] = pp_chr_table['!'] = PP_PUNC;

    pp_chr_table[' '] = pp_chr_table['\t'] = PP_WHITE;
    pp_chr_table['\r'] = PP_WHITE;

    pp_chr_table['\"'] = PP_QUOTE;

    pp_chr_table['#'] = PP_DIRECTIVE;
}

int pp_is_punc(int ch) { return (PP_PUNC == pp_chr_table[ch]); }
int pp_is_letter(int ch) { return (PP_LETTER == pp_chr_table[ch]); }
int pp_is_digit(int ch) { return (PP_DIGIT == pp_chr_table[ch]); }
int pp_is_ident(int ch) { return (pp_is_letter(ch) || pp_is_digit(ch)); }
int pp_is_white(int ch) { return (PP_WHITE == pp_chr_table[ch]); }
int pp_is_directive(int ch) { return (PP_DIRECTIVE == pp_chr_table[ch]); }
int pp_is_eof(int ch) { return (PP_EOF == pp_chr_table[ch]); }

void pp_line_marker() {
    if (pp_nomark)
        return;

    if (!pp_out)
        pp_error("pp_line_marker: pp_out is NULL!");

    if (!pfile)
        pp_error("pp_line_marker: pfile is NULL!");

    if (pfile->line_count < 0 || pfile->line_count > 1024 * 64)
        pp_error("pp_line_marker: bogus marker: %d", pfile->line_count);

    // pp_log(va("%s: line: %d\n", pfile->filename, pfile->line_count));

    fputc(2, pp_out);
    fwrite(&pfile->line_count, 1, 4, pp_out);
}

void pp_file_marker() {
    if (pp_nomark)
        return;

    if (!pp_out)
        pp_error("pp_file_marker: pp_out is NULL!");

    if (!pfile)
        pp_error("pp_file_marker: pfile is NULL!");

    // for (int n=0; n<includes.number_nodes()-1; n++)
    //	printf("  ");
    // dprint("- file marker: %s", pfile->filename);

    fputc(1, pp_out);
    fwrite(pfile->filename.c_str(), 1, pfile->filename.size() + 1, pp_out);
}

void pp_error(const char* error, ...) {
    va_list lst;
    char string[1024];

    va_start(lst, error);
    vsprintf(string, error, lst);
    va_end(lst);

    err(string);
}

void pp_line_error(const char* error, ...) {
    va_list lst;
    char string[1024];

    va_start(lst, error);
    vsprintf(string, error, lst);
    va_end(lst);

    if (!pfile)
        pp_error("pp_line_error: pfile is NULL!");

    printf("%s(%d): ", pfile->filename.c_str(), pfile->line_count);
    pp_error(string);
}

void pp_def_add(char* sym, char* resolve) {
    defines.push_back(new define_t{sym, resolve});
}

void push_file(const char* filename) {
    if (!includes.empty()) {
        for (include_t* inc : includes) {
            if (inc->filename == pp_tok) {
                pp_error("circular dependencies");
            }
        }
    }

    pfile = new include_t{filename};

    // add the include to the list
    includes.emplace_back(pfile);

    // starts fresh for each #include file
    pp_file_marker();
    pp_line_marker();

    // for (include_t* inc: includes) {
    //     putc(' ', stdout);
    // }
    // printf("push %s\n", pfile->filename.c_str());
}

void pop_file() {
    if (!pfile) {
        pp_error("pop_file: pfile is null");
    }

    // for (include_t* i: includes) {
    //     putc(' ', stdout);
    // }
    // printf("pop %s (%d lines)\n", pfile->filename.c_str(),
    // pfile->line_count);

    if (pfile->filename == "system.vc") {
        // dprint("> total lines: %d", pp_total_lines);
        tlines = pp_total_lines;
    }
    pp_total_lines += pfile->line_count;

    auto it = std::find(includes.begin(), includes.end(), pfile);
    if (it == includes.end()) {
        pp_error("include list corrupted");
    }

    includes.pop_back();

    pp_tok[0] = '\0';

    if (!includes.empty()) {
        pp_file_marker();
        pp_line_marker();
    }

    delete pfile;

    pfile = includes.empty() ? nullptr : includes.back();
}

static int newline_okay = 1;

int is_end_c_comment() {
    return ('*' == pfile->src[0] && '/' == pfile->src[1]);
}
int is_start_c_comment() {
    return ('/' == pfile->src[0] && '*' == pfile->src[1]);
}
int is_start_cpp_comment() {
    return ('/' == pfile->src[0] && '/' == pfile->src[1]);
}

int is_newline() {
    if ('\n' == *(pfile->src)) {
        pfile->line_count++;
        pp_line_marker();
    }
    return ('\n' == *(pfile->src));
}

void pp_skip_c_comment() {
    // assumes caller found /*
    pfile->src += 2;
    while (*(pfile->src) && !is_end_c_comment()) {
        if (is_newline()) {
            if (newline_okay) {
                pfile->src++;
                continue;
            }
            return;
        }

        // nested C comment?
        if (is_start_c_comment())
            pp_skip_c_comment();

        // otherwise, we're still inside the comment
        pfile->src++;
    }
    // */ was found, or EOF
    if (*(pfile->src))
        pfile->src += 2;
}

void pp_skip_cpp_comment() {
    // assumes caller found //
    pfile->src += 2;
    // just fallout when newline is reached; pp_skipwhite will handle newline
    // marker
    while (*(pfile->src) && '\n' != *(pfile->src)) {
        pfile->src++;
    }
}

static int line_continue = 0;

void pp_skipwhite() {
startwhite:
    // passive whitespace
    while (*(pfile->src) && pp_is_white(*(pfile->src))) {
        pfile->src++;
    }

    // line continuation
    if (!line_continue && '\\' == *(pfile->src)) {
        int nlok = 0;

        pfile->src++; // move past line continuation symbol

        line_continue = 1;
        nlok = newline_okay;
        newline_okay = 0;

        pp_skipwhite();

        // we expect a newline char after the line continuation symbol
        if ('\n' != *(pfile->src)) {
            pp_line_error("characters found after line continuation symbol");
        }
        pfile->src++; // move past newline char

        newline_okay = nlok;
        line_continue = 0;

        goto startwhite;
    }

    // newlines
    if (is_newline()) {
        if (newline_okay) {
            pfile->src++;
            goto startwhite;
        }
        // newline not okay; return so we can detect it
        return;
    }

    // comments
    if (is_start_cpp_comment()) {
        pp_skip_cpp_comment();
        goto startwhite;
    }
    if (is_start_c_comment()) {
        pp_skip_c_comment();
        goto startwhite;
    }
}

// shouldn't ever get the null char to start with
void pp_token_punc() {
    char* d = 0;

    d = pp_tok;
    do {
        *d++ = *(pfile->src)++;

        // this gets rid of: int gp;// current gold
        //   problem; *** aen; 28aug99
        if (is_start_cpp_comment())
            break;
        if ('\\' == *(pfile->src))
            break; // continuation

    } while (*(pfile->src) && pp_is_punc(*(pfile->src)));
    // terminate token
    *d = '\0';
}

// shouldn't ever get the null char to start with
void pp_token_digit() {
    char* d = 0;

    d = pp_tok;

    // special case for 'x' numbers
    if ('\'' == *(pfile->src)) {
        *d++ = *(pfile->src)++;
        if (*(pfile->src))
            *d++ = *(pfile->src)++;
        if (*(pfile->src))
            *d++ = *(pfile->src)++;
    }
    // digits
    else {
        do {
            *d++ = *(pfile->src)++;
        } while (*(pfile->src) && pp_is_digit(*(pfile->src)));
    }

    *d = '\0'; // terminate
}

// shouldn't ever get the null char to start with
void pp_token_letter() {
    char* d = 0;

    d = pp_tok;

    do {
        *d++ = *(pfile->src)++;
    } while (*(pfile->src) && pp_is_ident(*(pfile->src)));

    *d = '\0'; // terminate
}

void define_check_dup(char* sym) {
    int len;

    // check for duplicate #defines
    if (!defines.empty()) {
        len = strlen(sym);

        for (define_t* pdef : defines) {
            if (len == pdef->sym_len) {
                if (!stricmp(pdef->sym, sym))
                    pp_line_error("duplicate #define symbol found: %s", sym);
            }
        }
    }
}

void define_skip_arglist() {
    newline_okay = 0;
    pp_skipwhite();
    newline_okay = 1;

    if ('\n' == *(pfile->src)) {
        pfile->line_count--;
        pp_line_error("unexpected newline");
    }

    // found argument list; discard for now
    /*        if ('(' == *(pfile->src))
            {
                    pfile->src++;
                    while (*(pfile->src) && ')' != *(pfile->src) && '\n' !=
       *(pfile->src))
                    {
                            pfile->src++;
                    }
            }
            else return;

            if ('\n' == *(pfile->src))
            {
                    pp_line_error("unexpected newline");
            }

            if (')' != *(pfile->src))
            {
                    pp_line_error("unclosed arglist; missing paren");
            }
            pfile->src++;*/
}

// finds symbol, and dumbs it into sym_dest
void define_parse_symbol(char* sym_dest) {
    int n = 0;

    newline_okay = 0;
    pp_skipwhite();
    newline_okay = 1;

    if ('\n' == *(pfile->src)) {
        pfile->line_count--;
        pp_line_error("unexpected newline");
    }

    if (!pp_is_ident(*(pfile->src))) {
        pp_line_error("syntax error");
    }
    // parse symbol
    n = 0;
    while (*(pfile->src) && pp_is_ident(*(pfile->src)) && n < SYM_MAX_LEN &&
           '\n' != *(pfile->src) && '(' != *(pfile->src)) {
        sym_dest[n++] = *(pfile->src)++;
    }
    sym_dest[n] = '\0';
}

void define_check_for_replacement();

void define_parse_resolve(char* resolve_dest) {
    int n = 0;
    char* d = 0;
    int last_type = 0;
    int len = 0;

    newline_okay = 0;
    pp_skipwhite();

    if ('\n' == *(pfile->src)) {
        pfile->line_count--;
        pp_line_error("unexpected newline");
    }

    in_define = 1;

    // parse resolve
    last_type = PP_PUNC;
    pp_toktype = PP_PUNC;
    pp_tok[0] = '\0';
    n = 0;
    d = resolve_dest;
    do {
        // only emit whitespace if significant
        if (PP_LETTER == last_type &&
            (PP_LETTER == pp_toktype || PP_DIGIT == pp_toktype)) {
            *d++ = ' ';
        }

        // would this new token make our resolved code larger than we can hold?
        len = strlen(pp_tok);
        if (n + len > RES_MAX_LEN) {
            pp_line_error("#define is too long");
        }

        // all's well; copy it in
        memcpy(d, pp_tok, len);
        d += len;
        n += len;

        // for compacting
        last_type = pp_toktype;
        // get token and check if it matches a #define
        pp_get_token();

        define_check_for_replacement();

    } while (*(pfile->src) && n <= RES_MAX_LEN && '\n' != *(pfile->src));

    if ('\n' != *(pfile->src)) {
        pp_line_error("syntax error");
    }
    pfile->src++;

    *d = '\0'; // terminate resolve
    pp_tok[0] = '\0';

    newline_okay = 1;
    in_define = 0;
}

// all of this is highly experimental
void handle_define() {
    char sym[SYM_MAX_LEN + 1];
    char resolve[RES_MAX_LEN + 1];

    define_parse_symbol(sym);

    // make sure to check for dup before we do !*(pfile->src) check
    define_check_dup(sym);

    // just skip it; arglist handling is unimplemented
    define_skip_arglist();

    // grab the replacement code
    define_parse_resolve(resolve);

    // update define list
    pp_def_add(sym, resolve);
}

// FIXME: problem with #includes being at the end of a file, and stuff past EOF
// is read I think
void handle_include() {
    char* d = 0;
    char fname[32];

    // any whitespace after 'include'
    // newline_okay	=0;
    pp_skipwhite();

    if ('\n' == *(pfile->src)) {
        pfile->line_count--;
        pp_line_error("unexpected newline");
    }

    // expect opening "
    if ('\"' != *(pfile->src))
        pp_line_error("\" expected");
    pfile->src++;

    // grab the filename
    d = fname;
    while (*(pfile->src) && '\"' != *(pfile->src) && (int)(d - fname) < 32) {
        *d++ = *(pfile->src)++;
    }
    *d = '\0';

    // if (!*(pfile->src)) return;

    if ('\"' != *(pfile->src))
        pp_line_error("\" expected");
    pfile->src++;

    /*
    pp_skipwhite();
    //newline_okay=1;

    //if (!*(pfile->src)) return;

    if ('\n' != *(pfile->src))
    {
            pp_line_error("syntax error");
    }
    pfile->src++; // skip newline we just found
    */

    Process(fname);

    // pp_tok[0]='\0';
}

// shouldn't ever get the null char to start with
void pp_token_directive() {
    char* d = 0;

    d = pp_tok;

    *d++ = *(pfile->src)++; // leading #

    pp_skipwhite();

    // directive name
    if (!isalpha(*(pfile->src))) {
        pp_line_error("identifier expected");
    }
    // pp_is_ident(*(pfile->src))); // how come this stalls everything?
    while (
        *(pfile->src) && (isalpha(*(pfile->src)) || isdigit(*(pfile->src)))) {
        *d++ = *(pfile->src)++;
    }
    *d = '\0'; // terminate

    // #include
    if (!strcmp(pp_tok, "#include")) {
        handle_include();
        return;
    }
    // #define
    else if (!strcmp(pp_tok, "#define")) {
        handle_define();
        return;
    }

    // #???
    pp_line_error("unrecognized compiler directive: %s", pp_tok);
}

void pp_token_quote() {
    char* d = 0;

    d = pp_tok;

    *d++ = *(pfile->src)++; // leading quote

    while (*(pfile->src) && '\"' != *(pfile->src) && '\n' != *(pfile->src)) {
        *d++ = *(pfile->src)++;
    }

    if ('\"' != *(pfile->src)) {
        pp_line_error("quote missing");
    }

    *d++ = *(pfile->src)++; // trailing quote

    *d = '\0'; // terminate whatever we got
}

void pp_get_token() {
    pp_skipwhite();

    if ('\n' == *(pfile->src))
        return; // this means newline_okay is probably set to 0; bail

    // if pp_skipwhite hits null char, this will be PP_EOF
    pp_toktype = pp_chr_table[(int)*(pfile->src)];

    switch (pp_toktype) {
    case PP_PUNC:
        pp_token_punc();
        break;

    case PP_DIGIT:
        pp_token_digit();
        break;
    case PP_LETTER:
        pp_token_letter();
        break;

    case PP_QUOTE:
        pp_token_quote();
        break;

    case PP_DIRECTIVE:
        pp_token_directive();
        break;

    case PP_EOF:
        break;

    case PP_ERROR:
        pp_line_error("syntax error");
    }
}

void define_check_for_replacement() {
    int len;

    if (PP_LETTER != pp_toktype)
        return;

    if (defines.empty())
        return;

    len = strlen(pp_tok);

    // scan symbols in define list
    for (define_t* pdef : defines) {
        // test further if symbol lengths match
        if (pdef->sym_len == len) {
            // lengths match; are the symbols equal?
            if (!stricmp(pp_tok, pdef->sym)) {
                // yep, replace token with resolve
                memcpy(pp_tok, pdef->resolve, pdef->resolve_len + 1);

                // helper for compactor
                pp_toktype = pdef->end_resolve_type;

                break;
            }
        }
    }
}

void Process(const char* filename) {
    int last_type = 0;

    // load file
    push_file(filename); // sets cur_filename

    last_type = PP_PUNC;
    pp_toktype = PP_PUNC;
    pp_tok[0] = '\0';
    do {
        // only emit whitespace if significant
        if (PP_LETTER == last_type &&
            (PP_LETTER == pp_toktype || PP_DIGIT == pp_toktype)) {
            fputc(' ', pp_out);
        }

        fwrite(pp_tok, 1, strlen(pp_tok), pp_out);
        pp_tok[0] = '\0';

        // for code compactor
        last_type = pp_toktype;
        // get token and do and check #defines on it
        pp_get_token();

        define_check_for_replacement();

    } while (*(pfile->src));

    // was the last token flush against EOF? if so, make sure and dump it
    if (pp_tok[0])
        fwrite(pp_tok, 1, strlen(pp_tok), pp_out);

    pop_file();
}

void pp_dump_output() {
    static int pp_tempct = 0;

    system(va("copy vcctemp.$$$ vcc%d.$$$", pp_tempct++));
}

/*
// for debugging
void dump_define_list()
{
        if (!defines.number_nodes())
                return;
        defines.go_head();
        do
        {
                define_t* pdef=(define_t *)defines.current();

                printf("---\n");
                printf("sym: %s\n", pdef->sym);
                printf("res: %s\n", pdef->resolve);

                defines.go_next();
        } while (defines.current() != defines.head());
}
*/

void PreProcess(const char* filename) {
    vprint("First pass (pre-processing)...");

    init_pp_chr_table();

    pp_out = fopen("vcctemp.$$$", "wb");
    if (!pp_out) {
        pp_error("PreProcess: unable to open vcctemp.$$$ for writing");
    }

    Process(filename);

    fclose(pp_out);

    // dump_define_list();

    if (pp_dump)
        pp_dump_output();
}
