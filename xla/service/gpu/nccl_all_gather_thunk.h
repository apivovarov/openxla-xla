/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#ifndef XLA_SERVICE_GPU_NCCL_ALL_GATHER_THUNK_H_
#define XLA_SERVICE_GPU_NCCL_ALL_GATHER_THUNK_H_

#include <cstdint>
#include <vector>

#include "absl/types/span.h"
#include "xla/hlo/ir/hlo_instructions.h"
#include "xla/service/collective_ops_utils.h"
#include "xla/service/gpu/nccl_api.h"
#include "xla/service/gpu/nccl_collective_thunk.h"
#include "xla/status.h"

namespace xla {
namespace gpu {

struct NcclAllGatherConfig {
  NcclCollectiveConfig config;
};

// Thunk that performs a NCCL-based All-Gather among CUDA GPU-based replicas.
class NcclAllGatherStartThunk : public NcclCollectiveThunk {
 public:
  NcclAllGatherStartThunk(ThunkInfo thunk_info,
                          mlir::lmhlo_gpu::AllGatherStartOp op,
                          std::vector<Buffer> buffers);

  NcclAllGatherStartThunk(ThunkInfo thunk_info,
                          const HloAllGatherInstruction* inst,
                          std::vector<Buffer> buffers);

  static const char* GetHloOpName() { return "all-gather-start"; }

  static absl::Status CheckImplementable(mlir::lmhlo_gpu::AllGatherStartOp op,
                                         int64_t replica_count,
                                         int64_t partition_count);

  static absl::Status CheckImplementable(const HloAllGatherInstruction* inst,
                                         int64_t replica_count,
                                         int64_t partition_count);

  static CollectiveOpGroupMode GetGroupMode(
      mlir::lmhlo_gpu::AllGatherStartOp op);

  static CollectiveOpGroupMode GetGroupMode(
      const HloAllGatherInstruction* inst);

  const NcclCollectiveConfig& config() const override { return config_.config; }
  absl::Span<const Buffer> buffers() const { return buffers_; }

 protected:
  absl::Status RunNcclCollective(const ExecuteParams& params,
                                 se::Stream& stream, ncclComm_t comm) override;

 private:
  const NcclAllGatherConfig config_;
  const std::vector<Buffer> buffers_;
};

absl::Status RunAllGather(std::vector<DeviceBufferPair>& buffers,
                          se::Stream& stream, ncclComm_t comm);

inline absl::Status RunAllGather(std::vector<DeviceBufferPair>& buffers,
                                 se::Stream& stream, NcclCommHandle comm) {
  return RunAllGather(buffers, stream, reinterpret_cast<ncclComm_t>(comm));
}

}  // namespace gpu
}  // namespace xla

#endif  // XLA_SERVICE_GPU_NCCL_ALL_GATHER_THUNK_H_
