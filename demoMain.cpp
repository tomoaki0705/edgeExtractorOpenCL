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
    if (originalImage.empty())
    {
        std::cerr << "Failed to load from " << parser.get<cv::String>("@input") << std::endl;
        return -2;
    }

    bool loopFlag = true;
    bool gpuFlag = false;
    bool processFlag = false;
    int dDepth = CV_8U;
    int cDx = 1, cDy = 1;

    while (loopFlag)
    {
        cv::Mat result;
        if (processFlag)
        {
            if (gpuFlag)
            {
                cv::UMat uSrc = originalImage.getUMat(cv::USAGE_ALLOCATE_DEVICE_MEMORY), uDst;
                cv::Sobel(uSrc, uDst, dDepth, cDx, cDy);
                result = uDst.getMat(cv::ACCESS_READ).clone();
            }
            else
            {
                cv::Sobel(originalImage, result, dDepth, cDx, cDy);
            }
        }
        else
        {
            result = originalImage.clone();
        }

        cv::imshow(windowName, result);

        char c = cv::waitKey(1);
        switch (c)
        {
        case 'q':
        case 'Q':
        case ESC_KEY:
            loopFlag = false;
            break;
        case ' ':
            processFlag = !processFlag;
            break;
        case 'g':
        case 'G':
            gpuFlag = true;
            break;
        case 'c':
        case 'C':
            gpuFlag = false;
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
