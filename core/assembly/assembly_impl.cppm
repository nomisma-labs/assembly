// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <cstddef>
#include <type_traits>

#include <metal/lambda/lambda.hpp>
#include <metal/list/at.hpp>
#include <metal/list/copy_if.hpp>
#include <metal/list/list.hpp>
#include <metal/list/size.hpp>
#include <metal/number/number.hpp>

export module nom.assembly:assembly_impl;

namespace nom
{
    namespace detail
    {
        // Configuration is shared: each base receives the same const references or uses its default constructor.
        template <typename Base> struct constructible : Base
        {
            template <typename... Args>
                requires std::is_constructible_v<Base, const Args&...>
            explicit constructible(const Args&... args) : Base(args...)
            {
            }

            template <typename... Args>
                requires(!std::is_constructible_v<Base, const Args&...> && std::is_default_constructible_v<Base>)
            explicit constructible(const Args&...) : Base()
            {
            }
        };

        template <typename Nodes> struct constructible_nodes;
        template <typename... Nodes> struct constructible_nodes<metal::list<Nodes...>> : constructible<Nodes>...
        {
            template <typename... Args>
                requires(std::is_constructible_v<constructible<Nodes>, const Args&...> && ...)
            explicit constructible_nodes(const Args&... args) : constructible<Nodes>(args...)...
            {
            }
        };

        template <typename Node> using has_context = metal::number<requires { typename Node::context_mixin; }>;

        template <typename Nodes> struct context_holder;
        template <typename... Nodes>
        struct context_holder<metal::list<Nodes...>> : constructible<typename Nodes::context_mixin>...
        {
            template <typename... Args>
                requires(std::is_constructible_v<constructible<typename Nodes::context_mixin>, const Args&...> && ...)
            explicit context_holder(const Args&... args) : constructible<typename Nodes::context_mixin>(args...)...
            {
            }
        };

        template <typename Nodes>
        using collected_context = context_holder<metal::copy_if<Nodes, metal::lambda<has_context>>>;

        // Base order is intentional: every context service exists before the first node is constructed.
        template <typename Nodes> struct assembly_body : collected_context<Nodes>, constructible_nodes<Nodes>
        {
            using context = collected_context<Nodes>;

            template <typename... Args>
                requires(std::is_constructible_v<context, const Args&...> &&
                         std::is_constructible_v<constructible_nodes<Nodes>, const Args&...>)
            explicit assembly_body(const Args&... args) : context(args...), constructible_nodes<Nodes>(args...)
            {
            }

            context& ctx() noexcept
            {
                return *this;
            }

            const context& ctx() const noexcept
            {
                return *this;
            }

            template <std::size_t I> auto& get() noexcept
            {
                using node = metal::at<Nodes, metal::number<I>>;
                return static_cast<node&>(*this);
            }

            template <std::size_t I> const auto& get() const noexcept
            {
                using node = metal::at<Nodes, metal::number<I>>;
                return static_cast<const node&>(*this);
            }
        };
    } // namespace detail
} // namespace nom

export namespace nom
{
    // For context lookup and relay routing, Traits is the final recipe derived from this specialization.
    template <typename Traits, template <typename> class... Nodes> struct assembly
    {
        using traits = Traits;
        using nodes = metal::list<Nodes<Traits>...>;
        static constexpr std::size_t size = sizeof...(Nodes);
        using body = detail::assembly_body<nodes>;
    };

    // NodeOrMixin must be a Node<Traits> or Mixin<Traits> base of Traits::body.
    template <template <typename> class NodeOrMixin, typename Traits>
    inline auto& ctx(NodeOrMixin<Traits>* part) noexcept
    {
        // A node constructor can run before all body bases are initialized. Access the already
        // constructed context directly, without calling a member on the unfinished body.
        return static_cast<typename Traits::body::context&>(static_cast<typename Traits::body&>(*part));
    }

    template <template <typename> class NodeOrMixin, typename Traits>
    inline const auto& ctx(const NodeOrMixin<Traits>* part) noexcept
    {
        return static_cast<const typename Traits::body::context&>(static_cast<const typename Traits::body&>(*part));
    }
} // namespace nom
