#include <iostream>

void fun(int x) {
    std::cout << x << std::endl;
    fun(x+1);
    return;
}

int main() {
    fun(1);
    return 0;
}