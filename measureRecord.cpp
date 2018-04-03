#include "measureRecord.h"
#include <opencv2/opencv.hpp>
#include <numeric>

measureRecord::measureRecord()
    :sampleCount(10)
{
}

measureRecord::measureRecord(int _sampleCount)
    :sampleCount(_sampleCount)
{
}

measureRecord::~measureRecord()
{
}

double measureRecord::getRecord(reduceType type, recordType record) const
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
        result = (double)(std::accumulate(v->begin(), v->end(), (tickCount)0)) / v->size();
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

void measureRecord::clear()
{
    std::vector<tickCount> *v;
    v = getArray(MEMORY_UPLOAD);
    v->clear();
    v = getArray(MEMORY_DOWNLOAD);
    v->clear();
    v = getArray(PROCESS);
    v->clear();
}

std::vector<tickCount> * measureRecord::getArray(recordType record) const
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

void measureRecord::push_back(tickCount count, recordType record)
{
    std::vector<tickCount> *v = getArray(record);
    v->push_back(count);
    if (sampleCount < v->size())
    {
        v->erase(v->begin());
    }
}

void measureRecord::addRecord(tickCount memoryStart, tickCount processStart, tickCount processFinish, tickCount memoryFinish)
{
    push_back(processStart - memoryStart, MEMORY_UPLOAD);
    push_back(processFinish - processStart, PROCESS);
    push_back(memoryFinish - processFinish, MEMORY_DOWNLOAD);
}
