#pragma once

// ============================================================================
// Mock implementations for unit testing MaterialPool and MaterialConsumer.
//
// Since the real InventoryAccessor and StorageAccessor operate on raw game
// memory pointers, we provide testable mock versions that use local storage.
// The MaterialPool and MaterialConsumer classes are tested against these mocks
// via a simple interface adapter.
// ============================================================================

#include <vector>
#include <cstdint>
#include <algorithm>

namespace StorageCraft::Test {

struct MockItemStack {
    int32_t itemId = 0;
    int32_t count = 0;
};

// Mock inventory that stores items in a simple vector.
class MockInventory {
public:
    void AddItem(int32_t itemId, int32_t count) {
        for (auto& item : m_items) {
            if (item.itemId == itemId) {
                item.count += count;
                return;
            }
        }
        m_items.push_back({itemId, count});
    }

    int32_t GetItemCount(int32_t itemId) const {
        int32_t total = 0;
        for (const auto& item : m_items) {
            if (item.itemId == itemId) total += item.count;
        }
        return total;
    }

    bool ConsumeItem(int32_t itemId, int32_t amount) {
        int32_t available = GetItemCount(itemId);
        if (available < amount) return false;

        int32_t remaining = amount;
        for (auto it = m_items.begin(); it != m_items.end() && remaining > 0;) {
            if (it->itemId != itemId) { ++it; continue; }
            int32_t take = std::min(it->count, remaining);
            it->count -= take;
            remaining -= take;
            if (it->count <= 0) {
                it = m_items.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }

    bool IsValid() const { return true; }

    const std::vector<MockItemStack>& GetItems() const { return m_items; }

private:
    std::vector<MockItemStack> m_items;
};

// Mock storage with range and lock simulation.
class MockStorage {
public:
    void AddItem(int32_t itemId, int32_t count) {
        for (auto& item : m_items) {
            if (item.itemId == itemId) {
                item.count += count;
                return;
            }
        }
        m_items.push_back({itemId, count});
    }

    int32_t GetItemCount(int32_t itemId) const {
        int32_t total = 0;
        for (const auto& item : m_items) {
            if (item.itemId == itemId) total += item.count;
        }
        return total;
    }

    bool ConsumeItem(int32_t itemId, int32_t amount) {
        if (!m_locked) return false;
        int32_t available = GetItemCount(itemId);
        if (available < amount) return false;

        int32_t remaining = amount;
        for (auto it = m_items.begin(); it != m_items.end() && remaining > 0;) {
            if (it->itemId != itemId) { ++it; continue; }
            int32_t take = std::min(it->count, remaining);
            it->count -= take;
            remaining -= take;
            if (it->count <= 0) {
                it = m_items.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }

    bool IsValid() const { return true; }
    bool IsInRange(float) const { return m_inRange; }

    bool TryLock() {
        if (m_externalLock) return false;
        m_locked = true;
        return true;
    }
    void Unlock() { m_locked = false; }
    bool IsLocked() const { return m_locked; }

    void SetInRange(bool inRange) { m_inRange = inRange; }
    void SetExternalLock(bool locked) { m_externalLock = locked; }

private:
    std::vector<MockItemStack> m_items;
    bool m_locked = false;
    bool m_inRange = true;
    bool m_externalLock = false;
};

} // namespace StorageCraft::Test
