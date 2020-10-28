#include <iostream>
#include <boost/program_options.hpp>
#include <getopt.h>


int main(int argc, char **argv) {

    int opt;
    int option_index;
    const std::string help = "Usage: myrls  [-h|--help] <path>";
    static struct option long_options[] = {
            {"help",     no_argument, nullptr,  0 },
            {nullptr, 0, nullptr, 0}
    };


    while((opt = getopt_long(argc, argv, "hA", long_options, &option_index)) != -1) {
        switch(opt) {
            case 0:
                if (long_options[option_index].name != nullptr) {
                    write(1, help.data(), help.size());
                    exit(0);
                }
                std::cerr << "wrong long option givven" << std::endl;
            case 'h':
                write(1, help.data(), help.size());
                exit(0);
            case 'A':
                hex = true;
                break;

        }
    }
    return EXIT_SUCCESS;
}
