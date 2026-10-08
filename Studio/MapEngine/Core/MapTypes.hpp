#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace fantasy::studio::mapcore {

// Heap-backed standard-like containers keep the hot Tile/Item structs compact.
// Empty maps/vectors cost one pointer instead of embedding the full STL object,
// while copy operations stay deep so Undo/Redo snapshots never alias state.
template <typename Key, typename Value, typename Compare = std::less<Key>>
class LazyMap {
public:
    using Map = std::map<Key, Value, Compare>;
    using key_type = Key;
    using mapped_type = Value;
    using value_type = typename Map::value_type;
    using size_type = typename Map::size_type;
    using iterator = typename Map::iterator;
    using const_iterator = typename Map::const_iterator;

    LazyMap() = default;
    LazyMap(const LazyMap& other) {
        if (other.storage_) storage_ = std::make_unique<Map>(*other.storage_);
    }
    LazyMap(LazyMap&&) noexcept = default;

    LazyMap& operator=(const LazyMap& other) {
        if (this == &other) return *this;
        storage_ = other.storage_ ? std::make_unique<Map>(*other.storage_) : nullptr;
        return *this;
    }
    LazyMap& operator=(LazyMap&&) noexcept = default;

    [[nodiscard]] bool empty() const noexcept { return !storage_ || storage_->empty(); }
    [[nodiscard]] size_type size() const noexcept { return storage_ ? storage_->size() : 0U; }

    Value& operator[](const Key& key) { return ensure()[key]; }
    Value& operator[](Key&& key) { return ensure()[std::move(key)]; }

    Value& at(const Key& key) { return ensure().at(key); }
    const Value& at(const Key& key) const { return view().at(key); }

    iterator begin() { return ensure().begin(); }
    iterator end() { return ensure().end(); }
    const_iterator begin() const noexcept { return view().begin(); }
    const_iterator end() const noexcept { return view().end(); }
    const_iterator cbegin() const noexcept { return view().cbegin(); }
    const_iterator cend() const noexcept { return view().cend(); }

    iterator find(const Key& key) { return ensure().find(key); }
    const_iterator find(const Key& key) const noexcept { return view().find(key); }
    [[nodiscard]] bool contains(const Key& key) const { return view().find(key) != view().end(); }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const Key& key, M&& value) {
        return ensure().insert_or_assign(key, std::forward<M>(value));
    }

    template <typename M>
    std::pair<iterator, bool> insert_or_assign(Key&& key, M&& value) {
        return ensure().insert_or_assign(std::move(key), std::forward<M>(value));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return ensure().emplace(std::forward<Args>(args)...);
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& value) {
        return ensure().insert(std::forward<P>(value));
    }

    size_type erase(const Key& key) {
        if (!storage_) return 0U;
        const auto erased = storage_->erase(key);
        releaseIfEmpty();
        return erased;
    }

    iterator erase(iterator position) {
        auto& map = ensure();
        const auto next = map.erase(position);
        releaseIfEmpty();
        return next;
    }

    void clear() noexcept { storage_.reset(); }

    bool operator==(const LazyMap& other) const { return view() == other.view(); }

private:
    [[nodiscard]] static const Map& emptyMap() noexcept {
        static const Map empty;
        return empty;
    }

    [[nodiscard]] const Map& view() const noexcept {
        return storage_ ? *storage_ : emptyMap();
    }

    Map& ensure() {
        if (!storage_) storage_ = std::make_unique<Map>();
        return *storage_;
    }

    void releaseIfEmpty() {
        if (storage_ && storage_->empty()) storage_.reset();
    }

    std::unique_ptr<Map> storage_;
};

template <typename T>
class LazyVector {
public:
    using Vector = std::vector<T>;
    using value_type = T;
    using size_type = typename Vector::size_type;
    using iterator = typename Vector::iterator;
    using const_iterator = typename Vector::const_iterator;

    LazyVector() = default;
    LazyVector(std::initializer_list<T> values) {
        if (values.size() != 0U) storage_ = std::make_unique<Vector>(values);
    }
    LazyVector(const LazyVector& other) {
        if (other.storage_) storage_ = std::make_unique<Vector>(*other.storage_);
    }
    LazyVector(LazyVector&&) noexcept = default;

    LazyVector& operator=(const LazyVector& other) {
        if (this == &other) return *this;
        storage_ = other.storage_ ? std::make_unique<Vector>(*other.storage_) : nullptr;
        return *this;
    }
    LazyVector& operator=(LazyVector&&) noexcept = default;

    [[nodiscard]] bool empty() const noexcept { return !storage_ || storage_->empty(); }
    [[nodiscard]] size_type size() const noexcept { return storage_ ? storage_->size() : 0U; }
    [[nodiscard]] size_type capacity() const noexcept { return storage_ ? storage_->capacity() : 0U; }

    T& operator[](size_type index) { return (*storage_)[index]; }
    const T& operator[](size_type index) const { return (*storage_)[index]; }
    T& at(size_type index) { return ensure().at(index); }
    const T& at(size_type index) const { return view().at(index); }
    T& front() { return ensure().front(); }
    const T& front() const { return view().front(); }
    T& back() { return ensure().back(); }
    const T& back() const { return view().back(); }

    iterator begin() { return ensure().begin(); }
    iterator end() { return ensure().end(); }
    const_iterator begin() const noexcept { return view().begin(); }
    const_iterator end() const noexcept { return view().end(); }
    const_iterator cbegin() const noexcept { return view().cbegin(); }
    const_iterator cend() const noexcept { return view().cend(); }

    void reserve(size_type count) {
        if (count != 0U) ensure().reserve(count);
    }

    void resize(size_type count) {
        if (count == 0U) {
            storage_.reset();
            return;
        }
        ensure().resize(count);
    }

    void push_back(const T& value) { ensure().push_back(value); }
    void push_back(T&& value) { ensure().push_back(std::move(value)); }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        return ensure().emplace_back(std::forward<Args>(args)...);
    }

    iterator insert(iterator position, const T& value) {
        return ensure().insert(position, value);
    }
    iterator insert(iterator position, T&& value) {
        return ensure().insert(position, std::move(value));
    }
    template <typename InputIt>
    iterator insert(iterator position, InputIt first, InputIt last) {
        return ensure().insert(position, first, last);
    }

    iterator erase(iterator position) {
        auto& vector = ensure();
        const auto next = vector.erase(position);
        releaseIfEmpty();
        return next;
    }
    iterator erase(iterator first, iterator last) {
        auto& vector = ensure();
        const auto next = vector.erase(first, last);
        releaseIfEmpty();
        return next;
    }

    void pop_back() {
        auto& vector = ensure();
        vector.pop_back();
        releaseIfEmpty();
    }

    void clear() noexcept { storage_.reset(); }

    bool operator==(const LazyVector& other) const { return view() == other.view(); }

private:
    [[nodiscard]] static const Vector& emptyVector() noexcept {
        static const Vector empty;
        return empty;
    }

    [[nodiscard]] const Vector& view() const noexcept {
        return storage_ ? *storage_ : emptyVector();
    }

    Vector& ensure() {
        if (!storage_) storage_ = std::make_unique<Vector>();
        return *storage_;
    }

    void releaseIfEmpty() {
        if (storage_ && storage_->empty()) storage_.reset();
    }

    std::unique_ptr<Vector> storage_;
};

template <typename T>
class OptionalBox {
public:
    OptionalBox() = default;
    OptionalBox(std::nullopt_t) noexcept {}
    OptionalBox(const T& value) : value_(std::make_unique<T>(value)) {}
    OptionalBox(T&& value) : value_(std::make_unique<T>(std::move(value))) {}
    OptionalBox(const OptionalBox& other) {
        if (other.value_) value_ = std::make_unique<T>(*other.value_);
    }
    OptionalBox(OptionalBox&&) noexcept = default;

    OptionalBox& operator=(const OptionalBox& other) {
        if (this == &other) return *this;
        value_ = other.value_ ? std::make_unique<T>(*other.value_) : nullptr;
        return *this;
    }
    OptionalBox& operator=(OptionalBox&&) noexcept = default;
    OptionalBox& operator=(std::nullopt_t) noexcept {
        value_.reset();
        return *this;
    }
    OptionalBox& operator=(const T& value) {
        if (value_) *value_ = value;
        else value_ = std::make_unique<T>(value);
        return *this;
    }
    OptionalBox& operator=(T&& value) {
        if (value_) *value_ = std::move(value);
        else value_ = std::make_unique<T>(std::move(value));
        return *this;
    }

    [[nodiscard]] bool has_value() const noexcept { return static_cast<bool>(value_); }
    explicit operator bool() const noexcept { return has_value(); }

    T& operator*() { return *value_; }
    const T& operator*() const { return *value_; }
    T* operator->() { return value_.get(); }
    const T* operator->() const { return value_.get(); }

    T& value() { return *value_; }
    const T& value() const { return *value_; }

    template <typename U>
    T value_or(U&& fallback) const {
        return value_ ? *value_ : static_cast<T>(std::forward<U>(fallback));
    }

    template <typename... Args>
    T& emplace(Args&&... args) {
        value_ = std::make_unique<T>(std::forward<Args>(args)...);
        return *value_;
    }

    void reset() noexcept { value_.reset(); }

    bool operator==(const OptionalBox& other) const {
        if (has_value() != other.has_value()) return false;
        return !value_ || *value_ == *other.value_;
    }

private:
    std::unique_ptr<T> value_;
};

struct Position {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 7;
    bool operator==(const Position&) const = default;
    auto operator<=>(const Position&) const = default;
};

using AttributeValue = std::variant<std::int64_t, std::string, Position>;
using AttributeMap = LazyMap<std::string, AttributeValue>;

struct Item {
    std::uint32_t serverId = 0;
    std::uint32_t clientId = 0;
    std::uint16_t countOrSubtype = 1;
    AttributeMap attributes;
    LazyVector<Item> contents;
    bool operator==(const Item&) const = default;
};

struct CreaturePlacement {
    std::string name;
    std::uint32_t lookType = 0;
    std::uint8_t direction = 2;
    bool operator==(const CreaturePlacement&) const = default;
};

struct SpawnPlacement {
    std::uint32_t radius = 1;
    std::uint32_t intervalSeconds = 60;
    bool operator==(const SpawnPlacement&) const = default;
};

struct Tile {
    Position position;
    std::optional<Item> ground;
    LazyVector<Item> items;
    AttributeMap attributes;
    OptionalBox<CreaturePlacement> creature;
    OptionalBox<SpawnPlacement> spawn;
    std::uint32_t houseId = 0;
    std::uint32_t flags = 0;

    [[nodiscard]] bool empty() const noexcept {
        return !ground.has_value() && items.empty() && attributes.empty() && !creature.has_value() && !spawn.has_value() && houseId == 0 && flags == 0;
    }

    bool operator==(const Tile&) const = default;
};

struct Town {
    std::uint32_t id = 0;
    std::string name;
    Position templePosition;
    bool operator==(const Town&) const = default;
};

struct House {
    std::uint32_t id = 0;
    std::string name;
    Position exit;
    std::uint32_t rent = 0;
    std::uint32_t townId = 0;
    bool guildhall = false;
    bool operator==(const House&) const = default;
};

struct Waypoint {
    std::string name;
    Position position;
    bool operator==(const Waypoint&) const = default;
};

enum class SpawnEntryKind {
    Monster,
    Npc,
    MonsterSet,
};

struct SpawnMonsterOption {
    std::string name;
    std::uint16_t chance = 0;
    bool operator==(const SpawnMonsterOption&) const = default;
};

struct SpawnEntry {
    SpawnEntryKind kind = SpawnEntryKind::Monster;
    Position position;
    std::string name;
    std::uint16_t direction = 0;
    std::uint32_t intervalSeconds = 0;
    std::vector<SpawnMonsterOption> monsters;
    bool operator==(const SpawnEntry&) const = default;
};

struct SpawnArea {
    Position center;
    std::int32_t radius = 0;
    std::vector<SpawnEntry> entries;
    bool operator==(const SpawnArea&) const = default;
};

struct MapMetadata {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::string name;
    std::string description;
    std::string spawnFile;
    std::string houseFile;
    std::string sourceProfileId;
    bool operator==(const MapMetadata&) const = default;
};

} // namespace fantasy::studio::mapcore
