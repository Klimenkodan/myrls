#include <iostream>
#include <boost/program_options.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>
#include <boost/program_options.hpp>
#include <pwd.h>
#include <sys/types.h>

namespace po = boost::program_options;

//std::tuple<std::string, struct stat, int> file_data;

std::map<std::string, std::vector<std::tuple<std::string, struct stat, int>>> dir_entries;

static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    printf("%-3s %2d %7jd  %d %s\n",
           (tflag == FTW_DP) ?   "/"   : (tflag == FTW_F) ?
                                     (S_ISBLK(sb->st_mode) ? "f b" :
                                      S_ISCHR(sb->st_mode) ? "f c" :
                                      S_ISFIFO(sb->st_mode) ? "f p" :
                                      S_ISREG(sb->st_mode) ? "f r" :
                                      S_ISSOCK(sb->st_mode) ? "=" : "f ?") :
                                     (tflag == FTW_NS) ?  "ns"  : (tflag == FTW_SL) ?  "sl" :
                                                                  (tflag == FTW_SLN) ? "sln" : "?",
           ftwbuf->level, (intmax_t) sb->st_size,
           sb->st_mode, fpath + ftwbuf->base);
    std::string info = "---------";
    return 0;           /* To tell nftw() to continue */
}

static int keep_fnames(const char *fpath, const struct stat *sb, const int tflag, struct FTW *ftwbuf) {
    std::string file_name = fpath;
    if (tflag == FTW_D) {
        dir_entries[file_name].emplace_back(file_name, *sb, tflag);
    }

    if (ftwbuf -> level == 0 || tflag == FTW_F) {
        dir_entries[file_name.substr(0, file_name.find_last_not_of(basename(file_name.data()))
        - strlen(basename(file_name.data())))].emplace_back(file_name, *sb, tflag);
    }
    return 0;
}

std::string get_user_name(uid_t uid) {
    auto user_info = getpwuid(uid);
    if (user_info == nullptr) {
        std::cerr << "error determining user" <<  std::endl;
        return "";
    }
    return user_info->pw_name;
}

bool inline comparator(const std::tuple<std::string, struct stat, int>& file_1, const std::tuple<std::string, struct stat, int>& file_2) {
    return std::get<0>(file_1).compare(std::get<0>(file_2)) > 0;
}

int main(int argc, char **argv) {
    std::string path;
    const std::string help = "Usage: myrls  [-h|--help] <path>"
                             "recursively output detailed information of the files in the given directory(path)\n"
                             "if no path is given -- displays info about the current directory\n"
                             "if file is given -- displays info about it\n"
                             "if given path doesn't exist -- ends with an error\n"
                             "if more than one path is given -- ends with an error\n"
                             "if at least one of the dirs is not available or file info is not available --\n"
                             "error is added to the stderr and program go on working\n";



    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << help << std::endl;
        return 0;
    }

    if (argc > 2) {
        std::cerr << "more than one path is given";
        return 2;
    }


    if (nftw((argc < 2) ? "." : argv[1], keep_fnames, 20, false | FTW_NS | FTW_MOUNT | FTW_PHYS | FTW_DEPTH) == -1){
        perror("nftw couldn't open file");
        exit(EXIT_FAILURE);
    }
    char link[256];
    std::string time;
    time.reserve(511);
    std::vector<std::string> dir_names;
    dir_names.reserve(dir_entries.size());

    for (auto &dir : dir_entries) {
        dir_names.emplace_back(dir.first);
    }

    std::sort(dir_names.begin(), dir_names.end());

    for (auto& dir_name : dir_names) {
        std::sort(dir_entries[dir_name].begin(), dir_entries[dir_name].end(), comparator);

        std::cout << "\n" << dir_name << ":\n";

        for (auto &file : dir_entries[dir_name]) {
            auto stat = std::get<1>(file);

            strftime(time.data(), 511, "%d.%m.%Y %H:%M:%S", localtime(&stat.st_mtim.tv_sec));
            if (auto tmp = (S_ISLNK(stat.st_mode) ? readlink(std::get<0>(file).c_str(), link, 255) : -1)) {
                link[tmp] = '\0';
            }

            std::cout << ((stat.st_mode & S_IRUSR) ? "r" : "-")
                      << ((stat.st_mode & S_IWUSR) ? "w" : "-")
                      << ((stat.st_mode & S_IXUSR) ? "x" : "-")
                      << ((stat.st_mode & S_IRGRP) ? "r" : "-")
                      << ((stat.st_mode & S_IWGRP) ? "w" : "-")
                      << ((stat.st_mode & S_IXGRP) ? "x" : "-")
                      << ((stat.st_mode & S_IROTH) ? "r" : "-")
                      << ((stat.st_mode & S_IWOTH) ? "w" : "-")
                      << ((stat.st_mode & S_IXOTH) ? "x" : "-")
                      << " " << get_user_name(stat.st_uid) << "\t"
                      << static_cast<intmax_t>(stat.st_size) << "\t"
                      << time.data() << "\t"
                      << (((S_IEXEC & stat.st_mode ||
                          !S_ISREG(stat.st_mode))) ?
                          ((S_ISDIR(stat.st_mode)) ? "/" : \
                          (S_ISLNK(stat.st_mode)) ? "@" : \
                          (S_ISSOCK(stat.st_mode)) ? "=" : \
                          (S_ISFIFO(stat.st_mode)) ? "|" : \
                          (S_IEXEC & stat.st_mode) ? "*" : "?") : "")
                      << basename(std::get<0>(file).data())
                      << ((S_ISLNK(stat.st_mode)) ? " => " : "")
                      << ((S_ISLNK(stat.st_mode)) ? link : "")
                      << std::endl;

        }
    }

    return 0;
}
