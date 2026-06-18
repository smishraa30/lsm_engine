#pragma once
#include <vector>
#include <string>

class BloomFilter {
private:
    std::vector<bool> bit_array;
    int num_hashes;
    size_t size;

    // A helper function to generate multiple hash values for a single key
    std::vector<size_t> hash_string(const std::string& key) const;

public:
    // Initialize with default size (1000 bits) and 3 hash functions
    BloomFilter(size_t size = 1000, int num_hashes = 3);

    // Add a key into the filter
    void add(const std::string& key);

    // Check if a key MIGHT exist
    bool possibly_contains(const std::string& key) const;
};