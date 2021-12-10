#pragma once
#include "array_ptr.h"

#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <cassert>

#include <iostream>

struct ReserveProxyObj{
    size_t capacity_= 0;

    ReserveProxyObj(size_t capacity_to_reserve)
        : capacity_(capacity_to_reserve)
    {
    }
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
    	: SimpleVector(size, Type())
    {
    }

    SimpleVector(ReserveProxyObj reserve_proxy_obj)
        : size_(0)
        , capacity_(reserve_proxy_obj.capacity_)
        , items_(reserve_proxy_obj.capacity_)
    {
    }

    SimpleVector(size_t size, const Type& value)
        : size_(size)
        , capacity_(size)
        , items_(size)
    {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init)
        : size_(init.size())
        , capacity_(init.size())
        , items_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector& other)
        : size_(other.size_)
        , capacity_(other.size_)
        , items_(other.size_)
    {
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector(SimpleVector&& other)
    	: size_(std::move(other.size_))
        , capacity_(std::move(other.size_))
        , items_(std::move(other.items_))
   {
    	other.size_ = 0;
    	other.capacity_ = 0;
   }

    SimpleVector& operator=(const SimpleVector& rhs)
    {
        if (this != &rhs){
            auto copy_rhs(rhs);
            this->swap(copy_rhs);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs)
    {
        if (this != &rhs){
        	auto copy_rhs(rhs);
        	this->swap(copy_rhs);
        }
        return *this;
    }


    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
       return GetSize() == 0;
    }

    void PushBack(const Type& item) {
    	if (size_ == capacity_){
    		const size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
            ArrayPtr<Type> new_items(new_capacity);

            std::move(begin(), end(), new_items.Get());

            std::swap(items_, new_items);

            capacity_ = new_capacity;

        }
        items_[size_] = item;
        ++size_;
    }

    void PushBack(Type&& item){
    	if (size_ == capacity_){
        	const size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
            ArrayPtr<Type> new_items(new_capacity);

            std::move(begin(), end(), new_items.Get());

            std::swap(items_, new_items);

            capacity_ = new_capacity;

        }
        items_[size_] = std::move(item);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
    	Iterator mutable_pos = begin() + (pos - cbegin());
    	const size_t insert_index = pos - begin();
    	if (size_ < capacity_){
        	std::move_backward(mutable_pos, end(), end() + 1);
        	items_[insert_index] = value;
        }else{
        	const size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
            ArrayPtr<Type> new_items(new_capacity);

            std::move(begin(), mutable_pos, new_items.Get());
            new_items[insert_index] = value;
            std::move(mutable_pos, end(), new_items.Get() + insert_index + 1);
            items_.swap(new_items);

            capacity_ = new_capacity;
        }
        ++size_;
    	return &items_[insert_index];
    }

    Iterator Insert(ConstIterator pos, Type&& value){
    	Iterator mutable_pos = begin() + (pos - cbegin());
    	const size_t insert_index = pos - cbegin();
        if (size_ < capacity_){
            std::move_backward(mutable_pos, end(), end() + 1);
            items_[insert_index] = std::move(value);
        }else{
        	const size_t new_capacity = std::max(static_cast<size_t>(1), capacity_ * 2);
            ArrayPtr<Type> new_items(new_capacity);

            std::move(begin(), mutable_pos, new_items.Get());
            new_items[insert_index] = std::move(value);
            std::move(mutable_pos, end(), new_items.Get() + insert_index + 1);

            items_.swap(new_items);

            capacity_ = new_capacity;
        }
        ++size_;
        return &items_[insert_index];
    }

    void PopBack() noexcept {
    	assert(size_ != 0);
    	--size_;
    }

    Iterator Erase(ConstIterator pos) {
        Iterator mutable_pos = begin() + (pos - cbegin());
        std::move(mutable_pos + 1, end(), mutable_pos);
        --size_;
        return mutable_pos;
    }

    void swap(SimpleVector& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        items_.swap(other.items_);
    }

    void swap(SimpleVector&& other) noexcept {
    	auto copy_other(other);
    	other.size_ = std::move(size_);
    	other.capacity_ = std::move(capacity_);
    	other.items_ = std::move(items_);
    	size_ = std::move(copy_other.size_);
    	capacity_ = std::move(copy_other.capacity_);
    	items_ = std::move(copy_other.items_);
    }

    void Reserve(size_t new_capacity){
        if (new_capacity > capacity_){
            ArrayPtr<Type> new_items(new_capacity);
            std::move(begin(), end(), new_items.Get());
            capacity_ = new_capacity;
            items_.swap(new_items);
        }
    }

    Type& operator[](size_t index) noexcept {
    	assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
    	assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_){
            throw std::out_of_range("index out of range");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_){
            throw std::out_of_range("index out of range");
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_){
            size_ = new_size;
        }else if (new_size <= capacity_){
        	size_ = new_size;
            for (size_t i = size_; i != new_size; ++i){
                items_[i] = Type();
            }
        }else{
            SimpleVector<Type> new_items(std::max(new_size, capacity_ * 2));
            std::move(begin(), end(), new_items.begin());
            std::fill(new_items.begin() + size_, new_items.end(), Type());

            items_.swap(new_items.items_);

            size_ = new_size;
            capacity_ = new_items.GetCapacity();
        }
    }

    Iterator begin() noexcept {
        return &items_.Get()[0];
    }

    Iterator end() noexcept {
        return &items_.Get()[size_];
    }

    ConstIterator begin() const noexcept {
        return cbegin();
    }

    ConstIterator end() const noexcept {
        return cend();
    }

    ConstIterator cbegin() const noexcept {
        return &items_.Get()[0];
    }

    ConstIterator cend() const noexcept {
        return &items_.Get()[size_];
    }
private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs)
        || (lhs.GetSize() == rhs.GetSize()
            && std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
   return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
