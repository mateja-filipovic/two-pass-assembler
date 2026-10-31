#ifndef _LINKER_H_
#define _LINKER_H_

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

typedef unsigned int uint;

struct ObjSymbol
{
    std::string label;
    std::string section;
    long offset;
    char scope;
};

struct ObjRelocation
{
    uint offset;
    std::string type;
    int symbol;
};

struct ObjSection
{
    std::string name;
    uint size;
    std::vector<unsigned char> code;
    std::vector<ObjRelocation> relocations;
    uint base; // where this part starts inside the merged section
};

struct ObjectFile
{
    std::string filename;
    std::vector<ObjSection> sections;
    std::vector<ObjSymbol> symbols;
};

struct MergedSection
{
    std::string name;
    uint size;
    uint address;
    bool placed;
    std::vector<unsigned char> code;
};

struct SymbolRef
{
    int file;
    int symbol;
};

struct SymbolLocation
{
    bool defined;
    bool absolute;
    std::string section;
    long offset; // offset in the merged section, or the value when absolute
};

class Linker
{
public:
    Linker(const std::vector<std::string>& _input_files, const std::string& _output_file,
           const std::map<std::string, uint>& _places, bool _hex_output);

    void link();

private:
    // reading
    void read_object_file(const std::string& filename);
    int find_section(const ObjectFile& obj, const std::string& name);

    // merging and placing
    void merge_sections();
    int merged_index(const std::string& name);
    void place_sections();

    // symbols
    void collect_definitions();
    SymbolLocation locate(int file, int symbol);
    uint symbol_address(const SymbolLocation& loc);

    // output
    void apply_relocations();
    void write_hex();
    void write_linkable();
    void patch(std::vector<unsigned char>& code, uint offset, int value, const std::string& type, const std::string& context);

    // member variables
    std::vector<std::string> input_files;
    std::string output_file;
    std::map<std::string, uint> places;
    bool hex_output;

    std::vector<ObjectFile> files;
    std::vector<MergedSection> merged;
    std::map<std::string, SymbolRef> definitions; // global symbol name => where it is defined

    const uint ADDRESS_SPACE = 0x10000;

    const std::string UNDEFINED_SECTION = "UND";
    const std::string ABSOLUTE_SECTION = "ABS";
    const std::string RELOCATION_ABSOLUTE = "R_HYPO_16";
    const std::string RELOCATION_PCREL = "R_HYPO_PC16";
    const std::string RELOCATION_DATA = "R_HYPO_DATA16";
};

#endif
