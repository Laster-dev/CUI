#pragma once

#include <vector>
#include <string_view>
#include <cstdint>
#include <memory>
#include <string>

namespace EverythingNEO {

// Chunk-based contiguous string pool — filenames live here, null-terminated,
// eliminating per-string heap headers (Everything-style String Arena).
class StringArena {
public:
    static constexpr size_t CHUNK_SIZE = 4 * 1024 * 1024; // 4 MB — less RSS waste than 16MB

    StringArena();
    ~StringArena() = default;

    StringArena(const StringArena&) = delete;
    StringArena& operator=(const StringArena&) = delete;

    uint32_t AddString(std::string_view str);
    std::string_view GetString(uint32_t offset, uint16_t length) const;

    void Clear();
    void Swap(StringArena& other) noexcept;
    size_t GetTotalMemoryUsage() const;
    size_t GetUsedBytes() const;

    // Flatten used bytes into a contiguous buffer (for Everything.db).
    std::string Serialize() const;
    bool Deserialize(const void* data, size_t size);

private:
    void AllocateChunk();

    std::vector<std::unique_ptr<char[]>> m_chunks;
    size_t m_currentChunkIndex = 0;
    size_t m_currentChunkOffset = 0;
    size_t m_totalBytesAllocated = 0;
    size_t m_usedBytes = 0;
};

} // namespace EverythingNEO
