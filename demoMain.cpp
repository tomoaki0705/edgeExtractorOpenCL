#include <CL/cl.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "measureRecord.h"

#define KEY_HELP      "help"
#define KEY_DEVICE    "device"
#define KEY_HEADLESS  "headless"
#define KEY_LOOPCOUNT "loopCount"
#define KEY_TIME      "time"
#define KEY_INPUT     "@input"
#define VALUE_MAX     "max"
#define VALUE_MIN     "min"
#define VALUE_AVE     "ave"
cv::String keys =
    "{" KEY_HELP " h" "|false |help message}"
    "{" KEY_DEVICE    "|cpu   |cpu/gpu}"
    "{" KEY_HEADLESS  "|false |don't show window}"
    "{" KEY_LOOPCOUNT "|1000  |Loop count for headless mode}"
    "{" KEY_TIME      "|median|one of median/" VALUE_MAX "/" VALUE_MIN "/" VALUE_AVE "}"
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
    if (imageSource.empty() || parser.get<bool>(KEY_HELP))
    {
        parser.printMessage();
        if (imageSource.empty())
        {
            std::cerr << "Please specify an input image" << std::endl;
            return -1;
        }
        else
        {
            return 0;
        }
    }

    cv::Mat originalImage = cv::imread(imageSource, cv::IMREAD_GRAYSCALE);
    if (originalImage.empty())
    {
        std::cerr << "Failed to load from " << imageSource << std::endl;
        return -2;
    }

    bool loopFlag = true;
    bool gpuFlag = parser.get<cv::String>(KEY_DEVICE).compare("gpu") == 0;
    bool processFlag = parser.get<bool>(KEY_HEADLESS);
    bool showWindow = !processFlag;
    int dDepth = CV_8U;
    int cDx = 1, cDy = 1;
    int frameCount = 0;
    tickCount memoryStart, processStart, processFinish, memoryFinish;
    measureRecord recorder;
    reduceType reduce = parseReduceType(parser);

    while (loopFlag)
    {
        cv::Mat result;
        if (processFlag)
        {
            if (gpuFlag)
            {
                memoryStart = cv::getTickCount();
                cv::UMat uSrc = originalImage.getUMat(cv::USAGE_ALLOCATE_DEVICE_MEMORY), uDst;
                processStart = cv::getTickCount();
                cv::Sobel(uSrc, uDst, dDepth, cDx, cDy);
                processFinish = cv::getTickCount();
                result = uDst.getMat(cv::ACCESS_READ).clone();
                memoryFinish = cv::getTickCount();
            }
            else
            {
                memoryStart = processStart = cv::getTickCount();
                cv::Sobel(originalImage, result, dDepth, cDx, cDy);
                processFinish = memoryFinish = cv::getTickCount();
            }
        }
        else
        {
            memoryStart = cv::getTickCount();
            result = originalImage.clone();
            processStart = processFinish = memoryFinish = cv::getTickCount();
        }
        recorder.addRecord(memoryStart, processStart, processFinish, memoryFinish);
        std::cout << std::setprecision(4) << recorder.getRecord(reduce, MEMORY_UPLOAD) << '\t'
                  << std::setprecision(4) << recorder.getRecord(reduce, PROCESS) << '\t' 
                  << std::setprecision(4) << recorder.getRecord(reduce, MEMORY_DOWNLOAD) << "\t[ms]\r";

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
