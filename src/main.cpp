#include <format>
#include <getopt.h>
#include <iostream>

#include "encoder.hpp"
#include "decoder.hpp"

void print_help(char*, struct option*, size_t);

int32_t main(int32_t argc, char* argv[]) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"compress", required_argument, 0, 'c'},
        {"decompress", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    int32_t opt;
    int32_t option_index;

    while((opt = getopt_long(argc, argv, "hc:d:", long_options, &option_index)) != -1) {
        switch(opt) {
            case 'h':
                print_help(argv[0], long_options, sizeof(long_options) / sizeof(struct option) - 1);
                break;
            case 'c': {
                const auto file_path = std::string(optarg);
                create_compressed_file(file_path);
                break;
            }
            case 'd': {
                const auto file_path = std::string(optarg);
                Decoder decoder(file_path);
                decoder.create_decompressed_file();
                break;
            }
        }
    }
    return 0;
}

void print_help(char* executable_name, struct option* options, size_t options_size) {
    std::cout << std::format("Usage: {} [OPTIONS] INPUT\n\n", executable_name);
    std::cout << "Options:\n";
    for(size_t index = 0; index < options_size; index++) {
        const auto& current_option = options[index];

        std::cout << std::format("\t-{}, --{}", (char) current_option.val, current_option.name);
        if(current_option.has_arg == required_argument) std::cout << " [argument]";

        std::cout << "\n";
    }
}
