#include "core/sparse_set.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <random>

TEST(SparseSetTest, RejectsUnknownHandles)
{
    SparseSet<int> set;
    EXPECT_EQ(set.get({}), nullptr);
    EXPECT_EQ(set.get({100, 1}), nullptr);
    EXPECT_NO_THROW(set.remove({100, 1}));
    EXPECT_TRUE(set.dense().empty());
}

TEST(SparseSetTest, HandlesSurviveStorageGrowth)
{
    SparseSet<int> set;
    std::vector<SparseSet<int>::Handle> handles;
    for (int value = 0; value < 1000; ++value)
        handles.push_back(set.insert(value));
    for (int value = 0; value < 1000; ++value)
    {
        ASSERT_NE(set.get(handles[value]), nullptr);
        EXPECT_EQ(*set.get(handles[value]), value);
    }
}

class SparseSetRemovalTest : public testing::TestWithParam<int>
{
};

TEST_P(SparseSetRemovalTest, RemovingAnyDenseSlotPreservesOtherHandles)
{
    SparseSet<int> set;
    std::vector<SparseSet<int>::Handle> handles;
    for (int i = 0; i < 3; ++i)
        handles.push_back(set.insert(10 + i));
    set.remove(handles[GetParam()]);
    EXPECT_EQ(set.dense().size(), 2u);
    for (int i = 0; i < 3; ++i)
    {
        if (i == GetParam())
            EXPECT_EQ(set.get(handles[i]), nullptr);
        else
        {
            ASSERT_NE(set.get(handles[i]), nullptr);
            EXPECT_EQ(*set.get(handles[i]), 10 + i);
        }
    }
}
INSTANTIATE_TEST_SUITE_P(FirstMiddleLast, SparseSetRemovalTest, testing::Values(0, 1, 2));

TEST(SparseSetTest, ReusedSlotRejectsStaleHandleAndRepeatedRemoval)
{
    SparseSet<int> set;
    const auto oldHandle = set.insert(1);
    set.remove(oldHandle);
    set.remove(oldHandle);
    const auto newHandle = set.insert(2);
    EXPECT_EQ(oldHandle.index, newHandle.index);
    EXPECT_NE(oldHandle.generation, newHandle.generation);
    EXPECT_EQ(set.get(oldHandle), nullptr);
    set.remove(oldHandle);
    ASSERT_NE(set.get(newHandle), nullptr);
    EXPECT_EQ(*set.get(newHandle), 2);
    EXPECT_EQ(set.dense().size(), 1u);
}

TEST(SparseSetTest, OwnsMoveOnlyResourcesUntilRemoval)
{
    SparseSet<std::unique_ptr<int>> set;
    const auto first = set.insert(std::make_unique<int>(7));
    const auto second = set.insert(std::make_unique<int>(9));
    set.remove(first);
    ASSERT_NE(set.get(second), nullptr);
    EXPECT_EQ(**set.get(second), 9);
    set.remove(second);
    EXPECT_TRUE(set.dense().empty());
}

TEST(SparseSetTest, MixedInsertionsAndRemovalsKeepAllLiveHandlesValid)
{
    SparseSet<int> set;
    std::vector<std::pair<SparseSet<int>::Handle, int>> live;
    std::mt19937 random(42);
    for (int value = 0; value < 500; ++value)
    {
        if (!live.empty() && random() % 2 == 0)
        {
            const size_t index = random() % live.size();
            const auto removed = live[index].first;
            set.remove(removed);
            live.erase(live.begin() + index);
            EXPECT_EQ(set.get(removed), nullptr);
        }
        else
            live.emplace_back(set.insert(value), value);
        ASSERT_EQ(set.dense().size(), live.size());
        for (const auto &[handle, expected] : live)
        {
            ASSERT_NE(set.get(handle), nullptr);
            EXPECT_EQ(*set.get(handle), expected);
        }
    }
}
