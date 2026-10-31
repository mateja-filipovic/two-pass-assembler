<h1 align="center">Two-Pass Assembler</h1>

<p align="center">
  A two-pass assembler in C++ for a 16-bit teaching processor: sections, symbols, relocations and nine addressing modes.
</p>

<p align="center">
  <b>English</b> | <a href="README.sr.md">Srpski</a>
</p>

<p align="center">
  <a href="#build">Build</a> ·
  <a href="#run">Run</a> ·
  <a href="#usage">Usage</a> ·
  <a href="#design">Design</a> ·
  <a href="#tests">Tests</a> ·
  <a href="#project-layout">Project layout</a>
</p>

---

This is the first of three tasks in the System Software course project at the School of Electrical Engineering, University of Belgrade. The assignment describes an abstract 16-bit computer and asks for an assembler, a linker and an emulator for it. This repository holds the assembler. It reads a source file written in the assignment's assembly language and produces a text object file in a school variant of the ELF format: a section table, a symbol table, relocation records and machine code, ready for a linker to combine with other object files. The linker and emulator are separate programs and are not part of this repository.

## Build

Needs `g++` and `make`. It builds on Linux and macOS.

```sh
git clone https://github.com/mateja-filipovic/Two-Pass-Assembler.git
cd Two-Pass-Assembler
make
```

The binary `asembler` lands in the repository root. The spelling with one `s` is the name the assignment requires. `make clean` removes it along with the object files.

## Run

```sh
./asembler -o main.o tests/test_main.s
```

The `-o` option names the output file and may appear before or after the input file; without it the output is written to `a.out`. The output is plain text and can be opened in any editor. On a syntax error the assembler prints the line number and the problem, writes nothing, and exits with status 1.

## Usage

**Source format.** One instruction or directive per line. A comment starts with `#` and runs to the end of the line. A label ends with `:` and must be the first thing on its line; it may stand alone, in which case it marks the next line that has content. Operands are separated by commas. Literals are decimal or hexadecimal with a `0x` prefix and may be negative. Registers are `r0` to `r7`, with `sp` and `pc` accepted as names for `r6` and `r7`, plus the status word `psw`.

```asm
# file: main.s
.global myStart
.global myCounter
.section myCode
.equ tim_cfg, 0xFF10
myStart:
    ldr r0, $0x1
    str r0, tim_cfg
wait:
    ldr r0, myCounter
    ldr r1, $5
    cmp r0, r1
    jne wait
    halt
.section myData
myCounter:
    .word 0
.end
```

**Directives.**

| Directive | Effect |
|---|---|
| `.global sym, ...` | Export the listed symbols |
| `.extern sym, ...` | Import the listed symbols |
| `.section name` | Start a new section; the previous one ends |
| `.word init, ...` | Allocate two bytes per initializer, each a symbol or a literal, little-endian |
| `.skip n` | Allocate `n` zero bytes |
| `.equ sym, literal` | Define an absolute symbol |
| `.end` | Stop assembling; the rest of the file is ignored |

**Instructions.** Register operands are written `regD` and `regS`; `operand` is any of the addressing modes below.

| Mnemonic | Effect |
|---|---|
| `halt` | Stop the processor |
| `int regD` | Software interrupt through IVT entry `regD mod 8` |
| `iret` | Pop `psw`, pop `pc` |
| `call operand` | Push `pc`, jump to `operand` |
| `ret` | Pop `pc` |
| `jmp`, `jeq`, `jne`, `jgt operand` | Jump, unconditionally or on the flags from the last `cmp` |
| `push regD`, `pop regD` | Stack operations through `r6`; the stack grows toward lower addresses |
| `xchg regD, regS` | Swap two registers |
| `add`, `sub`, `mul`, `div`, `cmp regD, regS` | Arithmetic; `cmp` only sets the flags |
| `not regD` | Bitwise complement |
| `and`, `or`, `xor`, `test regD, regS` | Logic; `test` only sets the flags |
| `shl`, `shr regD, regS` | Shift `regD` by `regS` bits |
| `ldr regD, operand` | Load `operand` into `regD` |
| `str regD, operand` | Store `regD` to `operand` |

**Addressing modes.** Data instructions and jumps use different notation for the same hardware modes, because for a jump the operand is the target address while for `ldr` and `str` it is the value.

| Data operand | Jump operand | Meaning | Mode |
|---|---|---|---|
| `$lit`, `$sym` | `lit`, `sym` | The value itself | immediate |
| `lit`, `sym` | `*lit`, `*sym` | The word in memory at that address | memory |
| `%sym` | `%sym` | The same, addressed relative to `pc` | PC-relative |
| `reg` | `*reg` | The register | register direct |
| `[reg]` | `*[reg]` | The word in memory at the address in `reg` | register indirect |
| `[reg + lit]`, `[reg + sym]` | `*[reg + lit]`, `*[reg + sym]` | The word at `reg` plus a signed 16-bit offset | register indirect with offset |

`str` to an immediate operand is an error.

## Design

**Two passes.** The first pass walks the file once to build the symbol table: every label, section, `.extern` and `.equ` becomes an entry with its section and offset, and the location counter advances by the size of each instruction or directive. The second pass walks the file again, resolves every operand against the finished table, emits machine code and records a relocation wherever the linker will have to patch a value. Both passes classify operands through the same parser, so the size counted in the first pass and the encoding emitted in the second cannot disagree.

**Symbol table.** Each entry carries a name, a section, an offset within that section, a scope (`l` local or `g` global) and an index. Sections appear as symbols at offset 0. Imported symbols live in the pseudo-section `UND`, and `.equ` symbols in `ABS`, where the offset is the literal value. A symbol defined twice is an error.

**Instruction encoding.** Instructions are one to five bytes: an opcode byte, a register byte (`regD` in the high nibble, `regS` in the low), an addressing byte and an optional two-byte payload. The payload is written high byte first, as the assignment draws it. PC-relative operands use `r7` as the base register, with the direct-plus-offset mode for jumps and the indirect-plus-offset mode for data. Data words from `.word` are written low byte first, matching the little-endian memory of the target.

**Relocations.** A relocation is recorded for every symbol that is not absolute, in a per-section list of `offset type symbol`. `offset` is the start of the instruction or `.word` that holds the value; for an instruction the payload sits three bytes further on. `type` is `R_HYPO_16` for absolute references and `R_HYPO_PC16` for PC-relative ones. `symbol` is the index into the symbol table. The payload already contains the symbol's offset within its own section, and the linker replaces it with the final value rather than adding to it. For PC-relative references the assembler leaves the PC adjustment to the linker.

**Object file.** A text file with four parts, in this order: the section table, the symbol table, one relocation list per section that has any, and the machine code grouped by section. The object file for the program above looks like this.

```
# ------------------ SECTION TABLE ------------------
           NAME           SIZE
         myCode             28
         myData              2

# ------------------ SYMBOL TABLE ------------------
          LABEL        SECTION         OFFSET          SCOPE         NUMBER
         myCode         myCode              0              l              0
        tim_cfg            ABS          65296              l              1
        myStart         myCode              0              g              2
           wait         myCode             10              l              3
         myData         myData              0              l              4
      myCounter         myData              0              g              5

# ------------------ REL.myCode ------------------
10 R_HYPO_16 5
22 R_HYPO_16 3

# ------------------ OBJECT FILE ------------------
# ------------------ CODE.myCode ------------------
A0 00 00 00 01
B0 00 04 FF 10
A0 00 04 00 00
A0 10 00 00 05
74 01
52 F0 00 00 0A
00
# ------------------ CODE.myData ------------------
00 00
```

**Parsing.** The parser splits each line on whitespace, drops the comment and the commas, and hands the tokens to the assembler. A bracketed operand such as `[r1 + 5]` may be split across tokens by the whitespace, so the operand reader joins tokens until it sees the closing bracket. Every operand form is matched against a small set of regular expressions.

## Tests

The programs in `tests/` are the inputs used to exercise the assembler. There is no automated runner; assemble each one and inspect the output.

```sh
for f in tests/*.s; do ./asembler -o "${f%.s}.o" "$f"; done
```

| File | Covers |
|---|---|
| `test_one.s` | Most addressing modes of `ldr`, `str`, the jumps and `call`; `.skip`, `.word`, `.equ`, `.extern` and `.global` |
| `test_two.s` | Register indirect with symbol and literal offsets, a PC-relative jump, global and imported symbols |
| `test_main.s`, `test_interrupts.s` | The example program from the assignment: an interrupt vector table, timer and terminal handlers in one file, the main loop in the other |

## Project layout

```
inc/assembler.h    symbol, relocation, section and operand records; the Assembler class
inc/parser.h       line reader and tokenizer
src/assembler.cpp  both passes, operand parsing and encoding, output writer
src/parser.cpp     tokenizer
src/main.cpp       command line handling
tests/             example programs
docs/              implementation notes
Makefile
```

## Known limits

- A section name may appear only once per file; a second `.section` with the same name is not merged with the first.
- The object file is text only. The assignment allows an additional binary output, which is not implemented.
