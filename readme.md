

# Two Pass Assembler
An implementation of a two pass assembler, that converts code written in assembly to machine code. The assembly language used is x86-like - it supports sections, directives and instructions with various addressing modes.

## Some important notes
* the project is compiled using `g++` and tested on ubuntu
* the output is a file structured like an `ELF` object file, containing
a section table (name and size), a symbol table, relocation data and machine code grouped by section

## Project structure
* `inc` and `src` folders contain the code
* `tests` folder contains examples written in assembly
* `docs` folder contains some implementation details and useful info

## Usage
- clone the project using
    ``` bash
    git clone https://github.com/mateja-filipovic/Two-Pass-Assembler.git
    ```
- compile the project using the provided `makefile` (make sure you have g++ installed first)
- provide paths to input and output files as command line arguments
    ```bash
    ./assembler -o elf_output.txt tests/test_one.s
    ```
    - the output file will be created automatically if it doesn't exist
- you can now inspect the `elf_output` file containing the machine code
