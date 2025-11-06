#pragma once
#include <memory>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <type_traits>

namespace pyplusplus {

// Forward declarations
class RefCounted;
class RefCountedBase;
template<typename T> class RefPtr;
template<typename T> class WeakRef;

// Reference counting base class
class RefCountedBase {
private:
    mutable std::atomic<int> ref_count{1};
    mutable std::atomic<int> weak_count{0};
    
    // Thread-local cache for performance optimization
    thread_local static int cached_ref_count;
    thread_local static const RefCountedBase* cached_object;
    
    friend class RefCounted;
    template<typename T> friend class RefPtr;
    template<typename T> friend class WeakRef;
    
protected:
    virtual ~RefCountedBase() = default;
    
    void addRef() const noexcept {
        ref_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void release() const noexcept {
        if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
    
    int getRefCount() const noexcept {
        return ref_count.load(std::memory_order_acquire);
    }
    
    void addWeakRef() const noexcept {
        weak_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void releaseWeak() const noexcept {
        if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // Last weak reference gone, can clean up
        }
    }
    
    int getWeakCount() const noexcept {
        return weak_count.load(std::memory_order_acquire);
    }
    
public:
    virtual void dispose() = 0;
};

// Mixin class for reference counted objects
class RefCounted : virtual public RefCountedBase {
public:
    virtual ~RefCounted() = default;
    
    // Reference counting interface
    void ref() const noexcept {
        addRef();
    }
    
    void unref() const noexcept {
        release();
    }
    
    int refCount() const noexcept {
        return getRefCount();
    }
    
    bool hasOneRef() const noexcept {
        return getRefCount() == 1;
    }
    
    // For debugging
    void dumpRefCount() const {
        printf("RefCount: %d, WeakCount: %d\n", getRefCount(), getWeakCount());
    }
};

// Smart pointer with reference counting
template<typename T>
class RefPtr {
private:
    T* ptr = nullptr;
    
    void addRef() const noexcept {
        if (ptr) {
            ptr->addRef();
        }
    }
    
    void release() const noexcept {
        if (ptr) {
            ptr->release();
        }
    }
    
public:
    // Constructors
    constexpr RefPtr() noexcept = default;
    
    constexpr RefPtr(std::nullptr_t) noexcept : ptr(nullptr) {}
    
    explicit RefPtr(T* p) noexcept : ptr(p) {
        if (ptr) {
            ptr->addRef();
        }
    }
    
    RefPtr(const RefPtr& other) noexcept : ptr(other.ptr) {
        addRef();
    }
    
    RefPtr(RefPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    template<typename U>
    RefPtr(const RefPtr<U>& other) noexcept : ptr(other.get()) {
        addRef();
    }
    
    template<typename U>
    RefPtr(RefPtr<U>&& other) noexcept : ptr(other.get()) {
        other.release();
    }
    
    // Destructor
    ~RefPtr() noexcept {
        release();
    }
    
    // Assignment operators
    RefPtr& operator=(const RefPtr& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            addRef();
        }
        return *this;
    }
    
    RefPtr& operator=(RefPtr&& other) noexcept {
        if (this != &other) {
            release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    RefPtr& operator=(std::nullptr_t) noexcept {
        release();
        ptr = nullptr;
        return *this;
    }
    
    template<typename U>
    RefPtr& operator=(const RefPtr<U>& other) noexcept {
        release();
        ptr = other.get();
        addRef();
        return *this;
    }
    
    template<typename U>
    RefPtr& operator=(RefPtr<U>&& other) noexcept {
        release();
        ptr = other.get();
        other.release();
        return *this;
    }
    
    // Access operators
    T& operator*() const noexcept {
        return *ptr;
    }
    
    T* operator->() const noexcept {
        return ptr;
    }
    
    T* get() const noexcept {
        return ptr;
    }
    
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
    
    // Comparison operators
    bool operator==(const RefPtr& other) const noexcept {
        return ptr == other.ptr;
    }
    
    bool operator!=(const RefPtr& other) const noexcept {
        return ptr != other.ptr;
    }
    
    bool operator<(const RefPtr& other) const noexcept {
        return ptr < other.ptr;
    }
    
    bool operator==(std::nullptr_t) const noexcept {
        return ptr == nullptr;
    }
    
    bool operator!=(std::nullptr_t) const noexcept {
        return ptr != nullptr;
    }
    
    // Utility methods
    void reset() noexcept {
        release();
        ptr = nullptr;
    }
    
    void reset(T* p) noexcept {
        release();
        ptr = p;
        if (ptr) {
            ptr->addRef();
        }
    }
    
    T* release() noexcept {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }
    
    void swap(RefPtr& other) noexcept {
        T* temp = ptr;
        ptr = other.ptr;
        other.ptr = temp;
    }
    
    // Reference count access
    int refCount() const noexcept {
        return ptr ? ptr->getRefCount() : 0;
    }
    
    bool hasOneRef() const noexcept {
        return ptr && ptr->hasOneRef();
    }
};

// Weak reference implementation
template<typename T>
class WeakRef {
private:
    T* ptr = nullptr;
    
    void addWeakRef() const noexcept {
        if (ptr) {
            ptr->addWeakRef();
        }
    }
    
    void releaseWeak() const noexcept {
        if (ptr) {
            ptr->releaseWeak();
        }
    }
    
public:
    constexpr WeakRef() noexcept = default;
    
    WeakRef(const RefPtr<T>& ref) noexcept : ptr(ref.get()) {
        addWeakRef();
    }
    
    WeakRef(const WeakRef& other) noexcept : ptr(other.ptr) {
        addWeakRef();
    }
    
    WeakRef(WeakRef&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    ~WeakRef() noexcept {
        releaseWeak();
    }
    
    WeakRef& operator=(const WeakRef& other) noexcept {
        if (this != &other) {
            releaseWeak();
            ptr = other.ptr;
            addWeakRef();
        }
        return *this;
    }
    
    WeakRef& operator=(WeakRef&& other) noexcept {
        if (this != &other) {
            releaseWeak();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    WeakRef& operator=(const RefPtr<T>& ref) noexcept {
        releaseWeak();
        ptr = ref.get();
        addWeakRef();
        return *this;
    }
    
    RefPtr<T> lock() const noexcept {
        if (ptr && ptr->getRefCount() > 0) {
            return RefPtr<T>(ptr);
        }
        return RefPtr<T>();
    }
    
    bool expired() const noexcept {
        return !ptr || ptr->getRefCount() == 0;
    }
    
    void reset() noexcept {
        releaseWeak();
        ptr = nullptr;
    }
    
    void swap(WeakRef& other) noexcept {
        T* temp = ptr;
        ptr = other.ptr;
        other.ptr = temp;
    }
};

// Reference counting utilities
namespace RefCountUtils {
    
    // Make RefPtr from raw pointer
    template<typename T, typename... Args>
    RefPtr<T> makeRef(Args&&... args) {
        return RefPtr<T>(new T(std::forward<Args>(args)...));
    }
    
    // Static cast for RefPtr
    template<typename T, typename U>
    RefPtr<T> static_pointer_cast(const RefPtr<U>& ptr) noexcept {
        return RefPtr<T>(static_cast<T*>(ptr.get()));
    }
    
    // Dynamic cast for RefPtr
    template<typename T, typename U>
    RefPtr<T> dynamic_pointer_cast(const RefPtr<U>& ptr) noexcept {
        T* casted = dynamic_cast<T*>(ptr.get());
        return casted ? RefPtr<T>(casted) : RefPtr<T>();
    }
    
    // Const cast for RefPtr
    template<typename T, typename U>
    RefPtr<T> const_pointer_cast(const RefPtr<U>& ptr) noexcept {
        return RefPtr<T>(const_cast<T*>(ptr.get()));
    }
    
    // Reinterpret cast for RefPtr
    template<typename T, typename U>
    RefPtr<T> reinterpret_pointer_cast(const RefPtr<U>& ptr) noexcept {
        return RefPtr<T>(reinterpret_cast<T*>(ptr.get()));
    }
}

// Reference counted container classes
template<typename T>
class RefCountedVector : public RefCounted {
private:
    std::vector<T> data;
    
public:
    RefCountedVector() = default;
    
    explicit RefCountedVector(size_t size) : data(size) {}
    
    RefCountedVector(size_t size, const T& value) : data(size, value) {}
    
    template<typename Iterator>
    RefCountedVector(Iterator first, Iterator last) : data(first, last) {}
    
    RefCountedVector(std::initializer_list<T> init) : data(init) {}
    
    // Vector interface
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
    
    T& at(size_t index) { return data.at(index); }
    const T& at(size_t index) const { return data.at(index); }
    
    T& front() { return data.front(); }
    const T& front() const { return data.front(); }
    
    T& back() { return data.back(); }
    const T& back() const { return data.back(); }
    
    T* data() { return data.data(); }
    const T* data() const { return data.data(); }
    
    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }
    size_t capacity() const { return data.capacity(); }
    
    void reserve(size_t new_capacity) { data.reserve(new_capacity); }
    void resize(size_t new_size) { data.resize(new_size); }
    void resize(size_t new_size, const T& value) { data.resize(new_size, value); }
    
    void push_back(const T& value) { data.push_back(value); }
    void push_back(T&& value) { data.push_back(std::move(value)); }
    
    template<typename... Args>
    void emplace_back(Args&&... args) {
        data.emplace_back(std::forward<Args>(args)...);
    }
    
    void pop_back() { data.pop_back(); }
    
    void clear() { data.clear(); }
    
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    auto cbegin() const { return data.cbegin(); }
    auto cend() const { return data.cend(); }
    
    void dispose() override {
        data.clear();
    }
};

template<typename K, typename V>
class RefCountedMap : public RefCounted {
private:
    std::unordered_map<K, V> data;
    
public:
    RefCountedMap() = default;
    
    // Map interface
    V& operator[](const K& key) { return data[key]; }
    V& at(const K& key) { return data.at(key); }
    const V& at(const K& key) const { return data.at(key); }
    
    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }
    
    void clear() { data.clear(); }
    
    bool contains(const K& key) const { return data.find(key) != data.end(); }
    
    auto find(const K& key) { return data.find(key); }
    auto find(const K& key) const { return data.find(key); }
    auto end() { return data.end(); }
    auto end() const { return data.end(); }
    
    void erase(const K& key) { data.erase(key); }
    
    template<typename... Args>
    std::pair<typename std::unordered_map<K, V>::iterator, bool> emplace(Args&&... args) {
        return data.emplace(std::forward<Args>(args)...);
    }
    
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    auto cbegin() const { return data.cbegin(); }
    auto cend() const { return data.cend(); }
    
    void dispose() override {
        data.clear();
    }
};

// Reference counted string
class RefCountedString : public RefCounted {
private:
    std::string data;
    
public:
    RefCountedString() = default;
    
    explicit RefCountedString(const std::string& str) : data(str) {}
    explicit RefCountedString(std::string&& str) : data(std::move(str)) {}
    
    RefCountedString(const char* str) : data(str) {}
    RefCountedString(const char* str, size_t len) : data(str, len) {}
    
    // String interface
    const char* c_str() const { return data.c_str(); }
    const std::string& str() const { return data; }
    
    size_t size() const { return data.size(); }
    size_t length() const { return data.length(); }
    bool empty() const { return data.empty(); }
    
    char operator[](size_t index) const { return data[index]; }
    
    bool operator==(const RefCountedString& other) const { return data == other.data; }
    bool operator!=(const RefCountedString& other) const { return data != other.data; }
    bool operator<(const RefCountedString& other) const { return data < other.data; }
    
    void dispose() override {
        data.clear();
    }
};

// Memory pool for reference counted objects
class RefCountedMemoryPool {
private:
    struct Block {
        void* memory;
        size_t size;
        bool in_use;
        Block* next;
    };
    
    Block* free_blocks = nullptr;
    std::vector<std::unique_ptr<Block>> allocated_blocks;
    std::mutex pool_mutex;
    size_t total_allocated = 0;
    size_t total_freed = 0;
    
public:
    RefCountedMemoryPool() = default;
    ~RefCountedMemoryPool() {
        clear();
    }
    
    void* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // Find suitable free block
        Block* block = findFreeBlock(size);
        if (!block) {
            block = allocateNewBlock(size);
        }
        
        if (block) {
            block->in_use = true;
            total_allocated += size;
            return block->memory;
        }
        
        return nullptr;
    }
    
    void deallocate(void* ptr, size_t size) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // Find the block and mark as free
        for (auto& allocated_block : allocated_blocks) {
            if (allocated_block->memory == ptr) {
                allocated_block->in_use = false;
                total_freed += size;
                
                // Add to free list
                allocated_block->next = free_blocks;
                free_blocks = allocated_block.get();
                break;
            }
        }
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        for (auto& block : allocated_blocks) {
            free(block->memory);
        }
        
        allocated_blocks.clear();
        free_blocks = nullptr;
        total_allocated = 0;
        total_freed = 0;
    }
    
    size_t getTotalAllocated() const { return total_allocated; }
    size_t getTotalFreed() const { return total_freed; }
    size_t getCurrentUsage() const { return total_allocated - total_freed; }
    
    // Performance optimizations
    void optimize() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        // Coalesce adjacent free blocks
        coalesceFreeBlocks();
        
        // Release unused memory back to system
        releaseUnusedMemory();
    }
    
    void preallocate(size_t size, size_t count) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        for (size_t i = 0; i < count; ++i) {
            Block* block = allocateNewBlock(size);
            if (block) {
                block->in_use = false;
                block->next = free_blocks;
                free_blocks = block;
            }
        }
    }
    
    // Memory statistics
    struct MemoryStats {
        size_t total_blocks;
        size_t free_blocks;
        size_t used_blocks;
        size_t fragmentation_ratio;
        size_t average_block_size;
    };
    
    MemoryStats getStats() const {
        std::lock_guard<std::mutex> lock(pool_mutex);
        
        MemoryStats stats{};
        stats.total_blocks = allocated_blocks.size();
        stats.average_block_size = stats.total_blocks > 0 ? total_allocated / stats.total_blocks : 0;
        
        size_t free_count = 0;
        Block* current = free_blocks;
        while (current) {
            free_count++;
            current = current->next;
        }
        
        stats.free_blocks = free_count;
        stats.used_blocks = stats.total_blocks - free_count;
        
        // Calculate fragmentation (simplified)
        if (stats.used_blocks > 0) {
            stats.fragmentation_ratio = (stats.free_blocks * 100) / stats.total_blocks;
        } else {
            stats.fragmentation_ratio = 0;
        }
        
        return stats;
    }
    
private:
    Block* findFreeBlock(size_t size) {
        Block* prev = nullptr;
        Block* current = free_blocks;
        
        while (current) {
            if (!current->in_use && current->size >= size) {
                // Remove from free list
                if (prev) {
                    prev->next = current->next;
                } else {
                    free_blocks = current->next;
                }
                return current;
            }
            prev = current;
            current = current->next;
        }
        
        return nullptr;
    }
    
    Block* allocateNewBlock(size_t size) {
        void* memory = std::malloc(size);
        if (!memory) {
            return nullptr;
        }
        
        auto block = std::make_unique<Block>();
        block->memory = memory;
        block->size = size;
        block->in_use = false;
        block->next = nullptr;
        
        Block* block_ptr = block.get();
        allocated_blocks.push_back(std::move(block));
        
        return block_ptr;
    }
    
    void coalesceFreeBlocks() {
        // Simple coalescing - merge adjacent free blocks
        // This is a simplified implementation
        for (auto& block : allocated_blocks) {
            if (!block->in_use) {
                // Try to find adjacent free blocks to merge
                // Implementation would depend on memory layout
            }
        }
    }
    
    void releaseUnusedMemory() {
        // Release completely unused blocks back to system
        auto it = allocated_blocks.begin();
        while (it != allocated_blocks.end()) {
            if (!(*it)->in_use && (*it)->size > 1024) { // Only release large blocks
                free((*it)->memory);
                it = allocated_blocks.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// Global memory pool
extern RefCountedMemoryPool g_ref_memory_pool;

// Custom allocator for reference counted objects
template<typename T>
class RefCountedAllocator {
public:
    using value_type = T;
    
    RefCountedAllocator() = default;
    
    template<typename U>
    RefCountedAllocator(const RefCountedAllocator<U>&) {}
    
    T* allocate(size_t n) {
        return static_cast<T*>(g_ref_memory_pool.allocate(n * sizeof(T)));
    }
    
    void deallocate(T* ptr, size_t n) {
        g_ref_memory_pool.deallocate(ptr, n * sizeof(T));
    }
    
    template<typename U>
    bool operator==(const RefCountedAllocator<U>&) const {
        return true;
    }
    
    template<typename U>
    bool operator!=(const RefCountedAllocator<U>&) const {
        return false;
    }
};

} // namespace pyplusplus