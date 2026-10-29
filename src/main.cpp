#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>

#include "App.h"

int main()
{
    // Disable OpenCV info messages = display only warning and more severe
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    try {
        App app;
        if (!app.init())
            return EXIT_FAILURE;
        return app.run();
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
