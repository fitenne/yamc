// try to create a directory and a file

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <filesystem>
#include <cstring>
#include <unistd.h>

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    try {
        filesystem::create_directories("test");
    } catch (...) {
        cout << "failed to create dir";
        return EXIT_FAILURE;
    }

    ofstream ofs("out");
    ofs << "dir.cpp";
    if (!ofs.good()) {
        cout << "failed to write to out";
    }
    return EXIT_SUCCESS;
}
