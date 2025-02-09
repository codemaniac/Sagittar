#define DOCTEST_CONFIG_IMPLEMENT
#include "board.h"
#include "doctest/doctest.h"

int main(int argc, char** argv) {

    sagittar::board::Board::initialize();

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run();   // run
    if (context.shouldExit())  // important - query flags (and --exit) rely on the user doing this
        return res;            // propagate the result of the tests

    return res;
}
