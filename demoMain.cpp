#include <CL/cl.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <iostream>
#include "measureRecord.h"

#define KEY_HELP      "help"
#define KEY_DEVICE    "device"
#define KEY_HEADLESS  "headless"
#define KEY_LOOPCOUNT "loopCount"
#define KEY_TIME      "time"
#define KEY_SAMPLE    "sample"
#define KEY_INPUT     "@input"
#define VALUE_MAX     "max"
#define VALUE_MIN     "min"
#define VALUE_AVE     "ave"
#define VALUE_GPU     "gpu"
cv::String keys =
    "{" KEY_HELP " h" "|false |help message}"
    "{" KEY_DEVICE    "|cpu   |cpu/" VALUE_GPU "}"
    "{" KEY_HEADLESS  "|false |don't show window}"
    "{" KEY_LOOPCOUNT "|1000  |Loop count for headless mode}"
    "{" KEY_TIME      "|median|one of median/" VALUE_MAX "/" VALUE_MIN "/" VALUE_AVE "}"
    "{" KEY_SAMPLE    "|10    |number of sample for " KEY_TIME "}"
    "{" KEY_INPUT     "|      |filename}";
cv::String windowName = "output";
const char ESC_KEY = 27;

reduceType parseReduceType(const cv::CommandLineParser& parser)
{
    cv::String reduceString = parser.get<cv::String>(KEY_TIME).toLowerCase();
    if (reduceString.compare(VALUE_MAX) == 0 || reduceString.compare("maximum") == 0)
    {
        return REDUCE_MAX;
    }
    else if (reduceString.compare(VALUE_MIN) == 0 || reduceString.compare("minimum") == 0)
    {
        return REDUCE_MIN;
    }
    else if (reduceString.compare(VALUE_AVE) == 0 || reduceString.compare("average") == 0 || reduceString.compare("mean") == 0)
    {
        return REDUCE_AVERAGE;
    }
    else
    {
        return REDUCE_MEDIAN;
    }
}

int main(int argc, char** argv)
{
    cv::CommandLineParser parser(argc, argv, keys);
    cv::String imageSource = parser.get<cv::String>(KEY_INPUT);
    if (parser.get<bool>(KEY_HELP))
    {
        parser.printMessage();
        return 0;
    }

    cv::Mat originalImage;
    cv::VideoCapture capture(0);
    if (capture.isOpened())
    {
        std::cerr << "camera opened" << std::endl;
        capture.set(cv::CAP_PROP_FRAME_WIDTH,  640);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        std::cerr << "configured frame size" << std::endl;
    }
    else
        std::cerr << "failed to open camera" << std::endl;

    if (capture.isOpened() == false)
    {
        originalImage = cv::imread(parser.get<std::string>(KEY_INPUT));
        std::cerr << "Trying to read from file" << std::endl;
        if (originalImage.empty())
        {
            std::cerr << "Failed to load from " << imageSource << std::endl;
            std::cerr << "No camera device found" << std::endl;
            return -2;
        }
    }
    else
    {
        capture >> originalImage;
        capture >> originalImage;
    }

    bool loopFlag = true;
    bool gpuFlag = parser.get<cv::String>(KEY_DEVICE).compare(VALUE_GPU) == 0;
    bool processFlag = parser.get<bool>(KEY_HEADLESS);
    bool showWindow = !processFlag;
    int dDepth = CV_8U;
    int cDx = 1, cDy = 1;
    int frameCount = 0;
    int cPrecision = 3;
    tickCount memoryStart, processStart, processFinish, memoryFinish;
    measureRecord recorder(parser.get<int>(KEY_SAMPLE));
    reduceType reduce = parseReduceType(parser);

    while (loopFlag)
    {
        cv::Mat result;
        if (processFlag)
        {
            cv::ocl::setUseOpenCL(gpuFlag);
            memoryStart = cv::getTickCount();
            cv::UMat uSrc = originalImage.getUMat(gpuFlag ? cv::USAGE_ALLOCATE_DEVICE_MEMORY : cv::USAGE_ALLOCATE_HOST_MEMORY), uDst;
            processStart = cv::getTickCount();
            cv::Laplacian(uSrc, uDst, dDepth);
            processFinish = cv::getTickCount();
            result = uDst.getMat(cv::ACCESS_READ).clone();
            memoryFinish = cv::getTickCount();
        }
        else
        {
            memoryStart = cv::getTickCount();
            result = originalImage.clone();
            processStart = processFinish = memoryFinish = cv::getTickCount();
        }
        recorder.addRecord(memoryStart, processStart, processFinish, memoryFinish);
        std::cout << std::setprecision(cPrecision) << recorder.getRecord(reduce, MEMORY_UPLOAD) << '\t'
                  << std::setprecision(cPrecision) << recorder.getRecord(reduce, PROCESS) << '\t' 
                  << std::setprecision(cPrecision) << recorder.getRecord(reduce, MEMORY_DOWNLOAD) << '\t'
                  << std::setprecision(cPrecision) << recorder.getRecord(reduce, TOTAL) << "\t[ms]\r";

        if (capture.isOpened())
        {
            capture >> originalImage;
        }

        if (showWindow)
        {
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
                recorder.clear();
                std::cout << std::endl;
                break;
            case 'g':
            case 'G':
                gpuFlag = true;
                recorder.clear();
                std::cout << std::endl;
                break;
            case 'c':
            case 'C':
                gpuFlag = false;
                recorder.clear();
                std::cout << std::endl;
                break;
            }
        }
        else
        {
            if (parser.get<int>(KEY_LOOPCOUNT) < ++frameCount)
            {
                loopFlag = false;
            }
        }
    }
    std::cout << std::endl;

    cv::destroyAllWindows();
    return 0;
}
