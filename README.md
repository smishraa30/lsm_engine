# 🗄️ LSM-Tree Storage Engine (C++)

A high-performance, embedded Key-Value storage engine built from scratch in C++17. This project implements the core architecture of Log-Structured Merge-Trees (LSM-Trees), the foundational data structure behind enterprise databases like **Google's LevelDB** and **Meta's RocksDB**.

## 🚀 Core Features
* **Thread-Safe MemTable:** Concurrent in-memory data ingestion using `std::shared_mutex` (Read-Write Locks).
* **Crash Resilience (WAL):** Write-Ahead Logging ensures zero data loss upon abrupt system failure.
* **Tiered Disk Storage (SSTables):** Flushes immutable, sorted data chunks to disk to prevent RAM exhaustion.
* **Background Compaction:** Leveled merging of data files to purge duplicate/tombstoned records and optimize disk I/O.

## 🧠 System Architecture

```mermaid
graph TD;
    Client-->|Write| WAL[(Write-Ahead Log)]
    Client-->|Write| MemTable[Active MemTable / RAM]
    MemTable-- Flush Threshold Met -->SST1[(SSTable 1 / Disk)]
    MemTable-- Flush Threshold Met -->SST2[(SSTable 2 / Disk)]
    SST1-->Compaction{Background Compaction}
    SST2-->Compaction
    Compaction-->|Merge & Purge| SST3[(Cleaned SSTable)]