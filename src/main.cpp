#include <cstdlib>
#include <iostream>
#include <string>

#include "App.h"
#include "tracking/TrackingViewer.h"

int main(int argc, char** argv)
{
    // icp --track [video]  -> live tracking view (no video = camera)
    // icp --bench video    -> same, but video read as fast as possible: max FPS and cost per stage
    std::string mode = argc >= 2 ? argv[1] : "";
    std::string path = argc >= 3 ? argv[2] : "";
    if (mode == "--track")
        return run_tracking_viewer(path);
    if (mode == "--bench")
        return run_tracking_viewer(path, false);

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
