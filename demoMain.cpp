#include <CL/cl.h>
#include <opencv2/opencv.hpp>
#include <iostream>

cv::String keys =
    "{h|false|help message}"
    "{@input||filename}";
cv::String windowName = "output";
const char ESC_KEY = 27;

int main(int argc, char** argv)
{
    cv::CommandLineParser parser(argc, argv, keys);

    if (parser.get<cv::String>("@input").empty())
    {
        parser.printMessage();
        std::cerr << "Please specify an input image" << std::endl;
        return -1;
    }

    cv::Mat originalImage = cv::imread(parser.get<cv::String>("@input"), cv::IMREAD_GRAYSCALE);

    bool loopFlag = true;
    bool gpuFlag = false;
    bool processFlag = false;

    while (loopFlag)
    {
        cv::imshow(windowName, originalImage);
        char c = cv::waitKey(1);
        if (c == 'q' || c == 'Q' || c == ESC_KEY)
        {
            loopFlag = false;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
