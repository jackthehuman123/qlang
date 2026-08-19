#include <iostream>

int main() {
    // int unused = 5;
    int* p = new int[10];
    p[10] = 1;
    delete[] p;
    std::cout << "qlang alive\n";
    return 0;
}