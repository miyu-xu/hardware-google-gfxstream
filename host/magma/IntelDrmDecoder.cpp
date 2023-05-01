// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "host/magma/IntelDrmDecoder.h"

#include <i915_drm.h>
#include <magma_intel_gen_defs.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <vector>

#include "RenderThreadInfoMagma.h"
#include "host/magma/Connection.h"
#include "host/magma/DrmDevice.h"
#include "magma/magma_common_defs.h"

namespace gfxstream {
namespace magma {

std::unique_ptr<IntelDrmDecoder> IntelDrmDecoder::create(uint32_t context_id) {
    std::unique_ptr<IntelDrmDecoder> decoder(new IntelDrmDecoder());
    decoder->mContextId = context_id;
    INFO("IntelDrmDecoder created for context %" PRIu32, context_id);
    return decoder;
}

#define MAGMA_DECODER_BIND_METHOD(method)                               \
    magma_server_context_t::method = [](auto... args) {                 \
        auto decoder = RenderThreadInfoMagma::get() -> mMagmaDec.get(); \
        return static_cast<IntelDrmDecoder*>(decoder)->method(args...); \
    }

IntelDrmDecoder::IntelDrmDecoder() : Decoder() {
    MAGMA_DECODER_BIND_METHOD(magma_device_import);
    MAGMA_DECODER_BIND_METHOD(magma_device_release);
    MAGMA_DECODER_BIND_METHOD(magma_device_query_fudge);
    MAGMA_DECODER_BIND_METHOD(magma_device_create_connection);
    MAGMA_DECODER_BIND_METHOD(magma_connection_release);
    MAGMA_DECODER_BIND_METHOD(magma_connection_create_buffer);
    MAGMA_DECODER_BIND_METHOD(magma_connection_release_buffer);
    MAGMA_DECODER_BIND_METHOD(magma_connection_create_semaphore);
    MAGMA_DECODER_BIND_METHOD(magma_connection_release_semaphore);
    MAGMA_DECODER_BIND_METHOD(magma_buffer_export);
    MAGMA_DECODER_BIND_METHOD(magma_semaphore_signal);
    MAGMA_DECODER_BIND_METHOD(magma_semaphore_reset);
    MAGMA_DECODER_BIND_METHOD(magma_poll);
    MAGMA_DECODER_BIND_METHOD(magma_connection_get_error);
    MAGMA_DECODER_BIND_METHOD(magma_connection_create_context);
    MAGMA_DECODER_BIND_METHOD(magma_connection_release_context);
    MAGMA_DECODER_BIND_METHOD(magma_connection_map_buffer);
    MAGMA_DECODER_BIND_METHOD(magma_connection_unmap_buffer);
}

// TODO(b/279936417): Make objects and their IDs orthogonal.
#define MAGMA_OBJECT_TO_ID(x) ((x) << 32ull)
#define MAGMA_ID_TO_OBJECT(x) ((x) >> 32ull)

magma_status_t IntelDrmDecoder::magma_device_import(magma_handle_t device_channel,
                                                    magma_device_t* device_out) {
    *device_out = 0;
    auto device = DrmDevice::create();
    if (!device) {
        return MAGMA_STATUS_INTERNAL_ERROR;
    }
    *device_out = mDevices.create(std::move(*device));
    INFO("magma_device_import() -> %" PRIu64, *device_out);
    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_device_release(magma_device_t device) {
    INFO("magma_device_release(%" PRIu64 ")", device);
}

static uint64_t GetNsMonotonic(bool raw) {
    timespec ts{};
    int result = clock_gettime(raw ? CLOCK_MONOTONIC_RAW : CLOCK_MONOTONIC, &ts);
    if (result < 0) {
        return 0;
    }
    constexpr uint64_t kNsPerSec = 1'000'000'000ull;
    return static_cast<uint64_t>(ts.tv_sec) * kNsPerSec + ts.tv_nsec;
}

// Converts a DRM topology to a Magma topology.
static std::vector<uint8_t> MakeMagmaTopology(const drm_i915_query_topology_info* info) {
    auto read_bit = [](const uint8_t* ptr, size_t offset) -> bool {
        return (ptr[offset / 8] >> (offset % 8)) & 1;
    };

    auto append_buffer = [](std::vector<uint8_t>& buffer, const uint8_t* src, size_t len) -> void {
        auto offset = buffer.size();
        buffer.resize(offset + len);
        memcpy(buffer.data() + offset, src, len);
    };

    const auto* slice_base = info->data;
    const auto* subslice_base = info->data + info->subslice_offset;
    const auto* eu_base = info->data + info->eu_offset;

    // Start with a buffer just large enough to hold the magma struct.
    std::vector<uint8_t> buffer(sizeof(magma_intel_gen_topology));

    // Copy the slice mask.
    size_t slice_data_bytes = (info->max_slices + 7) / 8;
    append_buffer(buffer, slice_base, slice_data_bytes);

    for (uint32_t slice = 0; slice < info->max_slices; ++slice) {
        if (!read_bit(slice_base, slice)) {
            continue;
        }
        const auto* subslice_data = &subslice_base[slice * info->subslice_stride];

        // For each active slice, copy the subslice mask.
        size_t subslice_data_bytes = (info->max_subslices + 7) / 8;
        append_buffer(buffer, subslice_data, subslice_data_bytes);

        for (uint32_t subslice = 0; subslice < info->max_subslices; ++subslice) {
            if (!read_bit(subslice_data, subslice)) {
                continue;
            }
            const auto* eu_data =
                &eu_base[(slice * info->max_subslices + subslice) * info->eu_stride];

            // For each active subslice, copy the eu mask.
            size_t eu_data_bytes = (info->max_eus_per_subslice + 7) / 8;
            append_buffer(buffer, eu_data, eu_data_bytes);
        }
    }

    // Populate the base struct elements.
    auto magma_topology = reinterpret_cast<magma_intel_gen_topology*>(buffer.data());
    magma_topology->max_slice_count = info->max_slices;
    magma_topology->max_subslice_count = info->max_subslices;
    magma_topology->max_eu_count = info->max_eus_per_subslice;
    magma_topology->data_byte_count = buffer.size() - sizeof(magma_intel_gen_topology);

    return buffer;
}

magma_status_t IntelDrmDecoder::magma_device_query_fudge(magma_device_t device, uint64_t id,
                                                         magma_bool_t host_allocate,
                                                         uint64_t* result_buffer_mapping_id_inout,
                                                         uint64_t* result_buffer_size_inout,
                                                         uint64_t* result_out) {
    *result_out = 0;

    auto dev = mDevices.get(device);
    if (!dev) {
        return MAGMA_STATUS_INVALID_ARGS;
    }

    // TODO(b/275093891): query or standardize hard-coded values
    constexpr uint32_t kExtraPageCount = 9;
    constexpr uint64_t kIntelTimestampRegisterOffset = 0x23f8;
    switch (id) {
        case MAGMA_QUERY_VENDOR_ID: {
            *result_out = MAGMA_VENDOR_ID_INTEL;
            return MAGMA_STATUS_OK;
        }
        case MAGMA_QUERY_DEVICE_ID: {
            auto result = dev->getParam(I915_PARAM_CHIPSET_ID);
            if (!result) {
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            *result_out = *result;
            return MAGMA_STATUS_OK;
        }
        case MAGMA_QUERY_IS_TOTAL_TIME_SUPPORTED: {
            *result_out = 0;
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQuerySubsliceAndEuTotal: {
            auto subslice_result = dev->getParam(I915_PARAM_SUBSLICE_TOTAL);
            auto eu_result = dev->getParam(I915_PARAM_EU_TOTAL);
            if (!subslice_result || !eu_result) {
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            *result_out = (static_cast<uint64_t>(*subslice_result) << 32) | *eu_result;
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryGttSize: {
            *result_out = 0;
            drm_i915_gem_context_create_ext create_params{};
            int create_result = dev->ioctl(DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &create_params);
            if (create_result) {
                ERR("DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT failed: %d", errno);
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            drm_i915_gem_context_param query_params{.ctx_id = create_params.ctx_id,
                                                    .param = I915_CONTEXT_PARAM_GTT_SIZE};
            int query_result = dev->ioctl(DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, &query_params);
            if (query_result) {
                ERR("DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM failed: %d", errno);
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            drm_i915_gem_context_destroy destroy_params{.ctx_id = create_params.ctx_id};
            int destroy_result = dev->ioctl(DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy_params);
            if (destroy_result) {
                ERR("DRM_IOCTL_I915_GEM_CONTEXT_DESTROY failed: %d", errno);
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            *result_out = query_params.value;
            ERR("GTT size %" PRIu64, *result_out);
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryExtraPageCount: {
            *result_out = kExtraPageCount;
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryTimestamp: {
            if (!host_allocate) {
                WARN("Guest-allocated buffers are not currently supported.");
                return MAGMA_STATUS_UNIMPLEMENTED;
            }
            auto buffer =
                DrmBuffer::create(*dev, mContextId, sizeof(magma_intel_gen_timestamp_query));
            if (!buffer) {
                return MAGMA_STATUS_MEMORY_ERROR;
            }
            auto ptr = buffer->map();
            if (!ptr) {
                return MAGMA_STATUS_MEMORY_ERROR;
            }
            auto ts = reinterpret_cast<magma_intel_gen_timestamp_query*>(ptr);
            ts->monotonic_raw_timestamp[0] = GetNsMonotonic(true);
            ts->monotonic_timestamp = GetNsMonotonic(false);
            // Attempt to read device timestamp register.
            drm_i915_reg_read params{.offset = kIntelTimestampRegisterOffset | I915_REG_READ_8B_WA};
            int result = dev->ioctl(DRM_IOCTL_I915_REG_READ, &params);
            if (result == 0) {
                ts->device_timestamp = params.val;
            } else {
                ts->device_timestamp = 0;
            }
            // The driver uses the second timestamp to determine the sampling span.
            ts->monotonic_raw_timestamp[1] = GetNsMonotonic(true);
            *result_buffer_mapping_id_inout = buffer->getId();
            *result_buffer_size_inout = buffer->size();
            // Add the buffer to the container.
            auto gem_handle = buffer->getHandle();
            auto magma_handle = mBuffers.create(std::move(*buffer));
            mGemHandleToBuffer.emplace(gem_handle, magma_handle);
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryTopology: {
            if (!host_allocate) {
                WARN("Guest-allocated buffers are not currently supported.");
                return MAGMA_STATUS_UNIMPLEMENTED;
            }
            // Check how much space is needed to represent topology.
            drm_i915_query_item item{.query_id = DRM_I915_QUERY_TOPOLOGY_INFO};
            drm_i915_query query{.num_items = 1, .items_ptr = reinterpret_cast<uint64_t>(&item)};
            int result = dev->ioctl(DRM_IOCTL_I915_QUERY, &query);
            if (result != 0) {
                ERR("DRM_IOCTL_I915_QUERY failed: %d", errno);
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            std::vector<uint8_t> topology_buffer(item.length);
            item.data_ptr = reinterpret_cast<uint64_t>(topology_buffer.data());

            // Re-run the query with the allocated buffer.
            result = dev->ioctl(DRM_IOCTL_I915_QUERY, &query);
            if (result != 0) {
                ERR("DRM_IOCTL_I915_QUERY failed: %d", errno);
                return MAGMA_STATUS_INTERNAL_ERROR;
            }

            // Convert to the magma-compatible topology layout.
            auto magma_topology_buffer = MakeMagmaTopology(
                reinterpret_cast<drm_i915_query_topology_info*>(topology_buffer.data()));

            // Create a magma buffer and copy the layout struct to it.
            auto buffer = DrmBuffer::create(*dev, mContextId, magma_topology_buffer.size());
            if (!buffer) {
                return MAGMA_STATUS_MEMORY_ERROR;
            }
            auto ptr = buffer->map();
            if (!ptr) {
                return MAGMA_STATUS_MEMORY_ERROR;
            }
            memcpy(ptr, magma_topology_buffer.data(), magma_topology_buffer.size());
            *result_buffer_mapping_id_inout = buffer->getId();
            *result_buffer_size_inout = buffer->size();
            auto gem_handle = buffer->getHandle();
            auto magma_handle = mBuffers.create(std::move(*buffer));
            mGemHandleToBuffer.emplace(gem_handle, magma_handle);
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryHasContextIsolation: {
            auto result = dev->getParam(I915_PARAM_HAS_CONTEXT_ISOLATION);
            if (!result) {
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            *result_out = *result;
            return MAGMA_STATUS_OK;
        }
        case kMagmaIntelGenQueryTimestampFrequency: {
            auto result = dev->getParam(I915_PARAM_CS_TIMESTAMP_FREQUENCY);
            if (!result) {
                return MAGMA_STATUS_INTERNAL_ERROR;
            }
            *result_out = *result;
            return MAGMA_STATUS_OK;
        }
        default: {
            return MAGMA_STATUS_INVALID_ARGS;
        }
    }
}

magma_status_t IntelDrmDecoder::magma_device_create_connection(magma_device_t device,
                                                               magma_connection_t* connection_out) {
    *connection_out = MAGMA_INVALID_OBJECT_ID;
    auto dev = mDevices.get(device);
    if (!dev) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    *connection_out = mConnections.create(*dev);
    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_connection_release(magma_connection_t connection) {
    bool erased = mConnections.erase(connection);
    if (!erased) {
        WARN("invalid connection %" PRIu64, connection);
    }
}

magma_status_t IntelDrmDecoder::magma_connection_create_buffer(magma_connection_t connection,
                                                               uint64_t size, uint64_t* size_out,
                                                               magma_buffer_t* buffer_out,
                                                               magma_buffer_id_t* id_out) {
    *size_out = 0;
    *buffer_out = MAGMA_INVALID_OBJECT_ID;
    *id_out = MAGMA_INVALID_OBJECT_ID;
    auto con = mConnections.get(connection);
    if (!con) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto buffer = DrmBuffer::create(con->getDevice(), mContextId, size);
    if (!buffer) {
        return MAGMA_STATUS_MEMORY_ERROR;
    }
    auto gem_handle = buffer->getHandle();
    auto magma_handle = mBuffers.create(std::move(*buffer));
    mGemHandleToBuffer.emplace(gem_handle, magma_handle);
    *size_out = buffer->size();
    *buffer_out = magma_handle;
    *id_out = MAGMA_OBJECT_TO_ID(magma_handle);
    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_connection_release_buffer(magma_connection_t connection,
                                                      magma_buffer_t buffer) {
    auto con = mConnections.get(connection);
    if (!con) {
        return;
    }
    auto buf = mBuffers.get(buffer);
    if (!buf) {
        return;
    }
    mGemHandleToBuffer.erase(buf->getHandle());
    mBuffers.erase(buffer);
}

magma_status_t IntelDrmDecoder::magma_connection_create_semaphore(magma_connection_t connection,
                                                                  magma_semaphore_t* semaphore_out,
                                                                  magma_semaphore_id_t* id_out) {
    *semaphore_out = MAGMA_INVALID_OBJECT_ID;
    *id_out = MAGMA_INVALID_OBJECT_ID;

    auto con = mConnections.get(connection);
    if (!con) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto semaphore = DrmSemaphore::create(con->getDevice());
    if (!semaphore) {
        return MAGMA_STATUS_INTERNAL_ERROR;
    }
    auto magma_handle = mSemaphores.create(std::move(*semaphore));
    *semaphore_out = magma_handle;
    *id_out = MAGMA_OBJECT_TO_ID(magma_handle);
    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_connection_release_semaphore(magma_connection_t connection,
                                                         magma_semaphore_t semaphore) {
    auto con = mConnections.get(connection);
    if (!con) {
        return;
    }
    auto sem = mSemaphores.get(semaphore);
    if (!sem) {
        return;
    }
    mSemaphores.erase(semaphore);
}

magma_status_t IntelDrmDecoder::magma_buffer_get_info(magma_buffer_t buffer,
                                                      magma_buffer_info_t* info_out) {
    auto buf = mBuffers.get(buffer);
    if (!buf) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    info_out->size = buf->size();
    info_out->committed_byte_count = buf->size();
    return MAGMA_STATUS_OK;
}

magma_status_t IntelDrmDecoder::magma_buffer_get_handle(magma_buffer_t buffer,
                                                        magma_handle_t* handle_out) {
    auto buf = mBuffers.get(buffer);
    if (!buf) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    *handle_out = buf->getId();
    return MAGMA_STATUS_OK;
}

magma_status_t IntelDrmDecoder::magma_buffer_export(magma_buffer_t buffer,
                                                    magma_handle_t* buffer_handle_out) {
    *buffer_handle_out = MAGMA_INVALID_OBJECT_ID;
    WARN("%s not implemented", __FUNCTION__);
    return MAGMA_STATUS_UNIMPLEMENTED;
}

void IntelDrmDecoder::magma_semaphore_signal(magma_semaphore_t semaphore) {
    auto sem = mSemaphores.get(semaphore);
    if (!sem) {
        ERR("invalid semaphore %" PRIu64, semaphore);
        return;
    }
    sem->signal();
}

void IntelDrmDecoder::magma_semaphore_reset(magma_semaphore_t semaphore) {
    auto sem = mSemaphores.get(semaphore);
    if (!sem) {
        ERR("invalid semaphore %" PRIu64, semaphore);
        return;
    }
    sem->reset();
}

magma_status_t IntelDrmDecoder::magma_poll(magma_poll_item_t* items, uint32_t count,
                                           uint64_t timeout_ns) {
    WARN("%s not implemented", __FUNCTION__);
    return MAGMA_STATUS_UNIMPLEMENTED;
}

magma_status_t IntelDrmDecoder::magma_connection_get_error(magma_connection_t connection) {
    WARN("%s not implemented", __FUNCTION__);
    return MAGMA_STATUS_UNIMPLEMENTED;
}

magma_status_t IntelDrmDecoder::magma_connection_create_context(magma_connection_t connection,
                                                                uint32_t* context_id_out) {
    *context_id_out = MAGMA_INVALID_OBJECT_ID;
    auto con = mConnections.get(connection);
    if (!con) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto ctx = con->createContext();
    if (!ctx) {
        WARN("error creating context");
        return MAGMA_STATUS_INTERNAL_ERROR;
    }
    *context_id_out = ctx.value();
    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_connection_release_context(magma_connection_t connection,
                                                       uint32_t context_id) {
    WARN("%s not implemented", __FUNCTION__);
}

constexpr uint64_t CanonicalAddress(uint64_t addr) {
    constexpr int kShift = 63 - 47;
    return static_cast<int64_t>(addr << kShift) >> kShift;
}

magma_status_t IntelDrmDecoder::magma_connection_map_buffer(magma_connection_t connection,
                                                            uint64_t hw_va, magma_buffer_t buffer,
                                                            uint64_t offset, uint64_t length,
                                                            uint64_t map_flags) {
    auto con = mConnections.get(connection);
    if (!con) {
        ERR("invalid connection %" PRIu64, connection);
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto buf = mBuffers.get(buffer);
    if (!buf) {
        ERR("invalid buffer %" PRIu64, buffer);
        return MAGMA_STATUS_INVALID_ARGS;
    }
    if (offset + length > buf->size()) {
        ERR("invalid mapping (%" PRIu64 " + %" PRIu64 " > %" PRIu64 ")", offset, length,
            buf->size());
        return MAGMA_STATUS_INVALID_ARGS;
    }

    // Magma doesn't send canonical (sign extended) addresses, but Intel DRM requires them.
    auto canonical_va = CanonicalAddress(hw_va);

    // TODO(b/279921814): Fail on overlapping regions.
    mBufferMappings.emplace(buffer, canonical_va);

    return MAGMA_STATUS_OK;
}

void IntelDrmDecoder::magma_connection_unmap_buffer(magma_connection_t connection, uint64_t hw_va,
                                                    magma_buffer_t buffer) {
    auto con = mConnections.get(connection);
    if (!con) {
        ERR("invalid connection %" PRIu64, connection);
        return;
    }
    auto buf = mBuffers.get(buffer);
    if (!buf) {
        ERR("invalid buffer %" PRIu64, buffer);
        return;
    }
    auto it = mBufferMappings.find(buffer);
    if (it == mBufferMappings.end()) {
        ERR("buffer %" PRIu64 " not mapped", buffer);
        return;
    }
    if (hw_va != it->second) {
        ERR("buffer %" PRIu64 " not mapped at gpuva %" PRIu64, hw_va);
        return;
    }
    mBufferMappings.erase(buffer);
}

magma_status_t IntelDrmDecoder::magma_connection_read_notification_channel(
    magma_connection_t connection, void* buffer, uint64_t buffer_size, uint64_t* buffer_size_out,
    magma_bool_t* more_data_out) {
    WARN("%s not implemented", __FUNCTION__);
    return MAGMA_STATUS_UNIMPLEMENTED;
}

template <typename T>
std::optional<T> ExtractPacked(void* data, size_t size, size_t& offset) {
    if (offset + sizeof(T) > size) {
        return std::nullopt;
    }
    T element = *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(data) + offset);
    offset += sizeof(T);
    return element;
}

template <typename T>
std::optional<std::vector<T>> ExtractPacked(void* data, size_t size, size_t count, size_t& offset) {
    if (count == 0) {
        return std::vector<T>();
    }
    if (offset + sizeof(T) * count > size) {
        return std::nullopt;
    }
    auto begin = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(data) + offset);
    std::vector<T> elements(begin, begin + count);
    offset += sizeof(T) * count;
    return elements;
}

magma_status_t IntelDrmDecoder::magma_connection_execute_command_fudge(
    magma_connection_t connection, uint32_t context_id, void* descriptor,
    uint64_t descriptor_size) {
    auto con = mConnections.get(connection);
    if (!con) {
        ERR("invalid connection %" PRIu64, connection);
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto ctx = con->getContext(context_id);
    if (!ctx) {
        ERR("invalid context %" PRIu64, context_id);
        return MAGMA_STATUS_INVALID_ARGS;
    }

    size_t offset = 0;
    auto resource_count = ExtractPacked<uint32_t>(descriptor, descriptor_size, offset);
    auto command_buffer_count = ExtractPacked<uint32_t>(descriptor, descriptor_size, offset);
    auto wait_semaphore_count = ExtractPacked<uint32_t>(descriptor, descriptor_size, offset);
    auto signal_semaphore_count = ExtractPacked<uint32_t>(descriptor, descriptor_size, offset);
    auto flags = ExtractPacked<uint64_t>(descriptor, descriptor_size, offset);
    if (!resource_count || !command_buffer_count || !wait_semaphore_count ||
        !signal_semaphore_count || !flags) {
        return MAGMA_STATUS_INVALID_ARGS;
    }
    auto resources = ExtractPacked<magma_exec_resource_t>(descriptor, descriptor_size,
                                                          resource_count.value(), offset);
    auto command_buffers = ExtractPacked<magma_exec_command_buffer_t>(
        descriptor, descriptor_size, command_buffer_count.value(), offset);
    auto semaphore_ids = ExtractPacked<uint64_t>(descriptor, descriptor_size,
                                                 signal_semaphore_count.value(), offset);
    if (!resources || !command_buffers || !semaphore_ids) {
        return MAGMA_STATUS_INVALID_ARGS;
    }

    if (offset != descriptor_size) {
        WARN("unused data in packed descriptor (%" PRIu64 " used vs %" PRIu64 " sent)", offset,
             descriptor_size);
    }

    std::string dump = "DUMP:";
    dump += "\nresources = ";
    for (auto& res : resources.value()) {
        dump += "(" + std::to_string(res.buffer_id) + "," + std::to_string(res.offset) + "," +
                std::to_string(res.length) + "), ";
    }
    dump += "\ncommand_buffers = ";
    for (auto& cb : command_buffers.value()) {
        dump +=
            "(" + std::to_string(cb.resource_index) + "," + std::to_string(cb.start_offset) + "), ";
    }
    dump += "\nsemaphore_ids = ";
    for (auto& sid : semaphore_ids.value()) {
        dump += std::to_string(sid) + ", ";
    }
    INFO("%s", dump.c_str());

    std::vector<drm_i915_gem_exec_object2> exec_objects;
    for (auto& resource : resources.value()) {
        auto buffer = MAGMA_ID_TO_OBJECT(resource.buffer_id);
        auto buf = mBuffers.get(buffer);
        if (!buf) {
            ERR("invalid buffer id %" PRIu64, resource.buffer_id);
            // TODO: set connection err to INVALID_ARGS
            return MAGMA_STATUS_OK;
        }
        auto it = mBufferMappings.find(buffer);
        if (it == mBufferMappings.end()) {
            ERR("buffer %" PRIu64 " not mapped", buffer);
            // TODO: set connection err to INVALID_ARGS
            return MAGMA_STATUS_OK;
        }
        auto addr = it->second;
        exec_objects.push_back({.handle = buf->getHandle(),
                                .offset = addr,
                                .flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED});
    }

    std::vector<drm_i915_gem_exec_fence> exec_fences;
    uint32_t semaphore_index = 0;
    for (auto& semaphore_id : semaphore_ids.value()) {
        auto semaphore = MAGMA_ID_TO_OBJECT(semaphore_id);
        auto sem = mSemaphores.get(semaphore);
        if (!sem) {
            ERR("invalid semaphore id %" PRIu64, semaphore_id);
            // TODO: set connection err to INVALID_ARGS
            return MAGMA_STATUS_OK;
        }
        uint32_t flags = semaphore_index++ < wait_semaphore_count ? I915_EXEC_FENCE_WAIT
                                                                  : I915_EXEC_FENCE_SIGNAL;
        exec_fences.push_back({.handle = sem->getHandle(), .flags = flags});
    }

    auto completion_semaphore = DrmSemaphore::create(con->getDevice());
    if (!completion_semaphore) {
        return MAGMA_STATUS_INTERNAL_ERROR;
    }
    exec_fences.push_back(
        {.handle = completion_semaphore->getHandle(), .flags = I915_EXEC_FENCE_SIGNAL});

    drm_i915_gem_execbuffer2 exec_params{
        .buffers_ptr = reinterpret_cast<uintptr_t>(exec_objects.data()),
        .buffer_count = static_cast<uint32_t>(exec_objects.size()),
        .batch_start_offset = static_cast<uint32_t>(command_buffers.value()[0].start_offset),
        .batch_len = static_cast<uint32_t>(resources.value().rbegin()->length),
        .num_cliprects = static_cast<uint32_t>(exec_fences.size()),  // Actually FENCE_ARRAY
        .cliprects_ptr = reinterpret_cast<uintptr_t>(exec_fences.data()),
        .flags =
            I915_EXEC_NO_RELOC | I915_EXEC_RENDER | I915_EXEC_HANDLE_LUT | I915_EXEC_FENCE_ARRAY,
        .rsvd1 = ctx->getId()};
    int result = con->getDevice().ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2, &exec_params);
    if (result) {
        ERR("DRM_IOCTL_I915_GEM_EXECBUFFER2 failed: %d", errno);
        // TODO: set connection err to INTERNAL_ERROR
        return MAGMA_STATUS_OK;
    }

    std::vector<magma_buffer_id_t> buffer_ids;
    for (auto& resource : resources.value()) {
        buffer_ids.push_back(resource.buffer_id);
    }
    con->addCommand(std::move(buffer_ids), std::move(*completion_semaphore));

    INFO("successfully executed %" PRIu64 " buffers", resources->size());

    return MAGMA_STATUS_OK;
}

}  // namespace magma
}  // namespace gfxstream
