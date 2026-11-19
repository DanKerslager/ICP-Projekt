#pragma once

#include <condition_variable>
#include <mutex>
#include <stop_token>

// Single slot holding the newest value. put() overwrites, so a slow reader skips old values
// instead of falling behind. Lock is held only for the copy - use cheap T (shared_ptr, small struct).
//
// One thread may take() (waits for a value it has not seen yet),
// any thread may latest() (returns the newest value immediately, no waiting).
template <typename T>
class Mailbox {
public:
    void put(T value)
    {
        {
            std::scoped_lock lock(mtx);
            slot = std::move(value);
            fresh = true;
        }
        new_value.notify_all();
    }

    // wait for a value not taken yet; returns false if stop was requested
    bool take(T& out, std::stop_token st)
    {
        std::unique_lock lock(mtx);
        if (!new_value.wait(lock, st, [this] { return fresh; }))
            return false;
        out = slot;
        fresh = false;
        return true;
    }

    T latest()
    {
        std::scoped_lock lock(mtx);
        return slot;
    }

private:
    std::mutex mtx;
    std::condition_variable_any new_value;
    T slot{};
    bool fresh = false;
};
