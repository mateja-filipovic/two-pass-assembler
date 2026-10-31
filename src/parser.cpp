#include "../inc/parser.h"

Parser::Parser(std::string _filename) : filename(_filename)
{
}

void Parser::parse_file(std::vector<std::string>& output)
{
    std::ifstream file (this->filename);
    if(file.fail())
    {
        std::cout << "ERROR opening input file: " << filename << std::endl;
        exit(1);
    }
    std::string line;

    while(std::getline(file, line))
    {
            output.push_back(line);
    }
}

void Parser::tokenize(std::string input, std::vector<std::string>& output)
{
    std::stringstream ss(input.substr(0, input.find('#')));
    while(ss >> input)
        output.push_back(input);

    remove_commas(output);
}

void Parser::remove_commas(std::vector<std::string>& input)
{
    for(std::string& token : input){
        token.erase(std::remove(token.begin(), token.end(), ','), token.end());
    }
}