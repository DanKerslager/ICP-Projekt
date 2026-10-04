#include "app.hpp"

App::App() {}

bool App::init(void) {
    try{
        std::cout << "Hello\nApp initialization...\n";
        /*if (!face_cascade.load(cascade_path)) {
            throw std::runtime_error("Nepodarilo se nacist resources/haarcascade_frontalface_default.xml!");
        }*/
    } catch (const std::exception& e) {
        std::cerr << "Chyba pri inicializaci: " << e.what() << std::endl;
        throw; // Rethrow the exception to indicate failure
    }
    return true;
}

int App::run(void) {
    try {
        /*cv::Mat frame = cv::imread("resources/lightbulb.jpg");
        if (frame.empty()) 
            throw std::runtime_error("Nepodarilo se nacist resources/lightbulb.jpg!");

        auto start = std::chrono::steady_clock::now();
        
        cv::Point2f objectPosition = find_object_luma(frame);
        std::cout << "Normalizovaná pozice objektu: (" << objectPosition.x << ", " << objectPosition.y << ")\n";
        //cv::circle(frame, cv::Point(static_cast<int>(objectPosition.x * frame.cols), static_cast<int>(objectPosition.y * frame.rows)), 10, cv::Scalar(0, 255, 0), -1);
        this->draw_cross_normalized(frame, objectPosition, 25);
        cv::imshow("Zarovka", frame);
        cv::waitKey(0);*/

        capture = cv::VideoCapture(0);
        if (!capture.isOpened()) {
            capture.open("resources/video.mkv"); // cesta k videu, pokud neni webkamera
        }
        int width  = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

        cv::Size frame_size(width, height);
        face_detector->setInputSize(frame_size);
        video_loop();
        
    } catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
    
}

App::~App() {
    cv::destroyAllWindows();
    if (capture.isOpened()) {
        capture.release();
    }
    std::cout << "Bye...!\n";
}

cv::Point2f App::find_object_luma(const cv::Mat& frame) {
    double sumX = 0.0, sumY = 0.0;
    int count = 0;
    const uchar threshold = 230; // Hodnotu otestuj a dolad

    for (int y = 0; y < frame.rows; ++y) {
        const uchar* row = frame.ptr<uchar>(y);
        for (int x = 0; x < frame.cols; ++x) {
            uchar b = row[x * 3 + 0];
            uchar g = row[x * 3 + 1];
            uchar r = row[x * 3 + 2];

            // Y podle vzorce JPEG: 0.299*R + 0.587*G + 0.114*B
            uchar luma = static_cast<uchar>(0.299 * r + 0.587 * g + 0.114 * b);

            if (luma >= threshold) {
                sumX += x;
                sumY += y;
                count++;
            }
        }
    }

    if (count == 0) return cv::Point2f(-1.0f, -1.0f);

    // Vrat normalizovane souradnice [0.0 az 1.0]
    return cv::Point2f((sumX / count) / frame.cols, (sumY / count) / frame.rows);
}

cv::Point2f App::find_object_chroma(const cv::Mat& frame) {
    cv::Mat hsv, mask1, mask2, mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // Cervena barva ma v HSV dve casti spektra (kolem 0 a kolem 180)
    cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(10, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(165, 100, 100), cv::Scalar(180, 255, 255), mask2);
    cv::bitwise_or(mask1, mask2, mask);

    // Volitelne vycisteni sumu
    cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 1);
    cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

    std::vector<cv::Point> whitePixels;
    cv::findNonZero(mask, whitePixels);

    if (whitePixels.empty()) return cv::Point2f(-1.0f, -1.0f);

    cv::Point2f whiteAccum = std::reduce(whitePixels.begin(), whitePixels.end());
    cv::Point2f centroidAbs = whiteAccum / static_cast<float>(whitePixels.size());

    return cv::Point2f(centroidAbs.x / frame.cols, centroidAbs.y / frame.rows);
}

void App::draw_cross_normalized(cv::Mat& img, cv::Point2f center_normalized, int size)
{
    center_normalized.x = std::clamp(center_normalized.x, 0.0f, 1.0f);
    center_normalized.y = std::clamp(center_normalized.y, 0.0f, 1.0f);
    size = std::clamp(size, 1, std::min(img.cols, img.rows));

    cv::Point2f center_absolute(center_normalized.x * img.cols, center_normalized.y * img.rows);

    cv::Point2f p1(center_absolute.x - size / 2, center_absolute.y);
    cv::Point2f p2(center_absolute.x + size / 2, center_absolute.y);
    cv::Point2f p3(center_absolute.x, center_absolute.y - size / 2);
    cv::Point2f p4(center_absolute.x, center_absolute.y + size / 2);

    cv::line(img, p1, p2, CV_RGB(255, 0, 0), 3);
    cv::line(img, p3, p4, CV_RGB(255, 0, 0), 3);
}

void App::video_loop(void) {
    cv::Mat frame;
    do {
        capture.read(frame);
        if (frame.empty()) break;
        cv::Point2f face = find_object_face(frame);

        cv::Mat scene_cross;
        frame.copyTo(scene_cross);
        draw_cross_normalized(scene_cross, face, 25);
        cv::imshow("Camera Tracking", scene_cross);
    } while (cv::waitKey(1) != 27);
}

cv::Point2f App::find_object_face(const cv::Mat& frame) {
    cv::Point2f face(0.0f,0.0f);

    cv::Mat faces;
    

    cv::Mat small_frame;
    float scale = 200.f/static_cast<float>(frame.cols); // Změňte podle potřeby
    cv::resize(frame, small_frame, cv::Size(), scale, scale);
    face_detector->setInputSize(small_frame.size());
    face_detector->detect(small_frame, faces);

    if (faces.rows > 0) {
        float x = faces.at<float>(0, 0);
        float y = faces.at<float>(0, 1);
        float width = faces.at<float>(0, 2);
        float height = faces.at<float>(0, 3);

        face.x = (x + width / 2.0f) / small_frame.cols;
        face.y = (y + height / 2.0f) / small_frame.rows;
    }
    return face;
}