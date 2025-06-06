#pragma once

#include <cstdint>

namespace cryptonote
{
  enum class relay_method : std::uint8_t
  {
    none = 0,
    local,
    block,
    flood
  };
}
