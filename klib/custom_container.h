//
// Created by KH on 3/7/2020.
//

#ifndef IGEN4_CUSTOM_CONTAINER_H
#define IGEN4_CUSTOM_CONTAINER_H

namespace klib {

template<typename ItType>
class custom_iterable_container {
public:
    custom_iterable_container(const ItType &begin, const ItType &end)
            : begin_(begin), end_(end) {}

    [[nodiscard]] ItType begin() const { return begin_; };

    [[nodiscard]] ItType end() const { return end_; };
private:
    ItType begin_, end_;
};

}

#endif //IGEN4_CUSTOM_CONTAINER_H
