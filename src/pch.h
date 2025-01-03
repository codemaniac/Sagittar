#ifndef PCH_H
#define PCH_H

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifdef DEBUG
    #include <cassert>
#endif
#ifdef TEST
    #include <array>
    #include <filesystem>
    #include <fstream>
    #include <map>
    #include <regex>
#endif

#endif