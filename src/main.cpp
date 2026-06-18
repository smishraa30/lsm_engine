#include <iostream>
#include "include/memtable.h"

int main() {
    std::cout << "Booting up LSM-Tree Storage Engine...\n" << std::endl;
    MemTable memtable;

    std::cout << "--- CREATING A DATA MESS ---" << std::endl;
    // Batch 1 (Will flush to sstable_1)
    memtable.put("User_Alice", "alice@example.com");
    memtable.put("User_Bob", "bob_V1@example.com");
    memtable.put("User_Charlie", "charlie@example.com");
    memtable.put("User_Dave", "dave@example.com");

    // Batch 2 (Bob updates his email, Alice is deleted. Will flush to sstable_2)
    memtable.put("User_Bob", "bob_V2@example.com");
    memtable.remove("User_Alice"); // Inserts a TOMBSTONE
    memtable.put("User_Eve", "eve@example.com");
    memtable.put("User_Frank", "frank@example.com");
    
    // Batch 3 (Bob updates email AGAIN. Will flush to sstable_3)
    memtable.put("User_Bob", "bob_V3_FINAL@example.com");
    memtable.put("User_Grace", "grace@example.com");
    memtable.put("User_Hank", "hank@example.com");

    // Right now, you have 3 SSTable files on disk with old versions of Bob and a dead Alice.
    
    std::cout << "\n--- RUNNING COMPACTION ---" << std::endl;
    memtable.compact();

    std::cout << "\n--- VERIFYING CLEANUP ---" << std::endl;
    auto bob = memtable.get("User_Bob");
    if (bob.has_value()) {
        std::cout << "Bob's Final Email: " << bob.value() << std::endl;
    }

    auto alice = memtable.get("User_Alice");
    if (!alice.has_value()) {
        std::cout << "Alice was successfully purged from the system!" << std::endl;
    }

    std::cout << "\nEngine shutting down cleanly." << std::endl;
    return 0;
}