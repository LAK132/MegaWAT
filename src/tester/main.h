#include <stdint.h>
#include <iostream>
using std::istream;
#include <fstream>
using std::ifstream;
#include <vector>
using std::vector;
#include <memory>
using std::shared_ptr;
using std::make_shared;
#include <string>
using std::string;
#include <iostream>
using std::cin;
using std::cout;
using std::endl;
#include <tuple>
using std::tuple;
#include <cassert>

#include <GL/gl3w.h>
#include "lak/runtime/mainloop.h"

#ifndef TESTER_MAIN_H
#define TESTER_MAIN_H

struct userData_t
{
    float clearColor[4] = {0.0f, 0.3125f, 0.3125f, 1.0f};
};

#endif