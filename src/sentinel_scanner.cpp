// src/sentinel_scanner.cpp

#include "core/sentinel_scanner.h"
#include <algorithm>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    // pending_ is bounded (never more than sentinel_.size() - 1 chars),
    // concatenation is cheap and doesn't grow unbounded 
    std::string working = pending_ + std::string(chunk);

    std::size_t pos = working.find(sentinel_);
    if (pos != std::string::npos) {
        // Full sentinel found. Everything before it is safe; nothing after it matters
        std::string safe = working.substr(0, pos);
        pending_.clear();
        return {safe, true};
    }

    // No full match yet. Find longest suffix of `working` that could be the START of the sentinel 
    std::size_t max_check = std::min(working.size(), sentinel_.size() - 1);
    std::size_t overlap = 0;
    for (std::size_t len = max_check; len >= 1; --len) {
        if (working.compare(working.size() - len, len, sentinel_, 0, len) == 0) {
            overlap = len;
            break;
        }
    }

    std::string safe = working.substr(0, working.size() - overlap);
    pending_ = working.substr(working.size() - overlap);
    return {safe, false};
}

SentinelScanner::Out SentinelScanner::flush() {
    Out result{pending_, false}; // package whatever was being held back
    pending_.clear();            // empty pending_ so the scanner's state is clean 
    return result;
}