#pragma once

#include <vulkan/vulkan_raii.hpp>

// Borrow native handles for the existing C command recording and external APIs.
// Ownership always stays with the vk::raii object.
template <typename T> auto raw(const T &owner) { return static_cast<typename T::CType>(*owner); }

template <typename T> const typename T::CType *rawPtr(const T &owner)
{
    static_assert(sizeof(*owner) == sizeof(typename T::CType));
    return reinterpret_cast<const typename T::CType *>(&*owner);
}
