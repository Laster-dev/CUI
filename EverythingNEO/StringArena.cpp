#include "StringArena.h"
#include <cstring>
#include <algorithm>

namespace EverythingNEO {

StringArena::StringArena() {
    AllocateChunk();
}

void StringArena::AllocateChunk() {
    m_chunks.push_back(std::make_unique<char[]>(CHUNK_SIZE));
    m_currentChunkIndex = m_chunks.size() - 1;
    m_currentChunkOffset = 0;
    m_totalBytesAllocated += CHUNK_SIZE;
}

uint32_t StringArena::AddString(std::string_view str) {
    size_t needed = str.length() + 1;
    if (needed > CHUNK_SIZE) {
        needed = CHUNK_SIZE;
        str = str.substr(0, CHUNK_SIZE - 1);
    }

    if (m_currentChunkOffset + needed > CHUNK_SIZE) {
        AllocateChunk();
    }

    uint32_t globalOffset = static_cast<uint32_t>(m_currentChunkIndex * CHUNK_SIZE + m_currentChunkOffset);
    char* dest = m_chunks[m_currentChunkIndex].get() + m_currentChunkOffset;

    std::memcpy(dest, str.data(), str.length());
    dest[str.length()] = '\0';

    m_currentChunkOffset += needed;
    m_usedBytes += needed;
    return globalOffset;
}

std::string_view StringArena::GetString(uint32_t offset, uint16_t length) const {
    size_t chunkIdx = offset / CHUNK_SIZE;
    size_t localOffset = offset % CHUNK_SIZE;
    if (chunkIdx >= m_chunks.size()) {
        return {};
    }
    const char* ptr = m_chunks[chunkIdx].get() + localOffset;
    return std::string_view(ptr, length);
}

void StringArena::Clear() {
    m_chunks.clear();
    m_currentChunkIndex = 0;
    m_currentChunkOffset = 0;
    m_totalBytesAllocated = 0;
    m_usedBytes = 0;
    AllocateChunk();
}

size_t StringArena::GetTotalMemoryUsage() const {
    return m_totalBytesAllocated;
}

size_t StringArena::GetUsedBytes() const {
    return m_usedBytes;
}

std::string StringArena::Serialize() const {
    std::string out;
    out.resize(m_usedBytes);
    size_t written = 0;
    for (size_t i = 0; i < m_chunks.size() && written < m_usedBytes; ++i) {
        size_t chunkUsed = (i == m_currentChunkIndex) ? m_currentChunkOffset : CHUNK_SIZE;
        if (i < m_currentChunkIndex) chunkUsed = CHUNK_SIZE;
        size_t toCopy = (std::min)(chunkUsed, m_usedBytes - written);
        if (toCopy == 0) break;
        std::memcpy(out.data() + written, m_chunks[i].get(), toCopy);
        written += toCopy;
    }
    out.resize(written);
    return out;
}

bool StringArena::Deserialize(const void* data, size_t size) {
    Clear();
    if (!data || size == 0) return true;

    const char* src = static_cast<const char*>(data);
    size_t remaining = size;
    while (remaining > 0) {
        size_t toCopy = (std::min)(remaining, CHUNK_SIZE);
        if (m_currentChunkOffset != 0 && m_currentChunkOffset + toCopy > CHUNK_SIZE) {
            AllocateChunk();
        }
        // Prefer filling whole chunks sequentially from start
        if (m_currentChunkOffset == 0 && toCopy == CHUNK_SIZE) {
            std::memcpy(m_chunks[m_currentChunkIndex].get(), src, toCopy);
            m_currentChunkOffset = CHUNK_SIZE;
            m_usedBytes += toCopy;
            src += toCopy;
            remaining -= toCopy;
            if (remaining > 0) AllocateChunk();
        } else {
            if (m_currentChunkOffset + toCopy > CHUNK_SIZE) {
                AllocateChunk();
            }
            std::memcpy(m_chunks[m_currentChunkIndex].get() + m_currentChunkOffset, src, toCopy);
            m_currentChunkOffset += toCopy;
            m_usedBytes += toCopy;
            src += toCopy;
            remaining -= toCopy;
        }
    }
    return true;
}

} // namespace EverythingNEO
