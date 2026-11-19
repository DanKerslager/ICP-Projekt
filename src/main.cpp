#include <cstdlib>
#include <iostream>

#include "App.hpp"

int main()
{
    try {
        App app;
        app.init();
        return app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "App failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
