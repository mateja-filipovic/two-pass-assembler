#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include <iostream>
#include <iomanip>
#include <regex>
#include <fstream>

#include "parser.h"

typedef unsigned int uint;

struct Symbol
{
    std::string label;
    std::string section;
    long offset;
    char scope;
    int index;

    Symbol(std::string _label, std::string _section, long _offset, char _scope, int _index) :
        label(_label), section(_section), offset(_offset), scope(_scope), index(_index)
    {}
};

struct RelRecord
{
    int offset;
    std::string type;
    int symbol_number;
    std::string section;

    RelRecord(int _offset, std::string _type, int _symbol_number, std::string _section) :
    offset(_offset), type(_type), symbol_number(_symbol_number), section(_section)
    {}
};

struct Section
{
    std::string name;
    uint size;
    std::vector<std::string> code;
};

struct Operand
{
    std::string addr_mode;
    char reg; // RS nibble, '0' when unused
    int size;
    bool immediate;
    bool has_symbol;
    std::string symbol;
    int literal;
    std::string reloc_type;
};

class Assembler
{
public:
    Assembler(Parser* _parser, std::string _output_file);
    ~Assembler();

    void first_pass();
    void second_pass();


private:
    // first pass
    void label_handler();
    void directive_handler();
    void instruction_handler();

    // directive handlers first pass
    void extern_handler_fp();
    void section_handler_fp();
    void word_handler_fp();
    void skip_handler_fp();
    void equ_handler_fp();
    void end_handler_fp();
    void global_handler_fp();


    // second pass
    void directive_handler_sp();
    void instruction_handler_sp();

    //directive handlers second pass
    void section_handler_sp();
    void close_section(); // move output so far into the current section
    void word_handler_sp();
    void global_handler_sp();
    void skip_handler_sp();
    void end_handler_sp();

    // these are called by the instruction handler
    void twobyte_handler_sp(std::string opcode);
    void branch_handler_sp(); // handling jmp, jeq, jgt, jne and call
    void mem_handler_sp(); // used to handle ldr and str
    void stack_handler_sp(); // used to handle push and pop

    // helper functions
    Symbol* find_symbol(std::string label); // find simbol by name
    std::string write_hex(int val, int num_of_nibbles); // returns hex representation of a num, ex. 17 => 00 11
    std::string extract_register_num(std::string token); // get register number, ex. r3 => 3
    std::string read_operand(); // consume the rest of the line as one operand, ex. "[r1", "+", "5]" => "[r1+5]"
    Operand parse_operand(const std::string& text, bool is_branch); // classify an operand, shared by both passes
    std::string encode_operand(const Operand& op); // RS nibble, AddrMode byte and payload; records relocations
    int literal_to_number(std::string literal); // convert a literal (hex or decimal) to an integer

    // when compilation is done, print everything into the output file
    void print_data();
    void print_sections(std::ofstream& outfile);
    void print_symtab(std::ofstream& outfile);
    void print_reloc(std::ofstream& outfile);
    void print_object_file(std::ofstream& outfile);


    // member variables
    Parser* parser;

    std::string current_section;

    uint line_counter; // current line
    uint location_counter;
    uint token_counter; // current token in line

    std::vector<std::string> lines; // lines read from a file
    std::vector<std::string> line_tokens; // current line tokens

    std::vector<std::string> output; // object file output
    std::vector<Section> sections; // output grouped by section, in source order

    std::vector<Symbol*> symbol_table;
    std::vector<RelRecord *> relocation_table;

    bool end_reached; // stop the compilation

    std::string output_file; // simple text file

    // constants used

    // for external symbols
    const std::string UNDEFINED_SECTION = "UND";
    // for equ
    const std::string ABSOLUTE_SECTION = "ABS";
    // for relocations
    const std::string RELOCATION_ABSOLUTE = "R_HYPO_16";
    const std::string RELOCATION_PCREL = "R_HYPO_PC16";
    const std::string RELOCATION_DATA = "R_HYPO_DATA16"; // .word, little-endian

    // directive mneumonics
    const std::string GLOBAL_DIRECTIVE = ".global";
    const std::string EXTERN_DIRECTIVE = ".extern";
    const std::string SECTION_DIRECTIVE = ".section";
    const std::string WORD_DIRECTIVE = ".word";
    const std::string SKIP_DIRECTIVE = ".skip";
    const std::string EQU_DIRECTIVE = ".equ";
    const std::string END_DIRECTIVE = ".end";

    // instruction mneumonics
    const std::string HALT_MNE = "halt";
    const std::string IRET_MNE = "iret";
    const std::string RET_MNE = "ret";
    const std::string XCHG_MNE = "xchg"; 
    const std::string ADD_MNE = "add";
    const std::string SUB_MNE = "sub";
    const std::string MUL_MNE = "mul";
    const std::string DIV_MNE = "div";
    const std::string CMP_MNE = "cmp";
    const std::string AND_MNE = "and"; 
    const std::string OR_MNE = "or";
    const std::string XOR_MNE = "xor";
    const std::string TEST_MNE = "test";
    const std::string SHL_MNE = "shl";
    const std::string SHR_MNE = "shr";
    const std::string NOT_MNE = "not";
    const std::string INT_MNE = "int";
    const std::string JMP_MNE = "jmp";
    const std::string JEQ_MNE = "jeq";
    const std::string JNE_MNE = "jne";
    const std::string JGT_MNE = "jgt";
    const std::string CALL_MNE = "call";
    const std::string LDR_MNE = "ldr";
    const std::string STR_MNE = "str";
    const std::string PUSH_MNE = "push";
    const std::string POP_MNE = "pop";


    // regexes used
    static const std::regex LITERAL_REGEX;
    static const std::regex SYMBOL_REGEX;
    static const std::regex REGISTER_INSTRUCTION_REGEX;
    static const std::regex REGIND_REGEX;
    static const std::regex REGIND_LITERAL_REGEX;
    static const std::regex REGIND_SYMBOL_REGEX;


};


#endif