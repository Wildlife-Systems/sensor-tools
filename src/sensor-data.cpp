#include <iostream>
#include <string>

#include "sensor_data_converter.h"
#include "error_lister.h"
#include "error_summarizer.h"
#include "stats_analyser.h"

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <command> [options]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  convert           Convert JSON or CSV sensor data files to CSV" << std::endl;
    std::cerr << "  list-errors       List error readings in sensor data files" << std::endl;
    std::cerr << "  summarise-errors  Summarise error readings with counts" << std::endl;
    std::cerr << "  stats             Calculate statistics for numeric sensor data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "For command-specific help, use:" << std::endl;
    std::cerr << "  " << progName << " <command> --help" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "convert") {
        try {
            // Create new argv for converter (skip "convert" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            SensorDataConverter converter(newArgc, newArgv);
            converter.convert();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "list-errors") {
        try {
            // Create new argv for error lister (skip "list-errors" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            ErrorLister lister(newArgc, newArgv);
            lister.listErrors();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "summarise-errors") {
        try {
            // Create new argv for error summarizer (skip "summarise-errors" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            ErrorSummarizer summarizer(newArgc, newArgv);
            summarizer.summariseErrors();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "stats") {
        try {
            // Create new argv for stats analyzer (skip "stats" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            StatsAnalyser analyser(newArgc, newArgv);
            analyser.analyze();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "--help" || command == "-h") {
        printUsage(argv[0]);
        return 0;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << std::endl;
        return 1;
    }
}
