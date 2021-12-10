#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>
#include <exception>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) noexcept
    	: buffer_(std::move(other).buffer_)
    	, capacity_(std::move(other).capacity_)
    {
    	other.buffer_ = nullptr;
    	other.capacity_ = 0;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

public:
    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

public:
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory& operator=(RawMemory&& rhs) noexcept
    {
    	Swap(rhs);

    	return *this;
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

public:
    Vector() = default;

    explicit Vector(size_t size)
    	: data_(size)
    	, size_(size)
    {
    	std::uninitialized_value_construct_n(begin(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
    	std::uninitialized_copy_n(other.begin(), size_, begin());
    }

    Vector(Vector&& other) noexcept
    {
    	Swap(other);
    }

public:
    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
    	return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
    	return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
    	return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
    	return data_.GetAddress() + size_;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept{
        return data_.Capacity();
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
    	return *Emplace(end(), std::forward<Args>(args)...);
    }

    void Resize(size_t new_size){
    	if (new_size == size_) {
    		return;
    	}
    	if (new_size < size_) {
    		std::destroy_n(begin() + new_size, size_ - new_size);
    	}else{
    		Reserve(new_size);
    		std::uninitialized_value_construct_n(end(), new_size - size_);
    	}
		size_ = new_size;
    }

    void PushBack(const T& value){
    	EmplaceBack(value);
    }

    void PushBack(T&& value){
    	EmplaceBack(std::move(value));
    }

    void PopBack() noexcept{
    	assert(size_ != 0);
    	std::destroy_at(data_ + size_ - 1);
    	--size_;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }

        RawMemory<T> new_data(new_capacity);
        UninitializedMoveOrCopy(begin(), size_, new_data.GetAddress());
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    }

    void Swap(Vector& other) noexcept{
    	data_.Swap(other.data_);
    	std::swap(size_, other.size_);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
    	const auto before = pos - begin();
    	if (size_ == Capacity()){
    		const size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
    		RawMemory<T> new_data(new_capacity);
    		new (new_data + before) T(std::forward<Args>(args)...);

    		try{
    			UninitializedMoveOrCopy(begin(), before, new_data.GetAddress());
    		}catch(...){
    			std::destroy_at(new_data + before);
    			throw;
    		}

    		if (pos != cend()){
    			try{
    				UninitializedMoveOrCopy(begin() + before, size_ - before, new_data.GetAddress() + before + 1);
    			}catch(...){
    				std::destroy_n(new_data.GetAddress(), before + 1);
    				throw;
    			}
    		}

    		std::destroy_n(begin(), size_);
    		data_.Swap(new_data);
    	}else{
    		if (pos == cend()){
    			try{
        			new(end()) T(std::forward<Args>(args)...);
    			}catch(const std::exception& e){
    				throw e;
    			}
    		}else{
    			T t(std::forward<Args>(args)...);

    			try{
    				new(end()) T(std::move(*(end() - 1)));
    			}catch (const std::exception& e){
    				throw e;
    			}

    			try{
    				std::move_backward(begin() + before, end() - 1, end());
    			}catch (const std::exception& e){
    				std::destroy_at(end());
    				throw e;
    			}

    			try {
    				data_[before] = std::move(t);
    			}catch (const std::exception& e) {
    				std::move(begin() + before + 1, end(), begin() + before);
    				std::destroy_at(end());
    				throw e;
    			}
    		}
    	}

    	++size_;
    	return begin() + before;
    }

    iterator Erase(const_iterator pos){
    	assert(pos != end());
    	std::move(const_cast<iterator>(pos) + 1, end(), const_cast<iterator>(pos));
    	std::destroy_at(data_ + size_ - 1);
    	--size_;
    	return const_cast<iterator>(pos);
    }

    iterator Insert(const_iterator pos, const T& value){
    	return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value){
    	return Emplace(pos, std::move(value));
    }

public:
    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
            	Vector rhs_copy(rhs);
            	Swap(rhs_copy);
            } else {
            	if (rhs.size_ < size_){
            		std::copy(rhs.begin(), rhs.begin() + rhs.size_, begin());
            		std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
            	} else {
            		std::copy(rhs.begin(), rhs.begin() + size_, begin());
            		std::uninitialized_copy_n(rhs.begin() + size_, rhs.size_ - size_, end());
            	}
            	size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
		Swap(rhs);

		return *this;
	}

    ~Vector() {
    	std::destroy_n(data_.GetAddress(), size_);
    }

private:
	void UninitializedMoveOrCopy(T* from, size_t size, T* to) {
		if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
			std::uninitialized_move_n(from, size, to);
		} else {
			std::uninitialized_copy_n(from, size, to);
		}
	}

private:
	RawMemory<T> data_;
	size_t size_ = 0;
};
