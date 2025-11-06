#include "refcount.h"
#include <iostream>

namespace pyplusplus {

// Global memory pool instance
RefCountedMemoryPool g_ref_memory_pool;

// RefCountedMemoryPool implementation
void* RefCountedMemoryPool::allocate(size_t size) {
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

void RefCountedMemoryPool::deallocate(void* ptr, size_t size) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    
    // Find block and mark as free
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

void RefCountedMemoryPool::clear() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    
    for (auto& block : allocated_blocks) {
        std::free(block->memory);
    }
    
    allocated_blocks.clear();
    free_blocks = nullptr;
    total_allocated = 0;
    total_freed = 0;
}

RefCountedMemoryPool::Block* RefCountedMemoryPool::findFreeBlock(size_t size) {
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

RefCountedMemoryPool::Block* RefCountedMemoryPool::allocateNewBlock(size_t size) {
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

// Reference counting debugging utilities
namespace RefCountDebug {
    
    struct RefCountStats {
        std::atomic<size_t> total_objects{0};
        std::atomic<size_t> total_refs{0};
        std::atomic<size_t> peak_objects{0};
        std::atomic<size_t> peak_refs{0};
    };
    
    static RefCountStats g_stats;
    static std::mutex g_stats_mutex;
    
    void trackObjectCreation() {
        g_stats.total_objects++;
        size_t current = g_stats.total_objects.load();
        size_t peak = g_stats.peak_objects.load();
        while (current > peak && !g_stats.peak_objects.compare_exchange_weak(peak, current)) {
            peak = g_stats.peak_objects.load();
        }
    }
    
    void trackObjectDestruction() {
        g_stats.total_objects--;
    }
    
    void trackRefIncrement() {
        g_stats.total_refs++;
        size_t current = g_stats.total_refs.load();
        size_t peak = g_stats.peak_refs.load();
        while (current > peak && !g_stats.peak_refs.compare_exchange_weak(peak, current)) {
            peak = g_stats.peak_refs.load();
        }
    }
    
    void trackRefDecrement() {
        g_stats.total_refs--;
    }
    
    RefCountStats getStats() {
        return g_stats;
    }
    
    void printStats() {
        RefCountStats stats = getStats();
        std::cout << "Reference Counting Statistics:\n";
        std::cout << "  Total objects: " << stats.total_objects.load() << "\n";
        std::cout << "  Total references: " << stats.total_refs.load() << "\n";
        std::cout << "  Peak objects: " << stats.peak_objects.load() << "\n";
        std::cout << "  Peak references: " << stats.peak_refs.load() << "\n";
    }
    
    void resetStats() {
        g_stats.total_objects = 0;
        g_stats.total_refs = 0;
        g_stats.peak_objects = 0;
        g_stats.peak_refs = 0;
    }
}

// Enhanced RefCountedBase with debugging
RefCountedBase::~RefCountedBase() {
    RefCountDebug::trackObjectDestruction();
}

void RefCountedBase::addRef() const noexcept {
    ref_count.fetch_add(1, std::memory_order_relaxed);
    RefCountDebug::trackRefIncrement();
}

void RefCountedBase::release() const noexcept {
    RefCountDebug::trackRefDecrement();
    if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        RefCountDebug::trackObjectDestruction();
        delete this;
    }
}

// Reference counting utilities implementation
namespace RefCountUtils {
    
    // Specialized makeRef for RefCounted types
    template<typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<RefCounted, T>, RefPtr<T>> makeRef(Args&&... args) {
        auto* ptr = new T(std::forward<Args>(args)...);
        RefCountDebug::trackObjectCreation();
        return RefPtr<T>(ptr);
    }
    
    // Specialized makeRef for non-RefCounted types (wrap them)
    template<typename T, typename... Args>
    std::enable_if_t<!std::is_base_of_v<RefCounted, T>, RefPtr<T>> makeRef(Args&&... args) {
        // Create a wrapper that inherits from RefCounted
        struct RefCountedWrapper : public T, public RefCounted {
            RefCountedWrapper(Args&&... args) : T(std::forward<Args>(args)...) {}
            void dispose() override {}
        };
        
        auto* ptr = new RefCountedWrapper(std::forward<Args>(args)...);
        RefCountDebug::trackObjectCreation();
        return RefPtr<T>(static_cast<T*>(ptr));
    }
}

// Memory leak detection
class MemoryLeakDetector {
private:
    std::unordered_set<const void*> tracked_objects;
    std::mutex detector_mutex;
    
public:
    static MemoryLeakDetector& instance() {
        static MemoryLeakDetector detector;
        return detector;
    }
    
    void trackObject(const void* ptr) {
        std::lock_guard<std::mutex> lock(detector_mutex);
        tracked_objects.insert(ptr);
    }
    
    void untrackObject(const void* ptr) {
        std::lock_guard<std::mutex> lock(detector_mutex);
        tracked_objects.erase(ptr);
    }
    
    void reportLeaks() {
        std::lock_guard<std::mutex> lock(detector_mutex);
        
        if (!tracked_objects.empty()) {
            std::cout << "Memory leak detected! " << tracked_objects.size() << " objects leaked:\n";
            for (const void* ptr : tracked_objects) {
                std::cout << "  Leaked object at: " << ptr << "\n";
            }
        } else {
            std::cout << "No memory leaks detected.\n";
        }
    }
    
    size_t getLeakedCount() const {
        std::lock_guard<std::mutex> lock(detector_mutex);
        return tracked_objects.size();
    }
};

// Reference counting performance profiler
class RefCountProfiler {
private:
    struct ProfileData {
        std::atomic<uint64_t> add_ref_calls{0};
        std::atomic<uint64_t> release_calls{0};
        std::atomic<uint64_t> total_add_ref_time_ns{0};
        std::atomic<uint64_t> total_release_time_ns{0};
    };
    
    static ProfileData g_profile_data;
    
public:
    static void profileAddRef(uint64_t time_ns) {
        g_profile_data.add_ref_calls++;
        g_profile_data.total_add_ref_time_ns += time_ns;
    }
    
    static void profileRelease(uint64_t time_ns) {
        g_profile_data.release_calls++;
        g_profile_data.total_release_time_ns += time_ns;
    }
    
    static ProfileData getProfileData() {
        return g_profile_data;
    }
    
    static void printProfileData() {
        ProfileData data = getProfileData();
        
        std::cout << "Reference Counting Profile:\n";
        std::cout << "  add_ref calls: " << data.add_ref_calls.load() << "\n";
        std::cout << "  release calls: " << data.release_calls.load() << "\n";
        
        if (data.add_ref_calls.load() > 0) {
            uint64_t avg_add_ref = data.total_add_ref_time_ns.load() / data.add_ref_calls.load();
            std::cout << "  avg add_ref time: " << avg_add_ref << " ns\n";
        }
        
        if (data.release_calls.load() > 0) {
            uint64_t avg_release = data.total_release_time_ns.load() / data.release_calls.load();
            std::cout << "  avg release time: " << avg_release << " ns\n";
        }
    }
    
    static void resetProfileData() {
        g_profile_data = ProfileData{};
    }
};

// Profile data definition
RefCountProfiler::ProfileData RefCountProfiler::g_profile_data;

// Enhanced RefCountedBase with profiling
void RefCountedBase::addRef() const noexcept {
    auto start = std::chrono::high_resolution_clock::now();
    
    ref_count.fetch_add(1, std::memory_order_relaxed);
    RefCountDebug::trackRefIncrement();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    RefCountProfiler::profileAddRef(duration.count());
}

void RefCountedBase::release() const noexcept {
    auto start = std::chrono::high_resolution_clock::now();
    
    RefCountDebug::trackRefDecrement();
    if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        RefCountDebug::trackObjectDestruction();
        delete this;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    RefCountProfiler::profileRelease(duration.count());
}

// Thread-safe reference counting validation
class RefCountValidator {
private:
    std::unordered_map<const void*, int> validation_map;
    std::mutex validator_mutex;
    
public:
    static RefCountValidator& instance() {
        static RefCountValidator validator;
        return validator;
    }
    
    void validateAddRef(const void* ptr) {
        std::lock_guard<std::mutex> lock(validator_mutex);
        validation_map[ptr]++;
    }
    
    void validateRelease(const void* ptr) {
        std::lock_guard<std::mutex> lock(validator_mutex);
        auto it = validation_map.find(ptr);
        if (it != validation_map.end()) {
            it->second--;
            if (it->second <= 0) {
                validation_map.erase(it);
            }
        }
    }
    
    bool isValid(const void* ptr) const {
        std::lock_guard<std::mutex> lock(validator_mutex);
        auto it = validation_map.find(ptr);
        return it != validation_map.end() && it->second > 0;
    }
    
    void reportInvalidReferences() {
        std::lock_guard<std::mutex> lock(validator_mutex);
        
        for (const auto& [ptr, count] : validation_map) {
            if (count < 0) {
                std::cout << "Invalid reference count for object " << ptr << ": " << count << "\n";
            }
        }
    }
};

// Reference counting benchmarks
class RefCountBenchmark {
public:
    static void benchmarkRefPtrOperations(size_t iterations = 1000000) {
        std::cout << "Reference Counting Benchmark (" << iterations << " iterations):\n";
        
        // Benchmark RefPtr creation and destruction
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            RefPtr<RefCountedString> str = RefCountUtils::makeRef<RefCountedString>("test");
            // Use the string
            volatile size_t len = str->size();
            (void)len; // Prevent optimization
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  RefPtr create/destroy: " << duration.count() << " ms\n";
        std::cout << "  Average per operation: " << (duration.count() * 1000000.0 / iterations) << " ns\n";
        
        // Benchmark RefPtr copying
        RefPtr<RefCountedString> original = RefCountUtils::makeRef<RefCountedString>("benchmark");
        
        start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            RefPtr<RefCountedString> copy = original;
            volatile size_t len = copy->size();
            (void)len; // Prevent optimization
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  RefPtr copy: " << duration.count() << " ms\n";
        std::cout << "  Average per copy: " << (duration.count() * 1000000.0 / iterations) << " ns\n";
        
        // Benchmark RefPtr assignment
        RefPtr<RefCountedString> assign1 = RefCountUtils::makeRef<RefCountedString>("assign1");
        RefPtr<RefCountedString> assign2 = RefCountUtils::makeRef<RefCountedString>("assign2");
        
        start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            assign1 = assign2;
            assign2 = assign1;
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  RefPtr assignment: " << duration.count() << " ms\n";
        std::cout << "  Average per assignment: " << (duration.count() * 1000000.0 / iterations) << " ns\n";
    }
    
    static void benchmarkMemoryPool(size_t allocations = 100000) {
        std::cout << "Memory Pool Benchmark (" << allocations << " allocations):\n";
        
        // Benchmark pool allocation
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> ptrs;
        ptrs.reserve(allocations);
        
        for (size_t i = 0; i < allocations; ++i) {
            void* ptr = g_ref_memory_pool.allocate(1024);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Pool allocation: " << duration.count() << " ms\n";
        std::cout << "  Average per allocation: " << (duration.count() * 1000000.0 / allocations) << " ns\n";
        
        // Benchmark pool deallocation
        start = std::chrono::high_resolution_clock::now();
        
        for (void* ptr : ptrs) {
            g_ref_memory_pool.deallocate(ptr, 1024);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  Pool deallocation: " << duration.count() << " ms\n";
        std::cout << "  Average per deallocation: " << (duration.count() * 1000000.0 / allocations) << " ns\n";
        
        // Compare with malloc/free
        start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> malloc_ptrs;
        malloc_ptrs.reserve(allocations);
        
        for (size_t i = 0; i < allocations; ++i) {
            void* ptr = std::malloc(1024);
            if (ptr) {
                malloc_ptrs.push_back(ptr);
            }
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  malloc allocation: " << duration.count() << " ms\n";
        
        start = std::chrono::high_resolution_clock::now();
        
        for (void* ptr : malloc_ptrs) {
            std::free(ptr);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  free deallocation: " << duration.count() << " ms\n";
    }
};

} // namespace pyplusplus