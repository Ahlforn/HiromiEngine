#pragma once

#include "ComponentTypeId.hpp"
#include <vector>
#include <algorithm>
#include <functional>

namespace hiromi {

// An Archetype is a sorted, canonical set of ComponentTypeIds that uniquely
// identifies "which components an entity has". {A,B} and {B,A} produce the
// same Archetype after construction.
class Archetype {
public:
    Archetype() = default;

    explicit Archetype(std::vector<ComponentTypeId> types) : types_(std::move(types)) {
        std::sort(types_.begin(), types_.end());
    }

    [[nodiscard]] bool has(ComponentTypeId id) const noexcept {
        return std::binary_search(types_.begin(), types_.end(), id);
    }

    [[nodiscard]] Archetype with(ComponentTypeId id) const {
        if (has(id)) return *this;
        auto v = types_;
        auto it = std::lower_bound(v.begin(), v.end(), id);
        v.insert(it, id);
        return Archetype(std::move(v));
    }

    [[nodiscard]] Archetype without(ComponentTypeId id) const {
        auto v = types_;
        auto it = std::lower_bound(v.begin(), v.end(), id);
        if (it != v.end() && *it == id) v.erase(it);
        return Archetype(std::move(v));
    }

    // Returns true if this archetype contains all types in other (superset check).
    [[nodiscard]] bool is_superset_of(const Archetype& other) const noexcept {
        return std::includes(types_.begin(), types_.end(),
                             other.types_.begin(), other.types_.end());
    }

    [[nodiscard]] const std::vector<ComponentTypeId>& types() const noexcept { return types_; }
    [[nodiscard]] std::size_t component_count() const noexcept { return types_.size(); }
    [[nodiscard]] bool empty() const noexcept { return types_.empty(); }

    bool operator==(const Archetype&) const noexcept = default;

    struct Hash {
        std::size_t operator()(const Archetype& a) const noexcept {
            // FNV-1a style hash over the sorted type ID vector.
            std::size_t h = 14695981039346656037ull;
            for (auto id : a.types_) {
                h ^= static_cast<std::size_t>(id);
                h *= 1099511628211ull;
            }
            return h;
        }
    };

private:
    std::vector<ComponentTypeId> types_; // always sorted ascending
};

} // namespace hiromi
