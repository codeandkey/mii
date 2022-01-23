#pragma once

#include <stdexcept>
#include <string>

#include <time.h>
#include <unistd.h>

namespace mii {
namespace options {

    std::string prefix(std::string arg0="");
    std::string version();

} // namespace options
} // namespace mii
