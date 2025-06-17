#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>    // for system()
#include <algorithm>  // for find_if
#include <filesystem> // for path handling (C++17)

namespace fs = std::filesystem;

bool get_default_opts(fs::path &exe_path, std::vector<std::string> &c_opts, std::vector<std::string> &cpp_opts)
{
    fs::path base_dir = exe_path.parent_path().parent_path();
    fs::path c_conf_dir = base_dir / "conf" / "gcc.conf";
    fs::path cpp_conf_dir = base_dir / "conf" / "g++.conf";

    std::ifstream c_conf(c_conf_dir);
    std::ifstream cpp_conf(cpp_conf_dir);

    if (c_conf.is_open() && cpp_conf.is_open())
    {
        std::string line;
        while (std::getline(c_conf, line))
        {
            c_opts.push_back(line);
        }
        while (std::getline(cpp_conf, line))
        {
            cpp_opts.push_back(line);
        }
        return true;
    }
    else
        return false;
}

// 判断文件是否是 C 源文件
bool is_c_file(const fs::path &file)
{
    const std::string ext = file.extension().string();
    return (ext == ".c");
}

// 判断文件是否是 C++ 源文件
bool is_cpp_file(const fs::path &file)
{
    const std::string ext = file.extension().string();
    return (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".C");
}

// 执行编译命令
int compile(const std::vector<std::string> &args, const std::string &compiler)
{
    std::string cmd = compiler;
    for (const auto &arg : args)
    {
        cmd += " " + arg;
    }

    std::cout << "[INFO] Running: " << cmd << std::endl;
    return system(cmd.c_str());
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " [options] <source-files...>\n";
        return EXIT_FAILURE;
    }

    // 内置的默认选项
    fs::path exe_path(argv[0]);
    exe_path = fs::absolute(exe_path);
    std::vector<std::string> default_c_opts;
    std::vector<std::string> default_cpp_opts;
    if (!get_default_opts(exe_path, default_c_opts, default_cpp_opts))
    {
        std::cerr << "[ERROR] Failed to load default options.\n";
        return EXIT_FAILURE;
    }

    // 收集用户传入的参数（跳过程序名 argv[0]）
    std::vector<std::string> user_args;
    for (int i = 1; i < argc; ++i)
    {
        user_args.emplace_back(argv[i]);
    }

    // 检查文件扩展名，决定使用 gcc 还是 g++
    bool use_gcc = false;
    bool use_gpp = false;

    for (const auto &arg : user_args)
    {
        fs::path file(arg);
        if (fs::is_regular_file(file))
        {
            if (is_c_file(file))
            {
                use_gcc = true;
            }
            else if (is_cpp_file(file))
            {
                use_gpp = true;
            }
        }
    }

    if (use_gcc && use_gpp)
    {
        std::cerr << "[ERROR] Mixed C and C++ files detected. This tool does not support hybrid compilation.\n";
        return EXIT_FAILURE;
    }
    else if (!use_gcc && !use_gpp)
    {
        std::cerr << "[ERROR] No valid C/C++ source files found (supported extensions: .c, .cpp, .cxx, .cc, .C).\n";
        return EXIT_FAILURE;
    }

    // 选择编译器和默认选项
    std::string compiler = use_gcc ? "gcc" : "g++";
    const auto &default_opts = use_gcc ? default_c_opts : default_cpp_opts;

    // 构建完整的编译命令
    std::vector<std::string> full_args;
    full_args.insert(full_args.end(), default_opts.begin(), default_opts.end()); // 内置选项
    full_args.insert(full_args.end(), user_args.begin(), user_args.end());       // 用户选项

    // 执行编译
    int result = compile(full_args, compiler);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
