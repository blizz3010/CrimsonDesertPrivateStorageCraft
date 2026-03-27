#include <gtest/gtest.h>
#include "mock_inventory.h"

using namespace StorageCraft::Test;

// ============================================================================
// MaterialPool logic tests using mock objects.
// These test the inventory-first consumption planning algorithm.
// ============================================================================

// Helper: plan consumption with inventory-first priority (mirrors MaterialPool logic)
struct MaterialSource {
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

std::optional<MaterialSource> PlanConsumption(
    const MockInventory& inv,
    const std::vector<MockStorage>& storages,
    int32_t itemId,
    int32_t needed
) {
    if (needed <= 0) return MaterialSource{0, 0};

    int32_t invCount = inv.GetItemCount(itemId);
    int32_t storCount = 0;
    for (const auto& s : storages) storCount += s.GetItemCount(itemId);

    if (invCount + storCount < needed) return std::nullopt;

    MaterialSource source;
    source.fromInventory = std::min(invCount, needed);
    source.fromStorage = needed - source.fromInventory;
    return source;
}

bool RequiresStorage(const MockInventory& inv, int32_t itemId, int32_t needed) {
    return inv.GetItemCount(itemId) < needed;
}

// --- Tests ---

TEST(MaterialPoolTest, InventoryOnlySufficient) {
    MockInventory inv;
    inv.AddItem(100, 10);
    std::vector<MockStorage> storages;

    auto result = PlanConsumption(inv, storages, 100, 5);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromInventory, 5);
    EXPECT_EQ(result->fromStorage, 0);
}

TEST(MaterialPoolTest, StorageOnlySufficient) {
    MockInventory inv;
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 10);

    auto result = PlanConsumption(inv, storages, 100, 5);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromInventory, 0);
    EXPECT_EQ(result->fromStorage, 5);
}

TEST(MaterialPoolTest, CombinedSources) {
    MockInventory inv;
    inv.AddItem(100, 3);
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 7);

    auto result = PlanConsumption(inv, storages, 100, 8);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromInventory, 3);  // All from inventory first
    EXPECT_EQ(result->fromStorage, 5);    // Remainder from storage
}

TEST(MaterialPoolTest, InsufficientTotal) {
    MockInventory inv;
    inv.AddItem(100, 3);
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(100, 2);

    auto result = PlanConsumption(inv, storages, 100, 10);
    ASSERT_FALSE(result.has_value());
}

TEST(MaterialPoolTest, ZeroNeeded) {
    MockInventory inv;
    std::vector<MockStorage> storages;

    auto result = PlanConsumption(inv, storages, 100, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromInventory, 0);
    EXPECT_EQ(result->fromStorage, 0);
}

TEST(MaterialPoolTest, MultipleStorages) {
    MockInventory inv;
    inv.AddItem(100, 2);
    std::vector<MockStorage> storages(3);
    storages[0].AddItem(100, 3);
    storages[1].AddItem(100, 4);
    storages[2].AddItem(100, 1);

    auto result = PlanConsumption(inv, storages, 100, 9);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromInventory, 2);
    EXPECT_EQ(result->fromStorage, 7);
}

TEST(MaterialPoolTest, RequiresStorageTrue) {
    MockInventory inv;
    inv.AddItem(100, 3);
    EXPECT_TRUE(RequiresStorage(inv, 100, 5));
}

TEST(MaterialPoolTest, RequiresStorageFalse) {
    MockInventory inv;
    inv.AddItem(100, 10);
    EXPECT_FALSE(RequiresStorage(inv, 100, 5));
}

TEST(MaterialPoolTest, DifferentItemIds) {
    MockInventory inv;
    inv.AddItem(100, 5);
    inv.AddItem(200, 3);
    std::vector<MockStorage> storages(1);
    storages[0].AddItem(200, 7);

    // Item 100: only in inventory
    auto r1 = PlanConsumption(inv, storages, 100, 5);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->fromInventory, 5);
    EXPECT_EQ(r1->fromStorage, 0);

    // Item 200: split across both
    auto r2 = PlanConsumption(inv, storages, 200, 8);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->fromInventory, 3);
    EXPECT_EQ(r2->fromStorage, 5);

    // Item 300: doesn't exist
    auto r3 = PlanConsumption(inv, storages, 300, 1);
    ASSERT_FALSE(r3.has_value());
}
