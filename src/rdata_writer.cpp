#include "rdata_writer.h"
#include <cstring>
#include <iostream>

// ===== Constructor and file operations =====

RDataWriter::RDataWriter() : needByteSwap(false) {
    needByteSwap = isMachineLittleEndian();
}

bool RDataWriter::open(const std::string& filename) {
    file.open(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot create RData file: " << filename << std::endl;
        return false;
    }
    refTable.clear();
    return true;
}

void RDataWriter::close() {
    if (file.is_open()) {
        file.close();
    }
}

// ===== Utility methods =====

bool RDataWriter::isMachineLittleEndian() {
    int test = 1;
    return *reinterpret_cast<char*>(&test) == 1;
}

uint32_t RDataWriter::byteSwap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

uint64_t RDataWriter::byteSwap64(uint64_t val) {
    val = ((val & 0xFFFFFFFF00000000ULL) >> 32) | ((val & 0x00000000FFFFFFFFULL) << 32);
    val = ((val & 0xFFFF0000FFFF0000ULL) >> 16) | ((val & 0x0000FFFF0000FFFFULL) << 16);
    val = ((val & 0xFF00FF00FF00FF00ULL) >> 8)  | ((val & 0x00FF00FF00FF00FFULL) << 8);
    return val;
}

double RDataWriter::byteSwapDouble(double val) {
    uint64_t* ptr = reinterpret_cast<uint64_t*>(&val);
    uint64_t swapped = byteSwap64(*ptr);
    return *reinterpret_cast<double*>(&swapped);
}

// ===== Low-level write methods =====

void RDataWriter::writeBytes(const void* data, size_t len) {
    file.write(static_cast<const char*>(data), len);
}

void RDataWriter::writeInt32(int32_t val) {
    if (needByteSwap) {
        val = static_cast<int32_t>(byteSwap32(static_cast<uint32_t>(val)));
    }
    writeBytes(&val, sizeof(val));
}

void RDataWriter::writeDouble(double val) {
    if (needByteSwap) {
        val = byteSwapDouble(val);
    }
    writeBytes(&val, sizeof(val));
}

// ===== R serialization methods =====

void RDataWriter::writeHeader(int type, int flags) {
    // SEXP header is packed into a 32-bit integer
    // Bits 0-7: type, bit 8: object flag, bit 9: attributes flag, bit 10: tag flag
    int32_t header = type | flags;
    writeInt32(header);
}

void RDataWriter::writeString(const char* str) {
    // Write a CHARSXP (character string)
    writeHeader(CHARSXP, 0);
    
    if (str == nullptr) {
        // NA string - write -1 length
        writeInt32(-1);
    } else {
        size_t len = strlen(str);
        writeInt32(static_cast<int32_t>(len));
        if (len > 0) {
            writeBytes(str, len);
        }
    }
}

void RDataWriter::writePairlistKey(const std::string& key) {
    // Check if we've seen this key before (for reference optimization)
    for (size_t i = 0; i < refTable.size(); ++i) {
        if (refTable[i] == key) {
            // Write reference
            int32_t ref = static_cast<int32_t>((i + 1) << 8) | 0xFF;
            writeInt32(ref);
            return;
        }
    }
    
    // New key - add to reference table and write
    refTable.push_back(key);
    writeInt32(1);  // Not a reference
    writeString(key.c_str());
}

void RDataWriter::writePairlistHeader(const std::string& key) {
    writeHeader(LISTSXP, HAS_TAG);
    writePairlistKey(key);
}

void RDataWriter::writeAttributedVectorHeader(int type, int32_t size) {
    writeHeader(type, HAS_OBJECT | HAS_ATTR);
    writeInt32(size);
}

void RDataWriter::writeSimpleVectorHeader(int type, int32_t size) {
    writeHeader(type, 0);
    writeInt32(size);
}

void RDataWriter::writeClassPairlist(const std::string& className) {
    writePairlistHeader("class");
    writeSimpleVectorHeader(STRSXP, 1);
    writeString(className.c_str());
}

// ===== High-level write methods =====

void RDataWriter::writeStringColumn(const std::vector<std::string>& values) {
    writeSimpleVectorHeader(STRSXP, static_cast<int32_t>(values.size()));
    for (const auto& val : values) {
        if (val.empty()) {
            writeString("");
        } else {
            writeString(val.c_str());
        }
    }
}

void RDataWriter::writeDataFrameAttributes(const std::vector<std::string>& headers,
                                            int32_t rowCount,
                                            const std::string& label) {
    // Write "names" attribute (column names)
    writePairlistHeader("names");
    writeSimpleVectorHeader(STRSXP, static_cast<int32_t>(headers.size()));
    for (const auto& name : headers) {
        writeString(name.c_str());
    }
    
    // Write "class" attribute
    writeClassPairlist("data.frame");
    
    // Write "row.names" attribute
    writePairlistHeader("row.names");
    writeSimpleVectorHeader(STRSXP, rowCount);
    for (int32_t i = 1; i <= rowCount; ++i) {
        writeString(std::to_string(i).c_str());
    }
    
    // End of attributes (NILSXP)
    writeHeader(NILVALUE_SXP, 0);
}

void RDataWriter::writeDataFrame(const std::string& tableName,
                                  const ReadingList& readings,
                                  const std::vector<std::string>& headers,
                                  const std::string& label) {
    int32_t rowCount = static_cast<int32_t>(readings.size());
    int32_t colCount = static_cast<int32_t>(headers.size());
    
    // For RData format, wrap in a pairlist with the table name
    if (!tableName.empty()) {
        writePairlistHeader(tableName);
    }
    
    // Write the data frame as a generic vector (list) with attributes
    writeAttributedVectorHeader(VECSXP, colCount);
    
    // Write each column
    for (const auto& colName : headers) {
        std::vector<std::string> columnValues;
        columnValues.reserve(readings.size());
        
        for (const auto& reading : readings) {
            auto it = reading.find(colName);
            if (it != reading.end()) {
                columnValues.push_back(it->second);
            } else {
                columnValues.push_back("");
            }
        }
        
        writeStringColumn(columnValues);
    }
    
    // Write data frame attributes
    writeDataFrameAttributes(headers, rowCount, label);
    
    // End pairlist for RData format
    if (!tableName.empty()) {
        writeHeader(NILVALUE_SXP, 0);
    }
}

// ===== Public static methods =====

bool RDataWriter::writeRData(const std::string& filename,
                              const ReadingList& readings,
                              const std::vector<std::string>& headers,
                              const std::string& tableName) {
    if (readings.empty()) {
        std::cerr << "Error: No data to write" << std::endl;
        return false;
    }
    
    RDataWriter writer;
    if (!writer.open(filename)) {
        return false;
    }
    
    // Write RData magic header
    writer.writeBytes(RDATA_MAGIC, 5);
    
    // Write binary header
    writer.writeBytes(BINARY_HEADER, 2);
    writer.writeInt32(FORMAT_VERSION);
    writer.writeInt32(READER_VERSION);
    writer.writeInt32(WRITER_VERSION);
    
    // Write the data frame
    writer.writeDataFrame(tableName, readings, headers, "Sensor data");
    
    // Final NILSXP to end the file
    writer.writeHeader(NILVALUE_SXP, 0);
    
    writer.close();
    return true;
}

bool RDataWriter::writeRDS(const std::string& filename,
                            const ReadingList& readings,
                            const std::vector<std::string>& headers,
                            const std::string& label) {
    if (readings.empty()) {
        std::cerr << "Error: No data to write" << std::endl;
        return false;
    }
    
    RDataWriter writer;
    if (!writer.open(filename)) {
        return false;
    }
    
    // RDS has no magic header, just the binary header
    writer.writeBytes(BINARY_HEADER, 2);
    writer.writeInt32(FORMAT_VERSION);
    writer.writeInt32(READER_VERSION);
    writer.writeInt32(WRITER_VERSION);
    
    // Write the data frame directly (no wrapping pairlist)
    writer.writeDataFrame("", readings, headers, label);
    
    writer.close();
    return true;
}
