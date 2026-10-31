

#include "../inc/assembler.h"

Assembler::Assembler(Parser* _parser, std::string _output_file) : parser(_parser), line_counter(1), location_counter(0),
token_counter(0), current_section("BLANK"), end_reached(false), output_file(_output_file)
{
    parser->parse_file(lines);
}

Assembler::~Assembler()
{
    for(Symbol* s : symbol_table)
        delete s;
    symbol_table.clear();

    for(RelRecord* r : relocation_table)
        delete r;
    relocation_table.clear();

    lines.clear();
    line_tokens.clear();
    output.clear();
}

void Assembler::first_pass()
{
    location_counter = 0;
    line_counter = 1;
    end_reached = false;

    for(std::string line : lines)
    {
        // ignore empty lines
        if(line.empty())
        {
            line_counter++;
            continue;
        }

        line_tokens.clear();
        parser->tokenize(line, line_tokens); 
        token_counter = 0;

        // just a comment, go to next line
        if(line_tokens.size() == 0){
            line_counter++;
            continue;
        }

        label_handler();
        // reached eol
        if(token_counter == line_tokens.size())
        {
            line_counter++;
            continue;
        }

        directive_handler();
        // reached eol
        if(token_counter == line_tokens.size() || end_reached)
        {
            line_counter++;
            continue;
        }

        instruction_handler();

        line_counter++;

        if(end_reached)
            break;
    }
}

void Assembler::label_handler()
{
    // no label in this line
    if(line_tokens.at(token_counter).back() != ':')
        return;

    std::string label = line_tokens.at(token_counter);
    // remove ':'
    label.erase(std::remove(label.begin(), label.end(), ':'), label.end());

    if(find_symbol(label) != nullptr)
    {
        std::cout << "ERROR in line " << line_counter << ", symbol " << label << " already defined" << std::endl;
        exit(1);
    }

    symbol_table.push_back(new Symbol(label, current_section, location_counter, 'l', symbol_table.size()));
    token_counter++;
}

void Assembler::directive_handler()
{
    // no directive in this line
    if(line_tokens.at(token_counter).at(0) != '.')
        return;

    std::string directive = line_tokens.at(token_counter);

    // mneumonic read, go next token
    token_counter++;
    
    if(directive == GLOBAL_DIRECTIVE)
    {
        global_handler_fp();
    }
    else if(directive == EXTERN_DIRECTIVE)
    {
        extern_handler_fp();
    }
    else if(directive == SECTION_DIRECTIVE)
    {
        section_handler_fp();
    }
    else if(directive == WORD_DIRECTIVE)
    {
        word_handler_fp();
    }
    else if(directive == SKIP_DIRECTIVE)
    {
        skip_handler_fp();
    }
    else if(directive == EQU_DIRECTIVE){
        equ_handler_fp();
    }
    else if(directive == END_DIRECTIVE){
        end_handler_fp();
    }
    else
    {
        std::cout << "ERROR in line " << line_counter << ", unkown directive: " << directive << std::endl;
        exit(1);
    }
}

void Assembler::extern_handler_fp()
{

    if(token_counter == line_tokens.size())
    {
        std::cout << "ERROR in line " << line_counter << ", no symbol list found" << std::endl;
        exit(1);
    }

    // add the extern symbols to symbol table
    while(token_counter < line_tokens.size())
    {
        // offset should be 0, section undefined
        symbol_table.push_back(new Symbol(line_tokens.at(token_counter), UNDEFINED_SECTION, 0, 'g', symbol_table.size()));
        token_counter++;
    }
}

void Assembler::section_handler_fp(){
    if(line_tokens.size() - token_counter != 1)
    {
        std::cout << "ERROR in line " << line_counter << ", junk after section name detected" << std::endl;
        exit(1);
    }

    // update current section and remove '.'
    current_section = line_tokens.at(token_counter);
    current_section.erase(std::remove(current_section.begin(), current_section.end(), '.'), current_section.end());
    
    // reset lc on section change
    location_counter = 0;

    symbol_table.push_back(new Symbol(current_section, current_section, location_counter, 'l', symbol_table.size()));

    // section name read, go next token
    token_counter++;
}

void Assembler::word_handler_fp()
{
    if(token_counter == line_tokens.size())
    {
        std::cout << "ERROR in line " << line_counter << ", no word symbol list detected" << std::endl;
        exit(1);
    }

    // each initializer is a literal or a symbol
    while(token_counter < line_tokens.size())
    {
        const std::string& token = line_tokens.at(token_counter);
        if(!std::regex_match(token, LITERAL_REGEX) && !std::regex_match(token, SYMBOL_REGEX))
        {
            std::cout << "ERROR in line " << line_counter << ",  " << token << " is not a symbol or literal" << std::endl;
            exit(1);
        }
        token_counter++;
        location_counter += 2;
    }
}

void Assembler::skip_handler_fp()
{
    if(token_counter == line_tokens.size())
    {
        std::cout << "ERROR in line " << line_counter << ", no literal detected after skip" << std::endl;
        exit(1);
    }

    std::string temp_token = line_tokens.at(token_counter);
    int offset;
    
    // check for hex value
    offset = literal_to_number(temp_token);
    if(offset < 0)
    {
        std::cout << "ERROR in line " << line_counter << ", .skip size must not be negative" << std::endl;
        exit(1);
    }

    // update lc by the number of bytes skipped
    location_counter += offset;

    token_counter++;
}

void Assembler::equ_handler_fp()
{
    if(line_tokens.size() - token_counter != 2)
    {
        std::cout << "ERROR in line " << line_counter << ", .equ directive syntax error" << std::endl;
        exit(1);
    }

    // .equ symbol_name, symbol_value
    std::string symbol_label = line_tokens.at(token_counter);
    token_counter++;

    std::string symbol_literal = line_tokens.at(token_counter);
    int offset;

    // convert literal to integer
    offset = literal_to_number(symbol_literal);

    symbol_table.push_back(new Symbol(symbol_label, ABSOLUTE_SECTION, offset, 'l', symbol_table.size()));

    token_counter++;
}

void Assembler::end_handler_fp()
{
    //token_counter++;
    end_reached = true;
}

void Assembler::global_handler_fp()
{
    while(token_counter < line_tokens.size()){
        if(!std::regex_match(line_tokens.at(token_counter), SYMBOL_REGEX))
        {
            std::cout << "ERROR in line" << line_counter << ", syntax error";
            exit(1);
        }
        token_counter++;
    }
}

void Assembler::instruction_handler()
{
    std::string instruction_mneumonic = line_tokens.at(token_counter);

    if(
        instruction_mneumonic == HALT_MNE ||
        instruction_mneumonic == IRET_MNE ||
        instruction_mneumonic == RET_MNE
    )
    {
        // this is a one word instruction, more words => syntax error
        if(line_tokens.size() - token_counter != 1)
        {
            std::cout << "ERROR IN LINE " << line_counter << ", junk after " << instruction_mneumonic << std::endl;
            exit(1);
        }

        location_counter++;
        token_counter++;

        return;
    }
    else if(
        //xchg, add, sub, mul, div, cmp, and, or, xor, test, shl, shr
        instruction_mneumonic == XCHG_MNE ||
        instruction_mneumonic == ADD_MNE ||
        instruction_mneumonic == SUB_MNE ||
        instruction_mneumonic == MUL_MNE ||
        instruction_mneumonic == DIV_MNE ||
        instruction_mneumonic == CMP_MNE ||
        instruction_mneumonic == AND_MNE ||
        instruction_mneumonic == OR_MNE ||
        instruction_mneumonic == XOR_MNE ||
        instruction_mneumonic == TEST_MNE ||
        instruction_mneumonic == SHL_MNE ||
        instruction_mneumonic == SHR_MNE 
    )
    {
        token_counter++;

        // there should be 2 operands, regD and regS
        if(line_tokens.size() - token_counter != 2)
        {
            std::cout << "ERROR IN LINE " << line_counter << " incorrect operand format" << instruction_mneumonic << std::endl;
            exit(1);
        }

                    //std::regex reg("^r[0-5]$");
        // check first operand
        if(!std::regex_match(line_tokens.at(token_counter++), REGISTER_INSTRUCTION_REGEX))
        {
            std::cout << "ERROR IN LINE " << line_counter << ", must use registers, first operand" << std::endl;
            exit(1);
        }

                        //std::regex reg2("^r[0-7]$");
        // check second operand
        if(!std::regex_match(line_tokens.at(token_counter++), REGISTER_INSTRUCTION_REGEX))
        {
            std::cout << "ERROR IN LINE " << line_counter << " must use register, second operand" << std::endl;
            exit(1);
        }

        location_counter += 2;
        // proveri format operanada
    }
    else if(instruction_mneumonic == NOT_MNE || instruction_mneumonic == INT_MNE)
    {
        token_counter++;

        if(line_tokens.size() - token_counter != 1)
        {
            std::cout << "ERROR IN LINE " << line_counter << ", incorrect operand format" << line_tokens.at(token_counter) << std::endl;
            exit(1);
        }

        if(!std::regex_match(line_tokens.at(token_counter++), REGISTER_INSTRUCTION_REGEX))
        {
            std::cout << "ERROR IN LINE " << line_counter << ", registers must be used" << std::endl;
            exit(1);
        }

        location_counter += 2;
    }
    else if(
        //jmp, jeq, jne, jge
        instruction_mneumonic == JMP_MNE ||
        instruction_mneumonic == JEQ_MNE ||
        instruction_mneumonic == JNE_MNE ||
        instruction_mneumonic == JGT_MNE ||
        instruction_mneumonic == CALL_MNE
    )
    {
        token_counter++;
        Operand op = parse_operand(read_operand(), true);
        location_counter += op.size;
    }
    else if
    (
        // load store
        instruction_mneumonic == LDR_MNE ||
        instruction_mneumonic == STR_MNE
    )
    {
        token_counter++;

        if(token_counter == line_tokens.size() || !std::regex_match(line_tokens.at(token_counter), REGISTER_INSTRUCTION_REGEX))
        {
            std::cout << "ERROR IN LINE " << line_counter << ", first operand must be a register" << std::endl;
            exit(1);
        }
        token_counter++;

        Operand op = parse_operand(read_operand(), false);
        if(instruction_mneumonic == STR_MNE && op.immediate)
        {
            std::cout << "ERROR in line " << line_counter << ", cannot store to immediate value" << std::endl;
            exit(1);
        }
        location_counter += op.size;
    }

    else if(instruction_mneumonic == PUSH_MNE || instruction_mneumonic == POP_MNE)
    {
        token_counter++;
        if(!std::regex_match(line_tokens.at(token_counter), REGISTER_INSTRUCTION_REGEX))
        {
            std::cout << "ERROR IN LINE " << line_counter << ", a register must be used " <<std::endl;
            exit(1); 
        }
        location_counter += 3;
    }
    else
    {
        std::cout << "ERROR IN LINE " << line_counter << "unknown instruction: " <<  instruction_mneumonic <<std::endl;
        exit(1);     
    }
}

void Assembler::second_pass()
{
    // reset all the flags
    location_counter = 0;
    line_counter = 1;
    end_reached = false;
    current_section = "BLANK";

    for(std::string line : lines)
    {
        // ignore empty lines
        if(line.empty())
        {
            line_counter++;
            continue;
        }

        line_tokens.clear();
        parser->tokenize(line, line_tokens);
        token_counter = 0;

        // just a comment line, go next
        if(line_tokens.size() == 0){
            line_counter++;
            continue;
        }

        // ignore labels
        if(line_tokens.at(token_counter).back() == ':'){
            token_counter++;
        }

        // reached eol
        if(token_counter == line_tokens.size())
        {
            line_counter++;
            continue;
        }

        directive_handler_sp();

        // reached eol
        if(token_counter == line_tokens.size() || end_reached)
        {
            line_counter++;
            continue;
        }

        instruction_handler_sp();

        line_counter++;
        if(end_reached)
            break;
    }
    close_section();
    // prints all the data into an output file
    print_data();
}

void Assembler::directive_handler_sp()
{
    // not a directive
    if(line_tokens.at(token_counter).at(0) != '.')
        return;

    // consume directive
    std::string directive = line_tokens.at(token_counter);
    token_counter++;

    if(directive == GLOBAL_DIRECTIVE)
    {
        global_handler_sp();
    }
    else if(directive == SECTION_DIRECTIVE)
    {
        section_handler_sp();
    }
    else if(directive == WORD_DIRECTIVE)
    {
        word_handler_sp();
    }
    else if(directive == SKIP_DIRECTIVE)
    {
        skip_handler_sp();
    }
    else if(directive == EXTERN_DIRECTIVE)
    {
        // skip line
        while(token_counter < line_tokens.size())
            token_counter++;
    }
    else if(directive == EQU_DIRECTIVE)
    {
        // skip line
        while(token_counter < line_tokens.size())
            token_counter++;
    }
    else if(directive == END_DIRECTIVE)
    {
        end_handler_fp();
    }
}

void Assembler::global_handler_sp()
{
    Symbol* s = find_symbol(line_tokens.at(token_counter));
    if(s == nullptr)
    {
        std::cout << "ERROR in line " << line_counter << ", symbol undefined" << std::endl;
        exit(1);
    }
    // change symbol scope to global
    s->scope = 'g';
    token_counter++;
}

Symbol* Assembler::find_symbol(std::string label)
{
    for(Symbol* s : symbol_table){
        if(s->label == label)
            return s; 
    }
    return nullptr;
}

void Assembler::close_section()
{
    if(current_section == "BLANK" && output.empty())
        return;

    sections.push_back({current_section, location_counter, output});
    output.clear();
}

void Assembler::section_handler_sp()
{
    close_section();

    // update current section, remove '.', reset lc
    current_section = line_tokens.at(token_counter);

    current_section.erase(std::remove(current_section.begin(), current_section.end(), '.'), current_section.end());

    location_counter = 0;
    token_counter++;
}

void Assembler::word_handler_sp()
{
    while(token_counter < line_tokens.size())
    {
        const std::string& token = line_tokens.at(token_counter);
        int val;

        if(std::regex_match(token, LITERAL_REGEX))
            val = literal_to_number(token);
        else
        {
            Symbol* tmp = find_symbol(token);
            if(tmp == nullptr)
            {
                std::cout << "ERROR in line " << line_counter << ", symbol " << token << " undefined" << std::endl;
                exit(1);
            }
            // dodaj u tabelu relokacija, ako nije equ koriscen!
            if(tmp->section != ABSOLUTE_SECTION)
                relocation_table.push_back(new RelRecord(location_counter, RELOCATION_ABSOLUTE, tmp->index, current_section));
            val = tmp->offset;
        }

        output.push_back(write_hex(((val & 0xFF) << 8) | ((val >> 8) & 0xFF), 4)); // low byte first
        location_counter += 2;
        token_counter++;
    }
}

// returns a number in hex format, ex. 16,4 => 00 10
std::string Assembler::write_hex(int val, int num_of_nibbles)
{
    int shift_val = (num_of_nibbles - 1) * 4;
    std::string out = "";
    int nibble_counter = 0;
    while(shift_val != -4)
    {
        std::stringstream ss;
        ss << std::hex << ((val >> shift_val) & 0xf);
        out += ss.str();
        shift_val -= 4;
        nibble_counter++;
        if(nibble_counter % 2 == 0)
            out += " ";
    }
    // to upper case
    transform(out.begin(), out.end(), out.begin(), ::toupper);
    
    return out;
}

void Assembler::skip_handler_sp()
{
    int val = literal_to_number(line_tokens.at(token_counter));

    // each byte has two nibbles, so val*2
    output.push_back(write_hex(0, val*2));

    token_counter++;
    location_counter += val;
}


void Assembler::instruction_handler_sp()
{
    std::string instruction_mneumonic = line_tokens.at(token_counter);
    if(instruction_mneumonic == HALT_MNE)
    {
        output.push_back("00");
        token_counter++;
        location_counter++;
    }
    else if(instruction_mneumonic == IRET_MNE)
    {
        token_counter++;
        output.push_back("20");
        location_counter++;
    }
    else if(instruction_mneumonic == RET_MNE)
    {
        token_counter++;
        output.push_back("40");
        location_counter++;
    }
    else if(instruction_mneumonic == INT_MNE)
    {
        token_counter++;
        std::string out = "10 ";
        out += extract_register_num(line_tokens.at(token_counter));
        out += "F";
        location_counter += 2;
        token_counter++;
        output.push_back(out);
    }
    else if(instruction_mneumonic == XCHG_MNE)
    {
        twobyte_handler_sp("60");
    }
    else if(instruction_mneumonic == ADD_MNE)
    {
        twobyte_handler_sp("70");
    }
    else if(instruction_mneumonic == SUB_MNE)
    {
        twobyte_handler_sp("71");
    }
    else if(instruction_mneumonic == MUL_MNE)
    {
        twobyte_handler_sp("72");
    }
    else if(instruction_mneumonic == DIV_MNE)
    {
        twobyte_handler_sp("73");
    }
    else if(instruction_mneumonic == CMP_MNE)
    {
        twobyte_handler_sp("74");
    }
    else if(instruction_mneumonic == NOT_MNE)
    {
        token_counter++;
        std::string out = "80 ";
        out += extract_register_num(line_tokens.at(token_counter));
        out += "0";
        location_counter += 2;
        token_counter++;
        output.push_back(out);
    }
    else if(instruction_mneumonic == AND_MNE)
    {
        twobyte_handler_sp("81");
    }
    else if(instruction_mneumonic == OR_MNE)
    {
        twobyte_handler_sp("82");
    }
    else if(instruction_mneumonic == XOR_MNE)
    {
        twobyte_handler_sp("83");
    }
    else if(instruction_mneumonic == TEST_MNE)
    {
        twobyte_handler_sp("84");
    }
    else if(instruction_mneumonic == SHL_MNE)
    {
        twobyte_handler_sp("90");
    }
    else if(instruction_mneumonic == SHR_MNE)
    {
        twobyte_handler_sp("91");
    }
    else if(instruction_mneumonic == JMP_MNE ||
            instruction_mneumonic == JEQ_MNE ||
            instruction_mneumonic == JNE_MNE ||
            instruction_mneumonic == JGT_MNE ||
            instruction_mneumonic == CALL_MNE
    )
    {
        branch_handler_sp();
    }
    else if(instruction_mneumonic == LDR_MNE ||
            instruction_mneumonic == STR_MNE
    )
    {
        mem_handler_sp();
    }
    else if(
        instruction_mneumonic == PUSH_MNE ||
        instruction_mneumonic == POP_MNE
    )
    {
        stack_handler_sp();
    }
}

// handles instructions sized 2 bytes, needs instruction opcode
void Assembler::twobyte_handler_sp(std::string opcode)
{
    token_counter++;
    std::string out = opcode;
    out += " ";

    // first register
    out += extract_register_num(line_tokens.at(token_counter));
    token_counter++;
    // second register
    out += extract_register_num(line_tokens.at(token_counter));

    token_counter++;
    location_counter += 2;

    output.push_back(out);
}

void Assembler::branch_handler_sp()
{
    std::string opcode;
    if(line_tokens.at(token_counter) == JMP_MNE)
        opcode = "50";
    else if(line_tokens.at(token_counter) == JEQ_MNE)
        opcode = "51";
    else if(line_tokens.at(token_counter) == JNE_MNE)
        opcode = "52";
    else if(line_tokens.at(token_counter) == JGT_MNE)
        opcode = "53";
    else if(line_tokens.at(token_counter) == CALL_MNE)
        opcode = "30";

    token_counter++;

    Operand op = parse_operand(read_operand(), true);
    std::string out = opcode + " F" + encode_operand(op);
    location_counter += op.size;

    output.push_back(out);
}


std::string Assembler::extract_register_num(std::string token)
{
    if(token == "sp")
        return "6";
    if(token == "pc")
        return "7";
    if(token == "psw")
        return "8";
    // format ce biti rX, gde je x neki broj
    return std::string(1, token.at(1));
}


int Assembler::literal_to_number(std::string literal)
{
    long val;
    try
    {
        if(literal.size() > 1 && literal.at(0) == '0' && (literal.at(1) == 'x' || literal.at(1) == 'X'))
            val = std::stol(literal, 0, 16);
        else
            val = std::stol(literal);
    }
    catch(const std::exception&)
    {
        val = 65536;
    }

    if(val < -32768 || val > 65535)
    {
        std::cout << "ERROR in line " << line_counter << ", literal " << literal << " does not fit in 16 bits" << std::endl;
        exit(1);
    }
    return val;
}

std::string Assembler::read_operand()
{
    if(token_counter == line_tokens.size())
    {
        std::cout << "ERROR in line " << line_counter << ", operand missing" << std::endl;
        exit(1);
    }

    std::string text = line_tokens.at(token_counter++);

    // [r1 + x] may be split across tokens
    if(text.find('[') != std::string::npos)
    {
        while(text.back() != ']')
        {
            if(token_counter == line_tokens.size())
            {
                std::cout << "ERROR in line " << line_counter << ", missing ']' in operand" << std::endl;
                exit(1);
            }
            text += line_tokens.at(token_counter++);
        }
    }

    if(token_counter != line_tokens.size())
    {
        std::cout << "ERROR in line " << line_counter << ", junk after operand: " << line_tokens.at(token_counter) << std::endl;
        exit(1);
    }

    return text;
}

Operand Assembler::parse_operand(const std::string& text, bool is_branch)
{
    Operand op;
    op.reg = '0';
    op.literal = 0;
    op.has_symbol = false;
    op.immediate = false;
    op.reloc_type = RELOCATION_ABSOLUTE;

    if(text.at(0) == '%')
    {
        op.symbol = text.substr(1);
        if(!std::regex_match(op.symbol, SYMBOL_REGEX))
        {
            std::cout << "ERROR in line " << line_counter << ", unknown addressing mode: " << text << std::endl;
            exit(1);
        }
        op.has_symbol = true;
        op.reg = '7';
        op.addr_mode = is_branch ? "05" : "03";
        op.size = 5;
        op.reloc_type = RELOCATION_PCREL;
        return op;
    }

    // branches: bare operand is immediate, '*' means memory
    // data: '$' is immediate, bare operand means memory
    std::string body;
    if(is_branch)
    {
        op.immediate = text.at(0) != '*';
        body = op.immediate ? text : text.substr(1);
    }
    else
    {
        op.immediate = text.at(0) == '$';
        body = op.immediate ? text.substr(1) : text;
    }

    bool is_literal = std::regex_match(body, LITERAL_REGEX);
    bool is_symbol = std::regex_match(body, SYMBOL_REGEX);
    std::string payload = body;

    if(op.immediate)
    {
        if(!is_literal && !is_symbol)
        {
            std::cout << "ERROR in line " << line_counter << ", unknown addressing mode: " << text << std::endl;
            exit(1);
        }
        op.addr_mode = "00";
        op.size = 5;
    }
    else if(std::regex_match(body, REGISTER_INSTRUCTION_REGEX))
    {
        op.reg = extract_register_num(body).at(0);
        op.addr_mode = "01";
        op.size = 3;
        return op;
    }
    else if(std::regex_match(body, REGIND_REGEX))
    {
        op.reg = extract_register_num(body.substr(1, body.size() - 2)).at(0);
        op.addr_mode = "02";
        op.size = 3;
        return op;
    }
    else if(std::regex_match(body, REGIND_LITERAL_REGEX) || std::regex_match(body, REGIND_SYMBOL_REGEX))
    {
        op.reg = extract_register_num(body.substr(1, body.find('+') - 1)).at(0);
        op.addr_mode = "03";
        op.size = 5;
        payload = body.substr(body.find('+') + 1);
        payload.pop_back();
    }
    else if(is_literal || is_symbol)
    {
        op.addr_mode = "04";
        op.size = 5;
    }
    else
    {
        std::cout << "ERROR in line " << line_counter << ", unknown addressing mode: " << text << std::endl;
        exit(1);
    }

    if(std::regex_match(payload, LITERAL_REGEX))
        op.literal = literal_to_number(payload);
    else
    {
        op.has_symbol = true;
        op.symbol = payload;
    }

    return op;
}

std::string Assembler::encode_operand(const Operand& op)
{
    std::string out(1, op.reg);
    out += " " + op.addr_mode;
    if(op.size == 3)
        return out;

    int payload = op.literal;
    if(op.has_symbol)
    {
        Symbol* s = find_symbol(op.symbol);
        if(s == nullptr)
        {
            std::cout << "ERROR in line " << line_counter << ", symbol " << op.symbol << " undefined" << std::endl;
            exit(1);
        }
        payload = s->offset;
        if(s->section != ABSOLUTE_SECTION)
            relocation_table.push_back(new RelRecord(location_counter, op.reloc_type, s->index, current_section));
    }

    out += " " + write_hex(payload, 4);
    return out;
}


void Assembler::stack_handler_sp()
{
    std::string instruction_mneumonic = line_tokens.at(token_counter);
    token_counter++;
    std::string out; 
    if(instruction_mneumonic == PUSH_MNE)
    {
        out = "B0 ";
        out += extract_register_num(line_tokens.at(token_counter));
        out += "6 12";
    }
    else
    {
        out = "A0 ";
        out += extract_register_num(line_tokens.at(token_counter));
        out += "6 42";
    }

    location_counter += 3;
    token_counter++;

    output.push_back(out);
}



void Assembler::mem_handler_sp()
{
    std::string opcode;
    if(line_tokens.at(token_counter) == LDR_MNE)
        opcode = "A0";
    else if(line_tokens.at(token_counter) == STR_MNE)
        opcode = "B0";

    token_counter++;

    std::string reg_d = extract_register_num(line_tokens.at(token_counter));
    token_counter++;

    Operand op = parse_operand(read_operand(), false);
    std::string out = opcode + " " + reg_d + encode_operand(op);
    location_counter += op.size;

    output.push_back(out);
}



void Assembler::print_reloc(std::ofstream& outfile)
{
    std::vector<std::string> names;
    std::vector< std::vector<int> > indexes;
    for(int i = 0; i < relocation_table.size(); i++)
    {
        RelRecord* rel = relocation_table.at(i);
        auto iter = std::find(names.begin(), names.end(), rel->section);
        int section_index;
        if (iter != names.end())
        {
            int section_index = iter - names.begin();
            indexes.at(section_index).push_back(i);
        }
        else
        {
            // make a new relocation section
            names.push_back(rel->section);
            std::vector<int> vec;
            vec.push_back(i);
            indexes.push_back(vec);
        }
    }
    
    for(int i = 0; i < names.size(); i++)
    {
        outfile << std::endl;
        outfile << std::endl;
        outfile << "# ------------------ REL." << names.at(i) << " ------------------" << std::endl;
        for(int index : indexes.at(i))
        {
            RelRecord* rel = relocation_table.at(index);
            outfile << rel->offset << " " << rel->type << " " << rel->symbol_number << " " << std::endl;
        }
    }
}

void Assembler::print_object_file(std::ofstream& outfile)
{
    outfile << std::endl;
    outfile << std::endl;
    outfile << "# ------------------ OBJECT FILE ------------------" << std::endl;
    for(const Section& sec : sections)
    {
        outfile << "# ------------------ CODE." << sec.name << " ------------------" << std::endl;
        for(const std::string& s : sec.code)
            outfile << s << std::endl;
    }
}

void Assembler::print_sections(std::ofstream& outfile)
{
    outfile << "# ------------------ SECTION TABLE ------------------" << std::endl;
    outfile << std::setw(15) << "NAME" << std::setw(15) << "SIZE" << std::endl;
    for(const Section& sec : sections)
        outfile << std::setw(15) << sec.name << std::setw(15) << sec.size << std::endl;
    outfile << std::endl;
    outfile << std::endl;
}

void Assembler::print_symtab(std::ofstream& outfile){
    outfile << "# ------------------ SYMBOL TABLE ------------------" << std::endl;
    outfile << std::setw(15) << "LABEL" << std::setw(15) << "SECTION" << std::setw(15) << "OFFSET" << std::setw(15) << "SCOPE" << std::setw(15) << "NUMBER" << std::endl;
    for(Symbol* sym : symbol_table)
    {
        outfile << std::setw(15) << sym->label << std::setw(15) << sym->section << std::setw(15) << sym->offset << std::setw(15) << sym->scope << std::setw(15) << sym->index << std::endl;
    }
}

void Assembler::print_data()
{
    std::ofstream outfile(output_file);

    print_sections(outfile);
    print_symtab(outfile);
    print_reloc(outfile);
    print_object_file(outfile);

    outfile.close();
}

    const std::regex Assembler::LITERAL_REGEX("^((-?[0-9]+)|(0[xX][0-9A-Fa-f]+))$");
    const std::regex Assembler::SYMBOL_REGEX("^[a-zA-Z]\\w*$");

    const std::regex Assembler::REGISTER_INSTRUCTION_REGEX("^(r[0-7]|sp|pc|psw)$");

    // register indirect operand forms, tokens already joined without spaces
    const std::regex Assembler::REGIND_REGEX("^\\[(r[0-7]|sp|pc|psw)\\]$");
    const std::regex Assembler::REGIND_LITERAL_REGEX("^\\[(r[0-7]|sp|pc|psw)\\+((-?[0-9]+)|(0[xX][0-9A-Fa-f]+))\\]$");
    const std::regex Assembler::REGIND_SYMBOL_REGEX("^\\[(r[0-7]|sp|pc|psw)\\+[a-zA-Z]\\w*\\]$");
