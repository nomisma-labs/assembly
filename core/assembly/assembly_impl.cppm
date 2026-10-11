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
        // Initialization arguments are shared: each base receives const references or uses its default constructor.
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

        // Inspect component constructors themselves: constructible<> also accepts ignored arguments.
        template <typename Node, typename... Args>
        concept init_argument_consumer = std::is_constructible_v<Node, const Args&...> || requires {
            requires std::is_constructible_v<typename Node::context_mixin, const Args&...>;
        };

        template <typename Nodes, typename... Args> inline constexpr bool init_arguments_are_accepted = false;
        template <typename... Nodes, typename... Args>
        inline constexpr bool init_arguments_are_accepted<metal::list<Nodes...>, Args...> =
            (init_argument_consumer<Nodes, Args...> || ...);

        template <typename Nodes, typename... Args>
        concept accepts_init_arguments = sizeof...(Args) == 0 || init_arguments_are_accepted<Nodes, Args...>;

        // Base order is intentional: every context service exists before the first node is constructed.
        template <typename Nodes> struct assembly_body : collected_context<Nodes>, constructible_nodes<Nodes>
        {
            using context = collected_context<Nodes>;

            template <typename... Args>
                requires accepts_init_arguments<Nodes, Args...> &&
                             (std::is_constructible_v<context, const Args&...> &&
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

        template <typename Assembly, typename Part>
        concept context_mixin =
            std::is_class_v<Part> && requires(Part* part) { static_cast<typename Assembly::body::context*>(part); };

        template <typename Assembly, typename Part>
        concept context_part =
            context_mixin<Assembly, Part> || (std::is_class_v<Part> && requires(Part* part) {
                static_cast<typename Assembly::body::context*>(static_cast<typename Assembly::body*>(part));
            });

        template <typename From, typename To>
        using copy_const = std::conditional_t<std::is_const_v<From>, const To, To>;

        // Part carries the source's constness through either cast path.
        template <typename Assembly, typename Part>
            requires context_part<Assembly, std::remove_const_t<Part>>
        inline auto& ctx(Part* part) noexcept
        {
            using context = copy_const<Part, typename Assembly::body::context>;
            if constexpr (context_mixin<Assembly, std::remove_const_t<Part>>)
                return *static_cast<context*>(part);
            else
            {
                using body = copy_const<Part, typename Assembly::body>;
                return *static_cast<context*>(static_cast<body*>(part));
            }
        }
    } // namespace detail
} // namespace nom

export namespace nom
{
    template <typename... Types> using type_list = metal::list<Types...>;

    // For context lookup and relay routing, Traits is the final assembly type derived from this specialization.
    template <typename Traits, template <typename> class... Nodes> struct assembly
    {
        using traits = Traits;
        using nodes = type_list<Nodes<Traits>...>;
        static constexpr std::size_t size = sizeof...(Nodes);
        using body = detail::assembly_body<nodes>;
    };

    // An explicit Assembly also supports nested mixins, whose enclosing Traits cannot be deduced.
    template <typename Assembly, typename Part>
        requires detail::context_part<Assembly, Part>
    inline auto& ctx(Part* part) noexcept
    {
        return detail::ctx<Assembly, Part>(part);
    }

    template <typename Assembly, typename Part>
        requires detail::context_part<Assembly, Part>
    inline const auto& ctx(const Part* part) noexcept
    {
        return detail::ctx<Assembly, const Part>(part);
    }

    // Deduce Traits from Node<Traits> or Mixin<Traits>.
    template <template <typename> class NodeOrMixin, typename Traits>
        requires detail::context_part<Traits, NodeOrMixin<Traits>>
    inline auto& ctx(NodeOrMixin<Traits>* part) noexcept
    {
        return detail::ctx<Traits, NodeOrMixin<Traits>>(part);
    }

    template <template <typename> class NodeOrMixin, typename Traits>
        requires detail::context_part<Traits, NodeOrMixin<Traits>>
    inline const auto& ctx(const NodeOrMixin<Traits>* part) noexcept
    {
        return detail::ctx<Traits, const NodeOrMixin<Traits>>(part);
    }
} // namespace nom
