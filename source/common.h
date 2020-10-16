#pragma once

#include <cstdint>

namespace camscii
{

struct Frame
{
    void* start;
    std::size_t length;
};

} // namespace camscii
