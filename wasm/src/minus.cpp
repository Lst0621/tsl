#include "minus.h"
#include <iostream>

int minus(int a, int b) {
    std::cout<<"minus using wasm "<<a<<" - "<<b<<std::endl;
    return a - b;
}
