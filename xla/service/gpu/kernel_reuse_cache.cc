/*Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "xla/service/gpu/kernel_reuse_cache.h"

#include <functional>
#include <string>
#include <utility>

#include "xla/hlo/ir/hlo_computation.h"
#include "xla/hlo/ir/hlo_instruction.h"
#include "xla/util.h"
#include "tsl/platform/logging.h"

namespace xla {
namespace gpu {
namespace {

// Calculates a fingerprint of the kernel arguments, which can be used for
// checking reusability.
//
// For example 2 arguments that are aligned to 16 bytes, aliased and also
// written by the kernel will be represented as "16aw,16aw".
//
// Overlapping arguments are only marked aliased, if at least one of them is
// written and their buffers are not exactly the same. If 2 arguments'
// buffers are exactly the same, then they are not marked aliased, but marked
// as duplicates, for example like this: "16,=0,16w,=2". The example means
// that the 1st argument is the same as the 0th and the 3rd is the same as
// the 2nd. These duplicated parameters are passed to the kernel only once.
std::string GetArgumentFingerprint(
    absl::Span<const KernelArgument> kernel_arguments) {
  return absl::StrJoin(
      kernel_arguments, ",", [](std::string* s, const KernelArgument& arg) {
        if (arg.first_with_same_slice().has_value()) {
          absl::StrAppend(s, "=", arg.first_with_same_slice().value());
          return;
        }
        absl::StrAppend(s, arg.alignment());
        if (arg.aliased()) {
          absl::StrAppend(s, "a");
        }
        if (arg.written()) {
          absl::StrAppend(s, "w");
        }
      });
}

}  // namespace

std::string GetComputationFingerprint(
    const HloComputation* fused_computation,
    absl::Span<const KernelArgument> kernel_arguments,
    absl::string_view discriminator) {
  // We have to print constants, because otherwise we would accidentally reuse
  // kernels which have different builtin constants.
  //
  // It is not a problem to recursively print sub-computations, because we don't
  // have them at this point.
  auto print_options = HloPrintOptions::Fingerprint()
                           .set_print_only_essential_constants(false)
                           .set_print_operand_shape(false);

  return absl::StrCat(discriminator, "(",
                      GetArgumentFingerprint(kernel_arguments), ")",
                      fused_computation->ToString(print_options));
}

std::pair<KernelReuseCache::Entry, bool> KernelReuseCache::Get(
    const HloComputation* fused_computation,
    absl::Span<const KernelArgument> kernel_arguments,
    absl::string_view discriminator,
    const std::function<KernelReuseCache::Entry()>& generator) {
  auto ret =
      GetWithStatus(fused_computation, kernel_arguments, discriminator,
                    [&]() -> absl::StatusOr<Entry> { return generator(); });
  return {*ret.first, ret.second};
}

std::pair<absl::StatusOr<KernelReuseCache::Entry>, bool>
KernelReuseCache::GetWithStatus(
    const HloComputation* fused_computation,
    absl::Span<const KernelArgument> kernel_arguments,
    absl::string_view discriminator,
    const std::function<absl::StatusOr<KernelReuseCache::Entry>()>& generator) {
  std::string fingerprint = GetComputationFingerprint(
      fused_computation, kernel_arguments, discriminator);
  VLOG(4) << "Fingerprint: ";
  XLA_VLOG_LINES(4, fingerprint);

  auto& entry = cache_[fingerprint];
  if (entry.kernel_name.empty()) {
    auto ret = generator();
    if (ret.ok()) entry = *ret;
    return {ret, false};
  }
  return {{entry}, true};
}

}  // namespace gpu
}  // namespace xla
