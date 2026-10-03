// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <type_traits>
#include <utility>

#include <metal/lambda.hpp>
#include <metal/list/at.hpp>
#include <metal/list/drop.hpp>
#include <metal/list/find.hpp>
#include <metal/list/find_if.hpp>
#include <metal/list/reverse.hpp>
#include <metal/list/size.hpp>
#include <metal/number/number.hpp>

export module nom.assembly:relay;

import :composition;

namespace nom
{
    namespace detail
    {
        template <typename Node, typename... Messages>
        using has_on_request = metal::number<requires(Node& node, Messages&&... messages) {
            node.on_request(std::forward<Messages>(messages)...);
        }>;

        template <typename Node, typename... Messages>
        using has_on_response = metal::number<requires(Node& node, Messages&&... messages) {
            node.on_response(std::forward<Messages>(messages)...);
        }>;

        template <typename... Messages>
        using request_condition = metal::bind<metal::lambda<has_on_request>, metal::_1, metal::always<Messages>...>;

        template <typename... Messages>
        using response_condition = metal::bind<metal::lambda<has_on_response>, metal::_1, metal::always<Messages>...>;
    } // namespace detail
} // namespace nom

export namespace nom
{
    template <typename Assembly> class relay
    {
    public:
        using assembly_type = Assembly;
        using context = typename Assembly::body::context;
        using nodes = typename Assembly::nodes;
        using reverse_nodes = metal::reverse<nodes>;

        static_assert(std::is_same_v<typename Assembly::traits, Assembly>,
                      "The recipe must derive from assembly<Recipe, Nodes...>");

        relay() = default;

        template <typename... Args>
            requires std::is_constructible_v<typename Assembly::body, const Args&...>
        explicit relay(const Args&... args) : assembly_(args...)
        {
        }

        context& ctx() noexcept
        {
            return assembly_.ctx();
        }

        const context& ctx() const noexcept
        {
            return assembly_.ctx();
        }

        template <typename... Messages> void request(Messages&&... messages)
        {
            dispatch_request<nodes>(assembly_, std::forward<Messages>(messages)...);
        }

        template <typename... Messages> void respond(Messages&&... messages)
        {
            dispatch_response<reverse_nodes>(assembly_, std::forward<Messages>(messages)...);
        }

        template <typename Node, typename... Messages> static void pass_request(Node* source, Messages&&... messages)
        {
            using position = metal::find<nodes, Node>;
            static_assert(position::value < Assembly::size, "The source must be a node of this assembly");
            if constexpr (position::value < Assembly::size)
            {
                using remaining = metal::drop<nodes, metal::number<position::value + 1>>;
                dispatch_request<remaining>(static_cast<body&>(*source), std::forward<Messages>(messages)...);
            }
        }

        template <typename Node, typename... Messages> static void pass_response(Node* source, Messages&&... messages)
        {
            using position = metal::find<reverse_nodes, Node>;
            static_assert(position::value < Assembly::size, "The source must be a node of this assembly");
            if constexpr (position::value < Assembly::size)
            {
                using remaining = metal::drop<reverse_nodes, metal::number<position::value + 1>>;
                dispatch_response<remaining>(static_cast<body&>(*source), std::forward<Messages>(messages)...);
            }
        }

    private:
        using body = typename Assembly::body;

        template <typename Candidates, typename... Messages>
        static void dispatch_request(body& owner, Messages&&... messages)
        {
            using position = metal::find_if<Candidates, detail::request_condition<Messages...>>;
            static_assert(position::value < metal::size<Candidates>::value,
                          "No reachable node handles on_request with these arguments");
            if constexpr (position::value < metal::size<Candidates>::value)
            {
                using node = metal::at<Candidates, position>;
                static_cast<node&>(owner).on_request(std::forward<Messages>(messages)...);
            }
        }

        template <typename Candidates, typename... Messages>
        static void dispatch_response(body& owner, Messages&&... messages)
        {
            using position = metal::find_if<Candidates, detail::response_condition<Messages...>>;
            static_assert(position::value < metal::size<Candidates>::value,
                          "No reachable node handles on_response with these arguments");
            if constexpr (position::value < metal::size<Candidates>::value)
            {
                using node = metal::at<Candidates, position>;
                static_cast<node&>(owner).on_response(std::forward<Messages>(messages)...);
            }
        }

        body assembly_;
    };
} // namespace nom
