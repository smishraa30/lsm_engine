#include "../include/bloom_filter.h"
#include <functional>

BloomFilter::BloomFilter(size_t size, int num_hashes) 
    : size(size), num_hashes(num_hashes) {
    bit_array.resize(size, false); // Create the array, set all bits to 0
}

std::vector<size_t> BloomFilter::hash_string(const std::string& key) const {
    std::vector<size_t> hashes;
    std::hash<std::string> hasher;
    
    // Generate multiple hashes by slightly altering the key
    for (int i = 0; i < num_hashes; i++) {
        size_t h = hasher(key + std::to_string(i));
        hashes.push_back(h % size); // Use modulo to keep the hash within the array bounds
    }
    return hashes;
}

void BloomFilter::add(const std::string& key) {
    std::vector<size_t> hashes = hash_string(key);
    for (size_t h : hashes) {
        bit_array[h] = true; // Flip the specific bits to 1
    }
}

bool BloomFilter::possibly_contains(const std::string& key) const {
    std::vector<size_t> hashes = hash_string(key);
    for (size_t h : hashes) {
        if (!bit_array[h]) {
            return false; // If ANY bit is 0, the key is DEFINITELY NOT here.
        }
    }
    return true; // All checked bits were 1. It MIGHT be here.
}