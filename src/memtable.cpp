#include <iostream>
#include <mutex>
#include "../include/memtable.h"

// 1. Update Constructor
MemTable::MemTable() : current_size_bytes(0), sstable_count(0), max_size_bytes(150) { 
    // Notice max_size_bytes is 150. Your 5 users took 129 bytes. 
    // The next user we add will trigger a flush!
    
    recover_from_wal();
    wal_file.open("wal.log", std::ios::app);
    if (!wal_file.is_open()) {
        std::cerr << "CRITICAL ERROR: Failed to open Write-Ahead Log!" << std::endl;
    }
}

// 2. The Flush Routine (ADD THIS ANYWHERE)
void MemTable::flush_to_sstable() {
    sstable_count++;
    std::string filename = "sstable_" + std::to_string(sstable_count) + ".db";
    
    std::ofstream ss_file(filename);
    if (!ss_file.is_open()) {
        std::cerr << "ERROR: Could not create SSTable!" << std::endl;
        return;
    }

    // Dump everything into the file
    for (const auto& [k, v] : table) {
        ss_file << k << "," << v << "\n";
    }
    ss_file.close();

    std::cout << "[SYSTEM] MemTable full! Flushed to " << filename << std::endl;

    // Reset RAM
    table.clear();
    current_size_bytes = 0;

    // Reset the WAL (Close it, delete contents by opening in trunc mode, reopen in app mode)
    wal_file.close();
    wal_file.open("wal.log", std::ios::trunc); // trunc wipes the file clean
    wal_file.close();
    wal_file.open("wal.log", std::ios::app);

    // Inside flush_to_sstable()...
    
    // Create a new Bloom Filter for this specific file
    BloomFilter filter(1000, 3);

    for (const auto& [k, v] : table) {
        ss_file << k << "," << v << "\n";
        filter.add(k); // <-- NEW: Add the key to the Bloom Filter!
    }
    ss_file.close();

    // Save the filter in RAM so we can use it later
    bloom_filters[filename] = filter;
}

// 2. The Recovery Routine (NEW)
void MemTable::recover_from_wal() {
    std::ifstream in_file("wal.log"); // Open the file in Read Mode
    
    // If the file doesn't exist, this is a brand new database. Just return.
    if (!in_file.is_open()) {
        return; 
    }

    std::string line;
    int recovered_count = 0;

    // Read the file line by line
    while (std::getline(in_file, line)) {
        std::stringstream ss(line);
        std::string op, key, value;

        // Split the line by commas
        std::getline(ss, op, ',');
        std::getline(ss, key, ',');
        std::getline(ss, value);

        // Rebuild the in-memory map directly
        if (op == "PUT") {
            table[key] = value;
            current_size_bytes += key.size() + value.size();
        } else if (op == "DEL") {
            table[key] = "[[TOMBSTONE]]";
            current_size_bytes += key.size() + 13;
        }
        recovered_count++;
    }
    
    in_file.close();
    if (recovered_count > 0) {
        std::cout << "[SYSTEM] Recovered " << recovered_count << " operations from WAL.\n" << std::endl;
    }
}

// 2. Destructor (NEW): Safely closes the file when the program ends
MemTable::~MemTable() {
    if (wal_file.is_open()) {
        wal_file.close();
    }
}

// 3. Update 'put' to trigger the flush
void MemTable::put(const std::string& key, const std::string& value) {
    std::unique_lock lock(rw_lock); 
    
    if (wal_file.is_open()) {
        wal_file << "PUT," << key << "," << value << "\n";
        wal_file.flush(); 
    }

    table[key] = value;
    current_size_bytes += key.size() + value.size(); 

    // --> NEW: Check if we are too full!
    if (current_size_bytes >= max_size_bytes) {
        flush_to_sstable();
    }
}

// 3. Get: Safely reads data using a read-lock (UPGRADED FOR DISK SEARCH)
std::optional<std::string> MemTable::get(const std::string& key) const {
    std::shared_lock lock(rw_lock); // Read lock
    
    // --- STEP 1: Search the Front Desk Cart (RAM) ---
    auto it = table.find(key);
    if (it != table.end()) {
        if (it->second == "[[TOMBSTONE]]") {
            return std::nullopt; // It was deleted
        }
        return it->second; // Found in RAM!
    }

    // --- STEP 2: Search the Backroom Boxes (SSTables on Disk) ---
    // We search backwards (from sstable_count down to 1) to get the newest data first.
    for (int i = sstable_count; i >= 1; i--) {
        std::string filename = "sstable_" + std::to_string(i) + ".db";
        // --> NEW: Ask the Bloom Filter FIRST!
// --> NEW: Ask the Bloom Filter FIRST! (Const-Safe Version)
        auto filter_it = bloom_filters.find(filename);
        if (filter_it != bloom_filters.end()) {
            // Use filter_it->second instead of bloom_filters[filename]
            if (!filter_it->second.possibly_contains(key)) {
                // The filter says it's definitely not here. Skip the slow disk read!
                std::cout << "[Bloom Filter] Skipped reading " << filename << " for key: " << key << std::endl;
                continue; 
            }
        }
        std::ifstream ss_file(filename);
        
        if (!ss_file.is_open()) continue;

        std::string line;
        while (std::getline(ss_file, line)) {
            std::stringstream ss(line);
            std::string disk_key, disk_val;
            
            std::getline(ss, disk_key, ',');
            std::getline(ss, disk_val);

            // Did we find the key in this file?
            if (disk_key == key) {
                if (disk_val == "[[TOMBSTONE]]") {
                    return std::nullopt; // It was deleted
                }
                return disk_val; // Found on Disk!
            }
        }
    }

    // --- STEP 3: Not in RAM, not on Disk. ---
    return std::nullopt; 
}

// 4. Remove: Log the tombstone to disk FIRST
void MemTable::remove(const std::string& key) {
    std::unique_lock lock(rw_lock); 
    
    // --> CRASH INSURANCE: Log the deletion
    if (wal_file.is_open()) {
        wal_file << "DEL," << key << ",[[TOMBSTONE]]\n";
        wal_file.flush(); 
    }

    table[key] = "[[TOMBSTONE]]";
    current_size_bytes += key.size() + 13; 
}
// 5. Get Size: Safely reads the current estimated size
size_t MemTable::get_size() const {
    std::shared_lock lock(rw_lock); // Read lock
    return current_size_bytes;
}

// 6. Print Table: Dumps the contents to the console
void MemTable::print_table() const {
    std::shared_lock lock(rw_lock); // Read lock
    
    std::cout << "--- Current MemTable Contents ---" << std::endl;
    for (const auto& [key, value] : table) {
        std::cout << "[Key]: " << key << " | [Value]: " << value << std::endl;
    }
    std::cout << "---------------------------------" << std::endl;
}

// 7. Compaction: Merge all SSTables, remove duplicates and tombstones
void MemTable::compact() {
    std::unique_lock lock(rw_lock); // Lock the whole system while we clean up
    
    if (sstable_count <= 1) {
        std::cout << "[SYSTEM] Compaction skipped. Not enough files to merge." << std::endl;
        return;
    }

    std::cout << "[SYSTEM] Starting Compaction. Merging " << sstable_count << " files..." << std::endl;

    // Use a temporary map to automatically sort and overwrite older duplicate keys
    std::map<std::string, std::string> merged_data;

    // Read all files from oldest (1) to newest (sstable_count)
    for (int i = 1; i <= sstable_count; i++) {
        std::string filename = "sstable_" + std::to_string(i) + ".db";
        std::ifstream ss_file(filename);
        
        if (!ss_file.is_open()) continue;

        std::string line;
        while (std::getline(ss_file, line)) {
            std::stringstream ss(line);
            std::string key, val;
            std::getline(ss, key, ',');
            std::getline(ss, val);
            
            // Because we read oldest to newest, newer values will naturally overwrite old ones in the map!
            merged_data[key] = val; 
        }
        ss_file.close();
        
        // Delete the old file from the hard drive
        std::remove(filename.c_str());
    }

    // Now write the clean, merged data into a brand new sstable_1.db
    std::string new_filename = "sstable_1.db";
    std::ofstream new_file(new_filename);
    
    int kept_records = 0;
    for (const auto& [k, v] : merged_data) {
        // Drop any records marked for deletion!
        if (v != "[[TOMBSTONE]]") {
            new_file << k << "," << v << "\n";
            kept_records++;
        }
    }
    new_file.close();

    // Reset our file counter back to 1
    sstable_count = 1;

    std::cout << "[SYSTEM] Compaction complete! Kept " << kept_records << " clean records in " << new_filename << ".\n" << std::endl;
}