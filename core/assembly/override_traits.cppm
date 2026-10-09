// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <type_traits>

export module nom.assembly:override_traits;

namespace nom
{
    namespace detail
    {
        template <typename Traits, typename NodeTraits> struct use_override_if_defined
        {
            using type = NodeTraits;
        };

        // Inheritance opts an override into this node's traits; unrelated nodes keep their defaults.
        template <typename Traits, typename NodeTraits>
            requires requires {
                typename Traits::override_traits;
                requires std::is_base_of_v<NodeTraits, typename Traits::override_traits>;
            }
        struct use_override_if_defined<Traits, NodeTraits>
        {
            using type = typename Traits::override_traits;
        };
    } // namespace detail
} // namespace nom

export namespace nom
{
    template <typename Traits, typename NodeTraits>
    using use_override_if_defined_t = typename detail::use_override_if_defined<Traits, NodeTraits>::type;
} // namespace nom
