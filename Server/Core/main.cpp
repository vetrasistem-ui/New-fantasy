#include <iostream>
#include <string_view>

#ifndef FANTASY_SERVER_VERSION
#define FANTASY_SERVER_VERSION "dev"
#endif

namespace fantasy {

int runSmokeTest() {
    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=READY\n";
    std::cout << "smoke=PASS\n";
    return 0;
}

int runServer() {
    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=STARTING\n";
    std::cout << "state=READY\n";
    std::cout << "F00 skeleton only: networking/world runtime not implemented yet.\n";
    std::cout << "state=STOPPED\n";
    return 0;
}

} // namespace fantasy

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view{argv[1]} == "--smoke-test") {
        return fantasy::runSmokeTest();
    }

    return fantasy::runServer();
}
