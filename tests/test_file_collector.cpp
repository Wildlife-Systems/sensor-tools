#include "../include/file_collector.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

// Helper to create temp directory structure
class TempTestDir {
public:
    std::string basePath;
    
    TempTestDir() {
        basePath = "test_file_collector_temp";
        mkdir(basePath.c_str(), 0755);
    }
    
    void createFile(const std::string& relativePath) {
        std::string fullPath = basePath + "/" + relativePath;
        std::ofstream f(fullPath);
        f << "test content";
        f.close();
    }
    
    void createDir(const std::string& relativePath) {
        std::string fullPath = basePath + "/" + relativePath;
        mkdir(fullPath.c_str(), 0755);
    }
    
    ~TempTestDir() {
        // Clean up files and directories
        cleanup(basePath);
        rmdir(basePath.c_str());
    }
    
private:
    void cleanup(const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name != "." && name != "..") {
                std::string fullPath = path + "/" + name;
                if (FileUtils::isDirectory(fullPath)) {
                    cleanup(fullPath);
                    rmdir(fullPath.c_str());
                } else {
                    std::remove(fullPath.c_str());
                }
            }
        }
        closedir(dir);
    }
};

void test_add_single_file() {
    // Create a temp file
    std::ofstream f("test_fc_single.txt");
    f << "test";
    f.close();
    
    FileCollector collector;
    collector.addPath("test_fc_single.txt");
    
    auto files = collector.getFiles();
    assert(files.size() == 1);
    assert(files[0] == "test_fc_single.txt");
    
    std::remove("test_fc_single.txt");
    std::cout << "[PASS] test_add_single_file" << std::endl;
}

void test_add_multiple_files() {
    std::ofstream f1("test_fc_1.txt");
    f1 << "test";
    f1.close();
    std::ofstream f2("test_fc_2.txt");
    f2 << "test";
    f2.close();
    
    FileCollector collector;
    collector.addPath("test_fc_1.txt");
    collector.addPath("test_fc_2.txt");
    
    auto files = collector.getFiles();
    assert(files.size() == 2);
    
    std::remove("test_fc_1.txt");
    std::remove("test_fc_2.txt");
    std::cout << "[PASS] test_add_multiple_files" << std::endl;
}

void test_collect_from_directory() {
    TempTestDir dir;
    dir.createFile("file1.out");
    dir.createFile("file2.out");
    dir.createFile("file3.txt");
    
    FileCollector collector;
    collector.addPath(dir.basePath);
    
    auto files = collector.getFiles();
    assert(files.size() == 3);
    
    std::cout << "[PASS] test_collect_from_directory" << std::endl;
}

void test_extension_filter() {
    TempTestDir dir;
    dir.createFile("file1.out");
    dir.createFile("file2.out");
    dir.createFile("file3.txt");
    dir.createFile("file4.csv");
    
    FileCollector collector(false, ".out");
    collector.addPath(dir.basePath);
    
    auto files = collector.getFiles();
    assert(files.size() == 2);
    
    // Verify all files have .out extension
    for (const auto& f : files) {
        assert(f.find(".out") != std::string::npos);
    }
    
    std::cout << "[PASS] test_extension_filter" << std::endl;
}

void test_recursive_collection() {
    TempTestDir dir;
    dir.createFile("root.out");
    dir.createDir("subdir1");
    dir.createFile("subdir1/sub1.out");
    dir.createDir("subdir2");
    dir.createFile("subdir2/sub2.out");
    
    // Non-recursive should only get root file
    FileCollector nonRecursive(false);
    nonRecursive.addPath(dir.basePath);
    assert(nonRecursive.getFiles().size() == 1);
    
    // Recursive should get all files
    FileCollector recursive(true);
    recursive.addPath(dir.basePath);
    assert(recursive.getFiles().size() == 3);
    
    std::cout << "[PASS] test_recursive_collection" << std::endl;
}

void test_max_depth_zero() {
    TempTestDir dir;
    dir.createFile("root.out");
    dir.createDir("subdir");
    dir.createFile("subdir/sub.out");
    
    // maxDepth=0 should only get files at the specified directory level
    FileCollector collector(true, "", 0);
    collector.addPath(dir.basePath);
    
    assert(collector.getFiles().size() == 1);
    
    std::cout << "[PASS] test_max_depth_zero" << std::endl;
}

void test_max_depth_one() {
    TempTestDir dir;
    dir.createFile("root.out");
    dir.createDir("level1");
    dir.createFile("level1/l1.out");
    dir.createDir("level1/level2");
    dir.createFile("level1/level2/l2.out");
    
    // maxDepth=1 should get root and level1, but not level2
    FileCollector collector(true, "", 1);
    collector.addPath(dir.basePath);
    
    auto files = collector.getFiles();
    assert(files.size() == 2);
    
    // Verify level2 file is not included
    for (const auto& f : files) {
        assert(f.find("level2") == std::string::npos);
    }
    
    std::cout << "[PASS] test_max_depth_one" << std::endl;
}

void test_empty_directory() {
    TempTestDir dir;
    // Don't create any files
    
    FileCollector collector;
    collector.addPath(dir.basePath);
    
    assert(collector.getFiles().empty());
    
    std::cout << "[PASS] test_empty_directory" << std::endl;
}

void test_nonexistent_path() {
    FileCollector collector;
    collector.addPath("nonexistent_directory_12345");
    
    // Non-existent paths are added as files (the user may want to handle missing files later)
    // The collector doesn't validate existence, it just collects paths
    auto files = collector.getFiles();
    assert(files.size() == 1);
    assert(files[0] == "nonexistent_directory_12345");
    
    std::cout << "[PASS] test_nonexistent_path" << std::endl;
}

void test_recursive_with_extension_filter() {
    TempTestDir dir;
    dir.createFile("root.out");
    dir.createFile("root.txt");
    dir.createDir("sub");
    dir.createFile("sub/sub.out");
    dir.createFile("sub/sub.csv");
    
    FileCollector collector(true, ".out");
    collector.addPath(dir.basePath);
    
    auto files = collector.getFiles();
    assert(files.size() == 2);
    
    for (const auto& f : files) {
        assert(f.find(".out") != std::string::npos);
    }
    
    std::cout << "[PASS] test_recursive_with_extension_filter" << std::endl;
}

void test_get_files_returns_reference() {
    std::ofstream f("test_fc_ref.txt");
    f << "test";
    f.close();
    
    FileCollector collector;
    collector.addPath("test_fc_ref.txt");
    
    const std::vector<std::string>& files1 = collector.getFiles();
    const std::vector<std::string>& files2 = collector.getFiles();
    
    // Should be the same reference
    assert(&files1 == &files2);
    
    std::remove("test_fc_ref.txt");
    std::cout << "[PASS] test_get_files_returns_reference" << std::endl;
}

int main() {
    std::cout << "================================" << std::endl;
    std::cout << "FileCollector Unit Tests" << std::endl;
    std::cout << "================================" << std::endl;
    
    test_add_single_file();
    test_add_multiple_files();
    test_collect_from_directory();
    test_extension_filter();
    test_recursive_collection();
    test_max_depth_zero();
    test_max_depth_one();
    test_empty_directory();
    test_nonexistent_path();
    test_recursive_with_extension_filter();
    test_get_files_returns_reference();
    
    std::cout << "================================" << std::endl;
    std::cout << "All FileCollector tests passed!" << std::endl;
    std::cout << "================================" << std::endl;
    
    return 0;
}
