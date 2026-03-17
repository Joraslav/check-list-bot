#pragma once

#include <ranges>
#include <type_traits>
#include <utility>

namespace utils {

/**
 * @brief Returns a range of enum values
 * @tparam Enum
 * @param last Last enum value. Each Enum must contain the last element - `Enum::COUNT`
 * @return Range of enum values
 */
template <typename Enum>
    requires std::is_enum_v<Enum>
constexpr auto EnumRange(Enum last = Enum::COUNT) {
    return std::views::iota(std::underlying_type_t<Enum>(0), std::to_underlying(last)) |
           std::views::transform(
               [](std::underlying_type_t<Enum> value) { return static_cast<Enum>(value); });
}

}  // namespace utils
