#include "../inc/linker.h"

static std::vector<std::string> tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while(ss >> token)
        tokens.push_back(token);
    return tokens;
}

// "# ------------------ REL.text ------------------" => "REL.text"
static std::string header_name(const std::vector<std::string>& tokens)
{
    std::string name;
    for(size_t i = 2; i + 1 < tokens.size(); i++)
        name += (name.empty() ? "" : " ") + tokens[i];
    return name;
}

static std::string hex_byte(unsigned char value)
{
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (int)value;
    return ss.str();
}

Linker::Linker(const std::vector<std::string>& _input_files, const std::string& _output_file,
               const std::map<std::string, uint>& _places, bool _hex_output)
    : input_files(_input_files), output_file(_output_file), places(_places), hex_output(_hex_output)
{
}

void Linker::link()
{
    for(const std::string& filename : input_files)
        read_object_file(filename);

    merge_sections();
    collect_definitions();

    if(hex_output)
    {
        place_sections();
        apply_relocations();
        write_hex();
    }
    else
    {
        write_linkable();
    }
}

void Linker::read_object_file(const std::string& filename)
{
    std::ifstream file(filename);
    if(file.fail())
    {
        std::cout << "ERROR opening input file: " << filename << std::endl;
        exit(1);
    }

    ObjectFile obj;
    obj.filename = filename;

    std::string line;
    std::string block;
    int row = 0;
    uint line_number = 0;

    while(std::getline(file, line))
    {
        line_number++;
        std::vector<std::string> tokens = tokenize(line);
        if(tokens.empty())
            continue;

        if(tokens.at(0) == "#")
        {
            block = header_name(tokens);
            row = 0;
            continue;
        }

        if(block == "SECTION TABLE")
        {
            if(tokens.at(0) == "NAME")
                continue;
            if(tokens.size() != 2)
            {
                std::cout << "ERROR in " << filename << " line " << line_number << ", malformed section table row" << std::endl;
                exit(1);
            }
            ObjSection section;
            section.name = tokens.at(0);
            section.size = std::stoul(tokens.at(1));
            section.base = 0;
            obj.sections.push_back(section);
        }
        else if(block == "SYMBOL TABLE")
        {
            if(tokens.at(0) == "LABEL")
                continue;
            if(tokens.size() != 5 || std::stoi(tokens.at(4)) != row)
            {
                std::cout << "ERROR in " << filename << " line " << line_number << ", malformed symbol table row" << std::endl;
                exit(1);
            }
            ObjSymbol symbol;
            symbol.label = tokens.at(0);
            symbol.section = tokens.at(1);
            symbol.offset = std::stol(tokens.at(2));
            symbol.scope = tokens.at(3).at(0);
            obj.symbols.push_back(symbol);
            row++;
        }
        else if(block.compare(0, 4, "REL.") == 0)
        {
            int s = find_section(obj, block.substr(4));
            if(s < 0 || tokens.size() != 3)
            {
                std::cout << "ERROR in " << filename << " line " << line_number << ", malformed relocation row" << std::endl;
                exit(1);
            }
            ObjRelocation rel;
            rel.offset = std::stoul(tokens.at(0));
            rel.type = tokens.at(1);
            rel.symbol = std::stoi(tokens.at(2));
            obj.sections.at(s).relocations.push_back(rel);
        }
        else if(block.compare(0, 5, "CODE.") == 0)
        {
            int s = find_section(obj, block.substr(5));
            if(s < 0)
            {
                std::cout << "ERROR in " << filename << " line " << line_number << ", code for unknown section " << block.substr(5) << std::endl;
                exit(1);
            }
            for(const std::string& byte : tokens)
                obj.sections.at(s).code.push_back(std::stoul(byte, nullptr, 16));
        }
        else if(block != "OBJECT FILE")
        {
            std::cout << "ERROR in " << filename << " line " << line_number << ", unexpected content" << std::endl;
            exit(1);
        }
    }

    for(const ObjSection& section : obj.sections)
    {
        if(section.code.size() != section.size)
        {
            std::cout << "ERROR in " << filename << ", section " << section.name << " declares " << section.size
                      << " bytes but holds " << section.code.size() << std::endl;
            exit(1);
        }
        for(const ObjRelocation& rel : section.relocations)
        {
            if(rel.symbol < 0 || rel.symbol >= (int)obj.symbols.size() || rel.offset + 2 > section.size)
            {
                std::cout << "ERROR in " << filename << ", relocation in section " << section.name << " is out of range" << std::endl;
                exit(1);
            }
        }
    }

    files.push_back(obj);
}

int Linker::find_section(const ObjectFile& obj, const std::string& name)
{
    for(size_t i = 0; i < obj.sections.size(); i++)
        if(obj.sections.at(i).name == name)
            return i;
    return -1;
}

void Linker::merge_sections()
{
    for(ObjectFile& obj : files)
    {
        for(ObjSection& section : obj.sections)
        {
            int m = merged_index(section.name);
            if(m < 0)
            {
                MergedSection fresh;
                fresh.name = section.name;
                fresh.size = 0;
                fresh.address = 0;
                fresh.placed = false;
                merged.push_back(fresh);
                m = merged.size() - 1;
            }
            section.base = merged.at(m).size;
            merged.at(m).size += section.size;
            merged.at(m).code.insert(merged.at(m).code.end(), section.code.begin(), section.code.end());
        }
    }
}

int Linker::merged_index(const std::string& name)
{
    for(size_t i = 0; i < merged.size(); i++)
        if(merged.at(i).name == name)
            return i;
    return -1;
}

void Linker::place_sections()
{
    for(const auto& place : places)
    {
        int m = merged_index(place.first);
        if(m < 0)
        {
            std::cout << "ERROR: -place names unknown section " << place.first << std::endl;
            exit(1);
        }
        merged.at(m).address = place.second;
        merged.at(m).placed = true;
    }

    // explicitly placed sections must not overlap
    std::vector<int> placed;
    for(size_t i = 0; i < merged.size(); i++)
        if(merged.at(i).placed)
            placed.push_back(i);
    std::sort(placed.begin(), placed.end(), [this](int a, int b) { return merged.at(a).address < merged.at(b).address; });

    uint next_free = 0;
    for(size_t i = 0; i < placed.size(); i++)
    {
        const MergedSection& current = merged.at(placed.at(i));
        if(i + 1 < placed.size())
        {
            const MergedSection& following = merged.at(placed.at(i + 1));
            if(current.address + current.size > following.address)
            {
                std::cout << "ERROR: sections " << current.name << " and " << following.name << " overlap" << std::endl;
                exit(1);
            }
        }
        next_free = std::max(next_free, current.address + current.size);
    }

    // the rest go after the highest placed section, in order of appearance
    for(MergedSection& section : merged)
    {
        if(section.placed)
            continue;
        section.address = next_free;
        next_free += section.size;
    }

    for(const MergedSection& section : merged)
    {
        if(section.address + section.size > ADDRESS_SPACE)
        {
            std::cout << "ERROR: section " << section.name << " ends beyond the 64 KB address space" << std::endl;
            exit(1);
        }
    }
}

void Linker::collect_definitions()
{
    for(size_t f = 0; f < files.size(); f++)
    {
        for(size_t s = 0; s < files.at(f).symbols.size(); s++)
        {
            const ObjSymbol& symbol = files.at(f).symbols.at(s);
            if(symbol.scope != 'g' || symbol.section == UNDEFINED_SECTION)
                continue;

            auto existing = definitions.find(symbol.label);
            if(existing != definitions.end())
            {
                std::cout << "ERROR: multiple definitions of symbol " << symbol.label << " in "
                          << files.at(existing->second.file).filename << " and " << files.at(f).filename << std::endl;
                exit(1);
            }
            definitions[symbol.label] = {(int)f, (int)s};
        }
    }
}

SymbolLocation Linker::locate(int file, int symbol)
{
    const ObjSymbol& sym = files.at(file).symbols.at(symbol);
    SymbolLocation loc;
    loc.defined = true;
    loc.absolute = false;
    loc.offset = 0;

    if(sym.section == UNDEFINED_SECTION)
    {
        auto def = definitions.find(sym.label);
        if(def == definitions.end())
        {
            loc.defined = false;
            return loc;
        }
        return locate(def->second.file, def->second.symbol);
    }

    if(sym.section == ABSOLUTE_SECTION)
    {
        loc.absolute = true;
        loc.offset = sym.offset;
        return loc;
    }

    loc.section = sym.section;
    loc.offset = files.at(file).sections.at(find_section(files.at(file), sym.section)).base + sym.offset;
    return loc;
}

uint Linker::symbol_address(const SymbolLocation& loc)
{
    if(loc.absolute)
        return loc.offset;
    return merged.at(merged_index(loc.section)).address + loc.offset;
}

void Linker::patch(std::vector<unsigned char>& code, uint offset, int value, const std::string& type, const std::string& context)
{
    if(type != RELOCATION_ABSOLUTE && type != RELOCATION_PCREL && type != RELOCATION_DATA)
    {
        std::cout << "ERROR in " << context << ", unknown relocation type " << type << std::endl;
        exit(1);
    }

    unsigned char low = value & 0xFF;
    unsigned char high = (value >> 8) & 0xFF;

    // instruction payloads are high byte first, .word data is little-endian
    if(type == RELOCATION_DATA)
    {
        code.at(offset) = low;
        code.at(offset + 1) = high;
    }
    else
    {
        code.at(offset) = high;
        code.at(offset + 1) = low;
    }
}

void Linker::apply_relocations()
{
    for(size_t f = 0; f < files.size(); f++)
    {
        for(const ObjSection& section : files.at(f).sections)
        {
            MergedSection& target = merged.at(merged_index(section.name));

            for(const ObjRelocation& rel : section.relocations)
            {
                SymbolLocation loc = locate(f, rel.symbol);
                if(!loc.defined)
                {
                    std::cout << "ERROR: unresolved symbol " << files.at(f).symbols.at(rel.symbol).label
                              << " referenced in " << files.at(f).filename << std::endl;
                    exit(1);
                }

                uint patch_offset = section.base + rel.offset;
                int value = symbol_address(loc);

                // pc points past the two payload bytes when the operand is formed
                if(rel.type == RELOCATION_PCREL)
                    value -= target.address + patch_offset + 2;

                patch(target.code, patch_offset, value, rel.type, files.at(f).filename);
            }
        }
    }
}

void Linker::write_hex()
{
    std::ofstream outfile(output_file);

    std::vector<int> order;
    for(size_t i = 0; i < merged.size(); i++)
        order.push_back(i);
    std::sort(order.begin(), order.end(), [this](int a, int b) { return merged.at(a).address < merged.at(b).address; });

    bool first = true;
    uint last_address = 0;
    int bytes_on_line = 0;

    for(int m : order)
    {
        const MergedSection& section = merged.at(m);
        for(uint i = 0; i < section.size; i++)
        {
            uint address = section.address + i;
            if(first || address != last_address + 1 || bytes_on_line == 8)
            {
                if(!first)
                    outfile << std::endl;
                outfile << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address << ":";
                bytes_on_line = 0;
            }
            outfile << " " << hex_byte(section.code.at(i));
            last_address = address;
            bytes_on_line++;
            first = false;
        }
    }
    outfile << std::endl;
    outfile.close();
}

void Linker::write_linkable()
{
    // one symbol per merged section, then every defined symbol, then whatever is still undefined
    std::vector<ObjSymbol> rows;
    std::vector<std::vector<int>> row_of(files.size());

    for(const MergedSection& section : merged)
        rows.push_back({section.name, section.name, 0, 'l'});

    for(size_t f = 0; f < files.size(); f++)
    {
        row_of.at(f).assign(files.at(f).symbols.size(), -1);
        for(size_t s = 0; s < files.at(f).symbols.size(); s++)
        {
            const ObjSymbol& symbol = files.at(f).symbols.at(s);
            if(symbol.section == UNDEFINED_SECTION)
                continue;
            if(symbol.label == symbol.section)
            {
                row_of.at(f).at(s) = merged_index(symbol.label);
                continue;
            }
            SymbolLocation loc = locate(f, s);
            rows.push_back({symbol.label, symbol.section, loc.offset, symbol.scope});
            row_of.at(f).at(s) = rows.size() - 1;
        }
    }

    std::map<std::string, int> undefined_row;
    for(size_t f = 0; f < files.size(); f++)
    {
        for(size_t s = 0; s < files.at(f).symbols.size(); s++)
        {
            const ObjSymbol& symbol = files.at(f).symbols.at(s);
            if(symbol.section != UNDEFINED_SECTION)
                continue;

            auto def = definitions.find(symbol.label);
            if(def != definitions.end())
            {
                row_of.at(f).at(s) = row_of.at(def->second.file).at(def->second.symbol);
                continue;
            }
            if(undefined_row.count(symbol.label) == 0)
            {
                rows.push_back({symbol.label, UNDEFINED_SECTION, 0, 'g'});
                undefined_row[symbol.label] = rows.size() - 1;
            }
            row_of.at(f).at(s) = undefined_row.at(symbol.label);
        }
    }

    // relocations keep the assembler's convention: payload holds the symbol's offset in its section
    std::vector<std::vector<ObjRelocation>> relocations(merged.size());
    for(size_t f = 0; f < files.size(); f++)
    {
        for(const ObjSection& section : files.at(f).sections)
        {
            int m = merged_index(section.name);
            for(const ObjRelocation& rel : section.relocations)
            {
                ObjRelocation out;
                out.offset = section.base + rel.offset;
                out.type = rel.type;
                out.symbol = row_of.at(f).at(rel.symbol);
                relocations.at(m).push_back(out);

                SymbolLocation loc = locate(f, rel.symbol);
                patch(merged.at(m).code, out.offset, loc.defined ? loc.offset : 0, rel.type, files.at(f).filename);
            }
        }
    }

    std::ofstream outfile(output_file);

    outfile << "# ------------------ SECTION TABLE ------------------" << std::endl;
    outfile << std::setw(15) << "NAME" << std::setw(15) << "SIZE" << std::endl;
    for(const MergedSection& section : merged)
        outfile << std::setw(15) << section.name << std::setw(15) << section.size << std::endl;
    outfile << std::endl;
    outfile << std::endl;

    outfile << "# ------------------ SYMBOL TABLE ------------------" << std::endl;
    outfile << std::setw(15) << "LABEL" << std::setw(15) << "SECTION" << std::setw(15) << "OFFSET" << std::setw(15) << "SCOPE" << std::setw(15) << "NUMBER" << std::endl;
    for(size_t i = 0; i < rows.size(); i++)
    {
        const ObjSymbol& sym = rows.at(i);
        outfile << std::setw(15) << sym.label << std::setw(15) << sym.section << std::setw(15) << sym.offset << std::setw(15) << sym.scope << std::setw(15) << i << std::endl;
    }

    for(size_t m = 0; m < merged.size(); m++)
    {
        if(relocations.at(m).empty())
            continue;
        outfile << std::endl;
        outfile << std::endl;
        outfile << "# ------------------ REL." << merged.at(m).name << " ------------------" << std::endl;
        for(const ObjRelocation& rel : relocations.at(m))
            outfile << rel.offset << " " << rel.type << " " << rel.symbol << " " << std::endl;
    }

    outfile << std::endl;
    outfile << std::endl;
    outfile << "# ------------------ OBJECT FILE ------------------" << std::endl;
    for(const MergedSection& section : merged)
    {
        outfile << "# ------------------ CODE." << section.name << " ------------------" << std::endl;
        for(uint i = 0; i < section.size; i++)
        {
            outfile << hex_byte(section.code.at(i));
            outfile << ((i % 8 == 7 || i + 1 == section.size) ? "\n" : " ");
        }
    }

    outfile.close();
}
