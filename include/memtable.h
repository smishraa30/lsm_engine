#pragma once
#include <map>
#include <string>
#include <shared_mutex>
#include <optional>
#include <fstream>
#include <sstream>
#include <cstdio>
#include "bloom_filter.h"

class MemTable {
private:
    std::map<std::string, std::string> table;
    mutable std::shared_mutex rw_lock; 
    size_t current_size_bytes;
    std::ofstream wal_file; 
    
    // --> NEW: Thresholds and tracking for SSTables
    size_t max_size_bytes; 
    int sstable_count;
    std::map<std::string, BloomFilter> bloom_filters;

    void flush_to_sstable(); // <-- NEW: Internal helper to dump data to disk

public:
    MemTable();
    ~MemTable(); 
    
    void recover_from_wal(); 
    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const; 
    void remove(const std::string& key);
    size_t get_size() const;
    void print_table() const;
    void compact();

};