#include <cstdlib>
#include <iostream>
#include <string>

#include "App.h"
#include "tracking/TrackingViewer.h"

int main(int argc, char** argv)
{
    // icp --track [video]  -> live tracking view (no video = camera)
    if (argc >= 2 && std::string(argv[1]) == "--track")
        return run_tracking_viewer(argc >= 3 ? argv[2] : "");

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
