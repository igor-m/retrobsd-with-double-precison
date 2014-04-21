/*
Copyright (c) 2012-2014, Alexey Frunze
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the FreeBSD Project.
*/

/*****************************************************************************/
/*                                                                           */
/*                                Smaller C                                  */
/*                                                                           */
/*       A simple and small single-pass C compiler ("small C" class).        */
/*                                                                           */
/*            Produces 16/32-bit 80386 assembly output for NASM.             */
/*             Produces 32-bit MIPS assembly output for gcc/as.              */
/*                                                                           */
/*                                Main file                                  */
/*                                                                           */
/*****************************************************************************/

// You need to declare __setargv__ as an extern symbol when bootstrapping with
// Turbo C++ in order to access main()'s argc and argv params.
//
// This is no longer supported since the compiler is too big to be compiled
// with itself into object files and then linked with Turbo C++'s standard C library.
//
// extern char _setargv__;

#ifdef NO_EXTRAS
#define NO_PPACK
#define NO_TYPEDEF_ENUM
#define NO_FUNC_
#endif

#ifndef __SMALLER_C__

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#if UINT_MAX >= 0xFFFFFFFF
#define CAN_COMPILE_32BIT
#endif

#else // #ifndef __SMALLER_C__

#define NULL 0
#define size_t unsigned int

#define CHAR_BIT (8)

#ifdef __SMALLER_C_SCHAR__
#define CHAR_MIN (-128)
#define CHAR_MAX (127)
#endif
#ifdef __SMALLER_C_UCHAR__
#define CHAR_MIN (0)
#define CHAR_MAX (255)
#endif

#ifndef __SMALLER_C_SCHAR__
#ifndef __SMALLER_C_UCHAR__
#error __SMALLER_C_SCHAR__ or __SMALLER_C_UCHAR__ must be defined
#endif
#endif

#ifdef __SMALLER_C_16__
#define INT_MAX (32767)
#define INT_MIN (-32767-1)
#define UINT_MAX (65535u)
#define UINT_MIN (0u)
#endif
#ifdef __SMALLER_C_32__
#define INT_MAX (2147483647)
#define INT_MIN (-2147483647-1)
#define UINT_MAX (4294967295u)
#define UINT_MIN (0u)
#define CAN_COMPILE_32BIT
#endif

#ifndef __SMALLER_C_16__
#ifndef __SMALLER_C_32__
#error __SMALLER_C_16__ or __SMALLER_C_32__ must be defined
#endif
#endif

void exit(int);
int atoi(char*);

size_t strlen(char*);
char* strcpy(char*, char*);
char* strchr(char*, int);
int strcmp(char*, char*);
int strncmp(char*, char*, size_t);
void* memmove(void*, void*, size_t);
void* memcpy(void*, void*, size_t);
void* memset(void*, int, size_t);

int isspace(int);
int isdigit(int);
int isalpha(int);
int isalnum(int);

#define FILE void
#define EOF (-1)
FILE* fopen(char*, char*);
int fclose(FILE*);
int putchar(int);
int fputc(int, FILE*);
int fgetc(FILE*);
int puts(char*);
int fputs(char*, FILE*);
int sprintf(char*, char*, ...);
//int vsprintf(char*, char*, va_list);
int vsprintf(char*, char*, void*);
int printf(char*, ...);
int fprintf(FILE*, char*, ...);
//int vprintf(char*, va_list);
int vprintf(char*, void*);
//int vfprintf(FILE*, char*, va_list);
int vfprintf(FILE*, char*, void*);

#endif // #ifndef __SMALLER_C__


////////////////////////////////////////////////////////////////////////////////

// all public macros

#ifndef MAX_IDENT_LEN
#define MAX_IDENT_LEN        31
#endif
#define MAX_STRING_LEN       127
#define MAX_CHAR_QUEUE_LEN   256

#ifndef MAX_MACRO_TABLE_LEN
#define MAX_MACRO_TABLE_LEN  4096
#endif

#ifndef MAX_STRING_TABLE_LEN
#define MAX_STRING_TABLE_LEN (512+128)
#endif

#ifndef MAX_IDENT_TABLE_LEN
#define MAX_IDENT_TABLE_LEN  (4096+656)
#endif

#ifndef SYNTAX_STACK_MAX
#define SYNTAX_STACK_MAX (2048+512+32)
#endif

#ifndef MAX_FILE_NAME_LEN
#define MAX_FILE_NAME_LEN    95
#endif

#ifndef NO_PREPROCESSOR
#define MAX_INCLUDES         8
#define PREP_STACK_SIZE      8
#define MAX_SEARCH_PATH      256
#else
#define MAX_INCLUDES         1
#define PREP_STACK_SIZE      1
#define MAX_SEARCH_PATH      1
#endif

/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */

#define tokEof        0
#define tokNumInt     1
#define tokNumUint    2
#define tokLitStr     3

#define tokLShift     4
#define tokRShift     5
#define tokLogAnd     6
#define tokLogOr      7
#define tokEQ         8
#define tokNEQ        9
#define tokLEQ        10
#define tokGEQ        11
#define tokInc        12
#define tokDec        13
#define tokArrow      14
#define tokEllipsis   15

#define tokIdent      16
#define tokVoid       17
#define tokChar       18
#define tokInt        19
#define tokReturn     20
#define tokGoto       21
#define tokIf         22
#define tokElse       23
#define tokWhile      24
#define tokCont       25
#define tokBreak      26
#define tokSizeof     27

#define tokAssignMul  'A'
#define tokAssignDiv  'B'
#define tokAssignMod  'C'
#define tokAssignAdd  'D'
#define tokAssignSub  'E'
#define tokAssignLSh  'F'
#define tokAssignRSh  'G'
#define tokAssignAnd  'H'
#define tokAssignXor  'I'
#define tokAssignOr   'J'

#define tokFloat      'a'
#define tokDouble     'b'
#define tokLong       'c'
#define tokShort      'd'
#define tokUnsigned   'e'
#define tokSigned     'f'
#define tokConst      'g'
#define tokVolatile   'h'
#define tokRestrict   'i'
#define tokStatic     'j'
#define tokInline     'k'
#define tokExtern     'l'
#define tokAuto       'm'
#define tokRegister   'n'
#define tokTypedef    'o'
#define tokEnum       'p'
#define tokStruct     'q'
#define tokUnion      'r'
#define tokDo         's'
#define tokFor        't'
#define tokSwitch     'u'
#define tokCase       'v'
#define tokDefault    'w'
#define tok_Bool      'x'
#define tok_Complex   'y'
#define tok_Imagin    'z'

#define tok_Asm       '`'

/* Pseudo-tokens (converted from others or generated) */
#define tokURShift    28
#define tokUDiv       29
#define tokUMod       30
#define tokAssignURSh 31
#define tokAssignUDiv '@'
#define tokAssignUMod 'K'
#define tokComma      '0'

#define tokIfNot      'L'

#define tokUnaryAnd   'M'
#define tokUnaryStar  'N'
#define tokUnaryPlus  'O'
#define tokUnaryMinus 'P'

#define tokPostInc    'Q'
#define tokPostDec    'R'
#define tokPostAdd    'S'
#define tokPostSub    'T'

#define tokULess      'U'
#define tokUGreater   'V'
#define tokULEQ       'W'
#define tokUGEQ       'X'

#define tokLocalOfs   'Y'
#define tokShortCirc  'Z'

#define tokSChar      0x80
#define tokUChar      0x81
#define tokUShort     0x82
#define tokULong      0x83
//#define tokLongLong   0x84
//#define tokULongLong  0x85
//#define tokLongDbl    0x86
#define tokGotoLabel  0x8F
#define tokStructPtr  0x90
#define tokTag        0x91
#define tokMemberIdent 0x92
#define tokEnumPtr    0x93

#define FormatFlat      0
#define FormatSegmented 1
#define FormatSegTurbo  2
#define FormatSegHuge   3

#define SymVoidSynPtr 0
#define SymIntSynPtr  1
#define SymUintSynPtr 2
#define SymFuncPtr    3

#ifndef STACK_SIZE
#define STACK_SIZE 128
#endif

#define SymFxn       1
#define SymGlobalVar 2
#define SymGlobalArr 3
#define SymLocalVar  4
#define SymLocalArr  5

// all public prototypes
int uint2int(unsigned);
unsigned truncUint(unsigned);
int truncInt(int);

int GetToken(void);
char* GetTokenName(int token);

int GetTokenValueInt(void);

char* GetTokenValueString(void);
int GetTokenValueStringLength(void);

char* GetTokenIdentName(void);

#ifndef NO_PREPROCESSOR
#ifndef NO_ANNOTATIONS
void DumpMacroTable(void);
#endif
#endif

void PurgeStringTable(void);
void AddString(int label, char* str, int len);
char* FindString(int label);

int AddIdent(char* name);
int FindIdent(char* name);
#ifndef NO_ANNOTATIONS
void DumpIdentTable(void);
#endif
char* lab2str(char* p, int n);

void GenInit(void);
void GenFin(void);
int GenInitParams(int argc, char** argv, int* idx);
void GenInitFinalize(void);
void GenStartCommentLine(void);
void GenWordAlignment(void);
void GenLabel(char* Label, int Static);
void GenNumLabel(int Label);
void GenZeroData(unsigned Size);
void GenIntData(int Size, int Val);
void GenStartAsciiString(void);
void GenAddrData(int Size, char* Label);

void GenJumpUncond(int Label);
void GenJumpIfZero(int Label);
void GenJumpIfNotZero(int Label);
void GenJumpIfNotEqual(int val, int Label);

void GenFxnProlog(void);
void GenFxnEpilog(void);

void GenLocalAlloc(int Size);

unsigned GenStrData(int generatingCode, unsigned requiredLen);
void GenExpr(void);

void PushSyntax(int t);
void PushSyntax2(int t, int v);

#ifndef NO_ANNOTATIONS
void DumpSynDecls(void);
#endif

void push2(int v, int v2);
void ins2(int pos, int v, int v2);
void ins(int pos, int v);
void del(int pos, int cnt);

int TokenStartsDeclaration(int t, int params);
int ParseDecl(int tok, unsigned structInfo[4], int cast, int label);

void ShiftChar(void);
int puts2(char*);
int printf2(char*, ...);

void error(char* format, ...);
void warning(char* format, ...);
void errorFile(char* n);
void errorFileName(void);
void errorInternal(int n);
void errorChrStr(void);
void errorUnexpectedToken(int tok);
void errorDirective(void);
void errorCtrlOutOfScope(void);
void errorDecl(void);
void errorVarSize(void);
void errorInit(void);
void errorUnexpectedVoid(void);
void errorOpType(void);
void errorNotLvalue(void);
void errorNotConst(void);
void errorLongExpr(void);

int FindSymbol(char* s);
int SymType(int SynPtr);
int FindTaggedDecl(char* s, int start, int* CurScope);
#ifndef NO_TYPEDEF_ENUM
int FindTypedef(char* s, int* CurScope, int forUse);
#endif
int GetDeclSize(int SyntaxPtr, int SizeForDeref);

int ParseExpr(int tok, int* GotUnary, int* ExprTypeSynPtr, int* ConstExpr, int* ConstVal, int option, int option2);
int GetFxnInfo(int ExprTypeSynPtr, int* MinParams, int* MaxParams, int* ReturnExprTypeSynPtr);

// all data

int verbose = 0;
int warnCnt = 0;

// prep.c data

int TokenValueInt = 0;
char TokenIdentName[MAX_IDENT_LEN + 1];
int TokenIdentNameLen = 0;
char TokenValueString[MAX_STRING_LEN + 1];
int TokenStringLen = 0;
int LineNo = 1;
int LinePos = 1;
char CharQueue[MAX_CHAR_QUEUE_LEN];
int CharQueueLen = 0;

#ifndef NO_PREPROCESSOR
/*
  Macro table entry format:
    idlen char:     identifier length (<= 127)
    id char[idlen]: identifier (ASCIIZ)
    exlen char:     length of what the identifier expands into (<= 127)
    ex char[exlen]: what the identifier expands into (ASCII)
*/
char MacroTable[MAX_MACRO_TABLE_LEN];
int MacroTableLen = 0;
#endif

/*
  String table entry format:
    labell uchar:   temporary identifier's (char*) label number low 8 bits
    labelh uchar:   temporary identifier's (char*) label number high 8 bits
    len char:       string length (<= 127)
    str char[len]:  string (ASCII)
*/
char StringTable[MAX_STRING_TABLE_LEN];
int StringTableLen = 0;

/*
  Identifier table entry format:
    id char[idlen]: string (ASCIIZ)
    idlen char:     string length (<= 127)
*/
char IdentTable[MAX_IDENT_TABLE_LEN];
int IdentTableLen = 0;

#ifndef MAX_GOTO_LABELS
#define MAX_GOTO_LABELS 16
#endif
int gotoLabels[MAX_GOTO_LABELS][2];
// gotoLabStat[]: bit 1 = used (by "goto label;"), bit 0 = defined (with "label:")
char gotoLabStat[MAX_GOTO_LABELS];
int gotoLabCnt = 0;

// Data structures to support #include
int FileCnt = 0;
char FileNames[MAX_INCLUDES][MAX_FILE_NAME_LEN + 1];
FILE* Files[MAX_INCLUDES];
FILE* OutFile;
char CharQueues[MAX_INCLUDES][3];
int LineNos[MAX_INCLUDES];
int LinePoss[MAX_INCLUDES];
char SearchPaths[MAX_SEARCH_PATH];
int SearchPathsLen = 0;

// Data structures to support #ifdef/#ifndef,#else,#endif
int PrepDontSkipTokens = 1;
int PrepStack[PREP_STACK_SIZE][2];
int PrepSp = 0;

// Data structures to support #pragma pack(...)
#ifndef NO_PPACK
#define PPACK_STACK_SIZE 16
int PragmaPackValue;
int PragmaPackValues[PPACK_STACK_SIZE];
int PragmaPackSp = 0;
#endif

// expr.c data

int ExprLevel = 0;

// TBD??? merge expression and operator stacks into one
int stack[STACK_SIZE][2];
int sp = 0;

#define OPERATOR_STACK_SIZE STACK_SIZE
int opstack[OPERATOR_STACK_SIZE][2];
int opsp = 0;

// smc.c data

int OutputFormat = FormatSegmented;
int GenExterns = 1;

#ifdef CAN_COMPILE_32BIT
// Name of the function to call in main()'s prolog to construct C++ objects/init data.
// gcc calls __main().
char* MainPrologCtorFxn = NULL;
#endif

// Names of C functions and variables are usually prefixed with an underscore.
// One notable exception is the ELF format used by gcc in Linux.
// Global C identifiers in the ELF format should not be predixed with an underscore.
int UseLeadingUnderscores = 1;

char* FileHeader = "";
char* CodeHeader = "";
char* CodeFooter = "";
char* DataHeader = "";
char* DataFooter = "";

int CharIsSigned = 1;
int SizeOfWord = 2; // in chars (char can be a multiple of octets); ints and pointers are of word size

// TBD??? implement a function to allocate N labels with overflow checks
int LabelCnt = 1; // label counter for jumps
int StructCpyLabel = 0; // label of the function to copy structures/unions

// call stack (from higher to lower addresses):
//   param n
//   ...
//   param 1
//   return address
//   saved xbp register
//   local var 1
//   ...
//   local var n
int CurFxnSyntaxPtr = 0;
int CurFxnParamCnt = 0;
int CurFxnParamCntMin = 0;
int CurFxnParamCntMax = 0;
int CurFxnParamOfs = 0; // positive
int CurFxnLocalOfs = 0; // negative
int CurFxnMinLocalOfs = 0; // negative

int CurFxnReturnExprTypeSynPtr = 0;
int CurFxnEpilogLabel = 0;

char* CurFxnName = NULL;
#ifndef NO_FUNC_
int CurFxnNameLabel = 0;
#endif

int ParseLevel = 0; // Parse level/scope (file:0, fxn:1+)
int ParamLevel = 0; // 1+ if parsing params, 0 otherwise

int SyntaxStack[SYNTAX_STACK_MAX][2];
int SyntaxStackCnt = 0;

// all code

int uint2int(unsigned n)
{
  int r;
  // Convert n to (int)n in such a way that (unsigned)(int)n == n,
  // IOW, avoid signed overflows in (int)n.
  // We're assuming ints are 2's complement.

  // "n < INT_MAX + 1u" is equivalent to "n <= INT_MAX" without the
  // possible warning about comparing signed and unsigned types
  if (n < INT_MAX + 1u)
  {
    r = n;
  }
  else
  {
    n = n - INT_MAX - 1; // Now, 0 <= n <= INT_MAX, n is representable in int
    r = n;
    r = r - INT_MAX - 1; // Now, INT_MIN <= r <= -1
  }

  return r;
}

unsigned truncUint(unsigned n)
{
  // Truncate n to SizeOfWord * 8 bits
  if (SizeOfWord == 2)
    n &= ~(~0u << 8 << 8);
  else if (SizeOfWord == 4)
    n &= ~(~0u << 8 << 12 << 12);
  return n;
}

int truncInt(int n)
{
  // Truncate n to SizeOfWord * 8 bits and then sign-extend it
  unsigned un = n;
  if (SizeOfWord == 2)
  {
    un &= ~(~0u << 8 << 8);
    un |= (((un >> 8 >> 7) & 1) * ~0u) << 8 << 8;
  }
  else if (SizeOfWord == 4)
  {
    un &= ~(~0u << 8 << 12 << 12);
    un |= (((un >> 8 >> 12 >> 11) & 1) * ~0u) << 8 << 12 << 12;
  }
  return uint2int(un);
}

// prep.c code

#ifndef NO_PREPROCESSOR
int FindMacro(char* name)
{
  int i;

  for (i = 0; i < MacroTableLen; )
  {
    if (!strcmp(MacroTable + i + 1, name))
      return i + 1 + MacroTable[i];

    i = i + 1 + MacroTable[i]; // skip id
    i = i + 1 + MacroTable[i]; // skip ex
  }

  return -1;
}

int UndefineMacro(char* name)
{
  int i;

  for (i = 0; i < MacroTableLen; )
  {
    if (!strcmp(MacroTable + i + 1, name))
    {
      int len = 1 + MacroTable[i]; // id part len
      len = len + 1 + MacroTable[i + len]; // + ex part len

      memmove(MacroTable + i,
              MacroTable + i + len,
              MacroTableLen - i - len);
      MacroTableLen -= len;

      return 1;
    }

    i = i + 1 + MacroTable[i]; // skip id
    i = i + 1 + MacroTable[i]; // skip ex
  }

  return 0;
}

void AddMacroIdent(char* name)
{
  int l = strlen(name);

  if (l >= 127)
    error("Macro identifier too long '%s'\n", name);

  if (MAX_MACRO_TABLE_LEN - MacroTableLen < l + 3)
    error("Macro table exhausted\n");

  MacroTable[MacroTableLen++] = l + 1; // idlen
  strcpy(MacroTable + MacroTableLen, name);
  MacroTableLen += l + 1;

  MacroTable[MacroTableLen] = 0; // exlen
}

void AddMacroExpansionChar(char e)
{
  if (e == '\0')
  {
    // finalize macro definition entry
    // remove trailing space first
    while (MacroTable[MacroTableLen] &&
           strchr(" \t", MacroTable[MacroTableLen + MacroTable[MacroTableLen]]))
      MacroTable[MacroTableLen]--;
    MacroTableLen += 1 + MacroTable[MacroTableLen];
    return;
  }

  if (MacroTableLen + 1 + MacroTable[MacroTableLen] >= MAX_MACRO_TABLE_LEN)
    error("Macro table exhausted\n");

  if (MacroTable[MacroTableLen] >= 127)
    error("Macro definition too long\n");

  MacroTable[MacroTableLen + 1 + MacroTable[MacroTableLen]] = e;
  MacroTable[MacroTableLen]++;
}

void DefineMacro(char* name, char* expansion)
{
  AddMacroIdent(name);
  do
  {
    AddMacroExpansionChar(*expansion);
  } while (*expansion++ != '\0');
}

#ifndef NO_ANNOTATIONS
void DumpMacroTable(void)
{
  int i, j;

  puts2("");
  GenStartCommentLine(); printf2("Macro table:\n");
  for (i = 0; i < MacroTableLen; )
  {
    GenStartCommentLine(); printf2("Macro %s = ", MacroTable + i + 1);
    i = i + 1 + MacroTable[i]; // skip id
    printf2("`");
    j = MacroTable[i++];
    while (j--)
      printf2("%c", MacroTable[i++]);
    printf2("`\n");
  }
  GenStartCommentLine(); printf2("Bytes used: %d/%d\n\n", MacroTableLen, MAX_MACRO_TABLE_LEN);
}
#endif
#endif // #ifndef NO_PREPROCESSOR

int KeepStringTable = 0;

void PurgeStringTable(void)
{
  if (!KeepStringTable)
    StringTableLen = 0;
}

void AddString(int label, char* str, int len)
{
  if (len > 127)
    error("String literal too long\n");

  if (MAX_STRING_TABLE_LEN - StringTableLen < 2 + 1 + len)
    error("String table exhausted\n");

  StringTable[StringTableLen++] = label & 0xFF;
  StringTable[StringTableLen++] = (label >> 8) & 0xFF;

  StringTable[StringTableLen++] = len;
  memcpy(StringTable + StringTableLen, str, len);
  StringTableLen += len;
}

char* FindString(int label)
{
  int i;

  for (i = 0; i < StringTableLen; )
  {
    int lab;

    lab = StringTable[i] & 0xFF;
    lab += (StringTable[i + 1] & 0xFFu) << 8;

    if (lab == label)
      return StringTable + i + 2;

    i += 2;
    i += 1 + StringTable[i];
  }

  return NULL;
}

int FindIdent(char* name)
{
  int i;
  for (i = IdentTableLen; i > 0; )
  {
    i -= 1 + IdentTable[i - 1];
    if (!strcmp(IdentTable + i, name))
      return i;
  }
  return -1;
}

int AddIdent(char* name)
{
  int i, len;

  if ((i = FindIdent(name)) >= 0)
    return i;

  i = IdentTableLen;
  len = strlen(name);

  if (len >= 127)
    error("Identifier too long\n");

  if (MAX_IDENT_TABLE_LEN - IdentTableLen < len + 2)
    error("Identifier table exhausted\n");

  strcpy(IdentTable + IdentTableLen, name);
  IdentTableLen += len + 1;
  IdentTable[IdentTableLen++] = len + 1;

  return i;
}

int AddNumericIdent__(int n)
{
  char s[1 + 2 + (2 + CHAR_BIT * sizeof n) / 3];
  char *p = s + sizeof s;
  *--p = '\0';
  p = lab2str(p, n);
  *--p = '_';
  *--p = '_';
  return AddIdent(p);
}

int AddGotoLabel(char* name, int label)
{
  int i;
  for (i = 0; i < gotoLabCnt; i++)
  {
    if (!strcmp(IdentTable + gotoLabels[i][0], name))
    {
      if (gotoLabStat[i] & label)
        error("Redefinition of label '%s'\n", name);
      gotoLabStat[i] |= 2*!label + label;
      return gotoLabels[i][1];
    }
  }
  if (gotoLabCnt >= MAX_GOTO_LABELS)
    error("Goto table exhausted\n");
  gotoLabels[gotoLabCnt][0] = AddIdent(name);
  gotoLabels[gotoLabCnt][1] = LabelCnt++;
  gotoLabStat[gotoLabCnt] = 2*!label + label;
  return gotoLabels[gotoLabCnt++][1];
}

void UndoNonLabelIdents(int len)
{
  int i;
  IdentTableLen = len;
  for (i = 0; i < gotoLabCnt; i++)
    if (gotoLabels[i][0] >= len)
    {
      char* pfrom = IdentTable + gotoLabels[i][0];
      char* pto = IdentTable + IdentTableLen;
      int l = strlen(pfrom) + 2;
      memmove(pto, pfrom, l);
      IdentTableLen += l;
      gotoLabels[i][0] = pto - IdentTable;
    }
}

#ifndef NO_ANNOTATIONS
void DumpIdentTable(void)
{
  int i;
  puts2("");
  GenStartCommentLine(); printf2("Identifier table:\n");
  for (i = 0; i < IdentTableLen; )
  {
    GenStartCommentLine(); printf2("Ident %s\n", IdentTable + i);
    i += strlen(IdentTable + i) + 2;
  }
  GenStartCommentLine(); printf2("Bytes used: %d/%d\n\n", IdentTableLen, MAX_IDENT_TABLE_LEN);
}
#endif

char* rws[] =
{
  "break", "case", "char", "continue", "default", "do", "else",
  "extern", "for", "if", "int", "return", "signed", "sizeof",
  "static", "switch", "unsigned", "void", "while", "asm", "auto",
  "const", "double", "enum", "float", "goto", "inline", "long",
  "register", "restrict", "short", "struct", "typedef", "union",
  "volatile", "_Bool", "_Complex", "_Imaginary"
};

unsigned char rwtk[] =
{
  tokBreak, tokCase, tokChar, tokCont, tokDefault, tokDo, tokElse,
  tokExtern, tokFor, tokIf, tokInt, tokReturn, tokSigned, tokSizeof,
  tokStatic, tokSwitch, tokUnsigned, tokVoid, tokWhile, tok_Asm, tokAuto,
  tokConst, tokDouble, tokEnum, tokFloat, tokGoto, tokInline, tokLong,
  tokRegister, tokRestrict, tokShort, tokStruct, tokTypedef, tokUnion,
  tokVolatile, tok_Bool, tok_Complex, tok_Imagin
};

int GetTokenByWord(char* word)
{
  unsigned i;

  for (i = 0; i < sizeof rws / sizeof rws[0]; i++)
    if (!strcmp(rws[i], word))
      return rwtk[i];

  return tokIdent;
}

unsigned char tktk[] =
{
  tokEof,
  // Single-character operators and punctuators:
  '+', '-', '~', '*', '/', '%', '&', '|', '^', '!',
  '<', '>', '(', ')', '[', ']',
  '{', '}', '=', ',', ';', ':', '.', '?',
  // Multi-character operators and punctuators:
  tokLShift, tokLogAnd, tokEQ, tokLEQ, tokInc, tokArrow, tokAssignMul,
  tokAssignMod, tokAssignSub, tokAssignRSh, tokAssignXor,
  tokRShift, tokLogOr, tokNEQ, tokGEQ, tokDec, tokEllipsis,
  tokAssignDiv, tokAssignAdd, tokAssignLSh, tokAssignAnd, tokAssignOr,
  // Some of the above tokens get converted into these in the process:
  tokUnaryAnd, tokUnaryPlus, tokPostInc, tokPostAdd,
  tokULess, tokULEQ, tokURShift, tokUDiv, tokUMod, tokComma,
  tokUnaryStar, tokUnaryMinus, tokPostDec, tokPostSub,
  tokUGreater, tokUGEQ, tokAssignURSh, tokAssignUDiv, tokAssignUMod,
  // Helper (pseudo-)tokens:
  tokNumInt, tokLitStr, tokLocalOfs, tokNumUint, tokIdent, tokShortCirc,
  tokSChar, tokShort, tokLong, tokUChar, tokUShort, tokULong,
};

char* tks[] =
{
  "<EOF>",
  // Single-character operators and punctuators:
  "+", "-", "~", "*", "/", "%", "&", "|", "^", "!",
  "<", ">", "(", ")", "[", "]",
  "{", "}", "=", ",", ",", ":", ".", "?",
  // Multi-character operators and punctuators:
  "<<", "&&", "==", "<=", "++", "->", "*=",
  "%=", "-=", ">>=", "^=",
  ">>", "||", "!=", ">=", "--", "...",
  "/=", "+=", "<<=", "&=", "|=",
  // Some of the above tokens get converted into these in the process:
  "&u", "+u", "++p", "+=p",
  "<u", "<=u", ">>u", "/u", "%u", ",b",
  "*u", "-u", "--p", "-=p",
  ">u", ">=u", ">>=u", "/=u", "%=u",
  // Helper (pseudo-)tokens:
  "<NumInt>",  "<LitStr>", "<LocalOfs>", "<NumUint>", "<Ident>", "<ShortCirc>",
  "signed char", "short", "long", "unsigned char", "unsigned short", "unsigned long",
};

char* GetTokenName(int token)
{
  unsigned i;

/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */
/*
  switch (token)
  {
  case tokEof: return "<EOF>";
  // Single-character operators and punctuators:
  case '+': return "+";                  case '-': return "-";
  case '~': return "~";                  case '*': return "*";
  case '/': return "/";                  case '%': return "%";
  case '&': return "&";                  case '|': return "|";
  case '^': return "^";                  case '!': return "!";
  case '<': return "<";                  case '>': return ">";
  case '(': return "(";                  case ')': return ")";
  case '[': return "[";                  case ']': return "]";
  case '{': return "{";                  case '}': return "}";
  case '=': return "=";                  case ',': return ",";
  case ';': return ";";                  case ':': return ":";
  case '.': return ".";                  case '?': return "?";
  // Multi-character operators and punctuators:
  case tokLShift: return "<<";           case tokRShift: return ">>";
  case tokLogAnd: return "&&";           case tokLogOr: return "||";
  case tokEQ: return "==";               case tokNEQ: return "!=";
  case tokLEQ: return "<=";              case tokGEQ: return ">=";
  case tokInc: return "++";              case tokDec: return "--";
  case tokArrow: return "->";            case tokEllipsis: return "...";
  case tokAssignMul: return "*=";        case tokAssignDiv: return "/=";
  case tokAssignMod: return "%=";        case tokAssignAdd: return "+=";
  case tokAssignSub: return "-=";        case tokAssignLSh: return "<<=";
  case tokAssignRSh: return ">>=";       case tokAssignAnd: return "&=";
  case tokAssignXor: return "^=";        case tokAssignOr: return "|=";

  // Some of the above tokens get converted into these in the process:
  case tokUnaryAnd: return "&u";         case tokUnaryStar: return "*u";
  case tokUnaryPlus: return "+u";        case tokUnaryMinus: return "-u";
  case tokPostInc: return "++p";         case tokPostDec: return "--p";
  case tokPostAdd: return "+=p";         case tokPostSub: return "-=p";
  case tokULess: return "<u";            case tokUGreater: return ">u";
  case tokULEQ: return "<=u";            case tokUGEQ: return ">=u";
  case tokURShift: return ">>u";         case tokAssignURSh: return ">>=u";
  case tokUDiv: return "/u";             case tokAssignUDiv: return "/=u";
  case tokUMod: return "%u";             case tokAssignUMod: return "%=u";
  case tokComma: return ",b";

  // Helper (pseudo-)tokens:
  case tokNumInt: return "<NumInt>";     case tokNumUint: return "<NumUint>";
  case tokLitStr: return "<LitStr>";     case tokIdent: return "<Ident>";
  case tokLocalOfs: return "<LocalOfs>"; case tokShortCirc: return "<ShortCirc>";

  case tokSChar: return "signed char";   case tokUChar: return "unsigned char";
  case tokShort: return "short";         case tokUShort: return "unsigned short";
  case tokLong: return "long";           case tokULong: return "unsigned long";
  }
*/

  // Tokens other than reserved keywords:
  for (i = 0; i < sizeof tktk / sizeof tktk[0]; i++)
    if (tktk[i] == token)
      return tks[i];

  // Reserved keywords:
  for (i = 0; i < sizeof rws / sizeof rws[0]; i++)
    if (rwtk[i] == token)
      return rws[i];

  //error("Internal Error: GetTokenName(): Invalid token %d\n", token);
  errorInternal(1);
  return "";
}

int GetTokenValueInt(void)
{
  return TokenValueInt;
}

char* GetTokenValueString(void)
{
  return TokenValueString;
}

int GetTokenValueStringLength(void)
{
  return TokenStringLen;
}

char* GetTokenIdentName(void)
{
  return TokenIdentName;
}

int GetNextChar(void)
{
  int ch = EOF;

  if (FileCnt && Files[FileCnt - 1])
  {
    if ((ch = fgetc(Files[FileCnt - 1])) == EOF)
    {
      fclose(Files[FileCnt - 1]);
      Files[FileCnt - 1] = NULL;

      // store the last line/pos, they may still be needed later
      LineNos[FileCnt - 1] = LineNo;
      LinePoss[FileCnt - 1] = LinePos;

      // don't drop the file record just yet
    }
  }

  return ch;
}

void ShiftChar(void)
{
  if (CharQueueLen)
    memmove(CharQueue, CharQueue + 1, --CharQueueLen);

  // make sure there always are at least 3 chars in the queue
  while (CharQueueLen < 3)
  {
    int ch = GetNextChar();
    if (ch == EOF)
      ch = '\0';
    CharQueue[CharQueueLen++] = ch;
  }
}

void ShiftCharN(int n)
{
  while (n-- > 0)
  {
    ShiftChar();
    LinePos++;
  }
}

#ifndef NO_PREPROCESSOR
void IncludeFile(int quot)
{
  int nlen = strlen(TokenValueString);

  if (CharQueueLen != 3)
    //error("#include parsing error\n");
    errorInternal(2);

  if (FileCnt >= MAX_INCLUDES)
    error("Too many include files\n");

  // store the including file's position and buffered chars
  LineNos[FileCnt - 1] = LineNo;
  LinePoss[FileCnt - 1] = LinePos;
  memcpy(CharQueues[FileCnt - 1], CharQueue, CharQueueLen);

  // open the included file

  if (nlen > MAX_FILE_NAME_LEN)
    //error("File name too long\n");
    errorFileName();

  // DONE: differentiate between quot == '\"' and quot == '<'

  // first, try opening "file" in the current directory
  if (quot == '\"')
  {
    strcpy(FileNames[FileCnt], TokenValueString);
    Files[FileCnt] = fopen(FileNames[FileCnt], "r");
  }

  // next, iterate the search paths trying to open "file" or <file>
  if (Files[FileCnt] == NULL)
  {
    int i;
    for (i = 0; i < SearchPathsLen; )
    {
      int plen = strlen(SearchPaths + i);
      if (plen + 1 + nlen < MAX_FILE_NAME_LEN)
      {
        strcpy(FileNames[FileCnt], SearchPaths + i);
        strcpy(FileNames[FileCnt] + plen + 1, TokenValueString);
        // first, try '/' as a separator (Linux/Unix)
        FileNames[FileCnt][plen] = '/';
        if ((Files[FileCnt] = fopen(FileNames[FileCnt], "r")) == NULL)
        {
          // next, try '\\' as a separator (DOS/Windows)
          FileNames[FileCnt][plen] = '\\';
          Files[FileCnt] = fopen(FileNames[FileCnt], "r");
        }
        if (Files[FileCnt])
          break;
      }
      i += plen + 1;
    }
  }

  if (Files[FileCnt] == NULL)
  {
    //if (quot == '<' && !SearchPathsLen)
    //  error("Cannot open file \"%s\", include search path unspecified\n", TokenValueString);

    //error("Cannot open file \"%s\"\n", TokenValueString);
    errorFile(TokenValueString);
  }

  // reset line/pos and empty the char queue
  CharQueueLen = 0;
  LineNo = LinePos = 1;
  FileCnt++;

  // fill the char queue with file data
  ShiftChar();
}
#endif // #ifndef NO_PREPROCESSOR

int EndOfFiles(void)
{
  // if there are no including files, we're done
  if (!--FileCnt)
    return 1;

  // restore the including file's position and buffered chars
  LineNo = LineNos[FileCnt - 1];
  LinePos = LinePoss[FileCnt - 1];
  CharQueueLen = 3;
  memcpy(CharQueue, CharQueues[FileCnt - 1], CharQueueLen);

  return 0;
}

void SkipSpace(int SkipNewLines)
{
  char* p = CharQueue;

  while (*p != '\0')
  {
    if (strchr(" \t\f\v", *p))
    {
      ShiftCharN(1);
      continue;
    }

    if (strchr("\r\n", *p))
    {
      if (!SkipNewLines)
        return;

      if (*p == '\r' && p[1] == '\n')
        ShiftChar();

      ShiftChar();
      LineNo++;
      LinePos = 1;
      continue;
    }

#ifndef NO_PREPROCESSOR
    if (*p == '/')
    {
      if (p[1] == '/')
      {
        // // comment
        ShiftCharN(2);
        while (!strchr("\r\n", *p))
          ShiftCharN(1);
        continue;
      }
      else if (p[1] == '*')
      {
        // /**/ comment
        ShiftCharN(2);
        while (*p != '\0' && !(*p == '*' && p[1] == '/'))
        {
          if (strchr("\r\n", *p))
          {
            if (!SkipNewLines)
              error("Invalid comment\n");

            if (*p == '\r' && p[1] == '\n')
              ShiftChar();

            ShiftChar();
            LineNo++;
            LinePos = 1;
          }
          else
          {
            ShiftCharN(1);
          }
        }
        if (*p == '\0')
          error("Invalid comment\n");
        ShiftCharN(2);
        continue;
      }
    } // endof if (*p == '/')
#endif

    break;
  } // endof while (*p != '\0')
}

#ifndef NO_PREPROCESSOR
void SkipLine(void)
{
  char* p = CharQueue;

  while (*p != '\0')
  {
    if (strchr("\r\n", *p))
    {
      if (*p == '\r' && p[1] == '\n')
        ShiftChar();

      ShiftChar();
      LineNo++;
      LinePos = 1;
      break;
    }
    else
    {
      ShiftCharN(1);
    }
  }
}
#endif

void GetIdent(void)
{
  char* p = CharQueue;

  if (*p != '_' && !isalpha(*p & 0xFFu))
    error("Identifier expected\n");

  if (*p == 'L' &&
      (p[1] == '\'' || p[1] == '\"'))
    //error("Wide characters and strings not supported\n");
    errorChrStr();

  TokenIdentNameLen = 0;
  TokenIdentName[TokenIdentNameLen++] = *p;
  TokenIdentName[TokenIdentNameLen] = '\0';
  ShiftCharN(1);

  while (*p == '_' || isalnum(*p & 0xFFu))
  {
    if (TokenIdentNameLen == MAX_IDENT_LEN)
      error("Identifier too long '%s'\n", TokenIdentName);
    TokenIdentName[TokenIdentNameLen++] = *p;
    TokenIdentName[TokenIdentNameLen] = '\0';
    ShiftCharN(1);
  }
}

void GetString(char terminator, int SkipNewLines)
{
  char* p = CharQueue;
  char ch;

  TokenStringLen = 0;
  TokenValueString[TokenStringLen] = '\0';

  for (;;)
  {
    ShiftCharN(1);
    while (!(*p == terminator || strchr("\a\b\f\n\r\t\v", *p)))
    {
      ch = *p;
      if (ch == '\\')
      {
        ShiftCharN(1);
        ch = *p;
        if (strchr("\a\b\f\n\r\t\v", ch))
          break;
        switch (ch)
        {
        case 'a': ch = '\a'; ShiftCharN(1); break;
        case 'b': ch = '\b'; ShiftCharN(1); break;
        case 'f': ch = '\f'; ShiftCharN(1); break;
        case 'n': ch = '\n'; ShiftCharN(1); break;
        case 'r': ch = '\r'; ShiftCharN(1); break;
        case 't': ch = '\t'; ShiftCharN(1); break;
        case 'v': ch = '\v'; ShiftCharN(1); break;
        // DONE: \nnn, \xnn
        case 'x':
          {
            // hexadecimal character codes \xN+
            int cnt = 0;
            int c = 0;
            ShiftCharN(1);
            while (*p != '\0' && (isdigit(*p & 0xFFu) || strchr("abcdefABCDEF", *p)))
            {
              c = (c * 16) & 0xFF;
              if (*p >= 'a') c += *p - 'a' + 10;
              else if (*p >= 'A') c += *p - 'A' + 10;
              else c += *p - '0';
              ShiftCharN(1);
              cnt++;
            }
            if (!cnt)
              //error("Unsupported or invalid character/string constant\n");
              errorChrStr();
            c -= (c >= 0x80 && CHAR_MIN) * 0x100;
            ch = c;
          }
          break;
        default:
          if (*p >= '0' && *p <= '7')
          {
            // octal character codes \N+
            int cnt = 0;
            int c = 0;
            while (*p >= '0' && *p <= '7')
            {
              c = (c * 8) & 0xFF;
              c += *p - '0';
              ShiftCharN(1);
              cnt++;
            }
            if (!cnt)
              //error("Unsupported or invalid character/string constant\n");
              errorChrStr();
            c -= (c >= 0x80 && CHAR_MIN) * 0x100;
            ch = c;
          }
          else
          {
            ShiftCharN(1);
          }
          break;
        } // endof switch (ch)
      } // endof if (ch == '\\')
      else
      {
        ShiftCharN(1);
      }

      if (terminator == '\'')
      {
        if (TokenStringLen != 0)
          //error("Character constant too long\n");
          errorChrStr();
      }
      else if (TokenStringLen == MAX_STRING_LEN)
        error("String literal too long\n");

      TokenValueString[TokenStringLen++] = ch;
      TokenValueString[TokenStringLen] = '\0';
    } // endof while (!(*p == '\0' || *p == terminator || strchr("\a\b\f\n\r\t\v", *p)))

    if (*p != terminator)
      //error("Unsupported or invalid character/string constant\n");
      errorChrStr();

    ShiftCharN(1);

    if (terminator != '\"')
      break; // done with character constants

    // Concatenate this string literal with all following ones, if any
    SkipSpace(SkipNewLines);
    if (*p != '\"')
      break; // nothing to concatenate with
    // Continue consuming string characters
  } // endof for (;;)
}

#ifndef NO_PREPROCESSOR
void pushPrep(int NoSkip)
{
  if (PrepSp >= PREP_STACK_SIZE)
    error("Too many #if(n)def's\n");
  PrepStack[PrepSp][0] = PrepDontSkipTokens;
  PrepStack[PrepSp++][1] = NoSkip;
  PrepDontSkipTokens &= NoSkip;
}

int popPrep(void)
{
  if (PrepSp <= 0)
    error("#else or #endif without #if(n)def\n");
  PrepDontSkipTokens = PrepStack[--PrepSp][0];
  return PrepStack[PrepSp][1];
}
#endif

int GetNumber(void)
{
  char* p = CharQueue;
  int ch = *p;
  unsigned n = 0;
  int type = 0;
  int uSuffix = 0;
#ifdef CAN_COMPILE_32BIT
  int lSuffix = 0;
#endif
  char* eTooBig = "Constant too big\n";

  if (ch == '0')
  {
    // this is either an octal or a hex constant
    type = 'o';
    ShiftCharN(1);
    if ((ch = *p) == 'x' || ch == 'X')
    {
      // this is a hex constant
      int cnt = 0;
      ShiftCharN(1);
      while ((ch = *p) != '\0' && (isdigit(ch & 0xFFu) || strchr("abcdefABCDEF", ch)))
      {
        if (ch >= 'a') ch -= 'a' - 10;
        else if (ch >= 'A') ch -= 'A' - 10;
        else ch -= '0';
        if (PrepDontSkipTokens && (n * 16 / 16 != n || n * 16 + ch < n * 16))
          error(eTooBig);
        n = n * 16 + ch;
        ShiftCharN(1);
        cnt++;
      }
      if (!cnt)
        error("Invalid hexadecimal constant\n");
      type = 'h';
    }
    // this is an octal constant
    else while ((ch = *p) >= '0' && ch <= '7')
    {
      ch -= '0';
      if (PrepDontSkipTokens && (n * 8 / 8 != n || n * 8 + ch < n * 8))
        error(eTooBig);
      n = n * 8 + ch;
      ShiftCharN(1);
    }
  }
  // this is a decimal constant
  else
  {
    type = 'd';
    while ((ch = *p) >= '0' && ch <= '9')
    {
      ch -= '0';
      if (PrepDontSkipTokens && (n * 10 / 10 != n || n * 10 + ch < n * 10))
        error(eTooBig);
      n = n * 10 + ch;
      ShiftCharN(1);
    }
  }

  // possible combinations of suffixes:
  //   none
  //   U
  //   UL
  //   L
  //   LU
  if ((ch = *p) == 'u' || ch == 'U')
  {
    uSuffix = 1;
    ShiftCharN(1);
  }
#ifdef CAN_COMPILE_32BIT
  if ((ch = *p) == 'l' || ch == 'L')
  {
    lSuffix = 1;
    ShiftCharN(1);
    if (!uSuffix && ((ch = *p) == 'u' || ch == 'U'))
    {
      uSuffix = 1;
      ShiftCharN(1);
    }
  }
#endif

  if (!PrepDontSkipTokens)
  {
    // Don't fail on big constants when skipping tokens under #if
    TokenValueInt = 0;
    return tokNumInt;
  }

  // Ensure the constant fits into 16(32) bits
  if ((SizeOfWord == 2 && n >> 8 >> 8) || // equiv. to SizeOfWord == 2 && n > 0xFFFF
#ifdef CAN_COMPILE_32BIT
      (SizeOfWord == 2 && lSuffix) || // long (which must have at least 32 bits) isn't supported in 16-bit models
#endif
      (SizeOfWord == 4 && n >> 8 >> 12 >> 12)) // equiv. to SizeOfWord == 4 && n > 0xFFFFFFFF
    error("Constant too big for %d-bit type\n", SizeOfWord * 8);

  TokenValueInt = uint2int(n);

  // Unsuffixed (with 'u') integer constants (octal, decimal, hex)
  // fitting into 15(31) out of 16(32) bits are signed ints
  if (!uSuffix &&
      ((SizeOfWord == 2 && !(n >> 8 >> 7)) || // equiv. to SizeOfWord == 2 && n <= 0x7FFF
       (SizeOfWord == 4 && !(n >> 8 >> 12 >> 11)))) // equiv. to SizeOfWord == 4 && n <= 0x7FFFFFFF
    return tokNumInt;

  // Unlike octal and hex constants, decimal constants are always
  // a signed type. Error out when a decimal constant doesn't fit
  // into an int since currently there's no next bigger signed type
  // (e.g. long) to use instead of int.
  if (!uSuffix && type == 'd')
    error("Constant too big for %d-bit signed type\n", SizeOfWord * 8);

  return tokNumUint;
}

int GetTokenInner(void)
{
  char* p = CharQueue;
  int ch = *p;

  // these single-character tokens/operators need no further processing
  if (strchr(",;:()[]{}~?", ch))
  {
    ShiftCharN(1);
    return ch;
  }

  // parse multi-character tokens/operators

  // DONE: other assignment operators
  switch (ch)
  {
  case '+':
    if (p[1] == '+') { ShiftCharN(2); return tokInc; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignAdd; }
    ShiftCharN(1); return ch;
  case '-':
    if (p[1] == '-') { ShiftCharN(2); return tokDec; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignSub; }
    if (p[1] == '>') { ShiftCharN(2); return tokArrow; }
    ShiftCharN(1); return ch;
  case '!':
    if (p[1] == '=') { ShiftCharN(2); return tokNEQ; }
    ShiftCharN(1); return ch;
  case '=':
    if (p[1] == '=') { ShiftCharN(2); return tokEQ; }
    ShiftCharN(1); return ch;
  case '<':
    if (p[1] == '=') { ShiftCharN(2); return tokLEQ; }
    if (p[1] == '<') { ShiftCharN(2); if (p[0] != '=') return tokLShift; ShiftCharN(1); return tokAssignLSh; }
    ShiftCharN(1); return ch;
  case '>':
    if (p[1] == '=') { ShiftCharN(2); return tokGEQ; }
    if (p[1] == '>') { ShiftCharN(2); if (p[0] != '=') return tokRShift; ShiftCharN(1); return tokAssignRSh; }
    ShiftCharN(1); return ch;
  case '&':
    if (p[1] == '&') { ShiftCharN(2); return tokLogAnd; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignAnd; }
    ShiftCharN(1); return ch;
  case '|':
    if (p[1] == '|') { ShiftCharN(2); return tokLogOr; }
    if (p[1] == '=') { ShiftCharN(2); return tokAssignOr; }
    ShiftCharN(1); return ch;
  case '^':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignXor; }
    ShiftCharN(1); return ch;
  case '.':
    if (p[1] == '.' && p[2] == '.') { ShiftCharN(3); return tokEllipsis; }
    ShiftCharN(1); return ch;
  case '*':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignMul; }
    ShiftCharN(1); return ch;
  case '%':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignMod; }
    ShiftCharN(1); return ch;
  case '/':
    if (p[1] == '=') { ShiftCharN(2); return tokAssignDiv; }
    // if (p[1] == '/' || p[1] == '*') { SkipSpace(1); continue; } // already taken care of
    ShiftCharN(1); return ch;
  }

  // DONE: hex and octal constants
  if (isdigit(ch & 0xFFu))
    return GetNumber();

  // parse character and string constants
  if (ch == '\'' || ch == '\"')
  {
    GetString(ch, 1);

    if (ch == '\'')
    {
      if (TokenStringLen != 1)
        //error("Character constant too short\n");
        errorChrStr();

      TokenValueInt = TokenValueString[0] & 0xFF;
      TokenValueInt -= (CharIsSigned && TokenValueInt >= 0x80) * 0x100;
      return tokNumInt;
    }

    return tokLitStr;
  } // endof if (ch == '\'' || ch == '\"')

  return tokEof;
}

// TBD??? implement file I/O for input source code and output code (use fxn ptrs/wrappers to make librarization possible)
// DONE: support string literals
int GetToken(void)
{
  char* p = CharQueue;
  int ch;
  int tok;

  for (;;)
  {
/* +-~* /% &|^! << >> && || < <= > >= == !=  () *[] ++ -- = += -= ~= *= /= %= &= |= ^= <<= >>= {} ,;: -> ... */

    // skip white space and comments
    SkipSpace(1);

    if ((ch = *p) == '\0')
    {
      // done with the current file, drop its record,
      // pick up the including files (if any) or terminate
      if (EndOfFiles())
        break;
      continue;
    }

    if ((tok = GetTokenInner()) != tokEof)
    {
      if (PrepDontSkipTokens)
        return tok;
      continue;
    }

    // parse identifiers and reserved keywords
    if (ch == '_' || isalpha(ch & 0xFFu))
    {
#ifndef NO_PREPROCESSOR
      int midx;
#endif

      GetIdent();

      if (!PrepDontSkipTokens)
        continue;

      tok = GetTokenByWord(TokenIdentName);

#ifndef NO_PREPROCESSOR
      // TBD!!! think of expanding macros in the context of concatenating string literals,
      // maybe factor out this piece of code
      if ((midx = FindMacro(TokenIdentName)) >= 0)
      {
        // this is a macro identifier, need to expand it
        int len = MacroTable[midx];

        if (MAX_CHAR_QUEUE_LEN - CharQueueLen < len + 1)
          error("Too long expansion of macro '%s'\n", TokenIdentName);

        memmove(CharQueue + len + 1, CharQueue, CharQueueLen);

        memcpy(CharQueue, MacroTable + midx + 1, len);
        CharQueue[len] = ' '; // space to avoid concatenation

        CharQueueLen += len + 1;

        continue;
      }
#endif

      // treat keywords auto, const, register, restrict and volatile as white space for now
      if (tok == tokConst || tok == tokVolatile ||
          tok == tokAuto || tok == tokRegister ||
          tok == tokRestrict)
        continue;

      return tok;
    } // endof if (ch == '_' || isalpha(ch))

    // parse preprocessor directives
    if (ch == '#')
    {
      int line = 0;

      ShiftCharN(1);

      // Skip space
      SkipSpace(0);

      // Allow # not followed by a directive
      if (strchr("\r\n", *p))
        continue;

      // Get preprocessor directive
      if (isdigit(*p & 0xFFu))
      {
        // gcc-style #line directive without "line"
        line = 1;
      }
      else
      {
        GetIdent();
        if (!strcmp(TokenIdentName, "line"))
        {
          // C89-style #line directive
          SkipSpace(0);
          if (!isdigit(*p & 0xFFu))
            errorDirective();
          line = 1;
        }
      }

      if (line)
      {
        // Support for external, gcc-like, preprocessor output:
        //   # linenum filename flags
        //
        // no flags, flag = 1 -- start of a file
        //           flag = 2 -- return to a file after #include
        //        other flags -- uninteresting

        // DONE: should also support the following C89 form:
        // # line linenum filename-opt

        if (GetNumber() != tokNumInt)
          //error("Invalid line number in preprocessor output\n");
          errorDirective();
        line = GetTokenValueInt();

        SkipSpace(0);

        if (*p == '\"' || *p == '<')
        {
          if (*p == '\"')
            GetString('\"', 0);
          else
            GetString('>', 0);

          if (strlen(GetTokenValueString()) > MAX_FILE_NAME_LEN)
            //error("File name too long in preprocessor output\n");
            errorFileName();
          strcpy(FileNames[FileCnt - 1], GetTokenValueString());
        }

        // Ignore gcc-style #line's flags, if any
        while (!strchr("\r\n", *p))
          ShiftCharN(1);
        
        LineNo = line - 1; // "line" is the number of the next line
        LinePos = 1;

        continue;
      } // endof if (line)

#ifndef NO_PPACK
      if (!strcmp(TokenIdentName, "pragma"))
      {
        int canHaveNumber = 1, hadNumber = 0;

        if (!PrepDontSkipTokens)
        {
          while (!strchr("\r\n", *p))
            ShiftCharN(1);
          continue;
        }

        SkipSpace(0);
        GetIdent();
        if (strcmp(TokenIdentName, "pack"))
          errorDirective();
        // TBD??? fail if inside a structure declaration
        SkipSpace(0);
        if (*p == '(')
          ShiftCharN(1);
        SkipSpace(0);

        if (*p == 'p')
        {
          GetIdent();
          if (!strcmp(TokenIdentName, "push"))
          {
            SkipSpace(0);
            if (*p == ',')
            {
              ShiftCharN(1);
              SkipSpace(0);
              if (!isdigit(*p & 0xFFu) || GetNumber() != tokNumInt)
                errorDirective();
              hadNumber = 1;
            }
            if (PragmaPackSp >= PPACK_STACK_SIZE)
              error("#pragma pack stack overflow\n");
            PragmaPackValues[PragmaPackSp++] = PragmaPackValue;
          }
          else if (!strcmp(TokenIdentName, "pop"))
          {
            if (PragmaPackSp <= 0)
              error("#pragma pack stack underflow\n");
            PragmaPackValue = PragmaPackValues[--PragmaPackSp];
          }
          else
            errorDirective();
          SkipSpace(0);
          canHaveNumber = 0;
        }

        if (canHaveNumber && isdigit(*p & 0xFFu))
        {
          if (GetNumber() != tokNumInt)
            errorDirective();
          hadNumber = 1;
          SkipSpace(0);
        }

        if (hadNumber)
        {
          PragmaPackValue = GetTokenValueInt();
          if (PragmaPackValue <= 0 ||
              PragmaPackValue > SizeOfWord ||
              PragmaPackValue & (PragmaPackValue - 1))
            error("Invalid alignment value\n");
        }
        else if (canHaveNumber)
        {
          PragmaPackValue = SizeOfWord;
        }

        if (*p != ')')
          errorDirective();
        ShiftCharN(1);

        SkipSpace(0);
        if (!strchr("\r\n", *p))
          errorDirective();
        continue;
      }
#endif

#ifndef NO_PREPROCESSOR
      if (!strcmp(TokenIdentName, "define"))
      {
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();

        if (!PrepDontSkipTokens)
        {
          SkipSpace(0);
          while (!strchr("\r\n", *p))
            ShiftCharN(1);
          continue;
        }

        if (FindMacro(TokenIdentName) >= 0)
          error("Redefinition of macro '%s'\n", TokenIdentName);
        if (*p == '(')
          //error("Unsupported type of macro '%s'\n", TokenIdentName);
          errorDirective();

        AddMacroIdent(TokenIdentName);

        SkipSpace(0);

        // accumulate the macro expansion text
        while (!strchr("\r\n", *p))
        {
          AddMacroExpansionChar(*p);
          ShiftCharN(1);
          if (*p != '\0' && (strchr(" \t", *p) || (*p == '/' && (p[1] == '/' || p[1] == '*'))))
          {
            SkipSpace(0);
            AddMacroExpansionChar(' ');
          }
        }
        AddMacroExpansionChar('\0');

        continue;
      }
      else if (!strcmp(TokenIdentName, "undef"))
      {
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();

        if (PrepDontSkipTokens)
          UndefineMacro(TokenIdentName);

        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        continue;
      }
      else if (!strcmp(TokenIdentName, "include"))
      {
        int quot;

        // Skip space and get file name
        SkipSpace(0);

        quot = *p;
        if (*p == '\"')
          GetString('\"', 0);
        else if (*p == '<')
          GetString('>', 0);
        else
          //error("Invalid file name\n");
          errorFileName();

        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Unsupported or invalid preprocessor directive\n");
          errorDirective();

        if (PrepDontSkipTokens)
          IncludeFile(quot);

        continue;
      }
      else if (!strcmp(TokenIdentName, "ifdef"))
      {
        int def;
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();
        def = FindMacro(TokenIdentName) >= 0;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        pushPrep(def);
        continue;
      }
      else if (!strcmp(TokenIdentName, "ifndef"))
      {
        int def;
        // Skip space and get macro name
        SkipSpace(0);
        GetIdent();
        def = FindMacro(TokenIdentName) >= 0;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        pushPrep(!def);
        continue;
      }
      else if (!strcmp(TokenIdentName, "else"))
      {
        int def;
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        def = popPrep();
        if (def >= 2)
          error("#else or #endif without #if(n)def\n");
        pushPrep(2 + !def); // #else works in opposite way to its preceding #if(n)def
        continue;
      }
      else if (!strcmp(TokenIdentName, "endif"))
      {
        SkipSpace(0);
        if (!strchr("\r\n", *p))
          //error("Invalid preprocessor directive\n");
          errorDirective();
        popPrep();
        continue;
      }

      if (!PrepDontSkipTokens)
      {
        // If skipping code and directives under #ifdef/#ifndef/#else,
        // ignore unsupported directives #if, #elif, #error (no error checking)
        if (!strcmp(TokenIdentName, "if"))
          pushPrep(0);
        else if (!strcmp(TokenIdentName, "elif"))
          popPrep(), pushPrep(0);
        SkipLine();
        continue;
      }
#endif // #ifndef NO_PREPROCESSOR

      //error("Unsupported or invalid preprocessor directive\n");
      errorDirective();
    } // endof if (ch == '#')

    error("Invalid or unsupported character with code 0x%02X\n", *p & 0xFFu);
  } // endof for (;;)

  return tokEof;
}

#ifdef MIPS
#ifndef CAN_COMPILE_32BIT
#error MIPS target requires a 32-bit compiler
#endif
#include "cgmips.c"
#else
#include "cgx86.c"
#endif // #ifdef MIPS

// expr.c code

void push2(int v, int v2)
{
  if (sp >= STACK_SIZE)
    //error("expression stack overflow!\n");
    errorLongExpr();
  stack[sp][0] = v;
  stack[sp++][1] = v2;
}

void push(int v)
{
  push2(v, 0);
}

int stacktop()
{
  if (sp == 0)
    //error("expression stack underflow!\n");
    errorInternal(3);
  return stack[sp - 1][0];
}

int pop2(int* v2)
{
  int v = stacktop();
  *v2 = stack[sp - 1][1];
  sp--;
  return v;
}

int pop()
{
  int v2;
  return pop2(&v2);
}

void ins2(int pos, int v, int v2)
{
  if (sp >= STACK_SIZE)
    //error("expression stack overflow!\n");
    errorLongExpr();
  memmove(&stack[pos + 1], &stack[pos], sizeof(stack[0]) * (sp - pos));
  stack[pos][0] = v;
  stack[pos][1] = v2;
  sp++;
}

void ins(int pos, int v)
{
  ins2(pos, v, 0);
}

void del(int pos, int cnt)
{
  memmove(stack[pos],
          stack[pos + cnt],
          sizeof(stack[0]) * (sp - (pos + cnt)));
  sp -= cnt;
}

void pushop2(int v, int v2)
{
  if (opsp >= OPERATOR_STACK_SIZE)
    //error("operator stack overflow!\n");
    errorLongExpr();
  opstack[opsp][0] = v;
  opstack[opsp++][1] = v2;
}

void pushop(int v)
{
  pushop2(v, 0);
}

int opstacktop()
{
  if (opsp == 0)
    //error("operator stack underflow!\n");
    errorInternal(4);
  return opstack[opsp - 1][0];
}

int popop2(int* v2)
{
  int v = opstacktop();
  *v2 = opstack[opsp - 1][1];
  opsp--;
  return v;
}

int popop()
{
  int v2;
  return popop2(&v2);
}

int isop(int tok)
{
  switch (tok)
  {
  case '!':
  case '~':
  case '&':
  case '*':
  case '/': case '%':
  case '+': case '-':
  case '|': case '^':
  case '<': case '>':
  case '=':
  case tokLogOr: case tokLogAnd:
  case tokEQ: case tokNEQ:
  case tokLEQ: case tokGEQ:
  case tokLShift: case tokRShift:
  case tokInc: case tokDec:
  case tokSizeof:
  case tokAssignMul: case tokAssignDiv: case tokAssignMod:
  case tokAssignAdd: case tokAssignSub:
  case tokAssignLSh: case tokAssignRSh:
  case tokAssignAnd: case tokAssignXor: case tokAssignOr:
  case tokComma:
  case '?':
    return 1;
  default:
    return 0;
  }
}

int isunary(int tok)
{
  return tok == '!' || tok == '~' || tok == tokInc || tok == tokDec || tok == tokSizeof;
}

int preced(int tok)
{
  switch (tok)
  {
  case '*': case '/': case '%': return 13;
  case '+': case '-': return 12;
  case tokLShift: case tokRShift: return 11;
  case '<': case '>': case tokLEQ: case tokGEQ: return 10;
  case tokEQ: case tokNEQ: return 9;
  case '&': return 8;
  case '^': return 7;
  case '|': return 6;
  case tokLogAnd: return 5;
  case tokLogOr: return 4;
  case '?': case ':': return 3;
  case '=':
  case tokAssignMul: case tokAssignDiv: case tokAssignMod:
  case tokAssignAdd: case tokAssignSub:
  case tokAssignLSh: case tokAssignRSh:
  case tokAssignAnd: case tokAssignXor: case tokAssignOr:
    return 2;
  case tokComma:
    return 1;
  }
  return 0;
}

int precedGEQ(int lfttok, int rhttok)
{
  // DONE: rethink the comma operator as it could be implemented similarly
  // DONE: is this correct:???
  int pl = preced(lfttok);
  int pr = preced(rhttok);
  // ternary/conditional operator ?: is right-associative
  if (pl == 3 && pr >= 3)
    pl = 0;
  // assignment is right-associative
  if (pl == 2 && pr >= 2)
    pl = 0;
  return pl >= pr;
}

int expr(int tok, int* gotUnary, int commaSeparator);

char* lab2str(char* p, int n)
{
  do
  {
    *--p = '0' + n % 10;
    n /= 10;
  } while (n);

  return p;
}

int exprUnary(int tok, int* gotUnary, int commaSeparator, int argOfSizeOf)
{
  int decl = 0;
  *gotUnary = 0;

  if (isop(tok) && (isunary(tok) || strchr("&*+-", tok)))
  {
    int lastTok = tok;
    tok = exprUnary(GetToken(), gotUnary, commaSeparator, lastTok == tokSizeof);
    if (!*gotUnary)
      //error("exprUnary(): primary expression expected after token %s\n", GetTokenName(lastTok));
      errorUnexpectedToken(tok);
    switch (lastTok)
    {
    // DONE: remove all collapsing of all unary operators.
    // It's wrong because type checking must occur before any optimizations.
    // WRONG: DONE: collapse alternating & and * (e.g. "*&*&x" "&*&*x")
    // WRONGISH: DONE: replace prefix ++/-- with +=1/-=1
    case '&':
      push(tokUnaryAnd);
      break;
    case '*':
      push(tokUnaryStar);
      break;
    case '+':
      push(tokUnaryPlus);
      break;
    case '-':
      push(tokUnaryMinus);
      break;
    case '!':
      // replace "!" with "== 0"
      push(tokNumInt);
      push(tokEQ);
      break;
    default:
      push(lastTok);
      break;
    }
  }
  else
  {
    int inspos = sp;

    if (tok == tokNumInt || tok == tokNumUint)
    {
      push2(tok, GetTokenValueInt());
      *gotUnary = 1;
      tok = GetToken();
    }
    else if (tok == tokLitStr)
    {
      int lbl = (LabelCnt += 2) - 2; // 1 extra label for the jump over the string
      int len, id;
      char s[1 + (2 + CHAR_BIT * sizeof lbl) / 3];
      char *p = s + sizeof s;

      // imitate definition: char #[len] = "...";

      AddString(lbl, GetTokenValueString(), len = 1 + GetTokenValueStringLength());

      *--p = '\0';
      p = lab2str(p, lbl);

      // DONE: can this break incomplete yet declarations???, e.g.: int x[sizeof("az")][5];
      PushSyntax2(tokIdent, id = AddIdent(p));
      PushSyntax('[');
      PushSyntax2(tokNumUint, len);
      PushSyntax(']');
      PushSyntax(tokChar);

      push2(tokIdent, id);
      *gotUnary = 1;
      tok = GetToken();
    }
    else if (tok == tokIdent)
    {
      push2(tok, AddIdent(GetTokenIdentName()));
      *gotUnary = 1;
      tok = GetToken();
    }
    else if (tok == '(')
    {
      tok = GetToken();
      decl = TokenStartsDeclaration(tok, 1);

      if (decl)
      {
        int synPtr;
        int lbl = LabelCnt++;
        char s[1 + (2 + CHAR_BIT * sizeof lbl) / 3 + sizeof "<something>" - 1];
        char *p = s + sizeof s;

        tok = ParseDecl(tok, NULL, !argOfSizeOf, 0);
        if (tok != ')')
          //error("exprUnary(): ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        synPtr = FindSymbol("<something>");

        // Rename "<something>" to "<something#>", where # is lbl.
        // This makes the nameless declaration uniquely identifiable by name.

        *--p = '\0';
        *--p = ")>"[argOfSizeOf]; // differentiate casts (something#) from not casts <something#>

        p = lab2str(p, lbl);

        p -= sizeof "<something>" - 2 - 1;
        memcpy(p, "something", sizeof "something" - 1);

        *--p = "(<"[argOfSizeOf]; // differentiate casts (something#) from not casts <something#>

        SyntaxStack[synPtr][1] = AddIdent(p);
        tok = GetToken();
        if (argOfSizeOf)
        {
          // expression: sizeof(type)
          *gotUnary = 1;
        }
        else
        {
          // unary type cast operator: (type)
          decl = 0;
          tok = exprUnary(tok, gotUnary, commaSeparator, 0);
          if (!*gotUnary)
            //error("exprUnary(): primary expression expected after '(type)'\n");
            errorUnexpectedToken(tok);
        }
        push2(tokIdent, SyntaxStack[synPtr][1]);
      }
      else
      {
        tok = expr(tok, gotUnary, 0);
        if (tok != ')')
          //error("exprUnary(): ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        if (!*gotUnary)
          //error("exprUnary(): primary expression expected in '()'\n");
          errorUnexpectedToken(tok);
        tok = GetToken();
      }
    }

    while (*gotUnary && !decl)
    {
      // DONE: f(args1)(args2) and the like: need stack order: args2, args1, f, (), ()
      // DONE: reverse the order of evaluation of groups of args in
      //       f(args1)(args2)(args3)
      // DONE: reverse the order of function argument evaluation for variadic functions
      //       we want 1st arg to be the closest to the stack top.
      // DONE: (args)[index] can be repeated interchangeably indefinitely
      // DONE: (expr)() & (expr)[]
      // DONE: [index] can be followed by ++/--, which can be followed by [index] and so on...
      // DONE: postfix ++/-- & differentiate from prefix ++/--

      if (tok == '(')
      {
        int acnt = 0;
        ins(inspos, '(');
        for (;;)
        {
          int pos2 = sp;

          tok = GetToken();
          tok = expr(tok, gotUnary, 1);

          // Reverse the order of argument evaluation, which is important for
          // variadic functions like printf():
          // we want 1st arg to be the closest to the stack top.
          // This also reverses the order of evaluation of all groups of
          // arguments.
          while (pos2 < sp)
          {
            // TBD??? not quite efficient
            int v, v2;
            v = pop2(&v2);
            ins2(inspos + 1, v, v2);
            pos2++;
          }

          if (tok == ',')
          {
            if (!*gotUnary)
              //error("exprUnary(): primary expression (fxn argument) expected before ','\n");
              errorUnexpectedToken(tok);
            acnt++;
            ins(inspos + 1, ','); // helper argument separator (hint for expression evaluator)
            continue; // off to next arg
          }
          if (tok == ')')
          {
            if (acnt && !*gotUnary)
              //error("exprUnary(): primary expression (fxn argument) expected between ',' and ')'\n");
              errorUnexpectedToken(tok);
            *gotUnary = 1; // don't fail for 0 args in ()
            break; // end of args
          }
          // DONE: think of inserting special arg pseudo tokens for verification purposes
          //error("exprUnary(): ',' or ')' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        } // endof for(;;) for fxn args
        push(')');
      }
      else if (tok == '[')
      {
        tok = GetToken();
        tok = expr(tok, gotUnary, 0);
        if (!*gotUnary)
          //error("exprUnary(): primary expression expected in '[]'\n");
          errorUnexpectedToken(tok);
        if (tok != ']')
          //error("exprUnary(): ']' expected, unexpected token %s\n", GetTokenName(tok));
          errorUnexpectedToken(tok);
        // TBD??? add implicit casts to size_t of array indicies.
        // E1[E2] -> *(E1 + E2)
        // push('[');
        push('+');
        push(tokUnaryStar);
      }
      // WRONG: DONE: replace postfix ++/-- with (+=1)-1/(-=1)+1
      else if (tok == tokInc)
      {
        push(tokPostInc);
      }
      else if (tok == tokDec)
      {
        push(tokPostDec);
      }
      else if (tok == '.' || tok == tokArrow)
      {
        // transform a.b into (&a)->b
        if (tok == '.')
          push(tokUnaryAnd);
        tok = GetToken();
        if (tok != tokIdent)
          errorUnexpectedToken(tok);
        push2(tok, AddIdent(GetTokenIdentName()));
        // "->" in "a->b" will function as "+" in "*(type_of_b*)((char*)a + offset_of_b_in_a)"
        push(tokArrow);
        push(tokUnaryStar);
      }
      else
      {
        break;
      }
      tok = GetToken();
    } // endof while (*gotUnary)
  }

  if (tok == ',' && !commaSeparator)
    tok = tokComma;

  return tok;
}

int expr(int tok, int* gotUnary, int commaSeparator)
{
  *gotUnary = 0;

  pushop(tokEof);

  tok = exprUnary(tok, gotUnary, commaSeparator, 0);

  while (tok != tokEof && strchr(",;:)]}", tok) == NULL && *gotUnary)
  {
    if (isop(tok) && !isunary(tok))
    {
      //int lastTok = tok;

      while (precedGEQ(opstacktop(), tok))
      {
        int v, v2;
        int c = 0;
        // move ?expr: as a whole to the expression stack as "expr?"
        do
        {
          v = popop2(&v2);
          if (v != ':')
            push2(v, v2);
          c += (v == ':') - (v == '?');
        } while (c);
      }

      // here: preced(postacktop()) < preced(tok)
      pushop(tok);

      // treat the ternary/conditional operator ?expr: as a pseudo binary operator
      if (tok == '?')
      {
        int ssp = sp;

        tok = expr(GetToken(), gotUnary, 0);
        if (!*gotUnary || tok != ':')
          errorUnexpectedToken(tok);

        // move ?expr: as a whole to the operator stack
        // this is beautiful and ugly at the same time
        while (sp > ssp)
        {
          int v, v2;
          v = pop2(&v2);
          pushop2(v, v2);
        }

        pushop(tok);
      }

      tok = exprUnary(GetToken(), gotUnary, commaSeparator, 0);
      // DONE: figure out a check to see if exprUnary() fails to add a rhs operand
      if (!*gotUnary)
        //error("expr(): primary expression expected after token %s\n", GetTokenName(lastTok));
        errorUnexpectedToken(tok);

      continue;
    }

    //error("expr(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }

  while (opstacktop() != tokEof)
  {
    int v, v2;
    v = popop2(&v2);
    if (v != ':')
      push2(v, v2);
  }

  popop();

  return tok;
}

void decayArray(int* ExprTypeSynPtr, int arithmetic)
{
  // Dacay arrays to pointers to their first elements in
  // binary + and - operators
  if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == '[')
  {
    while (SyntaxStack[*ExprTypeSynPtr][0] != ']')
      ++*ExprTypeSynPtr;
    ++*ExprTypeSynPtr;
    *ExprTypeSynPtr = -*ExprTypeSynPtr;
  }
  // Also, to simplify code, return all other pointers as
  // negative expression stack syntax indices/pointers
  else if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == '*')
  {
    ++*ExprTypeSynPtr;
    *ExprTypeSynPtr = -*ExprTypeSynPtr;
  }

  // DONE: disallow arithmetic on pointers to void
  // DONE: disallow function pointers
  if (arithmetic)
  {
    if (*ExprTypeSynPtr < 0)
    {
      if (SyntaxStack[-*ExprTypeSynPtr][0] == tokVoid)
        //error("decayArray(): cannot do pointer arithmetic on a pointer to 'void'\n");
        errorUnexpectedVoid();
      if (SyntaxStack[-*ExprTypeSynPtr][0] == '(' ||
          !GetDeclSize(-*ExprTypeSynPtr, 0))
        //error("decayArray(): cannot do pointer arithmetic on a pointer to a function\n");
        errorOpType();
    }
    else
    {
      if (SyntaxStack[*ExprTypeSynPtr][0] == '(')
        //error("decayArray(): cannot do arithmetic on a function\n");
        errorOpType();
    }
  }
}

void nonVoidTypeCheck(int ExprTypeSynPtr)
{
  if (ExprTypeSynPtr >= 0 && SyntaxStack[ExprTypeSynPtr][0] == tokVoid)
    //error("nonVoidTypeCheck(): unexpected operand type 'void' for operator '%s'\n", GetTokenName(tok));
    errorUnexpectedVoid();
}

void scalarTypeCheck(int ExprTypeSynPtr)
{
  nonVoidTypeCheck(ExprTypeSynPtr);

  if (ExprTypeSynPtr >= 0 && SyntaxStack[ExprTypeSynPtr][0] == tokStructPtr)
    errorOpType();
}

void numericTypeCheck(int ExprTypeSynPtr)
{
  if (ExprTypeSynPtr >= 0 &&
      (SyntaxStack[ExprTypeSynPtr][0] == tokChar ||
       SyntaxStack[ExprTypeSynPtr][0] == tokSChar ||
       SyntaxStack[ExprTypeSynPtr][0] == tokUChar ||
#ifdef CAN_COMPILE_32BIT
       SyntaxStack[ExprTypeSynPtr][0] == tokShort ||
       SyntaxStack[ExprTypeSynPtr][0] == tokUShort ||
#endif
       SyntaxStack[ExprTypeSynPtr][0] == tokInt ||
       SyntaxStack[ExprTypeSynPtr][0] == tokUnsigned))
    return;
  //error("numericTypeCheck(): unexpected operand type for operator '%s', numeric type expected\n", GetTokenName(tok));
  errorOpType();
}

void compatCheck(int* ExprTypeSynPtr, int TheOtherExprTypeSynPtr, int ConstExpr[2], int lidx, int ridx)
{
  int exprTypeSynPtr = *ExprTypeSynPtr;
  int c = 0;
  int lptr, rptr, lnum, rnum;

  // convert functions to pointers to functions
  if (exprTypeSynPtr >= 0 && SyntaxStack[exprTypeSynPtr][0] == '(')
    *ExprTypeSynPtr = exprTypeSynPtr = -exprTypeSynPtr;
  if (TheOtherExprTypeSynPtr >= 0 && SyntaxStack[TheOtherExprTypeSynPtr][0] == '(')
    TheOtherExprTypeSynPtr = -TheOtherExprTypeSynPtr;

  lptr = exprTypeSynPtr < 0;
  rptr = TheOtherExprTypeSynPtr < 0;
  lnum = !lptr && (SyntaxStack[exprTypeSynPtr][0] == tokInt ||
                   SyntaxStack[exprTypeSynPtr][0] == tokUnsigned);
  rnum = !rptr && (SyntaxStack[TheOtherExprTypeSynPtr][0] == tokInt ||
                   SyntaxStack[TheOtherExprTypeSynPtr][0] == tokUnsigned);

  // both operands have arithmetic type
  // (arithmetic operands have been already promoted):
  if (lnum && rnum)
    return;

  // both operands have void type:
  if (!lptr && SyntaxStack[exprTypeSynPtr][0] == tokVoid &&
      !rptr && SyntaxStack[TheOtherExprTypeSynPtr][0] == tokVoid)
    return;

  // TBD??? check for exact 0?
  // one operand is a pointer and the other is NULL constant
  // ((void*)0 is also a valid null pointer constant),
  // the type of the expression is that of the pointer:
  if (lptr &&
      ((rnum && ConstExpr[1]) ||
       (rptr && SyntaxStack[-TheOtherExprTypeSynPtr][0] == tokVoid &&
        stack[ridx][0] == tokUnaryPlus && // "(type*)constant" appears as "constant +(unary)"
        (stack[ridx - 1][0] == tokNumInt || stack[ridx - 1][0] == tokNumUint))))
    return;
  if (rptr &&
      ((lnum && ConstExpr[0]) ||
       (lptr && SyntaxStack[-exprTypeSynPtr][0] == tokVoid &&
        stack[lidx][0] == tokUnaryPlus && // "(type*)constant" appears as "constant +(unary)"
        (stack[lidx - 1][0] == tokNumInt || stack[lidx - 1][0] == tokNumUint))))
  {
    *ExprTypeSynPtr = TheOtherExprTypeSynPtr;
    return;
  }

  // not expecting non-pointers beyond this point
  if (!(lptr && rptr))
    errorOpType();

  // one operand is a pointer and the other is a pointer to void
  // (except (void*)0, which is different from other pointers to void),
  // the type of the expression is pointer to void:
  if (SyntaxStack[-exprTypeSynPtr][0] == tokVoid)
    return;
  if (SyntaxStack[-TheOtherExprTypeSynPtr][0] == tokVoid)
  {
    *ExprTypeSynPtr = TheOtherExprTypeSynPtr;
    return;
  }

  // both operands are pointers to compatible types:

  if (exprTypeSynPtr == TheOtherExprTypeSynPtr)
    return;

  exprTypeSynPtr = -exprTypeSynPtr;
  TheOtherExprTypeSynPtr = -TheOtherExprTypeSynPtr;

  for (;;)
  {
    int tok = SyntaxStack[exprTypeSynPtr][0];
    if (tok != SyntaxStack[TheOtherExprTypeSynPtr][0])
      errorOpType();

    if (tok != tokIdent &&
        SyntaxStack[exprTypeSynPtr][1] != SyntaxStack[TheOtherExprTypeSynPtr][1])
      errorOpType();

    c += (tok == '(') - (tok == ')') + (tok == '[') - (tok == ']');

    if (!c)
    {
      switch (tok)
      {
      case tokVoid:
      case tokChar: case tokSChar: case tokUChar:
#ifdef CAN_COMPILE_32BIT
      case tokShort: case tokUShort:
#endif
      case tokInt: case tokUnsigned:
      case tokStructPtr:
        return;
      }
    }

    exprTypeSynPtr++;
    TheOtherExprTypeSynPtr++;
  }
}

void shiftCountCheck(int *psr, int idx, int ExprTypeSynPtr)
{
  int sr = *psr;
  // can't shift by a negative count and by a count exceeding
  // the number of bits in int
  if ((SyntaxStack[ExprTypeSynPtr][0] != tokUnsigned && sr < 0) ||
      (sr + 0u) >= CHAR_BIT * sizeof(int) ||
      (sr + 0u) >= 8u * SizeOfWord)
  {
    //error("exprval(): Invalid shift count\n");
    warning("Shift count out of range\n");
    // truncate the count, so the assembler doesn't get an invalid count
    sr &= SizeOfWord * 8 - 1;
    *psr = sr;
    stack[idx][1] = sr;
  }
}

int divCheckAndCalc(int tok, int* psl, int sr, int Unsigned, int ConstExpr[2])
{
  int div0 = 0;
  int sl = *psl;

  if (!ConstExpr[1])
    return !div0;

  if (Unsigned)
  {
    sl = uint2int(truncUint(sl));
    sr = uint2int(truncUint(sr));
  }
  else
  {
    sl = truncInt(sl);
    sr = truncInt(sr);
  }

  if (sr == 0)
  {
    div0 = 1;
  }
  else if (!ConstExpr[0])
  {
    return !div0;
  }
  else if (!Unsigned && ((sl == INT_MIN && sr == -1) || sl / sr != truncInt(sl / sr)))
  {
    div0 = 1;
  }
  else
  {
    if (Unsigned)
    {
      if (tok == '/')
        sl = uint2int((sl + 0u) / sr);
      else
        sl = uint2int((sl + 0u) % sr);
    }
    else
    {
      // TBD!!! C89 gives freedom in how exactly division of negative integers
      // can be implemented w.r.t. rounding and w.r.t. the sign of the remainder.
      // A stricter, C99-conforming implementation, non-dependent on the
      // compiler used to compile Smaller C is needed.
      if (tok == '/')
        sl /= sr;
      else
        sl %= sr;
    }
    *psl = sl;
  }

  if (div0)
    warning("Division by 0 or division overflow\n");

  return !div0;
}

void promoteType(int* ExprTypeSynPtr, int* TheOtherExprTypeSynPtr)
{
  // chars must be promoted to ints in expressions as the very first thing
  if (*ExprTypeSynPtr >= 0 &&
      (SyntaxStack[*ExprTypeSynPtr][0] == tokChar ||
#ifdef CAN_COMPILE_32BIT
       SyntaxStack[*ExprTypeSynPtr][0] == tokShort ||
       SyntaxStack[*ExprTypeSynPtr][0] == tokUShort ||
#endif
       SyntaxStack[*ExprTypeSynPtr][0] == tokSChar ||
       SyntaxStack[*ExprTypeSynPtr][0] == tokUChar))
    *ExprTypeSynPtr = SymIntSynPtr;

  // ints must be converted to unsigned ints if they are used in binary
  // operators whose other operand is unsigned int (except <<,>>,<<=,>>=)
  if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == tokInt &&
      *TheOtherExprTypeSynPtr >= 0 && SyntaxStack[*TheOtherExprTypeSynPtr][0] == tokUnsigned)
    *ExprTypeSynPtr = SymUintSynPtr;
}

int GetFxnInfo(int ExprTypeSynPtr, int* MinParams, int* MaxParams, int* ReturnExprTypeSynPtr)
{
  int ptr = 0;

  *MaxParams = *MinParams = 0;

  if (ExprTypeSynPtr < 0)
  {
    ptr = 1;
    ExprTypeSynPtr = -ExprTypeSynPtr;
  }

  while (SyntaxStack[ExprTypeSynPtr][0] == tokIdent || SyntaxStack[ExprTypeSynPtr][0] == tokLocalOfs)
    ExprTypeSynPtr++;

  if (!(SyntaxStack[ExprTypeSynPtr][0] == '(' ||
        (!ptr && SyntaxStack[ExprTypeSynPtr][0] == '*' && SyntaxStack[ExprTypeSynPtr + 1][0] == '(')))
    return 0;

  // DONE: return syntax pointer to the function's return type

  // Count params

  while (SyntaxStack[ExprTypeSynPtr][0] != '(')
    ExprTypeSynPtr++;
  ExprTypeSynPtr++;

  if (SyntaxStack[ExprTypeSynPtr][0] == ')')
  {
    // "fxn()": unspecified parameters, so, there can be any number of them
    *MaxParams = 32767; // INT_MAX;
    *ReturnExprTypeSynPtr = ExprTypeSynPtr + 1;
    return 1;
  }

  if (SyntaxStack[ExprTypeSynPtr + 1][0] == tokVoid)
  {
    // "fxn(void)": 0 parameters
    *ReturnExprTypeSynPtr = ExprTypeSynPtr + 3;
    return 1;
  }

  for (;;)
  {
    int tok = SyntaxStack[ExprTypeSynPtr][0];

    if (tok == tokIdent)
    {
      if (SyntaxStack[ExprTypeSynPtr + 1][0] != tokEllipsis)
      {
        ++*MinParams;
        ++*MaxParams;
      }
      else
      {
        *MaxParams = 32767; // INT_MAX;
      }
    }
    else if (tok == '(')
    {
      // skip parameters in parameters
      int c = 1;
      while (c && ExprTypeSynPtr < SyntaxStackCnt)
      {
        tok = SyntaxStack[++ExprTypeSynPtr][0];
        c += (tok == '(') - (tok == ')');
      }
    }
    else if (tok == ')')
    {
      ExprTypeSynPtr++;
      break;
    }

    ExprTypeSynPtr++;
  }

  // get the function's return type
  *ReturnExprTypeSynPtr = ExprTypeSynPtr;

  return 1;
}

void simplifyConstExpr(int val, int isConst, int* ExprTypeSynPtr, int top, int bottom)
{
  if (!isConst || stack[top][0] == tokNumInt || stack[top][0] == tokNumUint)
    return;

  if (SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned)
    stack[top][0] = tokNumUint;
  else
    stack[top][0] = tokNumInt;
  stack[top][1] = val;

  del(bottom, top - bottom);
}

// DONE: sizeof(type)
// DONE: "sizeof expr"
// DONE: constant expressions
// DONE: collapse constant subexpressions into constants
int exprval(int* idx, int* ExprTypeSynPtr, int* ConstExpr)
{
  int tok;
  int s;
  int RightExprTypeSynPtr;
  int oldIdxRight;
  int oldSpRight;
  int constExpr[3];

  if (*idx < 0)
    //error("exprval(): idx < 0\n");
    errorInternal(5);

  tok = stack[*idx][0];
  s = stack[*idx][1];

  --*idx;

  oldIdxRight = *idx;
  oldSpRight = sp;

  switch (tok)
  {
  // Constants
  case tokNumInt:
    // return the constant's type: int
    *ExprTypeSynPtr = SymIntSynPtr;
    *ConstExpr = 1;
    break;
  case tokNumUint:
    // return the constant's type: unsigned int
    *ExprTypeSynPtr = SymUintSynPtr;
    *ConstExpr = 1;
    break;

  // Identifiers
  case tokIdent:
    {
      // DONE: support __func__
      char* ident = IdentTable + s;
      int synPtr, type;
#ifndef NO_FUNC_
      if (CurFxnName && !strcmp(ident, "__func__"))
      {
        if (CurFxnNameLabel >= 0)
          CurFxnNameLabel = -CurFxnNameLabel;
        stack[*idx + 1][1] = SyntaxStack[SymFuncPtr][1];
        synPtr = SymFuncPtr;
      }
      else
#endif
      {
        synPtr = FindSymbol(ident);
        // "Rename" static vars in function scope
        if (synPtr >= 0 && synPtr + 1 < SyntaxStackCnt && SyntaxStack[synPtr + 1][0] == tokIdent)
        {
          s = stack[*idx + 1][1] = SyntaxStack[++synPtr][1];
          ident = IdentTable + s;
        }
      }

      if (synPtr < 0)
      {
        if ((*idx + 2 >= sp) || stack[*idx + 2][0] != ')')
          error("Undeclared identifier '%s'\n", ident);
        else
        {
          warning("Call to undeclared function '%s()'\n", ident);
          // Implicitly declare "extern int ident();"
          PushSyntax2(tokIdent, s);
          PushSyntax('(');
          PushSyntax(')');
          PushSyntax(tokInt);
          synPtr = FindSymbol(ident);
        }
      }

#ifndef NO_TYPEDEF_ENUM
      if (synPtr + 1 < SyntaxStackCnt &&
          SyntaxStack[synPtr + 1][0] == tokNumInt)
      {
        // this is an enum constant
        stack[*idx + 1][0] = tokNumInt;
        s = stack[*idx + 1][1] = SyntaxStack[synPtr + 1][1];
        *ExprTypeSynPtr = SymIntSynPtr;
        *ConstExpr = 1;
        break;
      }
#endif

      // DONE: this declaration is actually a type cast
      if (!strncmp(IdentTable + SyntaxStack[synPtr][1], "(something", sizeof "(something)" - 1 - 1))
      {
        int castSize;

        if (SyntaxStack[++synPtr][0] == tokLocalOfs) // TBD!!! is this really needed???
          synPtr++;

        s = exprval(idx, ExprTypeSynPtr, ConstExpr);

        // can't cast void or structure/union to anything (except void)
        if (*ExprTypeSynPtr >= 0 &&
            (SyntaxStack[*ExprTypeSynPtr][0] == tokVoid ||
             SyntaxStack[*ExprTypeSynPtr][0] == tokStructPtr) &&
            SyntaxStack[synPtr][0] != tokVoid)
          errorOpType();

        // can't cast to function, array or structure/union
        if (SyntaxStack[synPtr][0] == '(' ||
            SyntaxStack[synPtr][0] == '[' ||
            SyntaxStack[synPtr][0] == tokStructPtr)
          errorOpType();

        // will try to propagate constants through casts
        if (!*ConstExpr &&
            (stack[oldIdxRight - (oldSpRight - sp)][0] == tokNumInt ||
             stack[oldIdxRight - (oldSpRight - sp)][0] == tokNumUint))
        {
          s = stack[oldIdxRight - (oldSpRight - sp)][1];
          *ConstExpr = 1;
        }

        castSize = GetDeclSize(synPtr, 1);

        // insertion of tokUChar, tokSChar and tokUnaryPlus transforms
        // lvalues (values formed by dereferences) into rvalues
        // (by hiding the dereferences), just as casts should do
        switch (castSize)
        {
        case 1:
          // cast to unsigned char
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUChar;
          s &= 0xFFu;
          break;
        case -1:
          // cast to signed char
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokSChar;
          if ((s &= 0xFFu) >= 0x80)
            s -= 0x100;
          break;
        default:
#ifdef CAN_COMPILE_32BIT
          if (castSize && castSize != SizeOfWord && -castSize != SizeOfWord)
          {
            if (castSize == 2)
            {
              // cast to unsigned short
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUShort;
              s &= 0xFFFFu;
            }
            else
            {
              // cast to signed short
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokShort;
              if ((s &= 0xFFFFu) >= 0x8000)
                s -= 0x10000;
            }
          }
          else
#endif
          {
            // cast to int/unsigned/pointer
            if (stack[oldIdxRight - (oldSpRight - sp)][0] == tokUnaryStar)
              // hide the dereference
              stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUnaryPlus;
            else
              // nothing to hide, remove the cast
              del(oldIdxRight + 1 - (oldSpRight - sp), 1);
          }
          break;
        }

        switch (SyntaxStack[synPtr][0])
        {
        case tokChar:
        case tokSChar:
        case tokUChar:
#ifdef CAN_COMPILE_32BIT
        case tokShort:
        case tokUShort:
#endif
        case tokInt:
        case tokUnsigned:
          break;
        default:
          *ConstExpr = 0;
          break;
        }

        *ExprTypeSynPtr = synPtr;
        simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
        break;
      }

      type = SymType(synPtr);

      if (type == SymLocalVar || type == SymLocalArr)
      {
        // replace local variables/arrays with their local addresses
        // (global variables/arrays' addresses are their names)
        stack[*idx + 1][0] = tokLocalOfs;
        stack[*idx + 1][1] = SyntaxStack[synPtr + 1][1];
      }
      if (type == SymLocalVar || type == SymGlobalVar)
      {
        // add implicit dereferences for local/global variables
        ins2(*idx + 2, tokUnaryStar, GetDeclSize(synPtr, 1));
      }

      // return the identifier's type
      while (SyntaxStack[synPtr][0] == tokIdent || SyntaxStack[synPtr][0] == tokLocalOfs)
        synPtr++;
      *ExprTypeSynPtr = synPtr;
    }
    *ConstExpr = 0;
    break;

  // sizeof operator
  case tokSizeof:
    s = exprval(idx, ExprTypeSynPtr, ConstExpr);

    if (*ExprTypeSynPtr >= 0)
      s = GetDeclSize(*ExprTypeSynPtr, 0);
    else
      s = SizeOfWord;
    if (s == 0)
      error("sizeof of incomplete type\n");

    // replace sizeof with its numeric value
    stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokNumUint;
    stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = s;

    // remove the sizeof's subexpression
    del(*idx + 1, oldIdxRight - (oldSpRight - sp) - *idx);

    *ExprTypeSynPtr = SymUintSynPtr;
    *ConstExpr = 1;
    break;

  // Address unary operator
  case tokUnaryAnd:
    exprval(idx, ExprTypeSynPtr, ConstExpr);

    if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == '[')
    {
      // convert an array into a pointer to the array,
      // remove the reference
      *ExprTypeSynPtr = -*ExprTypeSynPtr;
      del(oldIdxRight + 1 - (oldSpRight - sp), 1);
    }
    else if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == '(')
    {
      // convert a function into a pointer to the function,
      // remove the reference
      *ExprTypeSynPtr = -*ExprTypeSynPtr;
      del(oldIdxRight + 1 - (oldSpRight - sp), 1);
    }
    else if (*ExprTypeSynPtr >= 0 &&
             oldIdxRight - (oldSpRight - sp) >= 0 &&
             stack[oldIdxRight - (oldSpRight - sp)][0] == tokUnaryStar)
    {
      // it's an lvalue (with implicit or explicit dereference),
      // convert it into its address,
      // collapse/remove the reference and the dereference
      *ExprTypeSynPtr = -*ExprTypeSynPtr;
      del(oldIdxRight - (oldSpRight - sp), 2);
    }
    else
      //error("exprval(): lvalue expected after '&'\n");
      errorNotLvalue();

    *ConstExpr = 0;
    break;

  // Indirection unary operator
  case tokUnaryStar:
    exprval(idx, ExprTypeSynPtr, ConstExpr);

    if (*ExprTypeSynPtr < 0 || SyntaxStack[*ExprTypeSynPtr][0] == '*')
    {
      // type is a pointer to something,
      // transform it into that something
      if (*ExprTypeSynPtr < 0)
        *ExprTypeSynPtr = -*ExprTypeSynPtr;
      else
        ++*ExprTypeSynPtr;
      nonVoidTypeCheck(*ExprTypeSynPtr);
      if (SyntaxStack[*ExprTypeSynPtr][0] == tokStructPtr && !GetDeclSize(*ExprTypeSynPtr, 0))
        // incomplete structure/union type
        errorOpType();
      // remove the dereference if that something is an array or a function
      if (SyntaxStack[*ExprTypeSynPtr][0] == '[' ||
          SyntaxStack[*ExprTypeSynPtr][0] == '(')
        del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      // else add dereference size in bytes
      else
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = GetDeclSize(*ExprTypeSynPtr, 1);
    }
    else if (SyntaxStack[*ExprTypeSynPtr][0] == '[')
    {
      // type is an array,
      // transform it into the array's first element
      // (a subarray, if type is a multidimensional array)
      while (SyntaxStack[*ExprTypeSynPtr][0] != ']')
        ++*ExprTypeSynPtr;
      ++*ExprTypeSynPtr;
      // remove the dereference if that element is an array
      if (SyntaxStack[*ExprTypeSynPtr][0] == '[')
        del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      // else add dereference size in bytes
      else
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = GetDeclSize(*ExprTypeSynPtr, 1);
    }
    else
      //error("exprval(): pointer/array expected after '*' / before '[]'\n");
      errorOpType();

    *ConstExpr = 0;
    break;

  // Additive binary operators
  case '+':
  case '-':
  // WRONGISH: DONE: replace prefix ++/-- with +=1/-=1
  // WRONG: DONE: replace postfix ++/-- with (+=1)-1/(-=1)+1
    {
      int ptrmask;
      int oldIdxLeft, oldSpLeft;
      int sl, sr;
      int incSize;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      if (tok == '+')
        s = uint2int(sl + 0u + sr);
      else
        s = uint2int(sl + 0u - sr);

      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      // Decay arrays to pointers to their first elements
      decayArray(&RightExprTypeSynPtr, 1);
      decayArray(ExprTypeSynPtr, 1);

      ptrmask = (RightExprTypeSynPtr < 0) + (*ExprTypeSynPtr < 0) * 2;

      // DONE: index/subscript scaling
      if (ptrmask == 1 && tok == '+') // pointer in right-hand expression
      {
        incSize = GetDeclSize(-RightExprTypeSynPtr, 0);

        if (constExpr[0]) // integer constant in left-hand expression
        {
          s = uint2int((sl + 0u) * incSize);
          stack[oldIdxLeft - (oldSpLeft - sp)][1] = s;
          // optimize a little if possible
          {
            int i = oldIdxRight - (oldSpRight - sp);
            // Skip any type cast markers
            while (stack[i][0] == tokUnaryPlus || stack[i][0] == '+')
              i--;
            // See if the pointer is an integer constant or a local variable offset
            // and if it is, adjust it here instead of generating code for
            // addition/subtraction
            if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
            {
              s = uint2int(stack[i][1] + 0u + s);
              stack[i][1] = s; // TBD!!! need extra truncation?
              del(oldIdxLeft - (oldSpLeft - sp), 1);
              del(oldIdxRight - (oldSpRight - sp) + 1, 1);
            }
          }
        }
        else if (incSize != 1)
        {
          ins2(oldIdxLeft + 1 - (oldSpLeft - sp), tokNumInt, incSize);
          ins(oldIdxLeft + 1 - (oldSpLeft - sp), '*');
        }

        *ExprTypeSynPtr = RightExprTypeSynPtr;
      }
      else if (ptrmask == 2) // pointer in left-hand expression
      {
        incSize = GetDeclSize(-*ExprTypeSynPtr, 0);
        if (constExpr[1]) // integer constant in right-hand expression
        {
          s = uint2int((sr + 0u) * incSize);
          stack[oldIdxRight - (oldSpRight - sp)][1] = s;
          // optimize a little if possible
          {
            int i = oldIdxLeft - (oldSpLeft - sp);
            // Skip any type cast markers
            while (stack[i][0] == tokUnaryPlus || stack[i][0] == '+')
              i--;
            // See if the pointer is an integer constant or a local variable offset
            // and if it is, adjust it here instead of generating code for
            // addition/subtraction
            if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
            {
              if (tok == '-')
                s = uint2int(-(s + 0u));
              s = uint2int(stack[i][1] + 0u + s);
              stack[i][1] = s; // TBD!!! need extra truncation?
              del(oldIdxRight - (oldSpRight - sp), 2);
            }
          }
        }
        else if (incSize != 1)
        {
          ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 1 - (oldSpRight - sp), '*');
        }
      }
      else if (ptrmask == 3 && tok == '-') // pointers in both expressions
      {
        incSize = GetDeclSize(-*ExprTypeSynPtr, 0);
        // TBD!!! "ptr1-ptr2": better pointer type compatibility test needed, like compatCheck()?
        if (incSize != GetDeclSize(-RightExprTypeSynPtr, 0))
          //error("exprval(): incompatible pointers\n");
          errorOpType();
        if (incSize != 1)
        {
          ins2(oldIdxRight + 2 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 2 - (oldSpRight - sp), '/');
        }
        *ExprTypeSynPtr = SymIntSynPtr;
      }
      else if (ptrmask)
        //error("exprval(): invalid combination of operands for '+' or '-'\n");
        errorOpType();

      // Promote the result from char to int (and from int to unsigned) if necessary
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);

      *ConstExpr = constExpr[0] && constExpr[1];
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Prefix/postfix increment/decrement unary operators
  case tokInc:
  case tokDec:
  case tokPostInc:
  case tokPostDec:
    {
      int incSize = 1;
      int inc = tok == tokInc || tok == tokPostInc;
      int post = tok == tokPostInc || tok == tokPostDec;
      int opSize;

      exprval(idx, ExprTypeSynPtr, ConstExpr);

      scalarTypeCheck(*ExprTypeSynPtr);

      decayArray(ExprTypeSynPtr, 1);

      // lvalue check for ++, --
      if (!(oldIdxRight - (oldSpRight - sp) >= 0 &&
            stack[oldIdxRight - (oldSpRight - sp)][0] == tokUnaryStar))
        //error("exprval(): lvalue expected for '++' or '--'\n");
        errorNotLvalue();

      // "remove" the lvalue dereference as we don't need
      // to read the value and forget its location. We need to
      // keep the lvalue location.
      // Remember the operand size.
      opSize = stack[oldIdxRight - (oldSpRight - sp)][1];
      del(oldIdxRight - (oldSpRight - sp), 1);

      if (*ExprTypeSynPtr < 0)
        incSize = GetDeclSize(-*ExprTypeSynPtr, 0);

      if (incSize == 1)
      {
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
      }
      else
      {
        // replace ++/-- with "postfix" +=/-= incSize when incSize != 1
        if (inc)
        {
          if (post)
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokPostAdd;
          else
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokAssignAdd;
        }
        else
        {
          if (post)
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokPostSub;
          else
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokAssignSub;
        }
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;

        ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
      }

      *ConstExpr = 0;
    }
    break;

  // Simple assignment binary operator
  case '=':
    {
      int oldIdxLeft, oldSpLeft;
      int opSize;
      int structs;

      exprval(idx, &RightExprTypeSynPtr, ConstExpr);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      exprval(idx, ExprTypeSynPtr, ConstExpr);

      nonVoidTypeCheck(RightExprTypeSynPtr);
      nonVoidTypeCheck(*ExprTypeSynPtr);

      decayArray(&RightExprTypeSynPtr, 0);
      decayArray(ExprTypeSynPtr, 0);

      if (!(oldIdxLeft - (oldSpLeft - sp) >= 0 &&
            stack[oldIdxLeft - (oldSpLeft - sp)][0] == tokUnaryStar))
        //error("exprval(): lvalue expected before '='\n");
        errorNotLvalue();

      structs = (RightExprTypeSynPtr >= 0 && SyntaxStack[RightExprTypeSynPtr][0] == tokStructPtr) +
              (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == tokStructPtr) * 2;
      if (structs)
      {
        int sz;

        if (structs != 3 ||
            SyntaxStack[RightExprTypeSynPtr][1] != SyntaxStack[*ExprTypeSynPtr][1])
          errorOpType();

        // TBD??? (a = b) should be an rvalue and so &(a = b) and (&(a = b))->c shouldn't be
        // allowed, while (a = b).c should be allowed.

        // transform "*psleft = *psright" into "*fxn(sizeof *psright, psright, psleft)"

        if (stack[oldIdxLeft - (oldSpLeft - sp)][0] != tokUnaryStar ||
            stack[oldIdxRight - (oldSpRight - sp)][0] != tokUnaryStar)
          errorInternal(18);

        stack[oldIdxLeft - (oldSpLeft - sp)][0] = ','; // replace '*' with ','
        stack[oldIdxRight - (oldSpRight - sp)][0] = ','; // replace '*' with ','

        sz = GetDeclSize(RightExprTypeSynPtr, 0);

        stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokNumUint; // replace '=' with "sizeof *psright"
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = sz;

        ins(oldIdxRight + 2 - (oldSpRight - sp), ',');

        if (!StructCpyLabel)
          StructCpyLabel = LabelCnt++;
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokIdent, AddNumericIdent__(StructCpyLabel));

        ins2(oldIdxRight + 2 - (oldSpRight - sp), ')', SizeOfWord * 3);
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokUnaryStar, 0); // use 0 deref size to drop meaningless dereferences

        ins2(*idx + 1, '(', SizeOfWord * 3);
      }
      else
      {
        // "remove" the lvalue dereference as we don't need
        // to read the value and forget its location. We need to
        // keep the lvalue location.
        opSize = stack[oldIdxLeft - (oldSpLeft - sp)][1];
        // store the operand size in the operator
        stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
        del(oldIdxLeft - (oldSpLeft - sp), 1);
      }

      *ConstExpr = 0;
    }
    break;

  // DONE: other assignment operators

  // Arithmetic and bitwise unary operators
  case '~':
  case tokUnaryPlus:
  case tokUnaryMinus:
    s = exprval(idx, ExprTypeSynPtr, ConstExpr);
    scalarTypeCheck(*ExprTypeSynPtr);
    numericTypeCheck(*ExprTypeSynPtr);
    switch (tok)
    {
    case '~':           s = ~s; break;
    case tokUnaryPlus:  s = +s; break;
    case tokUnaryMinus: s = uint2int(~(s - 1u)); break;
    }
    promoteType(ExprTypeSynPtr, ExprTypeSynPtr);
    simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    break;

  // Arithmetic and bitwise binary operators
  case '*':
  case '/':
  case '%':
  case tokLShift:
  case tokRShift:
  case '&':
  case '^':
  case '|':
    {
      // int oldIdxLeft, oldSpLeft;
      int sr, sl;
      int Unsigned;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      // oldIdxLeft = *idx;
      // oldSpLeft = sp;
      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      numericTypeCheck(RightExprTypeSynPtr);
      numericTypeCheck(*ExprTypeSynPtr);

      *ConstExpr = constExpr[0] && constExpr[1];

      Unsigned = SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned || SyntaxStack[RightExprTypeSynPtr][0] == tokUnsigned;

      switch (tok)
      {
      // DONE: check for division overflows
      case '/':
      case '%':
        *ConstExpr &= divCheckAndCalc(tok, &sl, sr, Unsigned, constExpr);

        if (Unsigned)
        {
          if (tok == '/')
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUDiv;
          else
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokUMod;
        }
        break;

      case '*':
        sl = uint2int((sl + 0u) * sr);
        break;

      case tokLShift:
      case tokRShift:
        if (constExpr[1])
        {
          if (SyntaxStack[RightExprTypeSynPtr][0] != tokUnsigned)
            sr = truncInt(sr);
          else
            sr = uint2int(truncUint(sr));
          shiftCountCheck(&sr, oldIdxRight - (oldSpRight - sp), RightExprTypeSynPtr);
        }
        if (*ConstExpr)
        {
          if (tok == tokLShift)
          {
            // left shift is the same for signed and unsigned ints
            sl = uint2int((sl + 0u) << sr);
          }
          else
          {
            if (SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned)
            {
              // right shift for unsigned ints
              sl = uint2int(truncUint(sl) >> sr);
            }
            else if (sr)
            {
              // right shift for signed ints is arithmetic, sign-bit-preserving
              // don't depend on the compiler's implementation, do it "manually"
              sl = truncInt(sl);
              sl = uint2int((truncUint(sl) >> sr) |
                            ((sl < 0) * (~0u << (8 * SizeOfWord - sr))));
            }
          }
        }

        if (SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned && tok == tokRShift)
            stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = tokURShift;

        // ignore RightExprTypeSynPtr for the purpose of promotion/conversion of the result of <</>>
        RightExprTypeSynPtr = SymIntSynPtr;
        break;

      case '&': sl &= sr; break;
      case '^': sl ^= sr; break;
      case '|': sl |= sr; break;
      }

      s = sl;
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Relational binary operators
  // DONE: add (sub)tokens for unsigned >, >=, <, <= for pointers
  case '<':
  case '>':
  case tokLEQ:
  case tokGEQ:
  case tokEQ:
  case tokNEQ:
    {
      int ptrmask;
      int sr, sl;
      int Unsigned;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      decayArray(&RightExprTypeSynPtr, 0);
      decayArray(ExprTypeSynPtr, 0);

      ptrmask = (RightExprTypeSynPtr < 0) + (*ExprTypeSynPtr < 0) * 2;

      // Disallow >, <, >=, <= between a pointer and a number
      if (ptrmask >= 1 && ptrmask <= 2 &&
          tok != tokEQ && tok != tokNEQ)
        //error("exprval(): Invalid/unsupported combination of compared operands\n");
        errorOpType();

      *ConstExpr = constExpr[0] && constExpr[1];

      Unsigned = !ptrmask &&
        (SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned || SyntaxStack[RightExprTypeSynPtr][0] == tokUnsigned);

      if (*ConstExpr)
      {
        if (!Unsigned)
        {
          sl = truncInt(sl);
          sr = truncInt(sr);
          switch (tok)
          {
          case '<':    sl = sl <  sr; break;
          case '>':    sl = sl >  sr; break;
          case tokLEQ: sl = sl <= sr; break;
          case tokGEQ: sl = sl >= sr; break;
          case tokEQ:  sl = sl == sr; break;
          case tokNEQ: sl = sl != sr; break;
          }
        }
        else
        {
          sl = uint2int(truncUint(sl));
          sr = uint2int(truncUint(sr));
          switch (tok)
          {
          case '<':    sl = sl + 0u <  sr + 0u; break;
          case '>':    sl = sl + 0u >  sr + 0u; break;
          case tokLEQ: sl = sl + 0u <= sr + 0u; break;
          case tokGEQ: sl = sl + 0u >= sr + 0u; break;
          case tokEQ:  sl = sl == sr; break;
          case tokNEQ: sl = sl != sr; break;
          }
        }
      }

      if (ptrmask || Unsigned)
      {
        // Pointer comparison should be unsigned
        int t = tok;
        switch (tok)
        {
        case '<': t = tokULess; break;
        case '>': t = tokUGreater; break;
        case tokLEQ: t = tokULEQ; break;
        case tokGEQ: t = tokUGEQ; break;
        }
        if (t != tok)
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = t;
      }

      s = sl;
      *ExprTypeSynPtr = SymIntSynPtr;
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // implicit pseudo-conversion to _Bool of operands of && and ||
  case tok_Bool:
    s = exprval(idx, ExprTypeSynPtr, ConstExpr);
    s = truncInt(s) != 0;
    scalarTypeCheck(*ExprTypeSynPtr);
    decayArray(ExprTypeSynPtr, 0);
    *ExprTypeSynPtr = SymIntSynPtr;
    simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    break;

  // Logical binary operators
  case tokLogAnd: // DONE: short-circuit
  case tokLogOr: // DONE: short-circuit
    {
      int sr, sl;

      // DONE: think of pushing a special short-circuit (jump-to) token
      // to skip the rhs operand evaluation in && and ||
      // DONE: add implicit "casts to _Bool" of && and || operands,
      // do the same for control statements of if() while() and for(;;).

      int sc = LabelCnt++;
      // tag the logical operator as a numbered short-circuit jump target
      stack[*idx + 1][1] = sc;

      // insert "!= 0" for right-hand operand
      switch (stack[*idx][0])
      {
      case '<':
      case tokULess:
      case '>':
      case tokUGreater:
      case tokLEQ:
      case tokULEQ:
      case tokGEQ:
      case tokUGEQ:
      case tokEQ:
      case tokNEQ:
        break;
      default:
        ins(++*idx, tok_Bool);
        break;
      }

      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);

      // insert a reference to the short-circuit jump target
      if (tok == tokLogAnd)
        ins2(++*idx, tokShortCirc, sc);
      else
        ins2(++*idx, tokShortCirc, -sc);
      // insert "!= 0" for left-hand operand
      switch (stack[*idx - 1][0])
      {
      case '<':
      case tokULess:
      case '>':
      case tokUGreater:
      case tokLEQ:
      case tokULEQ:
      case tokGEQ:
      case tokUGEQ:
      case tokEQ:
      case tokNEQ:
        --*idx;
        break;
      default:
        ins(*idx, tok_Bool);
        break;
      }

      sl = exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      if (tok == tokLogAnd)
        s = sl && sr;
      else
        s = sl || sr;

      *ExprTypeSynPtr = SymIntSynPtr;
      *ConstExpr = constExpr[0] && constExpr[1];
      if (constExpr[0])
      {
        if (tok == tokLogAnd)
        {
          if (!sl)
            *ConstExpr = 1, s = 0;
          // TBD??? else can drop LHS expression
        }
        else
        {
          if (sl)
            *ConstExpr = s = 1;
          // TBD??? else can drop LHS expression
        }
      }
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Function call
  case ')':
    {
      int tmpSynPtr, c;
      int minParams, maxParams;
      exprval(idx, ExprTypeSynPtr, ConstExpr);

      if (!GetFxnInfo(*ExprTypeSynPtr, &minParams, &maxParams, ExprTypeSynPtr))
        //error("exprval(): function or function pointer expected\n");
        errorOpType();

      // DONE: validate the number of function parameters
      // TBD??? warnings/errors on int<->pointer substitution in params

      // evaluate function parameters
      c = 0;
      while (stack[*idx][0] != '(')
      {
        // add a comma after the first (last to be pushed) parameter,
        // so all parameters can be pushed whenever a comma is encountered
        if (!c)
          ins(*idx + 1, ',');

        exprval(idx, &tmpSynPtr, ConstExpr);
        //error("exprval(): function parameters cannot be of type 'void'\n");
        scalarTypeCheck(tmpSynPtr);

        if (++c > maxParams)
          error("Too many function parameters\n");

        if (stack[*idx][0] == ',')
          --*idx;
      }
      --*idx;

      if (c < minParams)
        error("Too few function parameters\n");

      // store the cumulative parameter size in the function call operators
      stack[1 + *idx][1] = stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = c * SizeOfWord;

      *ConstExpr = 0;
    }
    break;

  // Binary comma operator
  case tokComma:
    {
      int oldIdxLeft, oldSpLeft;
      s = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;

      // Signify uselessness of the result of the left operand's value
      ins(*idx + 1, tokVoid);

      exprval(idx, ExprTypeSynPtr, &constExpr[0]);
      *ConstExpr = constExpr[0] && constExpr[1];
      *ExprTypeSynPtr = RightExprTypeSynPtr;
      if (*ConstExpr)
      {
        // both subexprs are const, remove both and comma
        simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
      }
      else if (constExpr[0])
      {
        // only left subexpr is const, remove it and comma
        del(*idx + 1, oldIdxLeft - (oldSpLeft - sp) - *idx);
        del(oldIdxRight + 1 - (oldSpRight - sp), 1);
      }
    }
    break;

  // Compound assignment operators
  case tokAssignMul: case tokAssignDiv: case tokAssignMod:
  case tokAssignAdd: case tokAssignSub:
  case tokAssignLSh: case tokAssignRSh:
  case tokAssignAnd: case tokAssignXor: case tokAssignOr:
    {
      int ptrmask;
      int oldIdxLeft, oldSpLeft;
      int incSize;
      int opSize;
      int Unsigned;
      int sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[1]);
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      exprval(idx, ExprTypeSynPtr, &constExpr[0]);

      scalarTypeCheck(RightExprTypeSynPtr);
      scalarTypeCheck(*ExprTypeSynPtr);

      decayArray(&RightExprTypeSynPtr, 1);
      decayArray(ExprTypeSynPtr, 1);

      if (!(oldIdxLeft - (oldSpLeft - sp) >= 0 &&
            stack[oldIdxLeft - (oldSpLeft - sp)][0] == tokUnaryStar))
        //error("exprval(): lvalue expected before %s\n", GetTokenName(tok));
        errorNotLvalue();

      // "remove" the lvalue dereference as we don't need
      // to read the value and forget its location. We need to
      // keep the lvalue location.
      opSize = stack[oldIdxLeft - (oldSpLeft - sp)][1];
      // store the operand size in the operator
      stack[oldIdxRight + 1 - (oldSpRight - sp)][1] = opSize;
      del(oldIdxLeft - (oldSpLeft - sp), 1);

      ptrmask = (RightExprTypeSynPtr < 0) + (*ExprTypeSynPtr < 0) * 2;

      Unsigned = !ptrmask &&
        (SyntaxStack[*ExprTypeSynPtr][0] == tokUnsigned || SyntaxStack[RightExprTypeSynPtr][0] == tokUnsigned);

      if (tok != tokAssignAdd && tok != tokAssignSub)
      {
        if (ptrmask)
          //error("exprval(): invalid combination of operands for %s\n", GetTokenName(tok));
          errorOpType();
      }
      else
      {
        // No pointer to the right of += and -=
        if (ptrmask & 1)
          //error("exprval(): invalid combination of operands for %s\n", GetTokenName(tok));
          errorOpType();
      }

      if (tok == tokAssignLSh || tok == tokAssignRSh)
      {
        if (constExpr[1])
        {
          if (SyntaxStack[RightExprTypeSynPtr][0] != tokUnsigned)
            sr = truncInt(sr);
          else
            sr = uint2int(truncUint(sr));
          shiftCountCheck(&sr, oldIdxRight - (oldSpRight - sp), RightExprTypeSynPtr);
        }
      }

      if (tok == tokAssignDiv || tok == tokAssignMod)
      {
        int t, sl = 0;
        if (tok == tokAssignDiv)
          t = '/';
        else
          t = '%';
        divCheckAndCalc(t, &sl, sr, 1, constExpr);
      }

      // TBD??? replace +=/-= with prefix ++/-- if incSize == 1
      if (ptrmask == 2) // left-hand expression
      {
        incSize = GetDeclSize(-*ExprTypeSynPtr, 0);
        if (constExpr[1])
        {
          int t = uint2int(stack[oldIdxRight - (oldSpRight - sp)][1] * (incSize + 0u));
          stack[oldIdxRight - (oldSpRight - sp)][1] = t;
        }
        else if (incSize != 1)
        {
          ins2(oldIdxRight + 1 - (oldSpRight - sp), tokNumInt, incSize);
          ins(oldIdxRight + 1 - (oldSpRight - sp), '*');
        }
      }
      else if (Unsigned)
      {
        int t = tok;
        switch (tok)
        {
        case tokAssignDiv: t = tokAssignUDiv; break;
        case tokAssignMod: t = tokAssignUMod; break;
        case tokAssignRSh: t = tokAssignURSh; break;
        }
        if (t != tok)
          stack[oldIdxRight + 1 - (oldSpRight - sp)][0] = t;
      }

      *ConstExpr = 0;
    }
    break;

  // Ternary/conditional operator
  case '?':
    {
      int oldIdxLeft, oldSpLeft;
      int sr, sl, smid;
      int condTypeSynPtr;
      int sc = (LabelCnt += 2) - 2;
      int structs;

      // "exprL ? exprMID : exprR" appears on the stack as
      // "exprL exprR exprMID ?"

      stack[*idx + 1][0] = tokLogAnd; // piggyback on && for CG (ugly, but simple)
      stack[*idx + 1][1] = sc + 1;
      smid = exprval(idx, ExprTypeSynPtr, &constExpr[1]);

      ins2(*idx + 1, tokLogAnd, sc); // piggyback on && for CG (ugly, but simple)

      ins2(*idx + 1, tokGoto, sc + 1); // jump to end of ?:
      oldIdxLeft = *idx;
      oldSpLeft = sp;
      sr = exprval(idx, &RightExprTypeSynPtr, &constExpr[2]);

      ins2(*idx + 1, tokShortCirc, -sc); // jump to mid if left is non-zero
      sl = exprval(idx, &condTypeSynPtr, &constExpr[0]);

      scalarTypeCheck(condTypeSynPtr);

      decayArray(&RightExprTypeSynPtr, 0);
      decayArray(ExprTypeSynPtr, 0);
      promoteType(&RightExprTypeSynPtr, ExprTypeSynPtr);
      promoteType(ExprTypeSynPtr, &RightExprTypeSynPtr);

      // TBD??? move struct/union-related checks into compatChecks()

      structs = (RightExprTypeSynPtr >= 0 && SyntaxStack[RightExprTypeSynPtr][0] == tokStructPtr) +
              (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == tokStructPtr) * 2;
      if (structs)
      {
        if (structs != 3 ||
            SyntaxStack[RightExprTypeSynPtr][1] != SyntaxStack[*ExprTypeSynPtr][1])
          errorOpType();

        // transform "cond ? a : b" into "*(cond ? &a : &b)"

        if (stack[oldIdxLeft - (oldSpLeft - sp)][0] != tokUnaryStar ||
            stack[oldIdxRight - (oldSpRight - sp)][0] != tokUnaryStar)
          errorInternal(19);

        del(oldIdxLeft - (oldSpLeft - sp), 1); // delete '*'
        del(oldIdxRight - (oldSpRight - sp), 1); // delete '*'
        ins2(oldIdxRight + 2 - (oldSpRight - sp), tokUnaryStar, 0); // use 0 deref size to drop meaningless dereferences
      }
      else
      {
        compatCheck(ExprTypeSynPtr,
                    RightExprTypeSynPtr,
                    &constExpr[1],
                    oldIdxRight - (oldSpRight - sp),
                    oldIdxLeft - (oldSpLeft - sp));
      }

      *ConstExpr = s = 0;

      if (constExpr[0])
      {
        if (truncUint(sl))
        {
          if (constExpr[1])
            *ConstExpr = 1, s = smid;
          // TBD??? else can drop LHS and RHS expressions
        }
        else
        {
          if (constExpr[2])
            *ConstExpr = 1, s = sr;
          // TBD??? else can drop LHS and MID expressions
        }
      }
      simplifyConstExpr(s, *ConstExpr, ExprTypeSynPtr, oldIdxRight + 1 - (oldSpRight - sp), *idx + 1);
    }
    break;

  // Postfix indirect structure/union member selection operator
  case tokArrow:
    {
      int member, i, j = 0, c = 1, ofs = 0;

      stack[*idx + 1][0] = '+'; // replace -> with +
      member = stack[*idx][1]; // keep the member name, it will be replaced with member offset
      stack[*idx][0] = tokNumInt;

      --*idx;
      exprval(idx, ExprTypeSynPtr, ConstExpr);

      if (*ExprTypeSynPtr >= 0 && SyntaxStack[*ExprTypeSynPtr][0] == '*')
        *ExprTypeSynPtr = -(*ExprTypeSynPtr + 1); // TBD!!! shouldn't this be done elsewhere?

      if (*ExprTypeSynPtr >= 0 ||
          SyntaxStack[-*ExprTypeSynPtr][0] != tokStructPtr)
        error("Pointer to or structure or union expected\n");

      i = SyntaxStack[-*ExprTypeSynPtr][1];
      if (i + 2 > SyntaxStackCnt ||
          (SyntaxStack[i][0] != tokStruct && SyntaxStack[i][0] != tokUnion) ||
          SyntaxStack[i + 1][0] != tokTag)
        errorInternal(20);

      if (!GetDeclSize(i, 0))
        // incomplete structure/union type
        errorOpType();

      i += 5; // step inside the {} body of the struct/union
      while (c)
      {
        int t = SyntaxStack[i][0];
        c += (t == '(') - (t == ')') + (t == '{') - (t == '}');
        if (c == 1 &&
            t == tokMemberIdent && SyntaxStack[i][1] == member &&
            SyntaxStack[i + 1][0] == tokLocalOfs)
        {
          j = i;
          ofs = SyntaxStack[i + 1][1];
        }
        i++;
      }
      if (!j)
        error("Undefined structure or union member '%s'\n", IdentTable + member);

      j += 2;
      *ExprTypeSynPtr = -j; // type: pointer to member's type

      stack[oldIdxRight - (oldSpRight - sp)][1] = ofs; // member offset within structure/union

      // optimize a little, if possible
      {
        int i = oldIdxRight - (oldSpRight - sp) - 1;
        // Skip any type cast markers
        while (stack[i][0] == tokUnaryPlus)
          i--;
        // See if the pointer is an integer constant or a local variable offset
        // and if it is, adjust it here instead of generating code for
        // addition/subtraction
        if (stack[i][0] == tokNumInt || stack[i][0] == tokNumUint || stack[i][0] == tokLocalOfs)
        {
          stack[i][1] = uint2int(stack[i][1] + 0u + ofs); // TBD!!! need extra truncation?
          del(oldIdxRight - (oldSpRight - sp), 2);
        }
      }

      *ConstExpr = 0;
    }
    break;

  default:
    //error("exprval(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }

  return s;
}

int ParseExpr(int tok, int* GotUnary, int* ExprTypeSynPtr, int* ConstExpr, int* ConstVal, int option, int option2)
{
  int identFirst = tok == tokIdent;
  *ConstVal = *ConstExpr = 0;
  *ExprTypeSynPtr = SymVoidSynPtr;

  if (!ExprLevel++)
  {
    opsp = sp = 0;
    PurgeStringTable();
  }

  if (option == '=')
    push2(tokIdent, option2);

  tok = expr(tok, GotUnary, option == ',' || option == '=');

  if (tok == tokEof || strchr(",;:)]}", tok) == NULL)
    //error("ParseExpr(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  if (option == '=')
  {
    push('=');
  }
  else if (option == tokGotoLabel && identFirst && tok == ':' && *GotUnary && sp == 1 && stack[sp - 1][0] == tokIdent)
  {
    // This is a label.
    ExprLevel--;
    return tokGotoLabel;
  }

  if (*GotUnary)
  {
    int j;
    // Do this twice so we can see the stack before
    // and after manipulations
    for (j = 0; j < 2; j++)
    {
#ifndef NO_ANNOTATIONS
      int i;
      GenStartCommentLine();
      if (j) printf2("Expanded");
      else printf2("RPN'ized");
      printf2(" expression: \"");
      for (i = 0; i < sp; i++)
      {
        int tok = stack[i][0];
        switch (tok)
        {
        case tokNumInt:
          printf2("%d", truncInt(stack[i][1]));
          break;
        case tokNumUint:
          printf2("%uu", truncUint(stack[i][1]));
          break;
        case tokIdent:
          {
            char* p = IdentTable + stack[i][1];
            if (isdigit(*p))
              printf2("L");
            printf2("%s", p);
          }
          break;
        case tokShortCirc:
          if (stack[i][1] >= 0)
            printf2("[sh&&->%d]", stack[i][1]);
          else
            printf2("[sh||->%d]", -stack[i][1]);
          break;
        case tokLocalOfs:
          printf2("(@%d)", truncInt(stack[i][1]));
          break;
        case tokUnaryStar:
          if (j) printf2("*(%d)", stack[i][1]);
          else printf2("*u");
          break;
        case '(': case ',':
          if (!j) printf2("%c", tok);
          // else printf2("\b");
          break;
        case ')':
          if (j) printf2("(");
          printf2("%c", tok);
          if (j) printf2("%d", stack[i][1]);
          break;
        default:
          printf2("%s", GetTokenName(tok));
          if (j)
          {
            switch (tok)
            {
            case tokLogOr: case tokLogAnd:
              printf2("[%d]", stack[i][1]);
              break;
            case '=':
            case tokInc: case tokDec:
            case tokPostInc: case tokPostDec:
            case tokAssignAdd: case tokAssignSub:
            case tokPostAdd: case tokPostSub:
            case tokAssignMul:
            case tokAssignDiv: case tokAssignMod:
            case tokAssignUDiv: case tokAssignUMod:
            case tokAssignLSh: case tokAssignRSh: case tokAssignURSh:
            case tokAssignAnd: case tokAssignXor: case tokAssignOr:
              printf2("(%d)", stack[i][1]);
              break;
            }
          }
          break;
        }
        printf2(" ");
      }
      printf2("\"\n");
#endif
      if (!j)
      {
        int idx = sp - 1;
        *ConstVal = exprval(&idx, ExprTypeSynPtr, ConstExpr);
        // remove the unneeded unary +'s that have served their cast-substitute purpose
        // also remove dereferences of size 0 (dereferences of pointers to structures)
        for (idx = sp - 1; idx >= 0; idx--)
          if (stack[idx][0] == tokUnaryPlus ||
              (stack[idx][0] == tokUnaryStar && !stack[idx][1]))
            del(idx, 1);
      }
#ifndef NO_ANNOTATIONS
      else if (*ConstExpr)
      {
        GenStartCommentLine();

        switch (SyntaxStack[*ExprTypeSynPtr][0])
        {
        case tokChar:
        case tokSChar:
        case tokUChar:
#ifdef CAN_COMPILE_32BIT
        case tokShort:
        case tokUShort:
#endif
        case tokInt:
          printf2("Expression value: %d\n", truncInt(*ConstVal));
          break;
        default:
        case tokUnsigned:
          printf2("Expression value: %uu\n", truncUint(*ConstVal));
          break;
        }
      }
#endif
    }
  }

  ExprLevel--;

  return tok;
}

// smc.c code

#ifdef __SMALLER_C__
// 2 if va_list is a one-element array containing a pointer
//   (typical for x86 Open Watcom C/C++)
// 1 if va_list is a pointer
//   (typical for Turbo C++, x86 gcc)
// 0 if va_list is something else, and
//   the code may have long crashed by now
int VaListType = 0;

// Attempts to determine the type of va_list as
// expected by the standard library
void DetermineVaListType(void)
{
  void* testptr[2];
  // hopefully enough space to sprintf() 3 pointers using "%p"
  char testbuf[3][CHAR_BIT * sizeof(void*) + 1];

  // TBD!!! This is not good. Really need the va_something macros.
  // Test whether va_list is a pointer to the first optional parameter or
  // an array of one element containing said pointer
  testptr[0] = &testptr[1];
  testptr[1] = &testptr[0];
  memset(testbuf, '\0', sizeof(testbuf));
  sprintf(testbuf[0], "%p", testptr[0]);
  sprintf(testbuf[1], "%p", testptr[1]);
  vsprintf(testbuf[2], "%p", &testptr[0]);
  if (!strcmp(testbuf[2], testbuf[0]))
  {
    // va_list is a pointer
    VaListType = 1;
  }
  else if (!strcmp(testbuf[2], testbuf[1]))
  {
    // va_list is a one-element array containing a pointer
    VaListType = 2;
  }
  else
  {
    // va_list is something else, and
    // the code may have long crashed by now
    printf("Internal error: Indeterminate underlying type of va_list\n");
    exit(-1);
  }
}
#endif

// Equivalent to puts() but outputs to OutFile
// if it's not NULL.
int puts2(char* s)
{
  int res;
  if (OutFile)
  {
    // Turbo C++ 1.01's fputs() returns EOF if s is empty, which is wrong.
    // Hence the workaround.
    if (*s == '\0' || (res = fputs(s, OutFile)) >= 0)
    {
      // unlike puts(), fputs() doesn't append '\n', append it manually
      res = fputc('\n', OutFile);
    }
  }
  else
  {
    res = puts(s);
  }
  return res;
}

// Equivalent to printf() but outputs to OutFile
// if it's not NULL.
int printf2(char* format, ...)
{
  int res;

#ifndef __SMALLER_C__
  va_list vl;
  va_start(vl, format);
#else
  void* vl = &format + 1;
#endif

#ifndef __SMALLER_C__
  if (OutFile)
    res = vfprintf(OutFile, format, vl);
  else
    res = vprintf(format, vl);
#else
  // TBD!!! This is not good. Really need the va_something macros.
  if (VaListType == 1)
  {
    // va_list is a pointer
    if (OutFile)
      res = vfprintf(OutFile, format, vl);
    else
      res = vprintf(format, vl);
  }
  else // if (VaListType == 2)
  {
    // va_list is a one-element array containing a pointer
    if (OutFile)
      res = vfprintf(OutFile, format, &vl);
    else
      res = vprintf(format, &vl);
  }
#endif

#ifndef __SMALLER_C__
  va_end(vl);
#endif

  return res;
}

void error(char* format, ...)
{
  int i, fidx = FileCnt - 1 + !FileCnt;
#ifndef __SMALLER_C__
  va_list vl;
  va_start(vl, format);
#else
  void* vl = &format + 1;
#endif

  for (i = 0; i < FileCnt; i++)
    if (Files[i])
      fclose(Files[i]);

  puts2("");

#ifndef NO_ANNOTATIONS
  DumpSynDecls();
#endif
#ifndef NO_PREPROCESSOR
#ifndef NO_ANNOTATIONS
  DumpMacroTable();
#endif
#endif
#ifndef NO_ANNOTATIONS
  DumpIdentTable();
#endif

  // using stdout implicitly instead of stderr explicitly because:
  // - stderr can be a macro and it's unknown if standard headers
  //   aren't included (which is the case when SmallerC is compiled
  //   with itself and linked with some other compiler's standard
  //   libraries)
  // - output to stderr can interfere/overlap with buffered
  //   output to stdout and the result may literally look ugly

  GenStartCommentLine(); printf2("Compilation failed.\n");

  if (OutFile)
    fclose(OutFile);

  printf("Error in \"%s\" (%d:%d)\n", FileNames[fidx], LineNo, LinePos);

#ifndef __SMALLER_C__
  vprintf(format, vl);
#else
  // TBD!!! This is not good. Really need the va_something macros.
  if (VaListType == 1)
  {
    // va_list is a pointer
    vprintf(format, vl);
  }
  else // if (VaListType == 2)
  {
    // va_list is a one-element array containing a pointer
    vprintf(format, &vl);
  }
#endif

#ifndef __SMALLER_C__
  va_end(vl);
#endif

  exit(-1);
}

void warning(char* format, ...)
{
  int fidx = FileCnt - 1 + !FileCnt;
#ifndef __SMALLER_C__
  va_list vl;
  va_start(vl, format);
#else
  void* vl = &format + 1;
#endif

  warnCnt++;

  if (!(verbose && OutFile))
    return;

  printf("Warning in \"%s\" (%d:%d)\n", FileNames[fidx], LineNo, LinePos);

#ifndef __SMALLER_C__
  vprintf(format, vl);
#else
  // TBD!!! This is not good. Really need the va_something macros.
  if (VaListType == 1)
  {
    // va_list is a pointer
    vprintf(format, vl);
  }
  else // if (VaListType == 2)
  {
    // va_list is a one-element array containing a pointer
    vprintf(format, &vl);
  }
#endif

#ifndef __SMALLER_C__
  va_end(vl);
#endif
}

void errorFile(char* n)
{
  error("Unable to open, read, write or close file \"%s\"\n", n);
}

void errorFileName(void)
{
  error("Invalid or too long file name or path name\n");
}

void errorInternal(int n)
{
  error("%d (internal)\n", n);
}

void errorChrStr(void)
{
  error("Invalid or unsupported character constant or string literal\n");
}

void errorUnexpectedToken(int tok)
{
  error("Unexpected token %s\n", GetTokenName(tok));
}

void errorDirective(void)
{
  error("Invalid or unsupported preprocessor directive\n");
}

void errorCtrlOutOfScope(void)
{
  error("break, continue, case or default in wrong scope\n");
}

void errorDecl(void)
{
  error("Invalid or unsupported declaration\n");
}

void errorTagRedef(int ident)
{
  error("Redefinition of type tagged '%s'\n", IdentTable + ident);
}

void errorRedef(char* s)
{
  error("Redefinition of identifier '%s'\n", s);
}

void errorVarSize(void)
{
  error("Variable(s) take(s) too much space\n");
}

void errorInit(void)
{
  error("Invalid or unsupported initialization\n");
}

void errorUnexpectedVoid(void)
{
  error("Unexpected declaration or expression of type void\n");
}

void errorOpType(void)
{
  error("Unexpected operand type\n");
}

void errorNotLvalue(void)
{
  error("lvalue expected\n");
}

void errorNotConst(void)
{
  error("Non-constant expression\n");
}

void errorLongExpr(void)
{
  error("Too long expression\n");
}

int tsd[] =
{
  tokVoid, tokChar, tokInt,
  tokSigned, tokUnsigned, tokShort,
  tokStruct, tokUnion,
};

int TokenStartsDeclaration(int t, int params)
{
#ifndef NO_TYPEDEF_ENUM
  int CurScope;
#endif
  unsigned i;

  for (i = 0; i < sizeof tsd / sizeof tsd[0]; i++)
    if (tsd[i] == t)
      return 1;

  return
#ifdef CAN_COMPILE_32BIT
         (SizeOfWord != 2 && t == tokLong) ||
#endif
#ifndef NO_TYPEDEF_ENUM
         t == tokEnum ||
         (t == tokIdent &&
          FindTypedef(GetTokenIdentName(), &CurScope, 1) >= 0) ||
#endif
         (!params && (t == tokExtern ||
#ifndef NO_TYPEDEF_ENUM
                      t == tokTypedef ||
#endif
                      t == tokStatic));
}

void PushSyntax2(int t, int v)
{
  if (SyntaxStackCnt >= SYNTAX_STACK_MAX)
    error("Symbol table exhausted\n");
  SyntaxStack[SyntaxStackCnt][0] = t;
  SyntaxStack[SyntaxStackCnt++][1] = v;
}

void PushSyntax(int t)
{
  PushSyntax2(t, 0);
}

void InsertSyntax2(int pos, int t, int v)
{
  if (SyntaxStackCnt >= SYNTAX_STACK_MAX)
    error("Symbol table exhausted\n");
  memmove(SyntaxStack[pos + 1],
          SyntaxStack[pos],
          sizeof(SyntaxStack[0]) * (SyntaxStackCnt - pos));
  SyntaxStack[pos][0] = t;
  SyntaxStack[pos][1] = v;
  SyntaxStackCnt++;
}

void InsertSyntax(int pos, int t)
{
  InsertSyntax2(pos, t, 0);
}

void DeleteSyntax(int pos, int cnt)
{
  memmove(SyntaxStack[pos],
          SyntaxStack[pos + cnt],
          sizeof(SyntaxStack[0]) * (SyntaxStackCnt - (pos + cnt)));
  SyntaxStackCnt -= cnt;
}

int FindSymbol(char* s)
{
  int i;

  // TBD!!! return declaration scope number so
  // redeclarations can be reported if occur in the same scope.

  // TBD??? Also, I could first use FindIdent() and then just look for the
  // index into IdentTable[] instead of doing strcmp()

  for (i = SyntaxStackCnt - 1; i >= 0; i--)
  {
    int t = SyntaxStack[i][0];
    if (t == tokIdent &&
        !strcmp(IdentTable + SyntaxStack[i][1], s))
    {
      return i;
    }

    if (t == ')')
    {
      // Skip over the function params
      int c = -1;
      while (c)
      {
        t = SyntaxStack[--i][0];
        c += (t == '(') - (t == ')');
      }
    }
  }

  return -1;
}

int SymType(int SynPtr)
{
  int local = 0;

  if (SyntaxStack[SynPtr][0] == tokIdent)
    SynPtr++;

  if ((local = SyntaxStack[SynPtr][0] == tokLocalOfs) != 0)
    SynPtr++;

  switch (SyntaxStack[SynPtr][0])
  {
  case '(':
    return SymFxn;

  case '[':
    if (local)
      return SymLocalArr;
    return SymGlobalArr;

  default:
    if (local)
      return SymLocalVar;
    return SymGlobalVar;
  }
}

int FindTaggedDecl(char* s, int start, int* CurScope)
{
  int i;

  *CurScope = 1;

  for (i = start; i >= 0; i--)
  {
    int t = SyntaxStack[i][0];
    if (t == tokTag &&
        !strcmp(IdentTable + SyntaxStack[i][1], s))
    {
      return i - 1;
    }
    else if (t == ')')
    {
      // Skip over the function params
      int c = -1;
      while (c)
      {
        t = SyntaxStack[--i][0];
        c += (t == '(') - (t == ')');
      }
    }
    else if (t == '#')
    {
      // the scope has changed to the outer scope
      *CurScope = 0;
    }
  }

  return -1;
}

#ifndef NO_TYPEDEF_ENUM
// TBD??? rename this fxn? Cleanup/unify search functions?
int FindTypedef(char* s, int* CurScope, int forUse)
{
  int i;

  *CurScope = 1;

  for (i = SyntaxStackCnt - 1; i >= 0; i--)
  {
    int t = SyntaxStack[i][0];
    if ((t == tokTypedef || t == tokIdent) &&
        !strcmp(IdentTable + SyntaxStack[i][1], s))
    {
      // if the closest declaration isn't from typedef,
      // (i.e. if it's a variable/function declaration),
      // then the type is unknown for the purpose of
      // declaring a variable of this type
      if (forUse && t == tokIdent)
        return -1;
      return i;
    }

    if (t == ')')
    {
      // Skip over the function params
      int c = -1;
      while (c)
      {
        t = SyntaxStack[--i][0];
        c += (t == '(') - (t == ')');
      }
    }
    else if (t == '#')
    {
      // the scope has changed to the outer scope
      *CurScope = 0;
    }
  }

  return -1;
}
#endif

int GetDeclSize(int SyntaxPtr, int SizeForDeref)
{
  int i;
  unsigned size = 1;
  int arr = 0;

  if (SyntaxPtr < 0)
    errorInternal(10);

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    int tok = SyntaxStack[i][0];
    switch (tok)
    {
    case tokIdent: // skip leading identifiers, if any
    case tokLocalOfs: // skip local var offset, too
      break;
    case tokChar:
    case tokSChar:
      if (!arr && ((tok == tokSChar) || CharIsSigned) && SizeForDeref)
        return -1; // 1 byte, needing sign extension when converted to int/unsigned int
      // fallthrough
    case tokUChar:
      return uint2int(size);
#ifdef CAN_COMPILE_32BIT
    case tokShort:
      if (!arr && SizeForDeref)
        return -2; // 2 bytes, needing sign extension when converted to int/unsigned int
      // fallthrough
    case tokUShort:
      if (size * 2 / 2 != size)
        //error("Variable too big\n");
        errorVarSize();
      size *= 2;
      if (size != truncUint(size))
        //error("Variable too big\n");
        errorVarSize();
      return uint2int(size);
#endif
    case tokInt:
    case tokUnsigned:
    case '*':
    case '(': // size of fxn = size of ptr for now
      if (size * SizeOfWord / SizeOfWord != size)
        //error("Variable too big\n");
        errorVarSize();
      size *= SizeOfWord;
      if (size != truncUint(size))
        //error("Variable too big\n");
        errorVarSize();
      return uint2int(size);
    case '[':
      if (SyntaxStack[i + 1][0] != tokNumInt && SyntaxStack[i + 1][0] != tokNumUint)
        errorInternal(11);
      if (SyntaxStack[i + 1][1] &&
          size * SyntaxStack[i + 1][1] / SyntaxStack[i + 1][1] != size)
        //error("Variable too big\n");
        errorVarSize();
      size *= SyntaxStack[i + 1][1];
      if (size != truncUint(size))
        //error("Variable too big\n");
        errorVarSize();
      i += 2;
      arr = 1;
      break;
    case tokStructPtr:
      // follow the "type pointer"
      i = SyntaxStack[i][1] - 1;
      break;
    case tokStruct:
    case tokUnion:
      if (i + 2 < SyntaxStackCnt && SyntaxStack[i + 2][0] == tokSizeof && !SizeForDeref)
      {
        unsigned s = SyntaxStack[i + 2][1];
        if (s && size * s / s != size)
          errorVarSize();
        size *= s;
        if (size != truncUint(size))
          errorVarSize();
        return uint2int(size);
      }
      return 0;
    case tokVoid:
      return 0;
    default:
      errorInternal(12);
    }
  }

  errorInternal(13);
  return 0;
}

int GetDeclAlignment(int SyntaxPtr)
{
  int i;

  if (SyntaxPtr < 0)
    errorInternal(14);

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    int tok = SyntaxStack[i][0];
    switch (tok)
    {
    case tokIdent: // skip leading identifiers, if any
    case tokLocalOfs: // skip local var offset, too
      break;
    case tokChar:
    case tokSChar:
    case tokUChar:
      return 1;
#ifdef CAN_COMPILE_32BIT
    case tokShort:
    case tokUShort:
      return 2;
#endif
    case tokInt:
    case tokUnsigned:
    case '*':
    case '(':
      return SizeOfWord;
    case '[':
      if (SyntaxStack[i + 1][0] != tokNumInt && SyntaxStack[i + 1][0] != tokNumUint)
        errorInternal(15);
      i += 2;
      break;
    case tokStructPtr:
      // follow the "type pointer"
      i = SyntaxStack[i][1] - 1;
      break;
    case tokStruct:
    case tokUnion:
      if (i + 3 < SyntaxStackCnt && SyntaxStack[i + 2][0] == tokSizeof)
      {
        return SyntaxStack[i + 3][1];
      }
      return 1;
    case tokVoid:
      return 1;
    default:
      errorInternal(16);
    }
  }

  errorInternal(17);
  return 0;
}

#ifndef NO_ANNOTATIONS
void DumpDecl(int SyntaxPtr, int IsParam)
{
  int i;
  int icnt = 0;

  if (SyntaxPtr < 0)
    return;

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    int tok = SyntaxStack[i][0];
    int v = SyntaxStack[i][1];
    switch (tok)
    {
    case tokLocalOfs:
      printf2("(@%d): ", truncInt(v));
      break;

    case tokIdent:
      if (++icnt > 1 && !IsParam) // show at most one declaration, except params
        return;

      GenStartCommentLine();

      if (ParseLevel == 0)
        printf2("glb ");
      else if (IsParam)
        printf2("prm ");
      else
        printf2("loc ");

      {
        int j;
        for (j = 0; j < ParseLevel * 4; j++)
          printf2(" ");
      }

      if (IsParam && !strcmp(IdentTable + v, "<something>") && (i + 1 < SyntaxStackCnt))
      {
        if (SyntaxStack[i + 1][0] == tokEllipsis)
          continue;
      }

      printf2("%s : ", IdentTable + v);
      break;

    case '[':
      printf2("[");
      break;

    case tokNumInt:
      printf2("%d", truncInt(v));
      break;
    case tokNumUint:
      printf2("%uu", truncUint(v));
      break;

    case ']':
      printf2("] ");
      break;

    case '(':
      {
        int noparams;
        // Skip over the params to the base type
        int j = ++i, c = 1;
        while (c)
        {
          int t = SyntaxStack[j++][0];
          c += (t == '(') - (t == ')');
        }

        noparams = (i + 1 == j) || (SyntaxStack[i + 1][0] == tokVoid);

        printf2("(");

        // Print the params (recursively)
        if (noparams)
        {
          // Don't recurse if it's "fxn()" or "fxn(void)"
          if (i + 1 != j)
            printf2("void");
        }
        else
        {
          puts2("");
          ParseLevel++;
          DumpDecl(i, 1);
          ParseLevel--;
        }

        // Continue normally
        i = j - 1;
        if (!noparams)
        {
          GenStartCommentLine();
          printf2("    ");
          {
            int j;
            for (j = 0; j < ParseLevel * 4; j++)
              printf2(" ");
          }
        }

        printf2(") ");
      }
      break;

    case ')': // end of param list
      return;

    case tokStructPtr:
      DumpDecl(v, 0);
      break;

    default:
      switch (tok)
      {
      case tokVoid:
      case tokChar:
      case tokSChar:
      case tokUChar:
#ifdef CAN_COMPILE_32BIT
      case tokShort:
      case tokUShort:
#endif
      case tokInt:
      case tokUnsigned:
      case tokEllipsis:
        printf2("%s\n", GetTokenName(tok));
        break;
      default:
        printf2("%s ", GetTokenName(tok));
        break;
      case tokTag:
        printf2("%s\n", IdentTable + v);
        return;
      }
      break;
    }
  }
}
#endif

#ifndef NO_ANNOTATIONS
void DumpSynDecls(void)
{
  int used = SyntaxStackCnt * sizeof SyntaxStack[0];
  int total = SYNTAX_STACK_MAX * sizeof SyntaxStack[0];
  puts2("");
  GenStartCommentLine(); printf2("Syntax/declaration table/stack:\n");
  GenStartCommentLine(); printf2("Bytes used: %d/%d\n\n", used, total);
}
#endif

int ParseArrayDimension(int AllowEmptyDimension)
{
  int tok;
  int gotUnary, synPtr, constExpr, exprVal;
  unsigned exprValU;
  int oldssp, oldesp, undoIdents;

  tok = GetToken();
  // DONE: support arbitrary constant expressions
  oldssp = SyntaxStackCnt;
  oldesp = sp;
  undoIdents = IdentTableLen;
  tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0);
  IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof"
  SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" in the expression
  sp = oldesp;

  if (tok != ']')
    //error("ParseArrayDimension(): Unsupported or invalid array dimension (token %s)\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  if (!gotUnary)
  {
    if (!AllowEmptyDimension)
      //error("ParseArrayDimension(): missing array dimension\n");
      errorUnexpectedToken(tok);
    // Empty dimension is dimension of 0
    exprVal = 0;
  }
  else
  {
    if (!constExpr)
      //error("ParseArrayDimension(): non-constant array dimension\n");
      errorNotConst();

    exprValU = truncUint(exprVal);
    exprVal = truncInt(exprVal);

    promoteType(&synPtr, &synPtr);
    if ((SyntaxStack[synPtr][0] == tokInt && exprVal < 1) || (SyntaxStack[synPtr][0] == tokUnsigned && exprValU < 1))
      error("Array dimension less than 1\n");

    exprVal = uint2int(exprValU);
  }

  PushSyntax2(tokNumUint, exprVal);
  return tok;
}

void ParseFxnParams(int tok);
int ParseBlock(int BrkCntSwchTarget[4], int switchBody);
void AddFxnParamSymbols(int SyntaxPtr);

int ParseBase(int tok, int base[2])
{
#ifndef NO_TYPEDEF_ENUM
  int CurScope;
#endif
  int valid = 1;
  base[1] = 0;

  if ((tok == tokVoid) |
      (tok == tokChar) |
      (tok == tokInt))
  {
    *base = tok;
    tok = GetToken();
  }
  else if (tok == tokShort
#ifdef CAN_COMPILE_32BIT
           || tok == tokLong
#endif
          )
  {
    *base = tok;
    tok = GetToken();
    if (tok == tokInt)
      tok = GetToken();
  }
  else if ((tok == tokSigned) |
           (tok == tokUnsigned))
  {
    int sign = tok;
    tok = GetToken();

    if (tok == tokChar)
    {
      if (sign == tokUnsigned)
        *base = tokUChar;
      else
        *base = tokSChar;
      tok = GetToken();
    }
    else if (tok == tokShort)
    {
      if (sign == tokUnsigned)
        *base = tokUShort;
      else
        *base = tokShort;
      tok = GetToken();
      if (tok == tokInt)
        tok = GetToken();
    }
#ifdef CAN_COMPILE_32BIT
    else if (tok == tokLong)
    {
      if (sign == tokUnsigned)
        *base = tokULong;
      else
        *base = tokLong;
      tok = GetToken();
      if (tok == tokInt)
        tok = GetToken();
    }
#endif
    else
    {
      if (sign == tokUnsigned)
        *base = tokUnsigned;
      else
        *base = tokInt;
      if (tok == tokInt)
        tok = GetToken();
    }
  }
  else if ((tok == tokStruct) |
           (tok == tokUnion)
#ifndef NO_TYPEDEF_ENUM
           | (tok == tokEnum)
#endif
          )
  {
    int structType = tok;
    int empty = 1;
    int typePtr = SyntaxStackCnt;
    int gotTag = 0, tagIdent = 0, declPtr = -1, curScope = 0;

    tok = GetToken();

    if (tok == tokIdent)
    {
      // this is a structure/union/enum tag
      gotTag = 1;
      declPtr = FindTaggedDecl(GetTokenIdentName(), SyntaxStackCnt - 1, &curScope);
      tagIdent = AddIdent(GetTokenIdentName());

      if (declPtr >= 0)
      {
        // Within the same scope we can't declare more than one union, structure or enum
        // with the same tag.
        // There's one common tag namespace for structures, unions and enumerations.
        if (curScope && SyntaxStack[declPtr][0] != structType)
          errorTagRedef(tagIdent);
      }
      else if (ParamLevel)
      {
        // structure/union/enum declarations aren't supported in function parameters
        errorDecl();
      }

      tok = GetToken();
    }
    else
    {
      // structure/union/enum declarations aren't supported in expressions
      if (ExprLevel)
        errorDecl();
      PushSyntax(structType);
      PushSyntax2(tokTag, AddIdent("<something>"));
    }

    if (tok == '{')
    {
      unsigned structInfo[4], sz, alignment, tmp;

      // structure/union/enum declarations aren't supported in expressions and function parameters
      if (ExprLevel || ParamLevel)
        errorDecl();

      if (gotTag)
      {
        // Cannot redefine a tagged structure/union/enum within the same scope
        if (declPtr >= 0 &&
            curScope &&
            ((declPtr + 2 < SyntaxStackCnt && SyntaxStack[declPtr + 2][0] == tokSizeof)
#ifndef NO_TYPEDEF_ENUM
             || structType == tokEnum
#endif
            ))
          errorTagRedef(tagIdent);

        PushSyntax(structType);
        PushSyntax2(tokTag, tagIdent);
      }

#ifndef NO_TYPEDEF_ENUM
      if (structType == tokEnum)
      {
        int val = 0;
        int CurScope;

        tok = GetToken();
        while (tok != '}')
        {
          char* s;
          int ident;

          if (tok != tokIdent)
            errorUnexpectedToken(tok);

          s = GetTokenIdentName();
          if (FindTypedef(s, &CurScope, 0) >= 0 && CurScope)
            errorRedef(s);

          ident = AddIdent(s);

          empty = 0;

          tok = GetToken();
          if (tok == '=')
          {
            int gotUnary, synPtr, constExpr;
            int oldssp, oldesp, undoIdents;

            oldssp = SyntaxStackCnt;
            oldesp = sp;
            undoIdents = IdentTableLen;

            tok = ParseExpr(GetToken(), &gotUnary, &synPtr, &constExpr, &val, ',', 0);

            IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof"
            SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" in the expression
            sp = oldesp;

            if (!gotUnary)
              errorUnexpectedToken(tok);
            if (!constExpr)
              errorNotConst();
          }

          PushSyntax2(tokIdent, ident);
          PushSyntax2(tokNumInt, val);
          val = uint2int(val + 1u);

          if (tok == ',')
            tok = GetToken();
          else if (tok != '}')
            errorUnexpectedToken(tok);
        }

        if (empty)
          errorUnexpectedToken('}');

        base[0] = tokEnumPtr;
        base[1] = typePtr;

        tok = GetToken();
        return tok;
      }
      else
#endif
      {
        structInfo[0] = structType;
        structInfo[1] = 1; // initial member alignment
        structInfo[2] = 0; // initial member offset
        structInfo[3] = 0; // initial max member size (for unions)

        PushSyntax(tokSizeof); // 0 = initial structure/union size, to be updated
        PushSyntax2(tokSizeof, 1); // 1 = initial structure/union alignment, to be updated

        PushSyntax('{');

        tok = GetToken();
        while (tok != '}')
        {
          if (!TokenStartsDeclaration(tok, 1))
            errorUnexpectedToken(tok);
          tok = ParseDecl(tok, structInfo, 0, 0);
          empty = 0;
        }

        if (empty)
          errorUnexpectedToken('}');

        PushSyntax('}');

        // Update structure/union alignment
        alignment = structInfo[1];
        SyntaxStack[typePtr + 3][1] = alignment;

        // Update structure/union size and include trailing padding if needed
        sz = structInfo[2] + structInfo[3];
        tmp = sz;
        sz = (sz + alignment - 1) & ~(alignment - 1);
        if (sz < tmp || sz != truncUint(sz))
          errorVarSize();
        SyntaxStack[typePtr + 2][1] = uint2int(sz);

        tok = GetToken();
      }
    }
    else
    {
#ifndef NO_TYPEDEF_ENUM
      if (structType == tokEnum)
      {
        if (!gotTag || declPtr < 0)
          errorDecl(); // TBD!!! different error when enum tag is not found

        base[0] = tokEnumPtr;
        base[1] = declPtr;
        return tok;
      }
#endif

      if (gotTag)
      {
        if (declPtr >= 0 &&
            SyntaxStack[declPtr][0] == structType)
        {
          base[0] = tokStructPtr;
          base[1] = declPtr;
          return tok;
        }

        PushSyntax(structType);
        PushSyntax2(tokTag, tagIdent);

        empty = 0;
      }
    }

    if (empty)
      errorDecl();

    base[0] = tokStructPtr;
    base[1] = typePtr;

    // If we've just defined a structure/union and there are
    // preceding references to this tag within this scope,
    // IOW references to an incomplete type, complete the
    // type in the references
    if (gotTag && SyntaxStack[SyntaxStackCnt - 1][0] == '}')
    {
      int i;
      for (i = SyntaxStackCnt - 1; i >= 0; i--)
        if (SyntaxStack[i][0] == tokStructPtr)
        {
          int j = SyntaxStack[i][1];
          if (SyntaxStack[j + 1][1] == tagIdent)
            SyntaxStack[i][1] = typePtr;
        }
        else if (SyntaxStack[i][0] == '#')
        {
          // reached the beginning of the current scope
          break;
        }
    }
  }
#ifndef NO_TYPEDEF_ENUM
  else if (tok == tokIdent &&
           (base[1] = FindTypedef(GetTokenIdentName(), &CurScope, 1)) >= 0)
  {
    base[0] = tokTypedef;
    tok = GetToken();
  }
#endif
  else
  {
    valid = 0;
  }

#ifdef CAN_COMPILE_32BIT
  if (SizeOfWord == 2 &&
      (*base == tokLong || *base == tokULong))
    valid = 0;
  // to simplify matters, treat long and unsigned long as aliases for int and unsigned int
  // in 32-bit and huge mode(l)s
  if (*base == tokLong)
    *base = tokInt;
  if (*base == tokULong)
    *base = tokUnsigned;
#endif

  if (SizeOfWord == 2)
  {
    // to simplify matters, treat short and unsigned short as aliases for int and unsigned int
    // in 16-bit mode
    if (*base == tokShort)
      *base = tokInt;
    if (*base == tokUShort)
      *base = tokUnsigned;
  }

  // TBD!!! review/test this fxn
//  if (!valid || !tok || !(strchr("*([,)", tok) || tok == tokIdent))
  if (!valid | !tok)
    //error("ParseBase(): Invalid or unsupported type\n");
    error("Invalid or unsupported type\n");

  return tok;
}

/*
  base * name []  ->  name : [] * base
  base *2 (*1 name []1) []2  ->  name : []1 *1 []2 *2 base
  base *3 (*2 (*1 name []1) []2) []3  ->  name : []1 *1 []2 *2 []3 *3 base
*/

int ParseDerived(int tok)
{
  int stars = 0;
  int params = 0;

  while (tok == '*')
  {
    stars++;
    tok = GetToken();
  }

  if (tok == '(')
  {
    tok = GetToken();
    if (tok != ')' && !TokenStartsDeclaration(tok, 1))
    {
      tok = ParseDerived(tok);
      if (tok != ')')
        //error("ParseDerived(): ')' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
    else
    {
      params = 1;
    }
  }
  else if (tok == tokIdent)
  {
    PushSyntax2(tok, AddIdent(GetTokenIdentName()));
    tok = GetToken();
  }
  else
  {
    PushSyntax2(tokIdent, AddIdent("<something>"));
  }

  if (params | (tok == '('))
  {
    int t = SyntaxStack[SyntaxStackCnt - 1][0];
    if ((t == ')') | (t == ']'))
      errorUnexpectedToken('('); // array of functions or function returning function
    if (!params)
      tok = GetToken();
    else
      PushSyntax2(tokIdent, AddIdent("<something>"));
    PushSyntax('(');
    ParseLevel++;
    ParamLevel++;
    ParseFxnParams(tok);
    ParamLevel--;
    ParseLevel--;
    PushSyntax(')');
    tok = GetToken();
  }
  else if (tok == '[')
  {
    // DONE!!! allow the first [] without the dimension in function parameters
    int allowEmptyDimension = 1;
    if (SyntaxStack[SyntaxStackCnt - 1][0] == ')')
      errorUnexpectedToken('['); // function returning array
    while (tok == '[')
    {
      int oldsp = SyntaxStackCnt;
      PushSyntax(tokVoid); // prevent cases like "int arr[arr];" and "int arr[arr[0]];"
      PushSyntax(tok);
      tok = ParseArrayDimension(allowEmptyDimension);
      if (tok != ']')
        //error("ParseDerived(): ']' expected\n");
        errorUnexpectedToken(tok);
      PushSyntax(']');
      tok = GetToken();
      DeleteSyntax(oldsp, 1);
      allowEmptyDimension = 0;
    }
  }

  while (stars--)
    PushSyntax('*');

  if (!tok || !strchr(",;{=)", tok))
    //error("ParseDerived(): unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);

  return tok;
}

void PushBase(int base[2])
{
#ifndef NO_TYPEDEF_ENUM
  if (base[0] == tokTypedef)
  {
    int ptr = base[1];
    int c = 0, copying = 1;

    while (copying)
    {
      int tok = SyntaxStack[++ptr][0];
      int t = SyntaxStack[SyntaxStackCnt - 1][0];

      // Cannot have:
      //   function returning function
      //   array of functions
      //   function returning array
      if (((t == ')' || t == ']') && tok == '(') ||
          (t == ')' && tok == '['))
        errorDecl();

      PushSyntax2(tok, SyntaxStack[ptr][1]);

      c += (tok == '(') - (tok == ')') + (tok == '[') - (tok == ']');

      if (!c)
      {
        switch (tok)
        {
        case tokVoid:
        case tokChar: case tokSChar: case tokUChar:
#ifdef CAN_COMPILE_32BIT
        case tokShort: case tokUShort:
#endif
        case tokInt: case tokUnsigned:
        case tokStructPtr:
          copying = 0;
        }
      }
    }
  }
  else
#endif
  {
    PushSyntax2(base[0], base[1]);
  }

  // Cannot have array of void
  if (SyntaxStack[SyntaxStackCnt - 1][0] == tokVoid &&
      SyntaxStack[SyntaxStackCnt - 2][0] == ']')
    errorUnexpectedVoid();
}

int InitScalar(int synPtr, int tok);
int InitArray(int synPtr, int tok);
int InitStruct(int synPtr, int tok);

int InitVar(int synPtr, int tok)
{
  int p = synPtr, t;
  int undoIdents = IdentTableLen;

  while ((SyntaxStack[p][0] == tokIdent) | (SyntaxStack[p][0] == tokLocalOfs))
    p++;

  PurgeStringTable();
  KeepStringTable = 1;

  t = SyntaxStack[p][0];
  if (t == '[')
    tok = InitArray(p, tok);
  else if (t == tokStructPtr)
    tok = InitStruct(p, tok);
  else
    tok = InitScalar(p, tok);

  if (!strchr(",;", tok))
    errorUnexpectedToken(tok);

  {
    int lab;
    char s[1 + (2 + CHAR_BIT * sizeof lab) / 3];
    int i = 0;

    // Construct an expression for each buffered string for GenStrData()
    sp = 1;
    stack[0][0] = tokIdent;

    // Dump all buffered strings, one by one, the ugly way
    while (i < StringTableLen)
    {
      char *p = s + sizeof s;

      lab = StringTable[i] & 0xFF;
      lab += (StringTable[i + 1] & 0xFFu) << 8;

      // Reconstruct the identifier for the definition: char #[len] = "...";
      *--p = '\0';
      p = lab2str(p, lab);
      stack[0][1] = AddIdent(p);

      GenStrData(0, 0);

      // Drop the identifier from the identifier table so as not to
      // potentially overflow it when there are many initializing
      // string literals and the table is nearly full.
      IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof" or "str"

      i += 2;
      i += 1 + StringTable[i];
    }
  }

  PurgeStringTable();
  KeepStringTable = 0;

  return tok;
}

int InitScalar(int synPtr, int tok)
{
  unsigned elementSz = GetDeclSize(synPtr, 0);
  int gotUnary, synPtr2, constExpr, exprVal;
  int oldssp = SyntaxStackCnt;
  int undoIdents = IdentTableLen;

  tok = ParseExpr(tok, &gotUnary, &synPtr2, &constExpr, &exprVal, ',', 0);

  if (!gotUnary)
    errorUnexpectedToken(tok);

  scalarTypeCheck(synPtr2);

  if (stack[sp - 1][0] == tokNumInt || stack[sp - 1][0] == tokNumUint)
  {
    // TBD??? truncate values for types smaller than int (e.g. char and short),
    // so they are always in range?
    GenIntData(elementSz, stack[0][1]);
  }
  else if (elementSz == SizeOfWord + 0u && stack[sp - 1][0] == tokIdent)
  {
    GenAddrData(elementSz, IdentTable + stack[sp - 1][1]);
    // Defer storage of string literal data (if any) until the end.
    // This will let us generate the contiguous array of pointers to
    // string literals unperturbed by the string literal data
    // (e.g. "char* colors[] = { "red", "green", "blue" };").
  }
  else
    //error("ParseDecl(): cannot initialize a global variable with a non-constant expression\n");
    errorNotConst();

  IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof" or "str"
  SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" or "str" in the expression
  return tok;
}

int InitArray(int synPtr, int tok)
{
  int elementTypePtr = synPtr + 3;
  int elementType = SyntaxStack[elementTypePtr][0];
  unsigned elementSz = GetDeclSize(elementTypePtr, 0);
  int braces = 0;
  unsigned elementCnt = 0;
  unsigned elementsRequired = SyntaxStack[synPtr + 1][1];
  int arrOfChar = (elementType == tokChar) | (elementType == tokUChar) | (elementType == tokSChar);

  if (tok == '{')
  {
    braces = 1;
    tok = GetToken();
  }
  else
  {
    // Only fully-bracketed (sic) initialization of arrays is supported,
    // except for arrays of char initialized with string literals
    // (the string literals may be optionally enclosed in braces)
    if (!(arrOfChar & (tok == tokLitStr)))
      errorUnexpectedToken(tok);
  }

  if (arrOfChar & (tok == tokLitStr))
  {
    // this is 'someArray[someCountIfAny] = "some string"' or
    // 'someArray[someCountIfAny] = { "some string" }'
    int gotUnary, synPtr2, constExpr, exprVal;
    int oldssp = SyntaxStackCnt;
    int undoIdents = IdentTableLen;
    int slen = StringTableLen;

    tok = ParseExpr(tok, &gotUnary, &synPtr2, &constExpr, &exprVal, ',', 0);

    if (!gotUnary ||
        stack[sp - 1][0] != tokIdent ||
        !isdigit(IdentTable[stack[sp - 1][1]]))
      errorInit();

    elementCnt = GenStrData(0, elementsRequired);

    StringTableLen = slen; // don't accumulate strings initializing arrays of char
    IdentTableLen = undoIdents; // remove all temporary identifier names from e.g. "sizeof" or "str"
    SyntaxStackCnt = oldssp; // undo any temporary declarations from e.g. "sizeof" or "str" in the expression

    if (braces)
    {
      if (tok != '}')
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
  }
  else
  {
    while (tok != '}')
    {
      if (elementType == '[')
      {
        tok = InitArray(elementTypePtr, tok);
      }
      else if (elementType == tokStructPtr)
      {
        tok = InitStruct(elementTypePtr, tok);
      }
      else
      {
        tok = InitScalar(elementTypePtr, tok);
      }

      if (++elementCnt > elementsRequired && elementsRequired)
        error("Too many array initializers\n");

      if (tok == ',')
        tok = GetToken();
      else if (tok != '}')
        errorUnexpectedToken(tok);
    }

    if (!elementCnt)
      errorUnexpectedToken('}');

    if (elementCnt < elementsRequired)
      GenZeroData((elementsRequired - elementCnt) * elementSz);

    tok = GetToken();
  }

  // Store the element count if it's an incomplete array
  if (!elementsRequired)
    SyntaxStack[synPtr + 1][1] = elementCnt;

  return tok;
}

int InitStruct(int synPtr, int tok)
{
  int isUnion;
  unsigned size, ofs = 0;

  synPtr = SyntaxStack[synPtr][1];
  isUnion = SyntaxStack[synPtr++][0] == tokUnion;
  size = SyntaxStack[++synPtr][1];
  synPtr += 3; // step inside the {} body of the struct/union

  if (tok != '{')
    errorUnexpectedToken(tok);
  tok = GetToken();

  while (tok != '}')
  {
    int c = 1;
    int elementTypePtr, elementType;
    unsigned elementOfs, elementSz;

    // Find the next member or the closing brace
    while (c)
    {
      int t = SyntaxStack[synPtr][0];
      c += (t == '(') - (t == ')') + (t == '{') - (t == '}');
      if (c == 1 && t == tokMemberIdent)
        break;
      synPtr++;
    }
    if (!c)
      errorUnexpectedToken(tok); // too many initializers

    elementOfs = SyntaxStack[++synPtr][1];
    elementTypePtr = ++synPtr;
    elementType = SyntaxStack[elementTypePtr][0];
    elementSz = GetDeclSize(elementTypePtr, 0);

    // Alignment
    if (ofs < elementOfs)
      GenZeroData(elementOfs - ofs);

    if (elementType == '[')
    {
      tok = InitArray(elementTypePtr, tok);
    }
    else if (elementType == tokStructPtr)
    {
      tok = InitStruct(elementTypePtr, tok);
    }
    else
    {
      tok = InitScalar(elementTypePtr, tok);
    }

    ofs = elementOfs + elementSz;

    if (tok == ',')
      tok = GetToken();
    else if (tok != '}')
      errorUnexpectedToken(tok);

    // Only one member (first) is initialized in unions explicitly
    if (isUnion && tok != '}')
      errorUnexpectedToken(tok);
  }

  if (!ofs)
    errorUnexpectedToken('}');

  // Implicit initialization of the rest and trailing padding
  if (ofs < size)
    GenZeroData(size - ofs);

  tok = GetToken();

  return tok;
}

// DONE: support extern
// DONE: support static
// DONE: support basic initialization
// DONE: support simple non-array initializations with string literals
// DONE: support basic 1-d array initialization
// DONE: global/static data allocations
int ParseDecl(int tok, unsigned structInfo[4], int cast, int label)
{
  int base[2];
  int lastSyntaxPtr;
  int external = tok == tokExtern;
  int Static = tok == tokStatic;
#ifndef NO_TYPEDEF_ENUM
  int typeDef = tok == tokTypedef;
#endif

  if (external |
#ifndef NO_TYPEDEF_ENUM
      typeDef |
#endif
      Static)
  {
    tok = GetToken();
    if (!TokenStartsDeclaration(tok, 1))
      //error("ParseDecl(): unexpected token %s\n", GetTokenName(tok));
      // Implicit int (as in "extern x; static y;") isn't supported
      errorUnexpectedToken(tok);
  }
  tok = ParseBase(tok, base);

#ifndef NO_TYPEDEF_ENUM
  if (label && tok == ':' && base[0] == tokTypedef &&
      !(external | Static | typeDef) && ParseLevel)
  {
    // This is a label.
    return tokGotoLabel;
  }
#endif

  for (;;)
  {
    lastSyntaxPtr = SyntaxStackCnt;

    /* derived type */
    tok = ParseDerived(tok);

    /* base type */
    PushBase(base);

    if ((tok && strchr(",;{=", tok)) || (tok == ')' && ExprLevel))
    {
      int isLocal = 0, isGlobal = 0, isFxn, isStruct, isArray, isIncompleteArr;
      unsigned alignment = 0;

      // Disallow void variables
      if (SyntaxStack[SyntaxStackCnt - 1][0] == tokVoid)
      {
        if (SyntaxStack[SyntaxStackCnt - 2][0] == tokIdent &&
            !(cast
#ifndef NO_TYPEDEF_ENUM
              | typeDef
#endif
             ))
          //error("ParseDecl(): Cannot declare a variable ('%s') of type 'void'\n", IdentTable + SyntaxStack[lastSyntaxPtr][1]);
          errorUnexpectedVoid();
      }

      isFxn = SyntaxStack[lastSyntaxPtr + 1][0] == '(';

      if (isFxn &&
          SyntaxStack[SyntaxStackCnt - 1][0] == tokStructPtr &&
          SyntaxStack[SyntaxStackCnt - 2][0] == ')')
        // structure returning isn't supported currently
        errorDecl();

      isArray = SyntaxStack[lastSyntaxPtr + 1][0] == '[';
      isIncompleteArr = isArray && SyntaxStack[lastSyntaxPtr + 2][1] == 0;

      isStruct = SyntaxStack[lastSyntaxPtr + 1][0] == tokStructPtr;

      if (!(ExprLevel || structInfo) &&
          !(external |
#ifndef NO_TYPEDEF_ENUM
            typeDef |
#endif
            Static) && 
          !strcmp(IdentTable + SyntaxStack[lastSyntaxPtr][1], "<something>") &&
          tok == ';')
      {
        if (isStruct)
        {
          // This is either an incomplete tagged structure/union declaration, e.g. "struct sometag;",
          // or a tagged complete structure/union declaration, e.g. "struct sometag { ... };", without an instance variable,
          // or an untagged complete structure/union declaration, e.g. "struct { ... };", without an instance variable
          int declPtr, curScope;
          int j = SyntaxStack[lastSyntaxPtr + 1][1];

          if (j + 2 < SyntaxStackCnt &&
              IdentTable[SyntaxStack[j + 1][1]] == '<' && // without tag
              SyntaxStack[j + 2][0] == tokSizeof) // but with the {} "body"
            errorDecl();

          // If a structure/union with this tag has been declared in an outer scope,
          // this new declaration should override it
          declPtr = FindTaggedDecl(IdentTable + SyntaxStack[j + 1][1], lastSyntaxPtr - 1, &curScope);
          if (declPtr >= 0 && !curScope)
          {
            // If that's the case, unbind this declaration from the old declaration
            // and make it a new incomplete declaration
            PushSyntax(SyntaxStack[j][0]); // tokStruct or tokUnion
            PushSyntax2(tokTag, SyntaxStack[j + 1][1]);
            SyntaxStack[lastSyntaxPtr + 1][1] = SyntaxStackCnt - 2;
          }
          return GetToken();
        }
#ifndef NO_TYPEDEF_ENUM
        else if (SyntaxStack[lastSyntaxPtr + 1][0] == tokEnumPtr)
        {
          return GetToken();
        }
#endif
      }

#ifndef NO_TYPEDEF_ENUM
      // Convert enums into ints
      if (SyntaxStack[SyntaxStackCnt - 1][0] == tokEnumPtr)
      {
        SyntaxStack[SyntaxStackCnt - 1][0] = tokInt;
        SyntaxStack[SyntaxStackCnt - 1][1] = 0;
      }
#endif

      // Structure/union members can't be initialized nor be functions nor
      // be incompletely typed arrays inside structure/union declarations
      if (structInfo && ((tok == '=') | isFxn | (tok == '{') | isIncompleteArr))
        errorDecl();

#ifndef NO_TYPEDEF_ENUM
      if (typeDef & ((tok == '=') | (tok == '{')))
        errorDecl();
#endif

      // Error conditions in declarations(/definitions/initializations):
      // Legend:
      //   +  error
      //   -  no error
      //
      // file scope          fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            +                 +
      // file scope          fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     -       +            -                 +
      // file scope  extern  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            -                 -
      // file scope  extern  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +
      // file scope  static  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     -       -     -       -            +                 +
      // file scope  static  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     -       +            -                 +
      // fxn scope           fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     +       -     -       -            +                 +
      // fxn scope           fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             -     +       +            +                 +
      // fxn scope   extern  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     -     +       -     -       -            -                 -
      // fxn scope   extern  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +
      // fxn scope   static  fxn   fxn {}  var   arr[]   arr[]...[]   arr[incomplete]   arr[incomplete]...[]
      //                     +     +       +     +       +            +                 +
      // fxn scope   static  fxn=          var=  arr[]=  arr[]...[]=  arr[incomplete]=  arr[incomplete]...[]=
      //                     +             +     +       +            +                 +

      if (isFxn & (tok == '='))
        //error("ParseDecl(): cannot initialize a function\n");
        errorInit();

      if ((isFxn & (tok == '{')) && ParseLevel)
        //error("ParseDecl(): cannot define a nested function\n");
        errorDecl();

      if ((isFxn & Static) && ParseLevel)
        //error("ParseDecl(): cannot declare a static function in this scope\n");
        errorDecl();

      if (external & (tok == '='))
        //error("ParseDecl(): cannot initialize an external variable\n");
        errorInit();

      if (isIncompleteArr & !(external |
#ifndef NO_TYPEDEF_ENUM
                              typeDef |
#endif
                              (tok == '=')))
        //error("ParseDecl(): cannot define an array of incomplete type\n");
        errorDecl();

      // TBD!!! de-uglify
      if (!strcmp(IdentTable + SyntaxStack[lastSyntaxPtr][1], "<something>"))
      {
        // Disallow nameless variables, prototypes, structure/union members and typedefs.
        if (structInfo ||
#ifndef NO_TYPEDEF_ENUM
            typeDef ||
#endif
            !ExprLevel)
          error("Identifier expected in declaration\n");
      }
      else
      {
        // Disallow named variables and prototypes in sizeof(typedecl) and (typedecl).
        if (ExprLevel && !structInfo)
          error("Identifier unexpected in declaration\n");
      }

      if (!isFxn
#ifndef NO_TYPEDEF_ENUM
          && !typeDef
#endif
         )
      {
        // This is a variable or a variable (member) in a struct/union declaration
        int sz = GetDeclSize(lastSyntaxPtr, 0);

        if (!((sz | isIncompleteArr) || ExprLevel)) // incomplete type
          errorDecl(); // TBD!!! different error when struct/union tag is not found

        if (isArray && !GetDeclSize(lastSyntaxPtr + 4, 0))
          // incomplete type of array element (e.g. struct/union)
          errorDecl();

        alignment = GetDeclAlignment(lastSyntaxPtr);

        if (structInfo)
        {
          // It's a variable (member) in a struct/union declaration
          unsigned tmp;
          unsigned newAlignment = alignment;
#ifndef NO_PPACK
          if (alignment > PragmaPackValue + 0u)
            newAlignment = PragmaPackValue;
#endif
          // Update structure/union alignment
          if (structInfo[1] < newAlignment)
            structInfo[1] = newAlignment;
          // Align structure member
          tmp = structInfo[2];
          structInfo[2] = (structInfo[2] + newAlignment - 1) & ~(newAlignment - 1);
          if (structInfo[2] < tmp || structInfo[2] != truncUint(structInfo[2]))
            errorVarSize();
          // Change tokIdent to tokMemberIdent and insert a local var offset token
          SyntaxStack[lastSyntaxPtr][0] = tokMemberIdent;
          InsertSyntax2(lastSyntaxPtr + 1, tokLocalOfs, uint2int(structInfo[2]));

          // Advance member offset for structures, keep it zero for unions
          if (structInfo[0] == tokStruct)
          {
            tmp = structInfo[2];
            structInfo[2] += sz;
            if (structInfo[2] < tmp || structInfo[2] != truncUint(structInfo[2]))
              errorVarSize();
          }
          // Update max member size for unions
          else if (structInfo[3] < sz + 0u)
          {
            structInfo[3] = sz;
          }
        }
        else if (ParseLevel && !((external | Static) || ExprLevel))
        {
          // It's a local variable
          isLocal = 1;
          // Defer size calculation until initialization
          // Insert a local var offset token, the offset is to be updated
          InsertSyntax2(lastSyntaxPtr + 1, tokLocalOfs, 0);
        }
        else if (!ExprLevel)
        {
          // It's a global variable (external, static or neither)
          isGlobal = 1;
          if (Static && ParseLevel)
          {
            // It's a static variable in function scope, "rename" it by providing
            // an alternative unique numeric identifier right next to it and use it
            int staticLabel = LabelCnt++;
            InsertSyntax2(++lastSyntaxPtr, tokIdent, AddNumericIdent__(staticLabel));
          }
        }
      }

      // If it's a type declaration in a sizeof(typedecl) expression or
      // in an expression with a cast, e.g. (typedecl)expr, we're done
      if (ExprLevel && !structInfo)
      {
#ifndef NO_ANNOTATIONS
        DumpDecl(lastSyntaxPtr, 0);
#endif
        return tok;
      }

#ifndef NO_TYPEDEF_ENUM
      if (typeDef)
      {
        int CurScope;
        char* s = IdentTable + SyntaxStack[lastSyntaxPtr][1];
#ifndef NO_ANNOTATIONS
        DumpDecl(lastSyntaxPtr, 0);
#endif
        SyntaxStack[lastSyntaxPtr][0] = 0; // hide tokIdent for now
        if (FindTypedef(s, &CurScope, 0) >= 0 && CurScope)
          errorRedef(s);
        SyntaxStack[lastSyntaxPtr][0] = tokTypedef; // change tokIdent to tokTypedef
      }
      else
      // fallthrough
#endif
      if (isLocal | isGlobal)
      {
        int hasInit = tok == '=';
        int needsGlobalInit = isGlobal & !external;
        int sz = GetDeclSize(lastSyntaxPtr, 0);
        int localAllocSize = 0;
        int skipLabel = 0;
        int initLabel = 0;

#ifndef NO_ANNOTATIONS
        if (isGlobal)
          DumpDecl(lastSyntaxPtr, 0);
#endif

        if (hasInit)
        {
          tok = GetToken();
        }

        if (isLocal & hasInit)
          needsGlobalInit = isArray | (isStruct & (tok == '{'));

        if (needsGlobalInit)
        {
          if (isLocal | (Static && ParseLevel))
          {
            // Global data appears inside code of a function
            if (OutputFormat == FormatFlat)
            {
              skipLabel = LabelCnt++;
              GenJumpUncond(skipLabel);
            }
            else
            {
              puts2(CodeFooter);
              puts2(DataHeader);
            }
          }
          else
          {
            // Global data appears between functions
            if (OutputFormat != FormatFlat)
            {
              puts2(DataHeader);
            }
          }

          // DONE: imperfect condition for alignment
          if (alignment != 1)
            GenWordAlignment();

          if (isGlobal)
          {
            GenLabel(IdentTable + SyntaxStack[lastSyntaxPtr][1], Static);
          }
          else
          {
            // Generate numeric labels for global initializers of local vars
            char s[1 + 2 + (2 + CHAR_BIT * sizeof StructCpyLabel) / 3];
            char *p = s + sizeof s;
            initLabel = LabelCnt++;
            *--p = '\0';
            p = lab2str(p, initLabel);
            *--p = '_';
            *--p = '_';
            GenLabel(p, 1);
          }

          // Generate global initializers
          if (hasInit)
          {
#ifndef NO_ANNOTATIONS
            if (isGlobal)
            {
              GenStartCommentLine(); printf2("=\n");
            }
#endif
            tok = InitVar(lastSyntaxPtr, tok);
            // Update the size in case it's an incomplete array
            sz = GetDeclSize(lastSyntaxPtr, 0);
          }
          else
          {
            GenZeroData(sz);
          }

          if (isLocal | (Static && ParseLevel))
          {
            // Global data appears inside code of a function
            if (OutputFormat == FormatFlat)
            {
              GenNumLabel(skipLabel);
            }
            else
            {
              puts2(DataFooter);
              puts2(CodeHeader);
            }
          }
          else
          {
            // Global data appears between functions
            if (OutputFormat != FormatFlat)
            {
              puts2(DataFooter);
            }
          }
        }

        if (isLocal)
        {
          int oldOfs;
          // Let's calculate variable's relative on-stack location
          oldOfs = CurFxnLocalOfs;

          // Note: local vars are word-aligned on the stack
          CurFxnLocalOfs = uint2int((CurFxnLocalOfs + 0u - sz) & ~(SizeOfWord - 1u));
          if (CurFxnLocalOfs >= oldOfs || CurFxnLocalOfs != truncInt(CurFxnLocalOfs))
            //error("ParseDecl(): Local variables take too much space\n");
            errorVarSize();
#ifdef CAN_COMPILE_32BIT
          if (OutputFormat == FormatSegHuge && CurFxnLocalOfs < -0x7FFF)
            errorVarSize();
#endif

          // Now that the size of the local is certainly known,
          // update its offset in the offset token
          SyntaxStack[lastSyntaxPtr + 1][1] = CurFxnLocalOfs;

          localAllocSize = oldOfs - CurFxnLocalOfs;
          if (CurFxnMinLocalOfs > CurFxnLocalOfs)
            CurFxnMinLocalOfs = CurFxnLocalOfs;

#ifndef NO_ANNOTATIONS
          DumpDecl(lastSyntaxPtr, 0);
#endif
        }

        // Copy global initializers into local vars
        if (isLocal & needsGlobalInit)
        {
#ifndef NO_ANNOTATIONS
          GenStartCommentLine(); printf2("=\n");
#endif
          if (!StructCpyLabel)
            StructCpyLabel = LabelCnt++;

          sp = 0;

          push2('(', SizeOfWord * 3);

          push2(tokLocalOfs, SyntaxStack[lastSyntaxPtr + 1][1]);

          push(',');

          push2(tokIdent, AddNumericIdent__(initLabel));

          push(',');

          push2(tokNumUint, sz);

          push(',');

          push2(tokIdent, AddNumericIdent__(StructCpyLabel));

          push2(')', SizeOfWord * 3);

          GenExpr();
        }
        // Initialize local vars with expressions
        else if (hasInit & !needsGlobalInit)
        {
          int gotUnary, synPtr, constExpr, exprVal;

          // ParseExpr() will transform the initializer expression into an assignment expression here
          tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, '=', SyntaxStack[lastSyntaxPtr][1]);

          if (!gotUnary)
            errorUnexpectedToken(tok);

          if (!isStruct)
          {
            // This is a special case for initialization of integers smaller than int.
            // Since a local integer variable always takes as much space as a whole int,
            // we can optimize code generation a bit by storing the initializer as an int.
            // This is an old accidental optimization and I preserve it for now.
            stack[sp - 1][1] = SizeOfWord;
          }

          // Storage of string literal data from the initializing expression
          // occurs here.
          GenExpr();
        }
      }
      else if (tok == '{')
      {
        // It's a function body. Let's add function parameters as
        // local variables to the symbol table and parse the body.
        int undoSymbolsPtr = SyntaxStackCnt;
        int undoIdents = IdentTableLen;
        int locAllocLabel = (LabelCnt += 2) - 2;
        int i;
        int Main;

#ifndef NO_ANNOTATIONS
        DumpDecl(lastSyntaxPtr, 0);
#endif

        CurFxnName = IdentTable + SyntaxStack[lastSyntaxPtr][1];
        Main = !strcmp(CurFxnName, "main");

        gotoLabCnt = 0;

        if (verbose && OutFile)
          printf("%s()\n", CurFxnName);

        ParseLevel++;
        GetFxnInfo(lastSyntaxPtr, &CurFxnParamCntMin, &CurFxnParamCntMax, &CurFxnReturnExprTypeSynPtr); // get return type

        if (OutputFormat != FormatFlat)
          puts2(CodeHeader);

        GenLabel(IdentTable + SyntaxStack[lastSyntaxPtr][1], Static);
        CurFxnEpilogLabel = LabelCnt++;
        GenFxnProlog();
        GenJumpUncond(locAllocLabel + 1);
        GenNumLabel(locAllocLabel);

        AddFxnParamSymbols(lastSyntaxPtr);

#ifndef NO_FUNC_
        {
          CurFxnNameLabel = LabelCnt++;
          SyntaxStack[SymFuncPtr][1] = AddNumericIdent__(CurFxnNameLabel);
          SyntaxStack[SymFuncPtr + 2][1] = strlen(CurFxnName) + 1;
        }
#endif

#ifdef CAN_COMPILE_32BIT
        if (MainPrologCtorFxn &&
            Main &&
            OutputFormat == FormatSegmented && SizeOfWord == 4)
        {
          sp = 0;
          push('(');
          push2(tokIdent, AddIdent(MainPrologCtorFxn));
          push(')');
          GenExpr();
        }
#endif

        tok = ParseBlock(NULL, 0);
        ParseLevel--;
        if (tok != '}')
          //error("ParseDecl(): '}' expected\n");
          errorUnexpectedToken(tok);

        for (i = 0; i < gotoLabCnt; i++)
          if (gotoLabStat[i] == 2)
            error("Undeclared label '%s'\n", IdentTable + gotoLabels[i][0]);

        // DONE: if execution of main() reaches here, before the epilog (i.e. without using return),
        // main() should return 0.
        if (Main)
        {
          sp = 0;
          push(tokNumInt);
          GenExpr();
        }

        GenNumLabel(CurFxnEpilogLabel);
        GenFxnEpilog();
        GenNumLabel(locAllocLabel + 1);
        if (CurFxnMinLocalOfs)
          GenLocalAlloc(-CurFxnMinLocalOfs);
        GenJumpUncond(locAllocLabel);
        if (OutputFormat != FormatFlat)
          puts2(CodeFooter);

#ifndef NO_FUNC_
        if (CurFxnNameLabel < 0)
        {
          PurgeStringTable();
          AddString(-CurFxnNameLabel, CurFxnName, SyntaxStack[SymFuncPtr + 2][1]);

          if (OutputFormat != FormatFlat)
            puts2(DataHeader);

          GenLabel(IdentTable + SyntaxStack[SymFuncPtr][1], 1);

          sp = 1;
          stack[0][0] = tokIdent;
          stack[0][1] = SyntaxStack[SymFuncPtr][1] + 2;
          GenStrData(0, 0);

          if (OutputFormat != FormatFlat)
            puts2(DataFooter);

          CurFxnNameLabel = 0;
        }
#endif

        CurFxnName = NULL;
        IdentTableLen = undoIdents; // remove all identifier names
        SyntaxStackCnt = undoSymbolsPtr; // remove all params and locals
      }
#ifndef NO_ANNOTATIONS
      else if (isFxn)
      {
        // function prototype
        DumpDecl(lastSyntaxPtr, 0);
      }
#endif

      if ((tok == ';') | (tok == '}'))
        break;

      tok = GetToken();
      continue;
    }

    //error("ParseDecl(): unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }

  tok = GetToken();
  return tok;
}

void ParseFxnParams(int tok)
{
  int base[2];
  int lastSyntaxPtr;
  int cnt = 0;
  int ellCnt = 0;

  for (;;)
  {
    lastSyntaxPtr = SyntaxStackCnt;

    if (tok == ')') /* unspecified params */
      break;

    if (!TokenStartsDeclaration(tok, 1))
    {
      if (tok == tokEllipsis)
      {
        // "..." cannot be the first parameter and
        // it can be only one
        if (!cnt || ellCnt)
          //error("ParseFxnParams(): '...' unexpected here\n");
          errorUnexpectedToken(tok);
        ellCnt++;
      }
      else
        //error("ParseFxnParams(): Unexpected token %s\n", GetTokenName(tok));
        errorUnexpectedToken(tok);
      base[0] = tok; // "..."
      base[1] = 0;
      PushSyntax2(tokIdent, AddIdent("<something>"));
      tok = GetToken();
    }
    else
    {
      if (ellCnt)
        //error("ParseFxnParams(): '...' must be the last in the parameter list\n");
        errorUnexpectedToken(tok);

      /* base type */
      tok = ParseBase(tok, base);

      /* derived type */
      tok = ParseDerived(tok);
    }

    /* base type */
    PushBase(base);

#ifndef NO_TYPEDEF_ENUM
      // Convert enums into ints
      if (SyntaxStack[SyntaxStackCnt - 1][0] == tokEnumPtr)
      {
        SyntaxStack[SyntaxStackCnt - 1][0] = tokInt;
        SyntaxStack[SyntaxStackCnt - 1][1] = 0;
      }
#endif

    /* Decay arrays to pointers */
    lastSyntaxPtr++; /* skip name */
    if (SyntaxStack[lastSyntaxPtr][0] == '[')
    {
      int t;
      DeleteSyntax(lastSyntaxPtr, 1);
      t = SyntaxStack[lastSyntaxPtr][0];
      if (t == tokNumInt || t == tokNumUint)
        DeleteSyntax(lastSyntaxPtr, 1);
      SyntaxStack[lastSyntaxPtr][0] = '*';
    }
    /* "(Un)decay" functions to function pointers */
    else if (SyntaxStack[lastSyntaxPtr][0] == '(')
    {
      InsertSyntax(lastSyntaxPtr, '*');
    }
    lastSyntaxPtr--; /* "unskip" name */

    cnt++;

    if (tok == ')' || tok == ',')
    {
      int t = SyntaxStack[SyntaxStackCnt - 2][0];
      if (SyntaxStack[SyntaxStackCnt - 1][0] == tokVoid)
      {
        // Disallow void variables. TBD!!! de-uglify
        if (t == tokIdent &&
            !(!strcmp(IdentTable + SyntaxStack[SyntaxStackCnt - 2][1], "<something>") &&
              cnt == 1 && tok == ')'))
          //error("ParseFxnParams(): Cannot declare a variable ('%s') of type 'void'\n", IdentTable + SyntaxStack[lastSyntaxPtr][1]);
          errorUnexpectedVoid();
      }

      if (SyntaxStack[SyntaxStackCnt - 1][0] == tokStructPtr &&
          t != '*' &&
          t != ']')
        // structure passing and returning isn't supported currently
        errorDecl();

      if (tok == ')')
        break;

      tok = GetToken();
      continue;
    }

    //error("ParseFxnParams(): Unexpected token %s\n", GetTokenName(tok));
    errorUnexpectedToken(tok);
  }
}

void AddFxnParamSymbols(int SyntaxPtr)
{
  int i;

  if (SyntaxPtr < 0 ||
      SyntaxPtr > SyntaxStackCnt - 3 ||
      SyntaxStack[SyntaxPtr][0] != tokIdent ||
      SyntaxStack[SyntaxPtr + 1][0] != '(')
    //error("Internal error: AddFxnParamSymbols(): Invalid input\n");
    errorInternal(6);

  CurFxnSyntaxPtr = SyntaxPtr;
  CurFxnParamCnt = 0;
  CurFxnParamOfs = 2 * SizeOfWord; // ret addr, xbp
  CurFxnLocalOfs = 0;
  CurFxnMinLocalOfs = 0;

  SyntaxPtr += 2; // skip "ident("

  for (i = SyntaxPtr; i < SyntaxStackCnt; i++)
  {
    int tok = SyntaxStack[i][0];

    if (tok == tokIdent)
    {
      int sz;
      int paramPtr;

      if (i + 1 >= SyntaxStackCnt)
        //error("Internal error: AddFxnParamSymbols(): Invalid input\n");
        errorInternal(7);

      if (SyntaxStack[i + 1][0] == tokVoid) // "ident(void)" = no params
        break;
      if (SyntaxStack[i + 1][0] == tokEllipsis) // "ident(something,...)" = no more params
        break;

      sz = GetDeclSize(i, 0);
      if (sz == 0)
        //error("Internal error: AddFxnParamSymbols(): GetDeclSize() = 0\n");
        errorInternal(8);

      // Let's calculate this parameter's relative on-stack location
      CurFxnParamOfs = (CurFxnParamOfs + SizeOfWord - 1) / SizeOfWord * SizeOfWord;
      paramPtr = SyntaxStackCnt;
      PushSyntax2(SyntaxStack[i][0], SyntaxStack[i][1]);
      PushSyntax2(tokLocalOfs, CurFxnParamOfs);
      CurFxnParamOfs += sz;

      // Duplicate this parameter in the symbol table
      i++;
      while (i < SyntaxStackCnt)
      {
        tok = SyntaxStack[i][0];
        if (tok == tokIdent || tok == ')')
        {
          CurFxnParamCnt++;
#ifndef NO_ANNOTATIONS
          DumpDecl(paramPtr, 0);
#endif
          i--;
          break;
        }
        else if (tok == '(')
        {
          int c = 1;
          i++;
          PushSyntax(tok);
          while (c && i < SyntaxStackCnt)
          {
            tok = SyntaxStack[i][0];
            c += (tok == '(') - (tok == ')');
            PushSyntax2(SyntaxStack[i][0], SyntaxStack[i][1]);
            i++;
          }
        }
        else
        {
          PushSyntax2(SyntaxStack[i][0], SyntaxStack[i][1]);
          i++;
        }
      }
    }
    else if (tok == ')') // endof "ident(" ... ")"
      break;
    else
      //error("Internal error: AddFxnParamSymbols(): Unexpected token %s\n", GetTokenName(tok));
      errorInternal(9);
  }
}

int ParseStatement(int tok, int BrkCntSwchTarget[4], int switchBody)
{
/*
  labeled statements:
  + ident : statement
  + case const-expr : statement
  + default : statement

  compound statement:
  + { declaration(s)/statement(s)-opt }

  expression statement:
  + expression-opt ;

  selection statements:
  + if ( expression ) statement
  + if ( expression ) statement else statement
  + switch ( expression ) { statement(s)-opt }

  iteration statements:
  + while ( expression ) statement
  + do statement while ( expression ) ;
  + for ( expression-opt ; expression-opt ; expression-opt ) statement

  jump statements:
  + goto ident ;
  + continue ;
  + break ;
  + return expression-opt ;
*/
  int gotUnary, synPtr,  constExpr, exprVal;
  int brkCntSwchTarget[4];
  int statementNeeded;

  do
  {
    statementNeeded = 0;

    if (tok == ';')
    {
      tok = GetToken();
    }
    else if (tok == '{')
    {
      // A new {} block begins in the function body
      int undoSymbolsPtr = SyntaxStackCnt;
      int undoLocalOfs = CurFxnLocalOfs;
      int undoIdents = IdentTableLen;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("{\n");
#endif
      ParseLevel++;
      tok = ParseBlock(BrkCntSwchTarget, switchBody / 2);
      ParseLevel--;
      if (tok != '}')
        //error("ParseStatement(): '}' expected. Unexpected token %s\n", GetTokenName(tok));
        errorUnexpectedToken(tok);
      UndoNonLabelIdents(undoIdents); // remove all identifier names, except those of labels
      SyntaxStackCnt = undoSymbolsPtr; // remove all params and locals
      CurFxnLocalOfs = undoLocalOfs; // destroy on-stack local variables
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("}\n");
#endif
      tok = GetToken();
    }
    else if (tok == tokReturn)
    {
      // DONE: functions returning void vs non-void
      // TBD??? functions returning void should be able to return void
      //        return values from other functions returning void
      int retVoid = CurFxnReturnExprTypeSynPtr >= 0 &&
                    SyntaxStack[CurFxnReturnExprTypeSynPtr][0] == tokVoid;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("return\n");
#endif
      tok = GetToken();
      if (tok == ';')
      {
        gotUnary = 0;
        if (!retVoid)
          //error("ParseStatement(): missing return value\n");
          errorUnexpectedToken(tok);
      }
      else
      {
        if (retVoid)
          //error("Error: ParseStatement(): cannot return a value from a function returning 'void'\n");
          errorUnexpectedToken(tok);
        if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
          //error("ParseStatement(): ';' expected\n");
          errorUnexpectedToken(tok);
        if (gotUnary)
          //error("ParseStatement(): cannot return a value of type 'void'\n");
          scalarTypeCheck(synPtr);
      }
      if (gotUnary)
        GenExpr();
      GenJumpUncond(CurFxnEpilogLabel);
      tok = GetToken();
    }
    else if (tok == tokWhile)
    {
      int labelBefore = LabelCnt++;
      int labelAfter = LabelCnt++;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("while\n");
#endif

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'while'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'while ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'while ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'while ( expression )'\n");
      scalarTypeCheck(synPtr);

      GenNumLabel(labelBefore);

      switch (stack[sp - 1][0])
      {
      case '<':
      case '>':
      case tokEQ:
      case tokNEQ:
      case tokLEQ:
      case tokGEQ:
      case tokULess:
      case tokUGreater:
      case tokULEQ:
      case tokUGEQ:
        push2(tokIfNot, labelAfter);
        GenExpr();
        break;
      default:
        GenExpr();
        GenJumpIfZero(labelAfter);
        break;
      }

      tok = GetToken();
      brkCntSwchTarget[0] = labelAfter; // break target
      brkCntSwchTarget[1] = labelBefore; // continue target
      tok = ParseStatement(tok, brkCntSwchTarget, 0);

      GenJumpUncond(labelBefore);
      GenNumLabel(labelAfter);
    }
    else if (tok == tokDo)
    {
      int labelBefore = LabelCnt++;
      int labelWhile = LabelCnt++;
      int labelAfter = LabelCnt++;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("do\n");
#endif
      GenNumLabel(labelBefore);

      tok = GetToken();
      brkCntSwchTarget[0] = labelAfter; // break target
      brkCntSwchTarget[1] = labelWhile; // continue target
      tok = ParseStatement(tok, brkCntSwchTarget, 0);
      if (tok != tokWhile)
        //error("ParseStatement(): 'while' expected after 'do statement'\n");
        errorUnexpectedToken(tok);

#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("while\n");
#endif
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'while'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'while ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'while ( expression )'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != ';')
        //error("ParseStatement(): ';' expected after 'do statement while ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'while ( expression )'\n");
      scalarTypeCheck(synPtr);

      GenNumLabel(labelWhile);

      switch (stack[sp - 1][0])
      {
      case '<':
      case '>':
      case tokEQ:
      case tokNEQ:
      case tokLEQ:
      case tokGEQ:
      case tokULess:
      case tokUGreater:
      case tokULEQ:
      case tokUGEQ:
        push2(tokIf, labelBefore);
        GenExpr();
        break;
      default:
        GenExpr();
        GenJumpIfNotZero(labelBefore);
        break;
      }

      GenNumLabel(labelAfter);

      tok = GetToken();
    }
    else if (tok == tokIf)
    {
      int labelAfterIf = LabelCnt++;
      int labelAfterElse = LabelCnt++;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("if\n");
#endif

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'if'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'if ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'if ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'if ( expression )'\n");
      scalarTypeCheck(synPtr);

      switch (stack[sp - 1][0])
      {
      case '<':
      case '>':
      case tokEQ:
      case tokNEQ:
      case tokLEQ:
      case tokGEQ:
      case tokULess:
      case tokUGreater:
      case tokULEQ:
      case tokUGEQ:
        push2(tokIfNot, labelAfterIf);
        GenExpr();
        break;
      default:
        GenExpr();
        GenJumpIfZero(labelAfterIf);
        break;
      }

      tok = GetToken();
      tok = ParseStatement(tok, BrkCntSwchTarget, 0);

      // DONE: else
      if (tok == tokElse)
      {
        GenJumpUncond(labelAfterElse);
        GenNumLabel(labelAfterIf);
#ifndef NO_ANNOTATIONS
        GenStartCommentLine(); printf2("else\n");
#endif
        tok = GetToken();
        tok = ParseStatement(tok, BrkCntSwchTarget, 0);
        GenNumLabel(labelAfterElse);
      }
      else
      {
        GenNumLabel(labelAfterIf);
      }
    }
    else if (tok == tokFor)
    {
      int labelBefore = LabelCnt++;
      int labelExpr3 = LabelCnt++;
      int labelBody = LabelCnt++;
      int labelAfter = LabelCnt++;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("for\n");
#endif
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'for'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
        //error("ParseStatement(): ';' expected after 'for ( expression'\n");
        errorUnexpectedToken(tok);
      if (gotUnary)
      {
        GenExpr();
      }

      GenNumLabel(labelBefore);
      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ';')
        //error("ParseStatement(): ';' expected after 'for ( expression ; expression'\n");
        errorUnexpectedToken(tok);
      if (gotUnary)
      {
        // DONE: void control expressions
        //error("ParseStatement(): unexpected 'void' expression in 'for ( ; expression ; )'\n");
        scalarTypeCheck(synPtr);

        switch (stack[sp - 1][0])
        {
        case '<':
        case '>':
        case tokEQ:
        case tokNEQ:
        case tokLEQ:
        case tokGEQ:
        case tokULess:
        case tokUGreater:
        case tokULEQ:
        case tokUGEQ:
          push2(tokIfNot, labelAfter);
          GenExpr();
          break;
        default:
          GenExpr();
          GenJumpIfZero(labelAfter);
          break;
        }
      }
      GenJumpUncond(labelBody);

      GenNumLabel(labelExpr3);
      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'for ( expression ; expression ; expression'\n");
        errorUnexpectedToken(tok);
      if (gotUnary)
      {
        GenExpr();
      }
      GenJumpUncond(labelBefore);

      GenNumLabel(labelBody);
      tok = GetToken();
      brkCntSwchTarget[0] = labelAfter; // break target
      brkCntSwchTarget[1] = labelExpr3; // continue target
      tok = ParseStatement(tok, brkCntSwchTarget, 0);
      GenJumpUncond(labelExpr3);

      GenNumLabel(labelAfter);
    }
    else if (tok == tokBreak)
    {
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("break\n");
#endif
      if ((tok = GetToken()) != ';')
        //error("ParseStatement(): ';' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
      if (BrkCntSwchTarget == NULL)
        //error("ParseStatement(): 'break' must be within 'while', 'for' or 'switch' statement\n");
        errorCtrlOutOfScope();
      GenJumpUncond(BrkCntSwchTarget[0]);
    }
    else if (tok == tokCont)
    {
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("continue\n");
#endif
      if ((tok = GetToken()) != ';')
        //error("ParseStatement(): ';' expected\n");
        errorUnexpectedToken(tok);
      tok = GetToken();
      if (BrkCntSwchTarget == NULL || BrkCntSwchTarget[1] == 0)
        //error("ParseStatement(): 'continue' must be within 'while' or 'for' statement\n");
        errorCtrlOutOfScope();
      GenJumpUncond(BrkCntSwchTarget[1]);
    }
    else if (tok == tokSwitch)
    {
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("switch\n");
#endif

      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'switch'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ')')
        //error("ParseStatement(): ')' expected after 'switch ( expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary)
        //error("ParseStatement(): expression expected in 'switch ( expression )'\n");
        errorUnexpectedToken(tok);

      // DONE: void control expressions
      //error("ParseStatement(): unexpected 'void' expression in 'switch ( expression )'\n");
      scalarTypeCheck(synPtr);

      GenExpr();

      tok = GetToken();
      if (tok != '{')
        //error("ParseStatement(): '{' expected after 'switch ( expression )'\n");
        errorUnexpectedToken(tok);

      brkCntSwchTarget[0] = LabelCnt++; // break target
      brkCntSwchTarget[1] = 0; // continue target
      if (BrkCntSwchTarget)
      {
        // preserve the continue target
        brkCntSwchTarget[1] = BrkCntSwchTarget[1]; // continue target
      }
      brkCntSwchTarget[2] = LabelCnt++; // default target
      brkCntSwchTarget[3] = (LabelCnt += 2) - 2; // next case target
/*
    ParseBlock(0)
      ParseStatement(0)
        switch
          ParseStatement(2)            // 2 needed to disallow new locals
            {                          // { in switch(expr){
              ParseBlock(1)            // new locals are allocated here
                ParseStatement(1)      // 1 needed for case/default
                  {                    // inner {} in switch(expr){{}}
                    ParseBlock(0)
                    ...
                  switch               // another switch
                    ParseStatement(2)  // needed to disallow new locals
                    ...
*/
      GenJumpUncond(brkCntSwchTarget[3]); // next case target

      tok = ParseStatement(tok, brkCntSwchTarget, 2);

      // force 'break' if the last 'case'/'default' doesn't end with 'break'
      GenJumpUncond(brkCntSwchTarget[0]);

      // next, non-existent case (reached after none of the 'cases' have matched)
      GenNumLabel(brkCntSwchTarget[3]);

      // if there's 'default', 'goto default;' after all unmatched 'cases'
      if (brkCntSwchTarget[2] < 0)
        GenJumpUncond(-brkCntSwchTarget[2]);

      GenNumLabel(brkCntSwchTarget[0]); // break label
    }
    else if (tok == tokCase)
    {
      int lnext;
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("case\n");
#endif

      if (!switchBody)
        //error("ParseStatement(): 'case' must be within 'switch' statement\n");
        errorCtrlOutOfScope();

      tok = GetToken();
      if ((tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, 0, 0)) != ':')
        //error("ParseStatement(): ':' expected after 'case expression'\n");
        errorUnexpectedToken(tok);

      if (!gotUnary || !constExpr || (synPtr >= 0 && SyntaxStack[synPtr][0] == tokVoid)) // TBD???
        //error("ParseStatement(): constant integer expression expected in 'case expression :'\n");
        errorNotConst();

      tok = GetToken();

      lnext = (LabelCnt += 2) - 2;

      GenJumpUncond(BrkCntSwchTarget[3] + 1); // fallthrough
      GenNumLabel(BrkCntSwchTarget[3]);
      GenJumpIfNotEqual(exprVal, lnext);
      GenNumLabel(BrkCntSwchTarget[3] + 1);

      BrkCntSwchTarget[3] = lnext;

      // a statement is needed after "case:"
      statementNeeded = 1;
    }
    else if (tok == tokDefault)
    {
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("default\n");
#endif

      if (!switchBody)
        //error("ParseStatement(): 'default' must be within 'switch' statement\n");
        errorCtrlOutOfScope();

      tok = GetToken();
      if (tok != ':')
        //error("ParseStatement(): ':' expected after 'default'\n");
        errorUnexpectedToken(tok);

      if (BrkCntSwchTarget[2] < 0)
        //error("ParseStatement(): only one 'default' allowed in 'switch'\n");
        errorUnexpectedToken(tokDefault);

      tok = GetToken();

      GenNumLabel(BrkCntSwchTarget[2]); // default:

      BrkCntSwchTarget[2] *= -1; // remember presence of default:

      // a statement is needed after "default:"
      statementNeeded = 1;
    }
    else if (tok == tok_Asm)
    {
      tok = GetToken();
      if (tok != '(')
        //error("ParseStatement(): '(' expected after 'asm'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != tokLitStr)
        //error("ParseStatement(): string literal expression expected in 'asm ( expression )'\n");
        errorUnexpectedToken(tok);

      puts2(GetTokenValueString());

      tok = GetToken();
      if (tok != ')')
        //error("ParseStatement(): ')' expected after 'asm ( expression'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
      if (tok != ';')
        //error("ParseStatement(): ';' expected after 'asm ( expression )'\n");
        errorUnexpectedToken(tok);

      tok = GetToken();
    }
    else if (tok == tokGoto)
    {
      if ((tok = GetToken()) != tokIdent)
        errorUnexpectedToken(tok);
#ifndef NO_ANNOTATIONS
      GenStartCommentLine(); printf2("goto %s\n", GetTokenIdentName());
#endif
      GenJumpUncond(AddGotoLabel(GetTokenIdentName(), 0));
      if ((tok = GetToken()) != ';')
        errorUnexpectedToken(tok);
      tok = GetToken();
    }
    else
    {
      tok = ParseExpr(tok, &gotUnary, &synPtr, &constExpr, &exprVal, tokGotoLabel, 0);
      if (tok == tokGotoLabel)
      {
        // found a label
#ifndef NO_ANNOTATIONS
        GenStartCommentLine(); printf2("%s:\n", IdentTable + stack[0][1]);
#endif
        GenNumLabel(AddGotoLabel(IdentTable + stack[0][1], 1));
        // a statement is needed after "label:"
        statementNeeded = 1;
      }
      else
      {
        if (tok != ';')
          //error("ParseStatement(): ';' expected\n");
          errorUnexpectedToken(tok);
        if (gotUnary)
          GenExpr();
      }
      tok = GetToken();
    }
  } while (statementNeeded);

  return tok;
}

// TBD!!! think of ways of getting rid of switchBody
int ParseBlock(int BrkCntSwchTarget[4], int switchBody)
{
  int tok = GetToken();

  PushSyntax('#'); // mark the beginning of a new scope

  for (;;)
  {
    if (tok == 0)
      return tok;

    if (tok == '}' && ParseLevel > 0)
      return tok;

    if (TokenStartsDeclaration(tok, 0))
    {
      tok = ParseDecl(tok, NULL, 0, 1);
#ifndef NO_TYPEDEF_ENUM
      if (tok == tokGotoLabel)
      {
        // found a label
#ifndef NO_ANNOTATIONS
        GenStartCommentLine(); printf2("%s:\n", GetTokenIdentName());
#endif
        GenNumLabel(AddGotoLabel(GetTokenIdentName(), 1));
        tok = GetToken();
        // a statement is needed after "label:"
        tok = ParseStatement(tok, BrkCntSwchTarget, switchBody);
      }
#endif
    }
    else if (ParseLevel > 0 || tok == tok_Asm)
    {
      tok = ParseStatement(tok, BrkCntSwchTarget, switchBody);
    }
    else
      //error("ParseBlock(): Unexpected token %s\n", GetTokenName(tok));
      errorUnexpectedToken(tok);
  }
}

int main(int argc, char** argv)
{
  // gcc/MinGW inserts a call to __main() here.
  int i;

#ifdef __SMALLER_C__
  DetermineVaListType();
#endif

  GenInit();

  // Parse the command line parameters
  for (i = 1; i < argc; i++)
  {
    // DONE: move code-generator-specific options to
    // the code generator
    if (GenInitParams(argc, argv, &i))
    {
      continue;
    }
    else if (!strcmp(argv[i], "-signed-char"))
    {
      // this is the default option
      CharIsSigned = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-unsigned-char"))
    {
      CharIsSigned = 0;
      continue;
    }
#ifdef CAN_COMPILE_32BIT
    else if (!strcmp(argv[i], "-ctor-fxn"))
    {
      if (i + 1 < argc)
      {
        MainPrologCtorFxn = argv[++i];
        continue;
      }
    }
#endif
    else if (!strcmp(argv[i], "-leading-underscore"))
    {
      // this is the default option for x86
      UseLeadingUnderscores = 1;
      continue;
    }
    else if (!strcmp(argv[i], "-no-leading-underscore"))
    {
      // this is the default option for MIPS
      UseLeadingUnderscores = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-label"))
    {
      if (i + 1 < argc)
      {
        LabelCnt = atoi(argv[++i]);
        continue;
      }
    }
    else if (!strcmp(argv[i], "-no-externs"))
    {
      GenExterns = 0;
      continue;
    }
    else if (!strcmp(argv[i], "-verbose"))
    {
      verbose = 1;
      continue;
    }
#ifndef NO_PREPROCESSOR
    else if (!strcmp(argv[i], "-I"))
    {
      if (i + 1 < argc)
      {
        int len = strlen(argv[++i]);
        if (MAX_SEARCH_PATH - SearchPathsLen < len + 1)
          //error("Path name too long\n");
          errorFileName();
        strcpy(SearchPaths + SearchPathsLen, argv[i]);
        SearchPathsLen += len + 1;
        continue;
      }
    }
    // DONE: '-D macro[=expansion]': '#define macro 1' when there's no '=expansion'
    else if (!strcmp(argv[i], "-D"))
    {
      if (i + 1 < argc)
      {
        char id[MAX_IDENT_LEN + 1];
        char* e = strchr(argv[++i], '=');
        int len;
        if (e)
        {
          len = e - argv[i];
          e++;
        }
        else
        {
          len = strlen(argv[i]);
          e = "1";
        }
        if (len > 0 && len <= MAX_IDENT_LEN)
        {
          int j, bad = 1;
          memcpy(id, argv[i], len);
          id[len] = '\0';
          for (j = 0; j < len; j++)
            if ((bad = !(id[j] == '_' || (!j * isalpha(id[j] & 0xFFu) + j * isalnum(id[j] & 0xFFu)))) != 0)
              break;
          if (!bad)
          {
            DefineMacro(id, e);
            continue;
          }
        }
      }
    }
#endif // #ifndef NO_PREPROCESSOR
    else if (argv[i][0] == '-')
    {
      // unknown option
    }
    else if (FileCnt == 0)
    {
      // If it's none of the known options,
      // assume it's the source code file name
      if (strlen(argv[i]) > MAX_FILE_NAME_LEN)
        //error("File name too long\n");
        errorFileName();
      strcpy(FileNames[0], argv[i]);
      if ((Files[0] = fopen(FileNames[0], "r")) == NULL)
        //error("Cannot open file \"%s\"\n", FileNames[0]);
        errorFile(FileNames[0]);
      LineNos[0] = LineNo;
      LinePoss[0] = LinePos;
      FileCnt++;
      continue;
    }  
    else if (FileCnt == 1 && OutFile == NULL)
    {
      // This should be the output file name
      if ((OutFile = fopen(argv[i], "w")) == NULL)
        //error("Cannot open output file \"%s\"\n", argv[i]);
        errorFile(argv[i]);
      continue;
    }  

    error("Invalid or unsupported command line option\n");
  }

  if (!FileCnt)
    error("Input file not specified\n");

  GenInitFinalize();

  // some manual initialization because there's no 2d array initialization yet
  PushSyntax(tokVoid);     // SymVoidSynPtr
  PushSyntax(tokInt);      // SymIntSynPtr
  PushSyntax(tokUnsigned); // SymUintSynPtr
  PushSyntax(tokIdent);    // SymFuncPtr
  PushSyntax('[');
  PushSyntax(tokNumUint);
  PushSyntax(']');
  PushSyntax(tokChar);

#ifndef NO_PREPROCESSOR
  // Define a few macros useful for conditional compilation
  DefineMacro("__SMALLER_C__", "0x0100");
  if (SizeOfWord == 2)
    DefineMacro("__SMALLER_C_16__", "");
  else if (SizeOfWord == 4)
    DefineMacro("__SMALLER_C_32__", "");
  if (CharIsSigned)
    DefineMacro("__SMALLER_C_SCHAR__", "");
  else
    DefineMacro("__SMALLER_C_UCHAR__", "");
#endif

  // populate CharQueue[] with the initial file characters
  ShiftChar();

  puts2(FileHeader);

  // compile
#ifndef NO_PPACK
  PragmaPackValue = SizeOfWord;
#endif
  ParseBlock(NULL, 0);

  GenFin();

#ifndef NO_ANNOTATIONS
  DumpSynDecls();
#endif
#ifndef NO_PREPROCESSOR
#ifndef NO_ANNOTATIONS
  DumpMacroTable();
#endif
#endif
#ifndef NO_ANNOTATIONS
  DumpIdentTable();
#endif

  GenStartCommentLine(); printf2("Next label number: %d\n", LabelCnt);

  if (verbose && warnCnt && OutFile)
    printf("%d warnings\n", warnCnt);
  GenStartCommentLine(); printf2("Compilation succeeded.\n");

  if (OutFile)
    fclose(OutFile);

  return 0;
}
