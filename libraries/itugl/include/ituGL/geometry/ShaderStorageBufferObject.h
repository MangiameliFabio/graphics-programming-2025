#pragma once

#include <ituGL/core/BufferObject.h>
#include <ituGL/core/Data.h>
#include <cassert>

// Element Buffer Object (VBO) is the common term for a BufferObject when it is storing a list of indices to vertices
class ShaderStorageBufferObject : public BufferObjectBase<BufferObject::ShaderStorageBufferObject>
{
public:
    ShaderStorageBufferObject();

    // AllocateData template method that replaces the size with the element count
    template<typename T>
    void AllocateData(size_t elementCount, Usage usage = Usage::StaticDraw);
    // AllocateData template method for any type of data span. Type must be one of the supported types
    template<typename T>
    void AllocateData(std::span<const T> data, Usage usage = Usage::StaticDraw);
    template<typename T>
    inline void AllocateData(std::span<T> data, Usage usage = Usage::StaticDraw) { AllocateData(std::span<const T>(data), usage); }
    inline void AllocateData(size_t size, const void* data, Usage usage = Usage::StaticDraw);

    // UpdateData template method for any type of data span. Type must be one of the supported types
    template<typename T>
    void UpdateData(std::span<const T> data, size_t offsetBytes = 0);

    void BindSSBO(int index);
};

// Call the base implementation with the buffer size, computed with elementCount and size of T
template<typename T>
void ShaderStorageBufferObject::AllocateData(size_t elementCount, Usage usage)
{
    BufferObject::AllocateData(elementCount * sizeof(T), usage);
}

// Call the base implementation with the span converted to bytes
template<typename T>
void ShaderStorageBufferObject::AllocateData(std::span<const T> data, Usage usage)
{
    BufferObject::AllocateData(Data::GetBytes(data), usage);
}

void ShaderStorageBufferObject::AllocateData(size_t size, const void* data, Usage usage)
{
    assert(IsBound());
    Target target = GetTarget();
    glBufferData(target, size, data, usage);
}

// Call the base implementation with the span converted to bytes
template<typename T>
void ShaderStorageBufferObject::UpdateData(std::span<const T> data, size_t offsetBytes)
{
    BufferObject::UpdateData(Data::GetBytes(data), offsetBytes);
}
