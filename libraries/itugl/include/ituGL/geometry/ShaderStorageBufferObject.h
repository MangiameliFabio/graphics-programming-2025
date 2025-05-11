#pragma once

#include <ituGL/core/BufferObject.h>
#include <ituGL/core/Data.h>
#include <cassert>

// Element Buffer Object (VBO) is the common term for a BufferObject when it is storing a list of indices to vertices
class ShaderStorageBufferObject : public BufferObjectBase<BufferObject::ShaderStorageBufferObject>
{
public:
    ShaderStorageBufferObject();
};
