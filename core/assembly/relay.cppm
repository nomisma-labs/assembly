// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <type_traits>
#include <utility>

#include <metal/lambda.hpp>
#include <metal/list/append.hpp>
#include <metal/list/at.hpp>
#include <metal/list/contains.hpp>
#include <metal/list/drop.hpp>
#include <metal/list/find.hpp>
#include <metal/list/find_if.hpp>
#include <metal/list/reverse.hpp>
#include <metal/list/size.hpp>
#include <metal/number/number.hpp>

export module nom.assembly:relay;

import :assembly_impl;
import :custom_traits;

namespace nom
{
    namespace detail
    {
        template <typename Traits> struct request_suppressor
        {
            template <typename Event>
                requires(metal::contains<typename Traits::suppressed_requests, std::remove_cvref_t<Event>>::value != 0)
            void on_request(Event&&) noexcept
            {
            }
        };

        template <typename Traits> struct response_suppressor
        {
            template <typename Event>
                requires(metal::contains<typename Traits::suppressed_responses, std::remove_cvref_t<Event>>::value != 0)
            void on_response(Event&&) noexcept
            {
            }
        };

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

        template <typename Nodes, typename... Messages>
        using request_position = metal::find_if<Nodes, request_condition<Messages...>>;

        template <typename Nodes, typename... Messages>
        using response_position = metal::find_if<Nodes, response_condition<Messages...>>;

        template <typename Nodes, typename Node>
        using nodes_after = metal::drop<Nodes, metal::number<metal::find<Nodes, Node>::value + 1>>;

        // Keep Metal expressions behind booleans so failed constraints report Relay and Messages
        // without expanding the typelist implementation in compiler diagnostics.
        template <typename Assembly, typename Node>
        inline constexpr bool is_assembly_node = metal::find<typename Assembly::nodes, Node>::value < Assembly::size;

        template <typename Assembly, typename Node>
        concept assembly_node = is_assembly_node<Assembly, Node>;

        // Each direction has one suppressor after the application nodes (hence "+ 1").
        template <typename Relay, typename... Messages>
        inline constexpr bool request_is_handled =
            request_position<typename Relay::nodes, Messages...>::value < Relay::assembly::size + 1;

        template <typename Relay, typename... Messages>
        concept handles_request = request_is_handled<Relay, Messages...>;

        template <typename Relay, typename... Messages>
        inline constexpr bool response_is_handled =
            response_position<typename Relay::reverse_nodes, Messages...>::value < Relay::assembly::size + 1;

        template <typename Relay, typename... Messages>
        concept handles_response = response_is_handled<Relay, Messages...>;

        template <typename Relay, typename Node, typename... Messages>
        inline constexpr bool request_is_handled_after =
            request_position<nodes_after<typename Relay::nodes, Node>, Messages...>::value <
            metal::size<nodes_after<typename Relay::nodes, Node>>::value;

        template <typename Relay, typename Node, typename... Messages>
        inline constexpr bool response_is_handled_after =
            response_position<nodes_after<typename Relay::reverse_nodes, Node>, Messages...>::value <
            metal::size<nodes_after<typename Relay::reverse_nodes, Node>>::value;

        // Check membership before forming a tail: an unknown source would otherwise drop past the list.
        template <typename Relay, typename Node, typename... Messages>
        concept handles_request_after =
            assembly_node<typename Relay::assembly, Node> && request_is_handled_after<Relay, Node, Messages...>;

        template <typename Relay, typename Node, typename... Messages>
        concept handles_response_after =
            assembly_node<typename Relay::assembly, Node> && response_is_handled_after<Relay, Node, Messages...>;
    } // namespace detail
} // namespace nom

export namespace nom
{
    struct event_suppressors
    {
        using suppressed_requests = type_list<>;
        using suppressed_responses = type_list<>;
    };

    template <typename Assembly> class relay
    {
        struct event_suppressors : apply_custom_traits_t<Assembly, ::nom::event_suppressors>
        {
        };

        using request_suppressor = detail::request_suppressor<event_suppressors>;
        using response_suppressor = detail::response_suppressor<event_suppressors>;
        struct body : private request_suppressor, private response_suppressor, public Assembly::body
        {
            friend relay;

            using Assembly::body::body;
        };

    public:
        using assembly = Assembly;
        using context = typename Assembly::body::context;
        using nodes = metal::append<typename Assembly::nodes, request_suppressor>;
        using reverse_nodes = metal::append<metal::reverse<typename Assembly::nodes>, response_suppressor>;

        static_assert(std::is_same_v<typename Assembly::traits, Assembly>,
                      "Assembly must derive from assembly<Assembly, Nodes...>");

        relay() = default;

        template <typename... Args>
            requires std::is_constructible_v<body, const Args&...>
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

        template <typename... Messages>
            requires detail::handles_request<relay, Messages...>
        void request(Messages&&... messages)
        {
            using node = metal::at<nodes, detail::request_position<nodes, Messages...>>;
            static_cast<node&>(assembly_).on_request(std::forward<Messages>(messages)...);
        }

        template <typename... Messages>
            requires detail::handles_response<relay, Messages...>
        void respond(Messages&&... messages)
        {
            using node = metal::at<reverse_nodes, detail::response_position<reverse_nodes, Messages...>>;
            static_cast<node&>(assembly_).on_response(std::forward<Messages>(messages)...);
        }

        template <typename Node, typename... Messages>
            requires detail::handles_request_after<relay, Node, Messages...>
        static void pass_request(Node* source, Messages&&... messages)
        {
            using remaining = detail::nodes_after<nodes, Node>;
            using node = metal::at<remaining, detail::request_position<remaining, Messages...>>;
            static_cast<node&>(static_cast<body&>(*source)).on_request(std::forward<Messages>(messages)...);
        }

        template <typename Node, typename... Messages>
            requires detail::handles_response_after<relay, Node, Messages...>
        static void pass_response(Node* source, Messages&&... messages)
        {
            using remaining = detail::nodes_after<reverse_nodes, Node>;
            using node = metal::at<remaining, detail::response_position<remaining, Messages...>>;
            static_cast<node&>(static_cast<body&>(*source)).on_response(std::forward<Messages>(messages)...);
        }

    private:
        body assembly_;
    };
} // namespace nom
