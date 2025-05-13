# Textura

A lightweight terminal-based text editor built with ncurses in C.

## Features
- Gap buffer implementation for efficient text editing
- Basic editing operations (insert, delete, navigation)
- File saving and loading
- Undo/redo functionality
- Line number display
- Status bar with file information

## Requirements
- GCC compiler
- ncurses library

## Installation
```
git clone https://github.com/yourusername/Textura.git
cd Textura
make
```

## Usage
```
./Textura [filename]
```
If no filename is provided, you'll be prompted to create a new file.

## Key Bindings
- Ctrl+Q: Quit
- Ctrl+S: Save file
- Ctrl+Z: Undo
- Ctrl+Y: Redo
- Arrow keys: Navigate
- Backspace/Delete: Remove characters

## Project Structure
- `src/`: Source code files
  - `main.c`: Core editor functionality
  - `buffer.c`: Gap buffer implementation
  - `history.c`: Undo/redo functionality
  - `utils.c`: Helper functions
- `include/`: Header files

## Building
```
make
```

## Cleanup
```
make clean
```