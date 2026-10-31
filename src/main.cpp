

#include "../inc/parser.h"
#include "../inc/assembler.h"


int main(int argc, char* argv[]){

    std::string input_filename;
    std::string output_filename = "a.out";

    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "-o"){
            if(i + 1 == argc){
                std::cout << "ERROR: -o requires an output file name" << std::endl;
                return 1;
            }
            output_filename = argv[++i];
        }
        else if(!input_filename.empty()){
            std::cout << "ERROR: only one input file can be assembled at a time" << std::endl;
            return 1;
        }
        else
            input_filename = arg;
    }

    if(input_filename.empty()){
        std::cout << "usage: asembler [-o <output_file>] <input_file>" << std::endl;
        return 1;
    }

    Parser* parser = new Parser(input_filename);

    Assembler* as = new Assembler(parser, output_filename);

    as->first_pass();
    as->second_pass();

    delete as;
    delete parser;

    return 0;
}
