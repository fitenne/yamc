// try setgid(0)
#include <iostream>
#include <cstring>
#include <unistd.h>
using namespace std;


int main() {
    setgid(0);
    cout << "sgid: " << strerror(errno) << endl;
    cout << "gid:  " << getgid() << endl;
    return 0;
}