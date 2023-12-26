#pragma once
#include "Buffer.h"

template <typename T>
class UniformBuffer : public CpuBuffer
{
public:
    UniformBuffer(class Device &device);

    void Set(const T &data);
};

template <typename T>
inline UniformBuffer<T>::UniformBuffer(Device &device)
    : CpuBuffer(device,sizeof(T),BufferUsage::UNIFORM)
{
}

template <typename T>
inline void UniformBuffer<T>::Set(const T &data)
{
    this->Fill(0,sizeof(T), (void *)&data);
}