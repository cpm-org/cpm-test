// The main() of test programs that don't define their own. It is alone in this file so that the linker takes it from
// the library only when nothing else defines main().
#include <cpm/test.hpp>

int main(int argc, char* argv[]) { return cpm::test::main(argc, argv); }
