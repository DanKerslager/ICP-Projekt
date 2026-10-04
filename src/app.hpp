#pragma once
#include <iostream>
#include <numeric>
#include <vector>
#include <opencv2/opencv.hpp>
//#include <opencv2/xobjdetect.hpp>
#include <opencv2/core/utils/logger.hpp>

class App {
    public:
        App();
        ~App();

        bool init(void);
        int run(void);


    private:
        cv::Point2f find_object_luma(const cv::Mat& frame);
        cv::Point2f find_object_chroma(const cv::Mat& frame);
        cv::Point2f find_object_face(const cv::Mat& frame);
        void draw_cross_normalized(cv::Mat& img, cv::Point2f center_normalized, int size);
        void video_loop(void);
        cv::Ptr<cv::FaceDetectorYN> face_detector = cv::FaceDetectorYN::create("resources/face_detection_yunet_2026may.onnx", "", cv::Size(320, 320));

        cv::VideoCapture capture;
        //cv::CascadeClassifier face_cascade;

        std::thread detector_thread;
        std::atomic<bool> is_running{false};

        std::mutex frame_mutex;
        std::condition_variable frame_condition;
        cv::Mat latest_frame;
        bool new_frame_available{false};

        std::mutex face_mutex;
        cv::Point2f latest_face{0.0f, 0.0f};

        void detector_worker();
        void detect_face_internal(const cv::Mat& frame);
};