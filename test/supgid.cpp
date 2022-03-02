// try setgroups
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

using namespace std;

int main() {
    gid_t grps[32] = {0};
    setgroups(1, grps);
    cout << "setgroups: " << strerror(errno) << endl;
    
    
    auto sz = getgroups(32, grps);
    for (int i = 0; i < sz; ++i) {
        cout << "in group: " << grps[i] << endl;
    }
    return 0;
}