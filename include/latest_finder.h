#ifndef LATEST_FINDER_H
#define LATEST_FINDER_H

#include "command_base.h"
#include <string>
#include <map>

struct SensorLatest {
    std::string sensorId;
    long long timestamp = 0;
};

class LatestFinder : public CommandBase {
public:
    LatestFinder(int argc, char* argv[]);
    int main();
    static void usage();

private:
    int limitRows; // -n parameter: positive = first n, negative = last n, 0 = all
    std::string outputFormat; // "human" (default), "csv", or "json"
};

#endif // LATEST_FINDER_H
