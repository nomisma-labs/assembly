// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <type_traits>

export module nom.assembly:custom_traits;

namespace nom::detail
{
    template <typename Traits, typename NodeTraits> struct apply_custom_traits
    {
        using type = NodeTraits;
    };

    // Inheritance opts custom traits into this node's traits; unrelated nodes keep their defaults.
    template <typename Traits, typename NodeTraits>
        requires requires {
            typename Traits::custom_traits;
            requires std::is_base_of_v<NodeTraits, typename Traits::custom_traits>;
        }
    struct apply_custom_traits<Traits, NodeTraits>
    {
        using type = typename Traits::custom_traits;
    };
} // namespace nom::detail

export namespace nom
{
    template <typename Traits, typename NodeTraits>
    using apply_custom_traits_t = typename detail::apply_custom_traits<Traits, NodeTraits>::type;
} // namespace nom
