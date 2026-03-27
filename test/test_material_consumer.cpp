#include <gtest/gtest.h>
#include "mock_inventory.h"

using namespace StorageCraft::Test;

// ============================================================================
// MaterialConsumer logic tests using mock objects.
// Tests the two-phase commit (validate then consume) pattern.
// ============================================================================

struct ConsumptionEntry {
    int32_t itemId = 0;
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

// Mirrors MaterialConsumer::Execute logic against mocks
bool ExecuteConsumption(
    const std::vector<ConsumptionEntry>& entries,
    MockInventory& inv,
    std::vector<MockStorage>& storages
) {
    // Phase 1: Validate
    for (const auto& entry : entries) {
        if (inv.GetItemCount(entry.itemId) < entry.fromInventory)
            return false;
        if (entry.fromStorage > 0) {
            int32_t storAvail = 0;
            for (const auto& s : storages) storAvail += s.GetItemCount(entry.itemId);
            if (storAvail < entry.fromStorage) return false;
        }
    }

    // Phase 2: Consume
    for (const auto& entry : entries) {
        if (entry.fromInventory > 0) {
            if (!inv.ConsumeItem(entry.itemId, entry.fromInventory)) return false;
        }
        if (entry.fromStorage > 0) {
            int32_t remaining = entry.fromStorage;
            for (auto& s : storages) {
                if (remaining <= 0) break;
                if (!s.IsLocked()) continue;
                int32_t avail = s.GetItemCount(entry.itemId);
                if (avail <= 0) continue;
                int32_t take = std::min(avail, remaining);
                if (s.ConsumeItem(entry.itemId, take)) remaining -= take;
            }
            if (remaining > 0) return false;
        }
    }
    return true;
}

// --- Tests ---

TEST(MaterialConsumerTest, ConsumeFromInventoryOnly) {
    MockInventory inv;
    inv.AddItem(100, 10);
    std::vector<MockStorage> storages;

    std::vector<ConsumptionEntry> plan = {{100, 5, 0}};
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(inv.GetItemCount(100), 5);
}

TEST(MaterialConsumerTest, ConsumeFromStorageOnly) {
    MockInventory inv;
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 10);
    storages[0].TryLock();

    std::vector<ConsumptionEntry> plan = {{100, 0, 5}};
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(storages[0].GetItemCount(100), 5);
}

TEST(MaterialConsumerTest, ConsumeSplitAcrossSources) {
    MockInventory inv;
    inv.AddItem(100, 3);
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 7);
    storages[0].TryLock();

    std::vector<ConsumptionEntry> plan = {{100, 3, 5}};
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(inv.GetItemCount(100), 0);
    EXPECT_EQ(storages[0].GetItemCount(100), 2);
}

TEST(MaterialConsumerTest, ValidationFailsInsufficientInventory) {
    MockInventory inv;
    inv.AddItem(100, 2);
    std::vector<MockStorage> storages;

    std::vector<ConsumptionEntry> plan = {{100, 5, 0}};
    EXPECT_FALSE(ExecuteConsumption(plan, inv, storages));
    // Inventory should be unchanged after validation failure
    EXPECT_EQ(inv.GetItemCount(100), 2);
}

TEST(MaterialConsumerTest, ValidationFailsInsufficientStorage) {
    MockInventory inv;
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 3);
    storages[0].TryLock();

    std::vector<ConsumptionEntry> plan = {{100, 0, 5}};
    EXPECT_FALSE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(storages[0].GetItemCount(100), 3);
}

TEST(MaterialConsumerTest, MultipleItemsInPlan) {
    MockInventory inv;
    inv.AddItem(100, 5);
    inv.AddItem(200, 3);
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(200, 7);
    storages[0].TryLock();

    std::vector<ConsumptionEntry> plan = {
        {100, 4, 0},  // 4 iron from inventory
        {200, 3, 2},  // 3 wood from inventory + 2 from storage
    };
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(inv.GetItemCount(100), 1);
    EXPECT_EQ(inv.GetItemCount(200), 0);
    EXPECT_EQ(storages[0].GetItemCount(200), 5);
}

TEST(MaterialConsumerTest, StorageNotLockedFails) {
    MockInventory inv;
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 10);
    // Note: NOT locked

    std::vector<ConsumptionEntry> plan = {{100, 0, 5}};
    // Validation passes (items exist) but consumption fails (not locked)
    EXPECT_FALSE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(storages[0].GetItemCount(100), 10); // Unchanged
}

TEST(MaterialConsumerTest, ExternalLockPreventsAccess) {
    MockInventory inv;
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 10);
    storages[0].SetExternalLock(true);

    EXPECT_FALSE(storages[0].TryLock());
    EXPECT_FALSE(storages[0].IsLocked());
}

TEST(MaterialConsumerTest, ConsumeAcrossMultipleStorages) {
    MockInventory inv;
    std::vector<MockStorage> storages(2);
    storages[0].AddItem(100, 3);
    storages[0].TryLock();
    storages[1].AddItem(100, 4);
    storages[1].TryLock();

    std::vector<ConsumptionEntry> plan = {{100, 0, 6}};
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));

    // Should consume 3 from first, 3 from second
    EXPECT_EQ(storages[0].GetItemCount(100), 0);
    EXPECT_EQ(storages[1].GetItemCount(100), 1);
}

TEST(MaterialConsumerTest, EmptyPlanSucceeds) {
    MockInventory inv;
    std::vector<MockStorage> storages;
    std::vector<ConsumptionEntry> plan;
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
}

TEST(MaterialConsumerTest, ExactAmountConsumed) {
    MockInventory inv;
    inv.AddItem(100, 5);
    std::vector<MockStorage> storages;

    std::vector<ConsumptionEntry> plan = {{100, 5, 0}};
    EXPECT_TRUE(ExecuteConsumption(plan, inv, storages));
    EXPECT_EQ(inv.GetItemCount(100), 0);
}
