#include "data_reader.h"

// Define the static mutex for thread-safe output
std::mutex DataReader::outputMutex;