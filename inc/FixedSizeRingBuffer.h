#pragma once
#include <cstddef> // For std::size_t
#include <utility> // For std::swap()
#include <vector>

/* T is the type of stored items and N is the number of items */
template <class T, std::size_t N>
class FixedSizeRingBuffer {
    static constexpr std::size_t MAX_ELEMENTS = N;

public:
    FixedSizeRingBuffer();

    void reset();
    void push(T item);
    T pop();
    bool isFull() const;
    bool isEmpty() const;
    std::size_t getCapacity() const;
    std::size_t getCount() const;
    std::vector<T> getTail(std::size_t n) const;
    std::vector<T> toVector() const;

private:
    T buf[MAX_ELEMENTS];
    std::size_t head;   /*!< Offset of the head element (next push) */
    std::size_t tail;   /*!< Offset of the tail element (next pop) */
    bool full;  /*!< If the buffer full? */
};

template <class T, std::size_t N>
FixedSizeRingBuffer<T, N>::FixedSizeRingBuffer() {
    this->reset();
}

template <class T, std::size_t N>
void FixedSizeRingBuffer<T, N>::reset() {
	this->head = 0;
    this->tail = 0;
	this->full = false;
    for (std::size_t i; i < N; i++) {
        T emptyElement;
        std::swap(this->buf[i], emptyElement);
    }
}

template <class T, std::size_t N>
void FixedSizeRingBuffer<T, N>::push(T item) {
	this->buf[this->head] = item;

	if (this->isFull()) {
		this->tail = (this->tail + 1) % N;  /* Drop oldest element */
	}

	this->head = (this->head + 1) % N;

    if (this->head == this->tail)
        this->full = true;
}

template <class T, std::size_t N>
T FixedSizeRingBuffer<T, N>::pop() {
	if (this->isEmpty()) {
		return T(); // FIXME: error condition
	}

	T val = this->buf[this->tail];
	this->full = false;
	this->tail = (this->tail + 1) % N;

	return val;
}

template <class T, std::size_t N>
std::vector<T> FixedSizeRingBuffer<T, N>::getTail(std::size_t len) const {
    std::vector<T> result;
    if (len > this->getCount())
        len = this->getCount();
    result.reserve(len);
    result.assign(this->buf, &(this->buf[len]));
    return result;
}

template <class T, std::size_t N>
std::vector<T> FixedSizeRingBuffer<T, N>::toVector() const {
    return this->getTail(this->getCount());
}

template <class T, std::size_t N>
bool FixedSizeRingBuffer<T, N>::isFull() const {
	return this->full;
}

template <class T, std::size_t N>
bool FixedSizeRingBuffer<T, N>::isEmpty() const {
	return (!this->full && (this->head == this->tail));
}

template <class T, std::size_t N>
std::size_t FixedSizeRingBuffer<T, N>::getCapacity() const {
	return N;
}

template <class T, std::size_t N>
std::size_t FixedSizeRingBuffer<T, N>::getCount() const {
	if (!this->full) {
        return (N + this->head - this->tail) % N;
	}
    else {
        return N;
    }
}
