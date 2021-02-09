#pragma once

#include "row.hpp"

#include <libpq-fe.h>

#include <iterator>

namespace postgrespp {

class result_iterator
  : public std::random_access_iterator_tag {
public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

public:
  result_iterator(PGresult* res)
    : result_iterator(res, 0) {
  }

  result_iterator(PGresult* res, size_type row)
    : res_{res}
    , cur_row_{row} {
  }

  const row operator*() {
    return {res_, cur_row_};
  }

  result_iterator operator++(int) {
    auto copy = *this;
    return ++copy;
  }

  result_iterator operator--(int) {
    auto copy = *this;
    return --copy;
  }

  result_iterator operator+(size_type n) {
    auto copy = *this;
    return copy += n;
  }

  friend inline result_iterator operator+(size_type n, const result_iterator& rhs) {
    auto copy = rhs;
    return copy += n;
  }

  result_iterator operator-(size_type n) {
    auto copy = *this;
    return copy -= n;
  }

  result_iterator& operator++() {
    return (*this) += 1;
  }

  result_iterator& operator--() {
    return (*this) -= 1;
  }

  result_iterator& operator+=(size_type n) {
    cur_row_ += n;
    return *this;
  }

  result_iterator& operator-=(size_type n) {
    cur_row_ -= n;
    return *this;
  }

  result_iterator operator[](size_type n) {
    return (*this) + n;
  }

  friend inline difference_type operator-(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ - rhs.cur_row_;
  }

  friend inline bool operator==(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ == rhs.cur_row_;
  }

  friend inline bool operator!=(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ != rhs.cur_row_;
  }

  friend inline bool operator<(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ < rhs.cur_row_;
  }

  friend inline bool operator<=(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ <= rhs.cur_row_;
  }

  friend inline bool operator>(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ > rhs.cur_row_;
  }

  friend inline bool operator>=(const result_iterator& lhs, const result_iterator& rhs) {
    return lhs.cur_row_ >= rhs.cur_row_;
  }

private:
  PGresult* const res_;
  size_type cur_row_;
};

}
