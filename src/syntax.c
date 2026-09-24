#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include "syntax.h"

static int colors_enabled = 0;

Language detect_language(const char* filename) {
    if (!filename) return LANGUAGE_TEXT;
    const char* name = strrchr(filename, '/');
    name = name ? name + 1 : filename;
    const char* extension = strrchr(name, '.');
    if (!extension) return LANGUAGE_TEXT;
    if (!strcmp(extension, ".c") || !strcmp(extension, ".h")) return LANGUAGE_C;
    if (!strcmp(extension, ".cpp") || !strcmp(extension, ".cc") || !strcmp(extension, ".cxx") ||
        !strcmp(extension, ".hpp") || !strcmp(extension, ".hh") || !strcmp(extension, ".hxx") ||
        !strcmp(extension, ".C") || !strcmp(extension, ".H") ||
        !strcmp(extension, ".ipp") || !strcmp(extension, ".tpp") ||
        !strcmp(extension, ".c++") || !strcmp(extension, ".h++")) return LANGUAGE_CPP;
    if (!strcmp(extension, ".java")) return LANGUAGE_JAVA;
    if (!strcmp(extension, ".py") || !strcmp(extension, ".pyw") || !strcmp(extension, ".pyi")) return LANGUAGE_PYTHON;
    return LANGUAGE_TEXT;
}

const char* language_name(Language language) {
    switch (language) {
        case LANGUAGE_C: return "C";
        case LANGUAGE_CPP: return "C++";
        case LANGUAGE_JAVA: return "Java";
        case LANGUAGE_PYTHON: return "Python";
        default: return "Text";
    }
}

static int identifier_character(char ch) {
    return isalnum((unsigned char)ch) || ch == '_' || ch == '$';
}

static int token_in_list(const Buffer* buf, size_t start, size_t end, const char* words) {
    while (*words) {
        const char* word = words;
        while (*words && *words != ' ') words++;
        if ((size_t)(words - word) == end - start) {
            size_t i = 0;
            while (i < end - start && buffer_character(buf, start + i) == word[i]) i++;
            if (i == end - start) return 1;
        }
        if (*words) words++;
    }
    return 0;
}

static size_t quoted_end(const Buffer* buf, size_t position, int multiline) {
    char quote = buffer_character(buf, position);
    int triple = multiline && buffer_character(buf, position + 1) == quote && buffer_character(buf, position + 2) == quote;
    size_t end = position + (triple ? 3 : 1);
    while (end < buf->text_size) {
        char ch = buffer_character(buf, end);
        if (ch == '\n' && !triple) break;
        if (ch == '\\') {
            end++;
            if (end < buf->text_size) end++;
        } else if (ch == quote && (!triple ||
                   (buffer_character(buf, end + 1) == quote && buffer_character(buf, end + 2) == quote))) {
            return end + (triple ? 3 : 1);
        } else end++;
    }
    return end;
}

static size_t raw_string_end(const Buffer* buf, size_t quote) {
    size_t opening = quote + 1;
    while (opening < buf->text_size && opening - quote <= 17) {
        char ch = buffer_character(buf, opening);
        if (ch == '(') break;
        if (isspace((unsigned char)ch) || ch == ')' || ch == '\\') return quote;
        opening++;
    }
    if (opening >= buf->text_size || opening - quote > 17) return quote;
    size_t delimiter_size = opening - quote - 1;
    for (size_t i = opening + 1; i < buf->text_size; i++) {
        if (buffer_character(buf, i) != ')') continue;
        size_t j = 0;
        while (j < delimiter_size && buffer_character(buf, i + 1 + j) == buffer_character(buf, quote + 1 + j)) j++;
        if (j == delimiter_size && buffer_character(buf, i + 1 + j) == '"') return i + j + 2;
    }
    return buf->text_size;
}

static size_t skip_space(const Buffer* buf, size_t position) {
    while (position < buf->text_size && isspace((unsigned char)buffer_character(buf, position))) position++;
    return position;
}

static int follows_word(const Buffer* buf, size_t position, const char* words) {
    while (position > 0 && isspace((unsigned char)buffer_character(buf, position - 1))) position--;
    size_t end = position;
    while (position > 0 && identifier_character(buffer_character(buf, position - 1))) position--;
    return position != end && token_in_list(buf, position, end, words);
}

static int starts_line(const Buffer* buf, size_t position) {
    while (position > 0 && (buffer_character(buf, position - 1) == ' ' || buffer_character(buf, position - 1) == '\t')) position--;
    return !position || buffer_character(buf, position - 1) == '\n';
}

static int follows_directive(const Buffer* buf, size_t position, const char* words) {
    size_t start = position;
    while (start > 0 && buffer_character(buf, start - 1) != '\n') start--;
    while (buffer_character(buf, start) == ' ' || buffer_character(buf, start) == '\t') start++;
    if (buffer_character(buf, start) != '#') return 0;
    start++;
    while (buffer_character(buf, start) == ' ' || buffer_character(buf, start) == '\t') start++;
    size_t end = start;
    while (identifier_character(buffer_character(buf, end))) end++;
    if (!token_in_list(buf, start, end, words)) return 0;
    while (buffer_character(buf, end) == ' ' || buffer_character(buf, end) == '\t') end++;
    return end == position;
}

static SyntaxType identifier_type(const Buffer* buf, Language language, size_t start, size_t end) {
    const char* keywords;
    const char* types;
    const char* builtins = "";
    const char* constants;
    if (language == LANGUAGE_PYTHON) {
        keywords = "and as assert async await break case class continue def del elif else except finally for from global if import in is lambda match nonlocal not or pass raise return try while with yield";
        types = "bool bytearray bytes complex dict float frozenset int list memoryview object range set slice str tuple type";
        builtins = "abs aiter all anext any ascii bin breakpoint callable chr classmethod compile delattr dir divmod enumerate eval exec filter format getattr globals hasattr hash help hex id input isinstance issubclass iter len locals map max min next oct open ord pow print property repr reversed round setattr sorted staticmethod sum super vars zip __import__ self cls";
        constants = "False None True NotImplemented Ellipsis";
    } else if (language == LANGUAGE_JAVA) {
        keywords = "abstract assert break case catch class const continue default do else enum exports extends final finally for goto if implements import instanceof interface module native new non sealed open opens package permits private protected provides public record requires return sealed static strictfp super switch synchronized this throw throws to transient transitive try uses volatile when while with yield";
        types = "boolean byte char double float int long short void var";
        constants = "true false null";
    } else {
        keywords = "auto break case const continue default do else enum extern for goto if inline register restrict return sizeof static struct switch typedef union volatile while _Alignas _Alignof _Atomic _Generic _Noreturn _Static_assert _Thread_local alignas alignof constexpr static_assert thread_local typeof typeof_unqual _BitInt";
        types = "char double float int long short signed unsigned void _Bool _Complex _Imaginary bool FILE size_t ssize_t ptrdiff_t wchar_t wint_t int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t intptr_t uintptr_t intmax_t uintmax_t va_list";
        constants = "true false nullptr NULL stdin stdout stderr";
    }
    if (token_in_list(buf, start, end, keywords)) return SYNTAX_KEYWORD;
    if (token_in_list(buf, start, end, constants)) return SYNTAX_CONSTANT;
    if (token_in_list(buf, start, end, types)) return SYNTAX_TYPE;
    if (language == LANGUAGE_CPP) {
        if (token_in_list(buf, start, end, "and and_eq asm bitand bitor catch class compl concept consteval constinit const_cast co_await co_return co_yield decltype delete dynamic_cast explicit export friend mutable namespace new noexcept not not_eq operator or or_eq private protected public reinterpret_cast requires static_cast template this throw try typename using virtual xor xor_eq")) return SYNTAX_KEYWORD;
        if (token_in_list(buf, start, end, "char8_t char16_t char32_t wchar_t std string wstring u8string u16string u32string string_view vector array deque list forward_list map multimap set multiset unordered_map unordered_set pair tuple optional variant any unique_ptr shared_ptr weak_ptr span initializer_list exception istream ostream iostream ifstream ofstream filesystem")) return SYNTAX_TYPE;
        if (token_in_list(buf, start, end, "cout cerr cin clog endl")) return SYNTAX_BUILTIN;
    }
    size_t previous = start;
    while (previous > 0 && isspace((unsigned char)buffer_character(buf, previous - 1))) previous--;
    char before = previous ? buffer_character(buf, previous - 1) : '\0';
    char after = buffer_character(buf, skip_space(buf, end));
    int member = before == '.' || (before == '>' && previous > 1 && buffer_character(buf, previous - 2) == '-');
    if (follows_word(buf, start, "class struct union enum interface record extends implements new typename concept")) return SYNTAX_TYPE;
    if (language == LANGUAGE_PYTHON && follows_word(buf, start, "def")) return SYNTAX_FUNCTION;
    if ((language == LANGUAGE_C || language == LANGUAGE_CPP) && follows_directive(buf, start, "define undef ifdef ifndef")) return SYNTAX_CONSTANT;
    int uppercase = 0;
    int lowercase = 0;
    for (size_t i = start; i < end; i++) {
        unsigned char ch = (unsigned char)buffer_character(buf, i);
        if (isupper(ch)) uppercase = 1;
        if (islower(ch)) lowercase = 1;
    }
    if (uppercase && !lowercase) return SYNTAX_CONSTANT;
    if (!member && token_in_list(buf, start, end, builtins)) return SYNTAX_BUILTIN;
    if (!member && (isupper((unsigned char)buffer_character(buf, start)) ||
        ((language == LANGUAGE_C || language == LANGUAGE_CPP) && end - start > 2 &&
         buffer_character(buf, end - 2) == '_' && buffer_character(buf, end - 1) == 't'))) return SYNTAX_TYPE;
    if (after == '(') return SYNTAX_FUNCTION;
    if (member) return SYNTAX_MEMBER;
    return SYNTAX_VARIABLE;
}

static size_t number_end(const Buffer* buf, Language language, size_t position) {
    size_t end = position;
    int base = 10;
    if (buffer_character(buf, end) == '0') {
        char next = buffer_character(buf, end + 1);
        if (next == 'x' || next == 'X') base = 16;
        else if (next == 'b' || next == 'B') base = 2;
        else if (language == LANGUAGE_PYTHON && (next == 'o' || next == 'O')) base = 8;
        if (base != 10) end += 2;
    }
    int decimal = 0;
    int exponent = 0;
    while (end < buf->text_size) {
        unsigned char ch = (unsigned char)buffer_character(buf, end);
        int digit = base == 16 ? isxdigit(ch) : ch >= '0' && ch < '0' + base;
        if (digit || ch == '_' || (language == LANGUAGE_CPP && ch == '\'' &&
            isalnum((unsigned char)buffer_character(buf, end + 1)))) end++;
        else if (ch == '.' && !decimal && !exponent && (base == 10 || base == 16) &&
                 buffer_character(buf, end + 1) != '.') {
            decimal = 1;
            end++;
        } else if (!exponent && ((base == 10 && (ch == 'e' || ch == 'E')) ||
                   (base == 16 && (ch == 'p' || ch == 'P')))) {
            exponent = 1;
            base = 10;
            end++;
            if (buffer_character(buf, end) == '+' || buffer_character(buf, end) == '-') end++;
        } else break;
    }
    const char* suffixes = language == LANGUAGE_PYTHON ? "jJ" : "uUlLfFdDzZ";
    while (buffer_character(buf, end) && strchr(suffixes, buffer_character(buf, end))) end++;
    return end;
}

SyntaxToken next_syntax_token(const Buffer* buf, Language language, size_t position) {
    SyntaxToken token = {position, position, SYNTAX_NORMAL};
    if (position >= buf->text_size) return token;
    token.end++;
    if (language == LANGUAGE_TEXT) {
        token.end = buf->text_size;
        return token;
    }
    char ch = buffer_character(buf, position);
    char next = buffer_character(buf, position + 1);
    if ((language == LANGUAGE_PYTHON && ch == '#') ||
        (language != LANGUAGE_PYTHON && ch == '/' && next == '/')) {
        token.type = SYNTAX_COMMENT;
        while (token.end < buf->text_size && buffer_character(buf, token.end) != '\n') {
            if ((language == LANGUAGE_C || language == LANGUAGE_CPP) && buffer_character(buf, token.end) == '\\' &&
                buffer_character(buf, token.end + 1) == '\n') token.end++;
            token.end++;
        }
    } else if (language != LANGUAGE_PYTHON && ch == '/' && next == '*') {
        token.type = SYNTAX_COMMENT;
        token.end = position + 2;
        while (token.end < buf->text_size) {
            if (buffer_character(buf, token.end) == '*' && buffer_character(buf, token.end + 1) == '/') {
                token.end += 2;
                break;
            }
            token.end++;
        }
    } else if (ch == '"' || ch == '\'') {
        token.type = SYNTAX_STRING;
        token.end = quoted_end(buf, position, language == LANGUAGE_PYTHON || (language == LANGUAGE_JAVA && ch == '"'));
    } else if ((language == LANGUAGE_C || language == LANGUAGE_CPP) && ch == '#') {
        size_t start = position;
        while (start > 0 && (buffer_character(buf, start - 1) == ' ' || buffer_character(buf, start - 1) == '\t')) start--;
        if (!start || buffer_character(buf, start - 1) == '\n') {
            token.type = SYNTAX_DIRECTIVE;
            while (buffer_character(buf, token.end) == ' ' || buffer_character(buf, token.end) == '\t') token.end++;
            while (isalpha((unsigned char)buffer_character(buf, token.end))) token.end++;
        }
    } else if ((language == LANGUAGE_JAVA || language == LANGUAGE_PYTHON) && ch == '@' &&
               (isalpha((unsigned char)next) || next == '_') &&
               (language == LANGUAGE_JAVA || starts_line(buf, position))) {
        token.type = SYNTAX_DIRECTIVE;
        while (identifier_character(buffer_character(buf, token.end)) || buffer_character(buf, token.end) == '.') token.end++;
    } else if (isalpha((unsigned char)ch) || ch == '_' || ch == '$') {
        while (identifier_character(buffer_character(buf, token.end))) token.end++;
        char quote = buffer_character(buf, token.end);
        if (language == LANGUAGE_CPP && quote == '"' && token_in_list(buf, position, token.end, "R u8R uR UR LR")) {
            size_t end = raw_string_end(buf, token.end);
            if (end != token.end) {
                token.type = SYNTAX_STRING;
                token.end = end;
                return token;
            }
        }
        if ((quote == '"' || quote == '\'') &&
            ((language == LANGUAGE_PYTHON && token_in_list(buf, position, token.end, "r R u U b B f F br bR Br BR rb rB Rb RB fr fR Fr FR rf rF Rf RF")) ||
             ((language == LANGUAGE_C || language == LANGUAGE_CPP) && token_in_list(buf, position, token.end, "u8 u U L")))) {
            token.type = SYNTAX_STRING;
            token.end = quoted_end(buf, token.end, language == LANGUAGE_PYTHON);
        } else token.type = identifier_type(buf, language, position, token.end);
    } else if (ch == '<' && (language == LANGUAGE_C || language == LANGUAGE_CPP) &&
               follows_directive(buf, position, "include include_next import")) {
        token.type = SYNTAX_STRING;
        while (token.end < buf->text_size && buffer_character(buf, token.end) != '\n' && buffer_character(buf, token.end) != '>') token.end++;
        if (buffer_character(buf, token.end) == '>') token.end++;
    } else if (isdigit((unsigned char)ch) || (ch == '.' && isdigit((unsigned char)next))) {
        token.type = SYNTAX_NUMBER;
        token.end = number_end(buf, language, position);
    } else if (ch && strchr("+-*/%=!<>&|^~?:@", ch)) {
        token.type = SYNTAX_OPERATOR;
    } else if (ch && strchr("()[]{}.,;", ch)) {
        token.type = SYNTAX_PUNCTUATION;
    }

    return token;
}

static SyntaxToken scanner_token(SyntaxScanner* scanner, size_t end, SyntaxType type) {
    SyntaxToken token = {scanner->position, end, type};
    scanner->position = end;
    return token;
}

static int push_context(SyntaxScanner* scanner, SyntaxContext context) {
    if (scanner->context_count == sizeof(scanner->contexts) / sizeof(scanner->contexts[0])) return 0;
    scanner->contexts[scanner->context_count++] = context;
    return 1;
}

static int closes_string(const Buffer* buf, size_t position, SyntaxContext* context) {
    return buffer_character(buf, position) == context->quote && (!context->triple ||
           (buffer_character(buf, position + 1) == context->quote && buffer_character(buf, position + 2) == context->quote));
}

static size_t escape_end(const Buffer* buf, size_t position) {
    size_t end = position + 1;
    if (end == buf->text_size) return end;
    char ch = buffer_character(buf, end++);
    size_t digits = ch == 'u' ? 4 : ch == 'U' ? 8 : ch == 'x' ? 2 : 0;
    while (digits && end < buf->text_size && isxdigit((unsigned char)buffer_character(buf, end))) {
        end++;
        digits--;
    }
    if (ch >= '0' && ch <= '7') {
        for (size_t i = 0; i < 2 && buffer_character(buf, end) >= '0' && buffer_character(buf, end) <= '7'; i++) end++;
    }
    return end;
}

SyntaxToken next_highlight_token(SyntaxScanner* scanner) {
    const Buffer* buf = scanner->buf;
    size_t position = scanner->position;
    if (position >= buf->text_size) return scanner_token(scanner, position, SYNTAX_NORMAL);
    char ch = buffer_character(buf, position);
    char next = buffer_character(buf, position + 1);
    SyntaxContext* context = scanner->context_count ? &scanner->contexts[scanner->context_count - 1] : NULL;
    if (context && (context->mode == 0 || context->mode == 2)) {
        if (context->mode == 0 && closes_string(buf, position, context)) {
            size_t width = context->triple ? 3 : 1;
            scanner->context_count--;
            return scanner_token(scanner, position + width, SYNTAX_STRING);
        }
        if (context->mode == 0 && ch == '\n' && !context->triple) {
            scanner->context_count--;
            return next_highlight_token(scanner);
        }
        if (context->mode == 2 && ch == '}') {
            scanner->context_count--;
            return scanner_token(scanner, position + 1, SYNTAX_PUNCTUATION);
        }
        if (context->formatted && (ch == '{' || ch == '}') && ch == next && context->mode == 0) {
            return scanner_token(scanner, position + 2, SYNTAX_ESCAPE);
        }
        if (context->formatted && ch == '{') {
            SyntaxContext expression = {0};
            expression.mode = 1;
            push_context(scanner, expression);
            return scanner_token(scanner, position + 1, SYNTAX_PUNCTUATION);
        }
        if (ch == '\\') {
            size_t end = context->raw ? position + (next ? 2 : 1) : escape_end(buf, position);
            if (context->formatted && (next == '{' || next == '}')) end = position + 1;
            return scanner_token(scanner, end, context->raw ? SYNTAX_STRING : SYNTAX_ESCAPE);
        }
        size_t end = position + 1;
        while (end < buf->text_size) {
            char current = buffer_character(buf, end);
            if (current == '\\' || (context->mode == 0 && closes_string(buf, end, context)) ||
                (context->mode == 0 && !context->triple && current == '\n') ||
                (context->formatted && current == '{') ||
                (current == '}' && (context->mode == 2 || (context->formatted && buffer_character(buf, end + 1) == '}')))) break;
            end++;
        }
        return scanner_token(scanner, end, SYNTAX_STRING);
    }
    if (context && context->mode == 1) {
        if (ch == '}' && !context->depth) {
            scanner->context_count--;
            return scanner_token(scanner, position + 1, SYNTAX_PUNCTUATION);
        }
        if (ch == ':' && !context->depth) {
            context->mode = 2;
            context->formatted = 1;
            return scanner_token(scanner, position + 1, SYNTAX_PUNCTUATION);
        }
        if (ch == '!' && next != '=' && !context->depth && (next == 'r' || next == 's' || next == 'a')) {
            return scanner_token(scanner, position + 2, SYNTAX_ESCAPE);
        }
        if (ch && strchr("([{", ch)) context->depth++;
        else if (ch && strchr(")]}", ch) && context->depth) context->depth--;
    }
    SyntaxToken token = next_syntax_token(buf, scanner->language, position);
    if (token.type == SYNTAX_STRING) {
        size_t quote = position;
        while (quote < token.end && identifier_character(buffer_character(buf, quote))) quote++;
        char delimiter = buffer_character(buf, quote);
        if ((delimiter == '\'' || delimiter == '"') &&
            !(scanner->language == LANGUAGE_CPP && quote > position && buffer_character(buf, quote - 1) == 'R')) {
            SyntaxContext string = {0};
            string.quote = delimiter;
            string.triple = (scanner->language == LANGUAGE_PYTHON || (scanner->language == LANGUAGE_JAVA && delimiter == '"')) &&
                            buffer_character(buf, quote + 1) == delimiter && buffer_character(buf, quote + 2) == delimiter;
            if (scanner->language == LANGUAGE_PYTHON) {
                for (size_t i = position; i < quote; i++) {
                    char prefix = (char)tolower((unsigned char)buffer_character(buf, i));
                    if (prefix == 'r') string.raw = 1;
                    if (prefix == 'f') string.formatted = 1;
                }
            }
            if (push_context(scanner, string)) return scanner_token(scanner, quote + (string.triple ? 3 : 1), SYNTAX_STRING);
        }
    }
    scanner->position = token.end;
    return token;
}

void initialize_syntax_colors(int light_theme) {
    colors_enabled = 0;
    if (!has_colors() || start_color() == ERR || COLOR_PAIRS < SYNTAX_COUNT) return;
    int background = use_default_colors() == OK ? -1 : COLOR_BLACK;
    short basic[] = {
        COLOR_WHITE, COLOR_MAGENTA, COLOR_YELLOW, COLOR_GREEN, COLOR_WHITE,
        COLOR_RED, COLOR_MAGENTA, COLOR_CYAN, COLOR_CYAN, COLOR_YELLOW,
        COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN, COLOR_WHITE, COLOR_YELLOW
    };
    short dark[] = {252, 176, 222, 114, 246, 209, 176, 117, 81, 215, 153, 180, 117, 250, 221};
    short light[] = {238, 90, 94, 28, 240, 130, 90, 25, 30, 130, 24, 94, 25, 238, 130};
    short* extended = light_theme ? light : dark;
    if (light_theme) {
        basic[SYNTAX_COMMENT] = COLOR_BLACK;
        basic[SYNTAX_VARIABLE] = COLOR_BLACK;
        basic[SYNTAX_PUNCTUATION] = COLOR_BLACK;
        basic[SYNTAX_TYPE] = COLOR_BLUE;
        basic[SYNTAX_CONSTANT] = COLOR_RED;
        basic[SYNTAX_MEMBER] = COLOR_BLUE;
        basic[SYNTAX_ESCAPE] = COLOR_RED;
    }
    for (short i = SYNTAX_KEYWORD; i < SYNTAX_COUNT; i++) {
        if (init_pair(i, COLORS >= 256 ? extended[i] : basic[i], (short)background) == ERR) return;
    }
    colors_enabled = 1;
}

int syntax_attributes(SyntaxType type) {
    if (!colors_enabled || type <= SYNTAX_NORMAL || type >= SYNTAX_COUNT) return A_NORMAL;
    int emphasis = type == SYNTAX_KEYWORD || type == SYNTAX_TYPE || type == SYNTAX_FUNCTION || type == SYNTAX_DIRECTIVE;
    return COLOR_PAIR(type) | (emphasis ? A_BOLD : A_NORMAL);
}
