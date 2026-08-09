#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <cassert>

// ============================================================================
// GENERIC OBJECT POOL - Professional Memory Management
// ============================================================================
// Thread-safe, high-performance object pool for any type
// Eliminates Instantiate/Destroy overhead
// Auto-expands when needed, pre-warms on init
// Stats tracking for debugging
// ============================================================================

template<typename T>
class ObjectPool {
public:
    using FactoryFunc = std::function<T*()>;
    using ResetFunc = std::function<void(T*)>;
    using DestroyFunc = std::function<void(T*)>;

    struct Stats {
        size_t totalCreated = 0;
        size_t currentActive = 0;
        size_t currentPooled = 0;
        size_t peakActive = 0;
        size_t timesExpanded = 0;
    };

private:
    // Pool storage
    std::queue<T*> available_;
    std::vector<T*> allObjects_;  // Track all for cleanup

    // Factories
    FactoryFunc factory_;
    ResetFunc reset_;
    DestroyFunc destroy_;

    // Configuration
    size_t initialSize_;
    size_t expandSize_;
    size_t maxSize_;

    // Stats
    Stats stats_;

    // Thread safety
    mutable std::mutex mutex_;

    // Helper: Create new object
    T* createObject() {
        T* obj = factory_();
        if (obj) {
            allObjects_.push_back(obj);
            stats_.totalCreated++;
        }
        return obj;
    }

    // Helper: Expand pool
    void expand() {
        if (maxSize_ > 0 && stats_.totalCreated >= maxSize_) {
            return;  // Hit max capacity
        }

        size_t toCreate = expandSize_;
        if (maxSize_ > 0) {
            size_t remaining = maxSize_ - stats_.totalCreated;
            toCreate = (toCreate < remaining) ? toCreate : remaining;
        }

        for (size_t i = 0; i < toCreate; ++i) {
            T* obj = createObject();
            if (obj) {
                available_.push(obj);
            }
        }

        stats_.timesExpanded++;
    }

public:
    // Constructor
    ObjectPool(FactoryFunc factory,
               ResetFunc reset = nullptr,
               DestroyFunc destroy = nullptr,
               size_t initialSize = 100,
               size_t expandSize = 50,
               size_t maxSize = 0)  // 0 = unlimited
        : factory_(factory)
        , reset_(reset)
        , destroy_(destroy)
        , initialSize_(initialSize)
        , expandSize_(expandSize)
        , maxSize_(maxSize)
    {
        assert(factory_ && "Factory function cannot be null");

        // Pre-warm pool
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < initialSize_; ++i) {
            T* obj = createObject();
            if (obj) {
                available_.push(obj);
            }
        }
    }

    // Destructor
    ~ObjectPool() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Cleanup all objects
        for (T* obj : allObjects_) {
            if (destroy_) {
                destroy_(obj);
            } else {
                delete obj;
            }
        }

        allObjects_.clear();
        while (!available_.empty()) {
            available_.pop();
        }
    }

    // Get object from pool (or create if empty)
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        // If pool empty, expand
        if (available_.empty()) {
            expand();
        }

        // Still empty? Return nullptr (hit max capacity)
        if (available_.empty()) {
            return nullptr;
        }

        // Get from pool
        T* obj = available_.front();
        available_.pop();

        stats_.currentActive++;
        stats_.currentPooled--;

        if (stats_.currentActive > stats_.peakActive) {
            stats_.peakActive = stats_.currentActive;
        }

        return obj;
    }

    // Return object to pool
    void release(T* obj) {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(mutex_);

        // Reset object state
        if (reset_) {
            reset_(obj);
        }

        // Return to pool
        available_.push(obj);

        stats_.currentActive--;
        stats_.currentPooled++;
    }

    // Get stats (thread-safe)
    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats copy = stats_;
        copy.currentPooled = available_.size();
        return copy;
    }

    // Clear pool (returns all objects to available)
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Move all to available (assumes no active objects)
        stats_.currentActive = 0;
        stats_.currentPooled = available_.size();
    }

    // Disable copy/move
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;
};

// ============================================================================
// SCOPED POOLED OBJECT - RAII wrapper
// ============================================================================

template<typename T>
class ScopedPooledObject {
private:
    T* object_;
    ObjectPool<T>* pool_;

public:
    ScopedPooledObject(ObjectPool<T>* pool)
        : object_(pool->acquire())
        , pool_(pool)
    {
    }

    ~ScopedPooledObject() {
        if (object_ && pool_) {
            pool_->release(object_);
        }
    }

    T* get() const { return object_; }
    T* operator->() const { return object_; }
    T& operator*() const { return *object_; }

    explicit operator bool() const { return object_ != nullptr; }

    // Disable copy/move
    ScopedPooledObject(const ScopedPooledObject&) = delete;
    ScopedPooledObject& operator=(const ScopedPooledObject&) = delete;
};
