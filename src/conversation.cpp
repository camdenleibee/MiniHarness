// src/conversation.cpp

#include "core/conversation.h"
#include <stdexcept>
#include <utility>


//------------------------------------------------------------------------
// Copy Constructor
// Deep copy: allocates its own buffer, and copies every element individually
Conversation::Conversation(const Conversation& other)
    : size_(other.size_), capacity_(other.capacity_){
    data_ = new Message[capacity_];
    for (std::size_t i = 0; i < size_; ++i){
        data_[i] = other.data_[i];
    }

}

//------------------------------------------------------------------------
// Move Constructor
// Steal other's buffer with no allocation and no per-element copy.
// Other is left empty and safe to destroy
Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}


//------------------------------------------------------------------------
// Copy Assignment Operator 
// Deep-copy like the copy constructor, but reassigns into an existing object.
// Gaurds from self assignment
Conversation& Conversation::operator=(const Conversation& other) {
    if (this == &other) return *this;
    delete[] data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    data_ = new Message[capacity_];
    for (std::size_t i = 0; i < size_; ++i) data_[i] = other.data_[i];
    return *this;
}


//------------------------------------------------------------------------
// Move Assignment operator
// Frees the original object's buffer
// then steals the other's buffer and zeroes out size and capacity
Conversation& Conversation::operator=(Conversation&& other) noexcept {
    if (this == &other) return *this;
    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;
}


//------------------------------------------------------------------------
// Destructor
Conversation::~Conversation() { delete[] data_; }


//------------------------------------------------------------------------
//Append
// Doubles the capacity_ whenever the array is full.
// Copying cost stays at O(N), see design log for amortized-cost proof
//
void Conversation::append(Message m) {
    if (size_ == capacity_) {
        std::size_t new_cap = (capacity_ == 0) ? 1 : capacity_ * 2;
        Message* new_data = new Message[new_cap];
        for (std::size_t i = 0; i < size_; ++i) new_data[i] = std::move(data_[i]);
        delete[] data_;
        data_ = new_data;
        capacity_ = new_cap;
    }
    data_[size_++] = std::move(m);
}

std::size_t Conversation::size() const noexcept { return size_; }

//------------------------------------------------------------------------
// at 
// returns the value at the index given if within range
const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) throw std::out_of_range("Conversation::at index out of range");
    return data_[i];
}

// Show only the used portion of the buffer by using size_, not the full capacity_
const Message* Conversation::begin() const noexcept { return data_; }
const Message* Conversation::end()   const noexcept { return data_ + size_; }