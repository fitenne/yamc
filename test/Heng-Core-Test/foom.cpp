#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
int main(int argc, char *argv[]) {
    int n = std::stoi(argv[argc - 1]);
    std::vector<std::vector<char> *> vecs;
    int pid = fork();
    if (pid == 0) {
        while (n--) {
            vecs.push_back(new std::vector<char>(1024 * 1024, 12));
            // std::cerr << n << std::endl;
        }
    }
    int status;
    wait(&status);
    std::cout << "WIFEXITED: " << WIFEXITED(status) << ","
              << "WEXITSTATUS: " << WEXITSTATUS(status) << ","
              << "WIFSIGNALED: " << WIFSIGNALED(status) << ","
              << "WTERMSIG: " << WTERMSIG(status);
    return 0;
}