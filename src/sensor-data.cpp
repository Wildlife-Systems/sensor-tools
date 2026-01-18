#include <iostream>
#include <string>
#include <vector>

#include "sensor_data_transformer.h"
#include "error_lister.h"
#include "error_summarizer.h"
#include "stats_analyser.h"
#include "data_counter.h"

#ifndef VERSION
#define VERSION "unknown"
#endif

void printVersion() {
    std::cout << "sensor-data " << VERSION << std::endl;
    std::cout << "Copyright (C) 2026 Ed Baker" << std::endl;
    std::cout << "License: See debian/copyright for details" << std::endl;
}

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <command> [options]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  transform         Transform JSON or CSV sensor data files" << std::endl;
    std::cerr << "  list-rejects      List rejected readings (inverse of transform filters)" << std::endl;
    std::cerr << "  count             Count sensor data readings (with optional filters)" << std::endl;
    std::cerr << "  list-errors       List error readings in sensor data files" << std::endl;
    std::cerr << "  summarise-errors  Summarise error readings with counts" << std::endl;
    std::cerr << "  stats             Calculate statistics for numeric sensor data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --help, -h        Show this help message" << std::endl;
    std::cerr << "  --version, -V     Show version information" << std::endl;
    std::cerr << std::endl;
    std::cerr << "For command-specific help, use:" << std::endl;
    std::cerr << "  " << progName << " <command> --help" << std::endl;
}

// Helper to build argv for subcommands (skips the command name)
std::vector<char*> buildSubcommandArgv(int argc, char* argv[]) {
    int newArgc = argc - 1;
    std::vector<char*> newArgv(newArgc);
    newArgv[0] = argv[0];  // program name
    for (int i = 2; i < argc; ++i) {
        newArgv[i - 1] = argv[i];
    }
    return newArgv;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    // Handle --version and --help before command dispatch
    if (command == "--version" || command == "-V") {
        printVersion();
        return 0;
    }
    
    if (command == "--help" || command == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    
    if (command == "transform") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            SensorDataTransformer transformer(static_cast<int>(newArgv.size()), newArgv.data());
            transformer.transform();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "list-rejects") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            SensorDataTransformer transformer(static_cast<int>(newArgv.size()), newArgv.data(), true);
            transformer.transform();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "count") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            DataCounter counter(static_cast<int>(newArgv.size()), newArgv.data());
            counter.count();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "list-errors") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            ErrorLister lister(static_cast<int>(newArgv.size()), newArgv.data());
            lister.listErrors();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "summarise-errors") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            ErrorSummarizer summarizer(static_cast<int>(newArgv.size()), newArgv.data());
            summarizer.summariseErrors();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "stats") {
        try {
            std::vector<char*> newArgv = buildSubcommandArgv(argc, argv);
            StatsAnalyser analyser(static_cast<int>(newArgv.size()), newArgv.data());
            analyser.analyze();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << std::endl;
        return 1;
    }
}
