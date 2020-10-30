#include <iostream>
#include <boost/program_options.hpp>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>

// -- директорія
//* -- виконавчий,
//@ - symlink, вказувати, на що посилається (приклад: fd -> /proc/self/fd/)
//| -- іменований канал,
//= -- сокет,
//? - всі інші.

//Права доступу -- три тріади rwx, для user, group i other.
//Власник.
//Розмір в байтах.
//Дата останньої модифікації.
//Час останньої модифікації.
//Ім'я файлу, включаючи всі розширення.
//Перед ім'ям директорій потрібно виводити символ /. Перед ім'ям виконавчих файлів -- *. Як помічати інші спец-файли описано далі.
static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    printf("%-3s %2d %7jd  %d %s\n",
           (tflag == FTW_D) ?   "/"   : (tflag == FTW_F) ?
                                     (S_ISBLK(sb->st_mode) ? "f b" :
                                      S_ISCHR(sb->st_mode) ? "f c" :
                                      S_ISFIFO(sb->st_mode) ? "f p" :
                                      S_ISREG(sb->st_mode) ? "f r" :
                                      S_ISSOCK(sb->st_mode) ? "=" : "f ?") :
                                     (tflag == FTW_NS) ?  "ns"  : (tflag == FTW_SL) ?  "sl" :
                                                                  (tflag == FTW_SLN) ? "sln" : "?",
           ftwbuf->level, (intmax_t) sb->st_size,
           ftwbuf->base, fpath + ftwbuf->base);
    return 0;           /* To tell nftw() to continue */
}


int main(int argc, char **argv) {

    int opt;
    int option_index;
    std::string path;
    int flags = 0;

    const std::string help = "Usage: myrls  [-h|--help] <path>"
                             "recursively output detailed information of the files in the given directory(path)\n"
                             "if no path is given -- displays info about the current directory\n"
                             "if file is given -- displays info about it\n"
                             "if given path doesn't exist -- ends with an error\n"
                             "if more than one path is given -- ends with an error\n"
                             "if at least one of the dirs is not available or file info is not available --\n"
                             "error is added to the stderr and program go on working\n";
    static struct option long_options[] = {
            {"help",     no_argument, nullptr,  0 },
            {nullptr, 0, nullptr, 0}
    };


    while((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch(opt) {
            case 0:
                if (long_options[option_index].name == "help") {
                    write(1, help.data(), help.size());
                    exit(0);
                }
                std::cerr << "wrong long option given" << std::endl;
                exit(2);
            case 'h':
                write(1, help.data(), help.size());
                exit(0);
        }
    }
    if (argc > 2) {
        std::cerr << "more than one path is given";
        exit(2);
    }
    if (argc == 2) {
        path = argv[1];
    }

    else {
        path = ".";
    }

    if (argc > 2 && strchr(argv[2], 'd') != nullptr)
        flags |= FTW_DEPTH;
    if (argc > 2 && strchr(argv[2], 'p') != nullptr)
        flags |= FTW_PHYS;

    if (nftw(path.c_str(), display_info, 20, flags) == -1){
        perror("nftw couldn't open file");
//        exit(EXIT_FAILURE);
    }
}
