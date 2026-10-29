#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <numeric>
#include <vector>
#include <array>
#include <thread>

#include <opencv2/opencv.hpp>

#include "App.h"

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

bool App::init()
{
    try {
        capture = cv::VideoCapture(0, cv::CAP_ANY);
        if (!capture.isOpened())
            throw std::runtime_error("Can not open camera :(");

        std::cout << "Cam opened successfully.\n";

        // create windows once, not every frame
        cv::namedWindow("original");
        cv::namedWindow("decoded");
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}

// milliseconds elapsed since t0
static double ms_since(std::chrono::steady_clock::time_point t0)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

std::vector<uchar> App::lossy_bw_limit(cv::Mat& input_img, size_t size_limit, int& quality)
{
    std::string suff(".jpg"); // target format
    if (!cv::haveImageWriter(suff))
        throw std::runtime_error("Can not compress to format:" + suff);

    std::vector<uchar> bytes;
    std::vector<int> compression_params;

    // prepare parameters for JPEG compressor
    // we use only quality, but other parameters are available (progressive, optimization...)
    std::vector<int> compression_params_template;
    compression_params_template.push_back(cv::IMWRITE_JPEG_QUALITY); 

    std::cout << '[';

    //try step-by-step to decrease quality by 5%, until it fits into limit
    for (auto i = 100; i > 0; i -= 5) {
        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(i);                  // set desired quality
        quality = i;
        std::cout << i << ':';

        // try to encode, measure time of single compression
        auto t0 = std::chrono::steady_clock::now();
        cv::imencode(suff, input_img, bytes, compression_params);
        std::cout << ms_since(t0) << "ms,";

        // check the size limit
        if (bytes.size() <= size_limit)
            break; // ok, done 
    }
    std::cout << "]\n";

    return bytes;
}

int App::run(void)
{
    cv::Mat frame;
    std::vector<uchar> bytes;
    float bandwidth_mbps = 10.0f; // simulated network speed limit
    double camera_fps = capture.get(cv::CAP_PROP_FPS);
    if (camera_fps <= 0.0) camera_fps = 30.0; // some backends report 0 = unknown
    auto t_frame = std::chrono::steady_clock::now();
    double fps = camera_fps; // measured, smoothed
    try {
        while (capture.isOpened())
        {
            capture >> frame;

            if (frame.empty()) {
                std::cerr << "device closed (or video at the end)" << '\n';
                capture.release();
                break;
            }

            // encode image with bandwidth limit
            auto size_uncompressed = frame.elemSize() * frame.total();
            auto size_compressed_limit = bandwidth_mbps * 1'000'000.0 / 8.0 / camera_fps; // Mbit/s -> bytes per frame

            //
            // Encode single image with limitation by bandwidth (byte budget for single frame)
            //
            int quality = 0;
            auto t_enc = std::chrono::steady_clock::now();
            bytes = lossy_bw_limit(frame, size_compressed_limit, quality); // returns JPG compressed stream for single image
            double enc_ms = ms_since(t_enc); // all attempts together
            
            // display compression ratio
            auto size_compreessed = bytes.size();
            std::cout << "Size: uncompressed = " << size_uncompressed << ", compressed = " << size_compreessed << ", = " << size_compreessed / (size_uncompressed / 100.0) << " % \n";

            //
            // decode and display compressed and original data
            //  
            auto t0 = std::chrono::steady_clock::now();
            cv::Mat decoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);
            double dec_ms = ms_since(t0);
            std::cout << "Decode: " << dec_ms << " ms\n";

            // overlay with stats (red = does not fit even with lowest quality)
            fps = 0.9 * fps + 0.1 * 1000.0 / ms_since(t_frame);
            t_frame = std::chrono::steady_clock::now();
            auto color = bytes.size() > size_compressed_limit ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
            cv::putText(decoded_frame, std::format("limit {:.2f} Mbit/s  Q {}  rate {:.2f} Mbit/s", bandwidth_mbps, quality, bytes.size() * 8.0 * camera_fps / 1'000'000.0),
                cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
            cv::putText(decoded_frame, std::format("enc {:.1f} ms  dec {:.1f} ms  {:.1f} FPS", enc_ms, dec_ms, fps),
                cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);

            cv::imshow("original", frame);
            cv::imshow("decoded", decoded_frame);

            // key handling
            int c = cv::pollKey();
            switch (c) {
            case 27:
                return EXIT_SUCCESS;
                break;
            case 'q':
                bandwidth_mbps *= 1.25f;
                break;
            case 'a':
                bandwidth_mbps /= 1.25f;
                break;
            default:
                break;
            }

            bandwidth_mbps = std::clamp(bandwidth_mbps, 0.1f, 1000.0f);
            std::cout << "Bandwidth limit: " << bandwidth_mbps << " Mbit/s\n";
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

App::~App()
{
    // clean-up
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}
