#include <gtest/gtest.h>
#include "LockManager.hpp"

TEST(LockManagerTest, MultipleReadersAllowed) {
    LockManager lm;
    int id = 100;

    EXPECT_TRUE(lm.tryRead(id));
    EXPECT_EQ(lm.getReaders(id), 1);
    EXPECT_FALSE(lm.isWriterActive(id));

    EXPECT_TRUE(lm.tryRead(id));
    EXPECT_EQ(lm.getReaders(id), 2);
}

TEST(LockManagerTest, WriterBlockedByReader) {
    LockManager lm;
    int id = 100;

    lm.tryRead(id);

    EXPECT_FALSE(lm.tryWrite(id));
    EXPECT_FALSE(lm.isWriterActive(id));

    lm.finishRead(id);

    EXPECT_TRUE(lm.tryWrite(id));
    EXPECT_TRUE(lm.isWriterActive(id));
}

TEST(LockManagerTest, WriterBlocksEveryone) {
    LockManager lm;
    int id = 200;

    EXPECT_TRUE(lm.tryWrite(id));
    EXPECT_FALSE(lm.tryWrite(id));
    EXPECT_FALSE(lm.tryRead(id));

    lm.finishWrite(id);

    EXPECT_TRUE(lm.tryRead(id));
}