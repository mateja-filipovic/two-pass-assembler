#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>


class Parser{
public:

    Parser(std::string _filename);

    // read lines into output vector
    void parse_file(std::vector<std::string>& output);

    // divide strings into tokens, which are separated by whitespace
    void tokenize(std::string input, std::vector<std::string>& output);

    void remove_commas(std::vector<std::string>& input);

private:
    std::string filename;
};

#endif