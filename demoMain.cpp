#include <CL/cl.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <numeric>

cv::String keys =
    "{h help   |false|help message}"
    "{device   |cpu  |cpu/gpu}"
    "{headless |false|don't show window}"
    "{loopCount|1000 |Loop count for headless mode}"
    "{@input   |     |filename}";
cv::String windowName = "output";
const char ESC_KEY = 27;

typedef int64 tickCount;
enum reduceType
{
    REDUCE_MIN,
    REDUCE_MAX,
    REDUCE_AVERAGE,
    REDUCE_MEDIAN
};
enum recordType
{
    MEMORY_UPLOAD,
    PROCESS,
    MEMORY_DOWNLOAD
};

class measureRecord
{
public:
    measureRecord() :sampleCount(10){};
    measureRecord(int _sampleCount) :sampleCount(_sampleCount){};
    ~measureRecord(){};
    void addRecord(tickCount memoryStart, tickCount processStart, tickCount processFinish, tickCount memoryFinish)
    {
        push_back(processStart - memoryStart, MEMORY_UPLOAD);
        push_back(processFinish - processStart, PROCESS);
        push_back(memoryFinish - processFinish, MEMORY_DOWNLOAD);
    }
    double getRecord(reduceType type, recordType record) const
    {
        double result;
        std::vector<tickCount> *v = getArray(record);
        if (v->size() < sampleCount)
        {
            return 0;
        }
        switch (type)
        {
        case REDUCE_MIN:
            result = (double)*std::min_element(v->begin(), v->end());
            break;
        case REDUCE_MAX:
            result = (double)*std::max_element(v->begin(), v->end());
            break;
        case REDUCE_AVERAGE:
            result = (double)(std::accumulate(v->begin(), v->end(), (int64)0)) / v->size();
            break;
        case REDUCE_MEDIAN:
            {
                std::vector<tickCount> a;
                std::copy(v->begin(), v->end(), back_inserter(a));
                std::sort(a.begin(), a.end());
                result = (double)a[a.size() / 2];
            }
            break;
        }
        return result * 1000. / cv::getTickFrequency();
    };
    void clear()
    {
        std::vector<tickCount> *v;
        v = getArray(MEMORY_UPLOAD);
        v->clear();
        v = getArray(MEMORY_DOWNLOAD);
        v->clear();
        v = getArray(PROCESS);
        v->clear();
    }
private:
    std::vector<tickCount> * getArray(recordType record) const
    {
        std::vector<tickCount> *v;
        switch (record)
        {
        case MEMORY_UPLOAD:
            v = (std::vector<tickCount>*)&memoryUpload;
            break;
        case MEMORY_DOWNLOAD:
            v = (std::vector<tickCount>*)&memoryDownload;
            break;
        case PROCESS:
            v = (std::vector<tickCount>*)&process;
            break;
        }
        return v;
    }
    void push_back(tickCount count, recordType record)
    {
        std::vector<tickCount> *v = getArray(record);
        v->push_back(count);
        if (sampleCount < v->size())
        {
            v->erase(v->begin());
        }
    }
    int sampleCount;
    std::vector<tickCount> memoryUpload;
    std::vector<tickCount> process;
    std::vector<tickCount> memoryDownload;
};


int main(int argc, char** argv)
{
    cv::CommandLineParser parser(argc, argv, keys);

    if (parser.get<cv::String>("@input").empty() || parser.get<bool>("help"))
    {
        parser.printMessage();
        if (parser.get<cv::String>("@input").empty())
        {
            std::cerr << "Please specify an input image" << std::endl;
            return -1;
        }
        else
        {
            return 0;
        }
    }

    cv::Mat originalImage = cv::imread(parser.get<cv::String>("@input"), cv::IMREAD_GRAYSCALE);
    if (originalImage.empty())
    {
        std::cerr << "Failed to load from " << parser.get<cv::String>("@input") << std::endl;
        return -2;
    }

    bool loopFlag = true;
    bool gpuFlag = parser.get<cv::String>("device").compare("gpu") == 0;
    bool processFlag = false;
    bool showWindow = !parser.get<bool>("headless");
    int dDepth = CV_8U;
    int cDx = 1, cDy = 1;
    int frameCount = 0;
    tickCount memoryStart, processStart, processFinish, memoryFinish;
    measureRecord recorder;

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
        std::cout << std::setprecision(16) << recorder.getRecord(REDUCE_MEDIAN, MEMORY_UPLOAD) << '\t'
                  << std::setprecision(16) << recorder.getRecord(REDUCE_MEDIAN, PROCESS) << '\t' 
                  << std::setprecision(16) << recorder.getRecord(REDUCE_MEDIAN, MEMORY_DOWNLOAD) << "[ms]\r";

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
            if (parser.get<int>("loopCount") < ++frameCount)
            {
                loopFlag = false;
            }
        }
    }

    cv::destroyAllWindows();
    return 0;
}
