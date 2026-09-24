
#include "core/conversation.h"
#include <stdexcept>
#include <utility>

// Constructor
Conversation::Conversation(const Conversation& other)
    : size_(other.size_), capacity_(other.capacity_){
    data_ = new Message[capacity_];
    for (std::size_t i = 0; i < size_; ++i){
        data_[i] = other.data_[i];
    }

}

// Copy Constructor
Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

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
// Destructor
Conversation::~Conversation() { delete[] data_; }

//Append
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

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) throw std::out_of_range("Conversation::at index out of range");
    return data_[i];
}

const Message* Conversation::begin() const noexcept { return data_; }
const Message* Conversation::end()   const noexcept { return data_ + size_; }