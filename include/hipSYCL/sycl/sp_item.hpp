/*
 * This file is part of hipSYCL, a SYCL implementation based on CUDA/HIP
 *
 * Copyright (c) 2018-2020 Aksel Alpay
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HIPSYCL_SP_ITEM_HPP
#define HIPSYCL_SP_ITEM_HPP

#include "backend/backend.hpp"
#include "id.hpp"
#include "range.hpp"
#include "detail/thread_hierarchy.hpp"

namespace hipsycl {
namespace sycl {
namespace detail {

template<int Dim>
class sp_item
{
  template<int D>
  HIPSYCL_KERNEL_TARGET
  friend sp_item<D> make_sp_item(
    sycl::id<D> local_id, sycl::id<D> group_id,
    sycl::range<D> local_range, sycl::range<D> num_groups);
public:
  HIPSYCL_KERNEL_TARGET
  sycl::range<Dim> get_global_range() const {
    return _num_groups * _local_range;
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_global_range(int dimension) const {
    return _num_groups[dimension] * _local_range[dimension];
  }

  HIPSYCL_KERNEL_TARGET
  sycl::id<Dim> get_global_id() const {
    return _local_id + _group_id * _local_range;
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_global_id(int dimension) const {
    return _local_id[dimension] + 
      _group_id[dimension] * _local_range[dimension];
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_global_linear_id() const {
    return detail::linear_id<Dim>::get(get_global_id(), get_global_range());
  }

  HIPSYCL_KERNEL_TARGET
  sycl::range<Dim> get_local_range() const {
    return _local_range;
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_local_range(int dimension) const {
    return _local_range[dimension];
  }

  HIPSYCL_KERNEL_TARGET
  sycl::id<Dim> get_local_id() const {
    return _local_id;
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_local_id(int dimension) const {
    return _local_id[dimension];
  }

  HIPSYCL_KERNEL_TARGET
  size_t get_local_linear_id() const {
    return detail::linear_id<Dim>::get(_local_id, _local_range);
  }
private:
  sp_item(sycl::id<Dim> local_id, sycl::id<Dim> group_id,
          sycl::range<Dim> local_range, sycl::range<Dim> num_groups)
    : _local_id{local_id}, _group_id{group_id}, 
      _local_range{local_range}, _num_groups{num_groups}
  {}
  
  sycl::id<Dim> _local_id;
  sycl::id<Dim> _group_id;

  sycl::range<Dim> _local_range;
  sycl::range<Dim> _num_groups;
};

template<int Dim>
HIPSYCL_KERNEL_TARGET
sp_item<Dim> make_sp_item(sycl::id<Dim> local_id, sycl::id<Dim> group_id,
                          sycl::range<Dim> local_range, sycl::range<Dim> num_groups){

  return sp_item<Dim>{local_id, group_id, local_range, num_groups};
}

}

template<int Dim>
using logical_item=detail::sp_item<Dim>;

template<int Dim>
using physical_item=detail::sp_item<Dim>;

}
}

#endif
