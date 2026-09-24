#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "syntax.h"

static void assert_token(const char* text, Language language, const char* fragment, SyntaxType type) {
    Buffer* buf = create_buffer();
    assert(buf);
    assert(replace_buffer(buf, 0, 0, text, strlen(text)));
    move_buffer_cursor(buf, strlen(text) / 2);
    const char* match = strstr(text, fragment);
    assert(match);
    size_t position = (size_t)(match - text);
    SyntaxToken token = {0, 0, SYNTAX_NORMAL};
    while (token.end <= position) {
        token = next_syntax_token(buf, language, token.end);
        assert(token.end > token.start);
        assert(token.end <= buf->text_size);
    }
    assert(token.type == type);
    assert(token.end >= position + strlen(fragment));
    free_buffer(buf);
}

static void assert_highlight(const char* text, Language language, const char* fragment, SyntaxType type) {
    Buffer* buf = create_buffer();
    assert(replace_buffer(buf, 0, 0, text, strlen(text)));
    move_buffer_cursor(buf, strlen(text) / 2);
    SyntaxScanner scanner = {0};
    scanner.buf = buf;
    scanner.language = language;
    const char* match = strstr(text, fragment);
    assert(match);
    size_t position = (size_t)(match - text);
    SyntaxToken token = {0, 0, SYNTAX_NORMAL};
    while (token.end <= position) {
        token = next_highlight_token(&scanner);
        assert(token.end > token.start && token.end <= buf->text_size);
    }
    assert(token.type == type);
    assert(token.end >= position + strlen(fragment));
    while (scanner.position < buf->text_size) {
        token = next_highlight_token(&scanner);
        assert(token.end > token.start && token.end <= buf->text_size);
    }
    free_buffer(buf);
}

static void test_incomplete_tokens(void) {
    const char* text = "def label(x):\n    return f\"{str(x):>{width}} = {{value}}\\n\"\n";
    for (size_t length = 0; length <= strlen(text); length++) {
        Buffer* buf = create_buffer();
        assert(replace_buffer(buf, 0, 0, text, length));
        SyntaxScanner scanner = {0};
        scanner.buf = buf;
        scanner.language = LANGUAGE_PYTHON;
        while (scanner.position < length) {
            SyntaxToken token = next_highlight_token(&scanner);
            assert(token.end > token.start && token.end <= length);
        }
        free_buffer(buf);
    }
}

int main(void) {
    assert(detect_language("/tmp/test.c") == LANGUAGE_C);
    assert(detect_language("test.h") == LANGUAGE_C);
    assert(detect_language("test.cpp") == LANGUAGE_CPP);
    assert(detect_language("test.hpp") == LANGUAGE_CPP);
    assert(detect_language("test.C") == LANGUAGE_CPP);
    assert(detect_language("Test.java") == LANGUAGE_JAVA);
    assert(detect_language("test.py") == LANGUAGE_PYTHON);
    assert(detect_language("test.pyi") == LANGUAGE_PYTHON);
    assert(detect_language("/tmp/a.py/file") == LANGUAGE_TEXT);
    assert(detect_language(NULL) == LANGUAGE_TEXT);
    assert_token("int main() { return 42; }", LANGUAGE_C, "int", SYNTAX_TYPE);
    assert_token("int main() { return 42; }", LANGUAGE_C, "return", SYNTAX_KEYWORD);
    assert_token("int main() { return 42; }", LANGUAGE_C, "42", SYNTAX_NUMBER);
    assert_token("return_value", LANGUAGE_C, "return_value", SYNTAX_VARIABLE);
    assert_token("  # include <stdio.h>", LANGUAGE_C, "# include", SYNTAX_DIRECTIVE);
    assert_token("/* hello\nreturn 42; */ int x;", LANGUAGE_C, "return 42;", SYNTAX_COMMENT);
    assert_token("/* hello\nreturn 42; */ int x;", LANGUAGE_C, "int", SYNTAX_TYPE);
    assert_token("// hello \\\nreturn\nint x;", LANGUAGE_C, "return", SYNTAX_COMMENT);
    assert_token("// hello\nint x;", LANGUAGE_C, "int", SYNTAX_TYPE);
    assert_token("\"escaped \\\" // not a comment\"", LANGUAGE_C, "// not a comment", SYNTAX_STRING);
    assert_token("'\\''", LANGUAGE_C, "'\\''", SYNTAX_STRING);
    assert_token("0xff+1", LANGUAGE_C, "+", SYNTAX_OPERATOR);
    assert_token("1e-3", LANGUAGE_C, "1e-3", SYNTAX_NUMBER);
    assert_token("template<typename T> class Item {};", LANGUAGE_CPP, "template", SYNTAX_KEYWORD);
    assert_token("u8R\"tag(first\n\"quoted\" /* text */)tag\"; int x;", LANGUAGE_CPP, "/* text */", SYNTAX_STRING);
    assert_token("R\"(unterminated\nstring", LANGUAGE_CPP, "string", SYNTAX_STRING);
    assert_token("R\"(text)\"; int x;", LANGUAGE_CPP, "int", SYNTAX_TYPE);
    assert_token("1'000", LANGUAGE_CPP, "1'000", SYNTAX_NUMBER);
    assert_token("public class Main { boolean value = true; }", LANGUAGE_JAVA, "public", SYNTAX_KEYWORD);
    assert_token("public class Main { boolean value = true; }", LANGUAGE_JAVA, "boolean", SYNTAX_TYPE);
    assert_token("@Override\npublic void run() {}", LANGUAGE_JAVA, "@Override", SYNTAX_DIRECTIVE);
    assert_token("\"\"\"\ntext // still text\n\"\"\"; return;", LANGUAGE_JAVA, "// still text", SYNTAX_STRING);
    assert_token("\"\"\"\ntext\n\"\"\"; return;", LANGUAGE_JAVA, "return", SYNTAX_KEYWORD);
    assert_token("async def task():\n    return True", LANGUAGE_PYTHON, "async", SYNTAX_KEYWORD);
    assert_token("async def task():\n    return True", LANGUAGE_PYTHON, "True", SYNTAX_CONSTANT);
    assert_token("# return 1\nx = 2", LANGUAGE_PYTHON, "return 1", SYNTAX_COMMENT);
    assert_token("'''first\n# not a comment\n'''\nreturn", LANGUAGE_PYTHON, "# not a comment", SYNTAX_STRING);
    assert_token("'''first\n# not a comment\n'''\nreturn", LANGUAGE_PYTHON, "return", SYNTAX_KEYWORD);
    assert_token("rf\"value {item}\"", LANGUAGE_PYTHON, "rf\"value {item}\"", SYNTAX_STRING);
    assert_token("@functools.cache\ndef task(): pass", LANGUAGE_PYTHON, "@functools.cache", SYNTAX_DIRECTIVE);
    assert_token("\"unfinished\nreturn", LANGUAGE_PYTHON, "return", SYNTAX_KEYWORD);
    assert_token("// division", LANGUAGE_PYTHON, "/", SYNTAX_OPERATOR);
    assert_token("int return # hello", LANGUAGE_TEXT, "int return # hello", SYNTAX_NORMAL);

    assert_token("size_t count = strlen(text);", LANGUAGE_C, "size_t", SYNTAX_TYPE);
    assert_token("size_t count = strlen(text);", LANGUAGE_C, "strlen", SYNTAX_FUNCTION);
    assert_token("size_t count = strlen(text);", LANGUAGE_C, "count", SYNTAX_VARIABLE);
    assert_token("FILE* stream = fopen(path, mode);", LANGUAGE_C, "FILE", SYNTAX_TYPE);
    assert_token("Buffer* buf = create_buffer();", LANGUAGE_C, "Buffer", SYNTAX_TYPE);
    assert_token("buf->text_size += INITIAL_BUFFER_SIZE;", LANGUAGE_C, "text_size", SYNTAX_MEMBER);
    assert_token("buf->text_size += INITIAL_BUFFER_SIZE;", LANGUAGE_C, "INITIAL_BUFFER_SIZE", SYNTAX_CONSTANT);
    assert_token("#define capacity 1024", LANGUAGE_C, "capacity", SYNTAX_CONSTANT);
    assert_token("#include <stdio.h>", LANGUAGE_C, "<stdio.h>", SYNTAX_STRING);
    assert_token("struct node { int value; };", LANGUAGE_C, "node", SYNTAX_TYPE);
    assert_token("std::vector<std::string> names;", LANGUAGE_CPP, "std", SYNTAX_TYPE);
    assert_token("std::vector<std::string> names;", LANGUAGE_CPP, "vector", SYNTAX_TYPE);
    assert_token("names.push_back(value);", LANGUAGE_CPP, "push_back", SYNTAX_FUNCTION);
    assert_token("std::cout << value;", LANGUAGE_CPP, "cout", SYNTAX_BUILTIN);
    assert_token("0x1.fp-2 + 4", LANGUAGE_CPP, "0x1.fp-2", SYNTAX_NUMBER);
    assert_token("0x1e+2", LANGUAGE_CPP, "+", SYNTAX_OPERATOR);
    assert_token("public static void main(String[] args)", LANGUAGE_JAVA, "main", SYNTAX_FUNCTION);
    assert_token("public static void main(String[] args)", LANGUAGE_JAVA, "String", SYNTAX_TYPE);
    assert_token("System.out.println(value);", LANGUAGE_JAVA, "System", SYNTAX_TYPE);
    assert_token("System.out.println(value);", LANGUAGE_JAVA, "out", SYNTAX_MEMBER);
    assert_token("System.out.println(value);", LANGUAGE_JAVA, "println", SYNTAX_FUNCTION);
    assert_token("private List<String> values = new ArrayList<>();", LANGUAGE_JAVA, "List", SYNTAX_TYPE);
    assert_token("public class Box {}", LANGUAGE_JAVA, "Box", SYNTAX_TYPE);
    assert_token("def greet(name): print(name)", LANGUAGE_PYTHON, "greet", SYNTAX_FUNCTION);
    assert_token("def greet(name): print(name)", LANGUAGE_PYTHON, "print", SYNTAX_BUILTIN);
    assert_token("self.items.append(value)", LANGUAGE_PYTHON, "self", SYNTAX_BUILTIN);
    assert_token("self.items.append(value)", LANGUAGE_PYTHON, "items", SYNTAX_MEMBER);
    assert_token("self.items.append(value)", LANGUAGE_PYTHON, "append", SYNTAX_FUNCTION);
    assert_token("class lowercase:", LANGUAGE_PYTHON, "lowercase", SYNTAX_TYPE);
    assert_token("matrix@other", LANGUAGE_PYTHON, "@", SYNTAX_OPERATOR);
    assert_token("1_000 + 0b101 + .5e-3", LANGUAGE_PYTHON, ".5e-3", SYNTAX_NUMBER);
    assert_token("1_000 + 0b101 + .5e-3", LANGUAGE_PYTHON, "0b101", SYNTAX_NUMBER);
    assert_highlight("printf(\"value: %d\\n\", count);", LANGUAGE_C, "\\n", SYNTAX_ESCAPE);
    assert_highlight("String text = \"\\u0041\";", LANGUAGE_JAVA, "\\u0041", SYNTAX_ESCAPE);
    assert_highlight("f\"Hello {user.name}, {len(items)} items\"", LANGUAGE_PYTHON, "Hello ", SYNTAX_STRING);
    assert_highlight("f\"Hello {user.name}, {len(items)} items\"", LANGUAGE_PYTHON, "user", SYNTAX_VARIABLE);
    assert_highlight("f\"Hello {user.name}, {len(items)} items\"", LANGUAGE_PYTHON, "name", SYNTAX_MEMBER);
    assert_highlight("f\"Hello {user.name}, {len(items)} items\"", LANGUAGE_PYTHON, "len", SYNTAX_BUILTIN);
    assert_highlight("f\"{price:>{width}.2f}\"", LANGUAGE_PYTHON, "price", SYNTAX_VARIABLE);
    assert_highlight("f\"{price:>{width}.2f}\"", LANGUAGE_PYTHON, "width", SYNTAX_VARIABLE);
    assert_highlight("f\"{price:>{width}.2f}\"", LANGUAGE_PYTHON, ".2f", SYNTAX_STRING);
    assert_highlight("f\"{value!r}\"", LANGUAGE_PYTHON, "!r", SYNTAX_ESCAPE);
    assert_highlight("f\"{{literal}} {42 + value}\"", LANGUAGE_PYTHON, "{{", SYNTAX_ESCAPE);
    assert_highlight("f\"{{literal}} {42 + value}\"", LANGUAGE_PYTHON, "42", SYNTAX_NUMBER);
    assert_highlight("f\"{{literal}} {42 + value}\"", LANGUAGE_PYTHON, "+", SYNTAX_OPERATOR);
    assert_highlight("f\"{items[\"key\"]}\"", LANGUAGE_PYTHON, "key", SYNTAX_STRING);
    assert_highlight("rf\"\\{value}\"", LANGUAGE_PYTHON, "value", SYNTAX_VARIABLE);
    assert_highlight("r\"a\\n\"", LANGUAGE_PYTHON, "\\n", SYNTAX_STRING);
    assert_highlight("R\"(raw \\n)\"", LANGUAGE_CPP, "\\n", SYNTAX_STRING);
    test_incomplete_tokens();

    Buffer* buf = create_buffer();
    assert(replace_buffer(buf, 0, 0, "/* open\nint x;", 14));
    SyntaxToken token = next_syntax_token(buf, LANGUAGE_C, 0);
    assert(token.type == SYNTAX_COMMENT && token.end == 14);
    assert(replace_buffer(buf, 7, 0, "*/", 2));
    token = next_syntax_token(buf, LANGUAGE_C, 0);
    assert(token.type == SYNTAX_COMMENT && token.end == 9);
    token = next_syntax_token(buf, LANGUAGE_C, 10);
    assert(token.type == SYNTAX_TYPE && token.end == 13);
    free_buffer(buf);
    puts("Syntax highlighting tests passed");
    return 0;
}
