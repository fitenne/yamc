#include <iostream>
#include <random>
using namespace std;

default_random_engine engine;
uniform_int_distribution<int> dist('a', 'z');

int main() {
    long long sz = 0;
    const int bs = 1 * 1024 * 1024;
    bool flag = 0;
    while (true) {
        auto buf = new char[bs];
        sz += bs;
        cout << 1.0 * sz / 1024 / 1024 << " MB" << endl;
        if (!flag && sz / 1024 / 1024 > 40) {
            flag = 1;
        }
        if (sz / 1024 / 1024 > 1024) {
            // break anyway
            break;
        }

        for (int i = 0; i < bs; ++i) {
            buf[i] = dist(engine);
        }
    }
    return 0;
}
