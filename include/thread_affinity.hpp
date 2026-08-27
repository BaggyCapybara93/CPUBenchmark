#pragma once

#include <cstddef>

namespace thread_affinity {

// Pins the calling thread to a logical CPU when the platform supports it.
// macOS uses an advisory affinity tag; unsupported or rejected requests return false.
bool pin_current_thread(std::size_t logical_cpu) noexcept;

}  // namespace thread_affinity
