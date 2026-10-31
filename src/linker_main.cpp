#include "../inc/linker.h"


int main(int argc, char* argv[]){

    std::vector<std::string> input_files;
    std::string output_filename = "a.out";
    std::map<std::string, uint> places;
    bool hex = false;
    bool linkable = false;

    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "-o"){
            if(i + 1 == argc){
                std::cout << "ERROR: -o requires an output file name" << std::endl;
                return 1;
            }
            output_filename = argv[++i];
        }
        else if(arg == "-hex")
            hex = true;
        else if(arg == "-linkable")
            linkable = true;
        else if(arg.compare(0, 7, "-place=") == 0){
            std::string spec = arg.substr(7);
            size_t at = spec.find('@');
            unsigned long address = 0;
            bool ok = at != std::string::npos && at > 0;
            if(ok){
                try{
                    size_t consumed;
                    address = std::stoul(spec.substr(at + 1), &consumed, 0);
                    ok = consumed == spec.size() - at - 1 && address <= 0xFFFF;
                }
                catch(const std::exception&){
                    ok = false;
                }
            }
            if(!ok){
                std::cout << "ERROR: expected -place=<section>@<address>, got " << arg << std::endl;
                return 1;
            }
            places[spec.substr(0, at)] = address;
        }
        else if(arg.at(0) == '-'){
            std::cout << "ERROR: unknown option " << arg << std::endl;
            return 1;
        }
        else
            input_files.push_back(arg);
    }

    if(hex == linkable){
        std::cout << "ERROR: exactly one of -hex and -linkable must be given" << std::endl;
        return 1;
    }

    if(input_files.empty()){
        std::cout << "usage: linker (-hex | -linkable) [-o <output_file>] [-place=<section>@<address>]... <input_file>..." << std::endl;
        return 1;
    }

    Linker linker(input_files, output_filename, places, hex);
    linker.link();

    return 0;
}
