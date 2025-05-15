#include <ituGL/geometry/ShaderStorageBufferObject.h>

ShaderStorageBufferObject::ShaderStorageBufferObject() : BufferObjectBase()
{
    // Nothing to do here, it is done by the base class
}

void ShaderStorageBufferObject::BindSSBO(int index)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, GetHandle());
}
