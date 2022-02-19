#include <iostream>
#include <cstdio>

#include <random>
using namespace std;

default_random_engine engine;
uniform_int_distribution<int> dist('a', 'z');

int main() {

    int a = 1;
    for (;;) {
        a = dist(engine);
    }

    return 0;
}