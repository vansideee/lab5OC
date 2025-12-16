#pragma once
#include <map>

class LockManager {
    struct State {
        int readers = 0;
        bool writer = false;
    };
    std::map<int, State> locks_;

public:
    bool tryRead(int id) {
        if (locks_[id].writer) return false;
        locks_[id].readers++;
        return true;
    }

    void finishRead(int id) {
        if (locks_[id].readers > 0)
            locks_[id].readers--;
    }

    bool tryWrite(int id) {
        if (locks_[id].writer || locks_[id].readers > 0) return false;
        locks_[id].writer = true;
        return true;
    }

    void finishWrite(int id) {
        locks_[id].writer = false;
    }

    int getReaders(int id) { return locks_[id].readers; }
    bool isWriterActive(int id) { return locks_[id].writer; }
};