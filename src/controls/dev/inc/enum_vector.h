#pragma once

template<typename E, class T>
class enum_vector : public std::vector<T> {
public:
    T& operator[] (E e) {
        return std::vector<T, N>::operator[]((std::size_t)e);
    }

    const T& operator[] (E e) const {
        return std::vector<T, N>::operator[]((std::size_t)e);
    }
};

