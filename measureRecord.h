#pragma once
#include <vector>
typedef long long int tickCount;

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
    measureRecord();
    measureRecord(int _sampleCount);
    ~measureRecord();
    void addRecord(tickCount memoryStart, tickCount processStart, tickCount processFinish, tickCount memoryFinish);
    double getRecord(reduceType type, recordType record) const;
    void clear();
private:
    std::vector<tickCount> * getArray(recordType record) const;
    void push_back(tickCount count, recordType record);
    int sampleCount;
    std::vector<tickCount> memoryUpload;
    std::vector<tickCount> process;
    std::vector<tickCount> memoryDownload;
};

