#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace std;

int main() {
    cout << "uid: " << getuid() << endl;
    setuid(0);
    cout << "sid: " << strerror(errno) << endl;
    cout << "uid: " << getuid() << endl;

    ifstream ifs("/etc/shadow");
    copy(istream_iterator<char>(ifs), istream_iterator<char>{},
         ostream_iterator<char>{std::cout});

    return 0;
}