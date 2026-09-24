# Textura

A lightweight terminal-based text editor built with ncurses in C.

## Features

- Automatic syntax highlighting for C, C++, Java, and Python
- Gap buffer editing with preserved newlines, tabs, and whitespace
- Case-sensitive literal search with highlighted matches and wraparound navigation
- Replace all occurrences, including deletion with an empty replacement, with one-step undo
- Go to line, Home/End, and page navigation
- Automatic indentation that follows the current line's spaces and tabs
- Vertical and horizontal scrolling that adapts to terminal resizing
- Undo/redo for the last 1,000 edits, including grouped replacements and indentation
- Modified-file indicator and protection against accidentally quitting with unsaved changes
- Line numbers, live line/word counts, cursor position, and built-in shortcut help

## Requirements

- C compiler
- ncurses library
- Python 3 for terminal integration tests (optional)

## Building

```
make
```

## Usage

```
./Textura [filename]
```

Without a filename, Textura prompts for one. A new file is written when you save.
Ctrl+Q exits immediately when there are no unsaved edits. With unsaved edits,
press Ctrl+S to save or Ctrl+Q a second time to discard them. Any other command
cancels the quit confirmation.

Search and replacement are literal and case sensitive. Ctrl+F sets the search
text; Ctrl+N and Ctrl+P jump between matches and wrap around the document.
Ctrl+R replaces non-overlapping matches throughout the document. Its search
prompt starts with the current search text; Ctrl+U clears a prompt. An empty
replacement deletes matches. Esc cancels either prompt without changing text.
Ctrl+Z reverses an entire replace-all operation.

## Syntax Highlighting

The filename extension selects the language automatically: `.c` and `.h` for C;
`.cpp`, `.cc`, `.cxx`, `.hpp`, `.hh`, `.hxx`, `.ipp`, `.tpp`, `.c++`, `.h++`, `.C`, and `.H` for C++; `.java` for
Java; and `.py`, `.pyw`, and `.pyi` for Python. Other files remain plain text.
The status bar shows the active language. F3 cycles through automatic detection,
plain text, C, C++, Java, and Python, so C++ `.h` files and extensionless scripts
can use the appropriate highlighting.

The default palette is designed for dark backgrounds, with readable gray comments,
blue functions, purple keywords, gold types, green strings, and orange numbers.
Press F4 for a palette suited to a light terminal background. Terminals with 256
colors get the full palette; other color terminals use the basic ANSI palette.
Monochrome terminals remain usable without color.

Highlighting is lexical, not compiler-backed. User-defined types use declaration
context and naming conventions.

## Key Bindings

| Key | Action |
| --- | --- |
| Ctrl+Q | Quit; press again to discard unsaved edits |
| Ctrl+S | Save file |
| Ctrl+Z / Ctrl+Y | Undo / redo |
| Ctrl+F | Find text |
| Ctrl+N / Ctrl+P | Next / previous match |
| Ctrl+R | Replace all |
| Ctrl+G | Go to a line number |
| Home / Ctrl+A | Start of line |
| End / Ctrl+E | End of line |
| Page Up / Page Down | Move one screen |
| Arrow keys | Navigate |
| Backspace / Delete | Delete previous / next character |
| Enter | Insert newline with current indentation |
| Tab | Insert tab |
| F1 | Shortcut help |
| F2 | Toggle automatic indentation |
| F3 | Cycle syntax language |
| F4 | Switch dark / light syntax palette |
| Esc | Cancel a prompt or clear search highlights |
| Ctrl+U | Clear prompt input |

## Testing

```
make test
make test-terminal
```

## Project Structure

- `src/main.c`: Editor command handling
- `src/buffer.c`: Gap buffer and file operations
- `src/syntax.c`: Language detection, tokenization, and syntax colors
- `src/history.c`: Undo/redo and saved revision tracking
- `src/utils.c`: Search, replacement, navigation, prompts, and rendering
- `include/`: Header files
- `tests/`: Core and terminal integration tests

## Cleanup

```
make clean
```
