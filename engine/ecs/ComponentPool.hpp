#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <cassert>

namespace hiromi {

// Type-erased, densely-packed array for ONE component type.
// All operations use byte offsets / row indices.
// T must be nothrow-move-constructible (enforced by ComponentPool::create<T>).
class ComponentPool {
public:
    // Factory: creates a pool for type T.
    template<typename T>
    static std::unique_ptr<ComponentPool> create() {
        static_assert(std::is_nothrow_move_constructible_v<T>,
            "All components must have noexcept move constructors.");

        auto pool = std::make_unique<ComponentPool>();
        pool->stride_ = sizeof(T);
        pool->align_  = alignof(T);
        pool->destructor_ = [](void* p) {
            static_cast<T*>(p)->~T();
        };
        pool->move_construct_ = [](void* dst, void* src) noexcept {
            new (dst) T(std::move(*static_cast<T*>(src)));
        };
        return pool;
    }

    ComponentPool() = default;
    ~ComponentPool() { clear(); deallocate(); }

    ComponentPool(const ComponentPool&)            = delete;
    ComponentPool& operator=(const ComponentPool&) = delete;

    // Appends a default-initialized slot. Caller must then placement-new T into it.
    // Returns pointer to the new slot.
    void* push_back_uninitialized() {
        if (size_ == cap_) grow();
        void* slot = at(size_);
        ++size_;
        return slot;
    }

    // Moves src_ptr (pointer to a live T) into slot `row`. src_ptr must outlive
    // this call but will be left in a valid-but-unspecified state.
    void move_into(std::size_t row, void* src_ptr) noexcept {
        assert(row < size_);
        move_construct_(at(row), src_ptr);
    }

    // O(1) removal: destructs row, moves last element into the vacated slot,
    // shrinks size. Caller is responsible for updating the displaced entity's
    // row index in the World.
    void swap_remove(std::size_t row) {
        assert(row < size_);
        destructor_(at(row));
        if (row != size_ - 1) {
            // Move last element into vacated slot.
            move_construct_(at(row), at(size_ - 1));
            destructor_(at(size_ - 1));
        }
        --size_;
    }

    [[nodiscard]] void*       at(std::size_t row)       noexcept { return data_ + row * stride_; }
    [[nodiscard]] const void* at(std::size_t row) const noexcept { return data_ + row * stride_; }

    [[nodiscard]] std::size_t size()   const noexcept { return size_;   }
    [[nodiscard]] std::size_t stride() const noexcept { return stride_; }
    [[nodiscard]] std::size_t align()  const noexcept { return align_;  }

    void clear() {
        for (std::size_t i = 0; i < size_; ++i) destructor_(at(i));
        size_ = 0;
    }

private:
    void grow() {
        std::size_t new_cap = (cap_ == 0) ? 8 : cap_ * 2;
        std::byte*  new_data = static_cast<std::byte*>(
            ::operator new(new_cap * stride_, std::align_val_t{align_}));

        for (std::size_t i = 0; i < size_; ++i) {
            move_construct_(new_data + i * stride_, at(i));
            destructor_(at(i));
        }

        deallocate();
        data_ = new_data;
        cap_  = new_cap;
    }

    void deallocate() {
        if (data_) {
            ::operator delete(data_, std::align_val_t{align_});
            data_ = nullptr;
        }
    }

    std::byte*  data_    = nullptr;
    std::size_t size_    = 0;
    std::size_t cap_     = 0;
    std::size_t stride_  = 0;
    std::size_t align_   = alignof(std::max_align_t);

    std::function<void(void*)>        destructor_;
    std::function<void(void*, void*)> move_construct_; // (dst, src) noexcept
};

} // namespace hiromi
