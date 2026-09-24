#pragma once

#include "buffer.h"

typedef enum {
    LANGUAGE_TEXT,
    LANGUAGE_C,
    LANGUAGE_CPP,
    LANGUAGE_JAVA,
    LANGUAGE_PYTHON
} Language;

typedef enum {
    SYNTAX_NORMAL,
    SYNTAX_KEYWORD,
    SYNTAX_TYPE,
    SYNTAX_STRING,
    SYNTAX_COMMENT,
    SYNTAX_NUMBER,
    SYNTAX_DIRECTIVE,
    SYNTAX_FUNCTION,
    SYNTAX_BUILTIN,
    SYNTAX_CONSTANT,
    SYNTAX_VARIABLE,
    SYNTAX_MEMBER,
    SYNTAX_OPERATOR,
    SYNTAX_PUNCTUATION,
    SYNTAX_ESCAPE,
    SYNTAX_COUNT
} SyntaxType;

typedef struct {
    size_t start;
    size_t end;
    SyntaxType type;
} SyntaxToken;

typedef struct {
    char quote;
    int triple;
    int raw;
    int formatted;
    int mode;
    size_t depth;
} SyntaxContext;

typedef struct {
    const Buffer* buf;
    Language language;
    size_t position;
    SyntaxContext contexts[32];
    size_t context_count;
} SyntaxScanner;

SyntaxToken next_highlight_token(SyntaxScanner* scanner);
Language detect_language(const char* filename);
const char* language_name(Language language);
SyntaxToken next_syntax_token(const Buffer* buf, Language language, size_t position);
void initialize_syntax_colors(int light_theme);
int syntax_attributes(SyntaxType type);
