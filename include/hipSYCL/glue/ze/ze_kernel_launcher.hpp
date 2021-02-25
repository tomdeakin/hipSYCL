/*
 * This file is part of hipSYCL, a SYCL implementation based on CUDA/HIP
 *
 * Copyright (c) 2021 Aksel Alpay
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

#ifndef HIPSYCL_ZE_KERNEL_LAUNCHER_HPP
#define HIPSYCL_ZE_KERNEL_LAUNCHER_HPP


#include <cassert>
#include <tuple>

#include "hipSYCL/common/debug.hpp"
#include "hipSYCL/runtime/error.hpp"
#include "hipSYCL/runtime/ze/ze_queue.hpp"
#include "hipSYCL/sycl/libkernel/backend.hpp"
#include "hipSYCL/sycl/libkernel/range.hpp"
#include "hipSYCL/sycl/libkernel/id.hpp"
#include "hipSYCL/sycl/libkernel/item.hpp"
#include "hipSYCL/sycl/libkernel/nd_item.hpp"
#include "hipSYCL/sycl/libkernel/sp_item.hpp"
#include "hipSYCL/sycl/libkernel/group.hpp"
#include "hipSYCL/sycl/libkernel/reduction.hpp"

#ifdef SYCL_DEVICE_ONLY
#include "hipSYCL/sycl/libkernel/detail/thread_hierarchy.hpp"
#endif

#include "hipSYCL/runtime/device_id.hpp"
#include "hipSYCL/runtime/kernel_launcher.hpp"

namespace hipsycl {
namespace glue {

namespace ze_dispatch {


class auto_name {};

template <typename KernelName = auto_name, typename KernelType>
__attribute__((sycl_kernel)) void
kernel_single_task(const KernelType &kernelFunc) {
  kernelFunc();
}

template <typename KernelName, typename KernelType, int Dim>
__attribute__((sycl_kernel)) void
kernel_parallel_for(const KernelType &KernelFunc, sycl::range<Dim> num_items) {
#ifdef SYCL_DEVICE_ONLY
  sycl::id<Dim> gid = sycl::detail::get_global_id();
  auto item = sycl::detail::make_item(gid, num_items);

  bool is_within_range = true;

  for(int i = 0; i < Dim; ++i)
    if(gid[i] >= num_items[i])
      is_within_range = false;

  if(is_within_range)
    KernelFunc(item);
#endif
}
}

class ze_kernel_launcher : public rt::backend_kernel_launcher
{
public:
#ifdef SYCL_DEVICE_ONLY
#define __hipsycl_invoke_kernel(f, KernelNameT, KernelBodyT, num_groups,       \
                                group_size local_mem, ...)                     \
  f(__VA_ARGS__);
#else
#define __hipsycl_invoke_kernel(f, KernelNameT, KernelBodyT, num_groups,       \
                                group_size, local_mem, ...)                    \
  invoke_from_module<KernelName, KernelBodyT>(num_groups, group_size,          \
                                              local_mem, __VA_ARGS__);
#endif

  ze_kernel_launcher() : _queue{nullptr}{}
  virtual ~ze_kernel_launcher(){}

  virtual void set_params(void* q) override {
    _queue = static_cast<rt::ze_queue*>(q);
  }

  template <class KernelName, rt::kernel_type type, int Dim, class Kernel,
            typename... Reductions>
  void bind(sycl::id<Dim> offset, sycl::range<Dim> global_range,
            sycl::range<Dim> local_range, std::size_t dynamic_local_memory,
            Kernel k, Reductions... reductions) {

    this->_type = type;
    
    this->_invoker = [=]() {

      sycl::range<Dim> effective_local_range = local_range;
      if constexpr (type == rt::kernel_type::basic_parallel_for) {
        // If local range is non 0, we use it as a hint to override
        // the default selection
        if(local_range.size() == 0) {
          if constexpr (Dim == 1)
            effective_local_range = sycl::range<1>{128};
          else if constexpr (Dim == 2)
            effective_local_range = sycl::range<2>{16, 16};
          else if constexpr (Dim == 3)
            effective_local_range = sycl::range<3>{4, 8, 8};
        }
        HIPSYCL_DEBUG_INFO << "ze_kernel_launcher: Submitting high-level "
                              "parallel for with selected total group size of "
                          << effective_local_range.size() << std::endl;
      }

      sycl::range<Dim> num_groups;
      for(int i = 0; i < Dim; ++i) {
        num_groups[i] = (global_range[i] + effective_local_range[i] - 1) /
                        effective_local_range[i];
      }

      if constexpr(type == rt::kernel_type::single_task){
        __hipsycl_invoke_kernel(ze_dispatch::kernel_single_task<KernelName>,
                                KernelName, Kernel, rt::range<3>{1, 1, 1},
                                rt::range<3>{1, 1, 1}, 0, k);

      } else if constexpr (type == rt::kernel_type::basic_parallel_for) {

        __hipsycl_invoke_kernel(ze_dispatch::kernel_parallel_for<KernelName>,
                                KernelName, Kernel,
                                make_kernel_launch_range(num_groups),
                                make_kernel_launch_range(effective_local_range),
                                dynamic_local_memory, k, global_range);

      } else if constexpr (type == rt::kernel_type::ndrange_parallel_for) {

      } else if constexpr (type == rt::kernel_type::hierarchical_parallel_for) {

      } else if constexpr( type == rt::kernel_type::scoped_parallel_for) {

      } else if constexpr (type == rt::kernel_type::custom) {
        sycl::interop_handle handle{
            rt::device_id{rt::backend_descriptor{rt::hardware_platform::level_zero,
                                                 rt::api_platform::level_zero},
                          0},
            static_cast<void*>(nullptr)};

        // Need to perform additional copy to guarantee deferred_pointers/
        // accessors are initialized
        auto initialized_kernel_invoker = k;
        initialized_kernel_invoker(handle);
      }
      else {
        assert(false && "Unsupported kernel type");
      }
      
    };
  }

  virtual rt::backend_id get_backend() const final override {
    return rt::backend_id::level_zero;
  }

  virtual void invoke() final override {
    _invoker();
  }

  virtual rt::kernel_type get_kernel_type() const final override {
    return _type;
  }

private:
  template<int Dim>
  rt::range<3> make_kernel_launch_range(sycl::range<Dim> r) const {
    if constexpr(Dim == 1) {
      return rt::range<3>{r[0], 1, 1};
    } else if constexpr(Dim == 2) {
      return rt::range<3>{r[1], r[0], 1};
    } else {
      return rt::range<3>{r[2], r[1], r[0]};
    }
  }

  template <class KernelName, class KernelBodyT, typename... Args>
  void invoke_from_module(rt::range<3> num_groups, rt::range<3> group_size,
                          unsigned dynamic_local_mem, Args... args) {
    
    
#ifdef __HIPSYCL_MULTIPASS_SPIRV_HEADER__
#if !defined(__clang_major__) || __clang_major__ < 11
  #error Multipass compilation requires clang >= 11
#endif
    if (this_module::get_num_objects<rt::backend_id::level_zero>() == 0) {
      rt::register_error(
          __hipsycl_here(),
          rt::error_info{
              "hiplike_kernel_launcher: Cannot invoke SPIR-V kernel: No code "
              "objects present in this module."});
      return;
    }

    const std::string *kernel_image =
        this_module::get_code_object<Backend_id>("spirv");
    assert(kernel_image && "Invalid kernel image object");

    std::array<void *, sizeof...(Args)> kernel_args{
      static_cast<void *>(&args)...
    };

    std::string kernel_name_tag = __builtin_unique_stable_name(KernelName);
    std::string kernel_body_name = __builtin_unique_stable_name(KernelBodyT);

    rt::module_invoker *invoker = _queue->get_module_invoker();

    assert(invoker &&
            "Runtime backend does not support invoking kernels from modules");

    rt::result err = invoker->submit_kernel(
        this_module::get_module_id<Backend_id>(), "spirv",
        kernel_image, num_groups, group_size, dynamic_local_mem,
        kernel_args.data(), kernel_args.size(), kernel_name_tag,
        kernel_body_name);

    if (!err.is_success())
      rt::register_error(err);
#else
    assert(false && "No module available to invoke kernels from");
#endif
  
  }

  std::function<void ()> _invoker;
  rt::kernel_type _type;
  rt::ze_queue* _queue;
};

}
}

#undef __hipsycl_invoke_kernel

#endif
