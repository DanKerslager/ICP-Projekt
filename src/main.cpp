#include "main.hpp"

int main() {

    if (app.init())
        return app.run();
    else {
        std::cerr << "App initialization failed!\n";
        return EXIT_FAILURE;
    }

    /*cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    cv::Mat image = cv::imread("resources/lightbulb.jpg");
    if (image.empty()) {
        std::cerr << "Nepodarilo se nacist resources/lightbulb.jpg!\n";
        return -1;
    }

    cv::Point2f objectPosition = find_object_luma(image);
    std::cout << "Normalizovaná pozice objektu: (" << objectPosition.x << ", " << objectPosition.y << ")\n";

    cv::circle(image, cv::Point(objectPosition.x * image.cols, objectPosition.y * image.rows), 8, cv::Scalar(0, 255, 0), -1);
    cv::imshow("Zarovka", image);
    cv::waitKey(0);
    // Tady budeme volat nase funkce...

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        cap.open("resources/video.mp4"); // cesta k videu, pokud neni webkamera
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        int64 t0 = cv::getTickCount();

        // Pokud je snimek velky, zmensit pro zrychleni
        // cv::resize(frame, frame, cv::Size(640, 360));

        cv::Point2f center = find_object_chroma(frame);
        if (center.x >= 0) {
            cv::circle(frame, cv::Point(center.x * frame.cols, center.y * frame.rows), 8, cv::Scalar(0, 255, 0), -1);
        }

        double timeMs = (cv::getTickCount() - t0) * 1000.0 / cv::getTickFrequency();
        std::cout << "Cas na snimek: " << timeMs << " ms\n";

        cv::imshow("Camera Tracking", frame);
        if (cv::waitKey(1) == 27) break; // Konec po stisku ESC
    }
    return 0;*/
}