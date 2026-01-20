#ifndef RDATA_WRITER_H
#define RDATA_WRITER_H

/**
 * RDataWriter - A minimal RData file writer for sensor data
 * 
 * This implements R's serialization format (version 2) for writing data frames.
 * Based on the RData file format specification and librdata library.
 * 
 * The RData format is R's native binary format for storing R objects.
 * Files produced can be loaded in R using load("file.RData")
 * 
 * Format overview:
 * - Magic header: "RDX2\n" for RData, none for RDS
 * - Binary header: "X\n" + version 2 + R version info
 * - Serialized R objects using SEXP type markers
 * 
 * This implementation supports string columns only, which is sufficient
 * for sensor data where all values are stored as strings.
 */

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include "types.h"

class RDataWriter {
public:
    /**
     * Write sensor readings to an RData file
     * 
     * @param filename Output file path (should end with .RData or .rdata)
     * @param readings Vector of sensor readings (key-value maps)
     * @param headers Column names in order
     * @param tableName Name of the data frame variable in R (default: "sensor_data")
     * @return true if successful, false on error
     */
    static bool writeRData(const std::string& filename,
                           const ReadingList& readings,
                           const std::vector<std::string>& headers,
                           const std::string& tableName = "sensor_data");

    /**
     * Write sensor readings to an RDS file (single object, no variable name)
     * 
     * @param filename Output file path (should end with .rds)
     * @param readings Vector of sensor readings
     * @param headers Column names in order
     * @param label Description label for the data frame
     * @return true if successful, false on error
     */
    static bool writeRDS(const std::string& filename,
                         const ReadingList& readings,
                         const std::vector<std::string>& headers,
                         const std::string& label = "Sensor data");

private:
    // R serialization constants
    static constexpr const char* RDATA_MAGIC = "RDX2\n";
    static constexpr const char* BINARY_HEADER = "X\n";
    static constexpr uint32_t FORMAT_VERSION = 2;
    static constexpr uint32_t READER_VERSION = 131840;  // R 2.4.0
    static constexpr uint32_t WRITER_VERSION = 131840;

    // SEXP types from R internals
    static constexpr int NILSXP = 0;        // NULL
    static constexpr int SYMSXP = 1;        // Symbol
    static constexpr int LISTSXP = 2;       // Pairlist
    static constexpr int CLOSXP = 3;        // Closure
    static constexpr int ENVSXP = 4;        // Environment
    static constexpr int PROMSXP = 5;       // Promise
    static constexpr int LANGSXP = 6;       // Language object
    static constexpr int SPECIALSXP = 7;    // Special
    static constexpr int BUILTINSXP = 8;    // Builtin
    static constexpr int CHARSXP = 9;       // Character string (internal)
    static constexpr int LGLSXP = 10;       // Logical vector
    static constexpr int INTSXP = 13;       // Integer vector
    static constexpr int REALSXP = 14;      // Real (double) vector
    static constexpr int CPLXSXP = 15;      // Complex vector
    static constexpr int STRSXP = 16;       // String vector
    static constexpr int DOTSXP = 17;       // ...
    static constexpr int ANYSXP = 18;       // Any type
    static constexpr int VECSXP = 19;       // Generic vector (list)
    static constexpr int EXPRSXP = 20;      // Expression vector
    static constexpr int BCODESXP = 21;     // Byte code
    static constexpr int EXTPTRSXP = 22;    // External pointer
    static constexpr int WEAKREFSXP = 23;   // Weak reference
    static constexpr int RAWSXP = 24;       // Raw bytes
    static constexpr int S4SXP = 25;        // S4 object

    // Pseudo SEXP types for serialization
    static constexpr int REFSXP = 255;      // Reference
    static constexpr int NILVALUE_SXP = 254;
    static constexpr int GLOBALENV_SXP = 253;
    static constexpr int UNBOUNDVALUE_SXP = 252;
    static constexpr int MISSINGARG_SXP = 251;
    static constexpr int BASENAMESPACE_SXP = 250;
    static constexpr int NAMESPACESXP = 249;
    static constexpr int PACKAGESXP = 248;
    static constexpr int PERSISTSXP = 247;
    static constexpr int CLASSREFSXP = 246;
    static constexpr int GENERICREFSXP = 245;
    static constexpr int BCREPDEF = 244;
    static constexpr int BCREPREF = 243;
    static constexpr int EMPTYENV_SXP = 242;
    static constexpr int BASEENV_SXP = 241;

    // SEXP flags
    static constexpr int HAS_OBJECT = 0x100;
    static constexpr int HAS_ATTR = 0x200;
    static constexpr int HAS_TAG = 0x400;

    // Writer state
    std::ofstream file;
    bool needByteSwap;
    std::vector<std::string> refTable;  // For reference tracking

    RDataWriter();
    
    bool open(const std::string& filename);
    void close();
    
    // Low-level write methods
    void writeBytes(const void* data, size_t len);
    void writeInt32(int32_t val);
    void writeDouble(double val);
    
    // R serialization methods
    void writeHeader(int type, int flags = 0);
    void writeString(const char* str);
    void writePairlistHeader(const std::string& key);
    void writePairlistKey(const std::string& key);
    void writeAttributedVectorHeader(int type, int32_t size);
    void writeSimpleVectorHeader(int type, int32_t size);
    void writeClassPairlist(const std::string& className);
    
    // High-level write methods
    void writeDataFrame(const std::string& tableName,
                        const ReadingList& readings,
                        const std::vector<std::string>& headers,
                        const std::string& label);
    void writeStringColumn(const std::vector<std::string>& values);
    void writeDataFrameAttributes(const std::vector<std::string>& headers,
                                   int32_t rowCount,
                                   const std::string& label);
    
    // Utility
    bool isMachineLittleEndian();
    uint32_t byteSwap32(uint32_t val);
    uint64_t byteSwap64(uint64_t val);
    double byteSwapDouble(double val);
};

#endif // RDATA_WRITER_H
