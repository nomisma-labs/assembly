// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>

import std;

import nom.assembly;

namespace
{
    template <typename Relay, typename... Messages>
    concept accepts_request =
        requires(Relay& relay, Messages&&... messages) { relay.request(std::forward<Messages>(messages)...); };

    template <typename Relay, typename... Messages>
    concept accepts_response =
        requires(Relay& relay, Messages&&... messages) { relay.respond(std::forward<Messages>(messages)...); };

    template <typename Relay, typename Node, typename... Messages>
    concept forwards_request = requires(Node* source, Messages&&... messages) {
        Relay::pass_request(source, std::forward<Messages>(messages)...);
    };

    template <typename Relay, typename Node, typename... Messages>
    concept forwards_response = requires(Node* source, Messages&&... messages) {
        Relay::pass_response(source, std::forward<Messages>(messages)...);
    };

    struct raw_value
    {
        int value;
    };

    struct normalized_value
    {
        int value;
    };

    struct receipt
    {
        int value;
    };

    struct health_check
    {
    };

    template <typename Traits> struct client
    {
        struct context_mixin
        {
            std::vector<std::string> trace;
            int received = 0;
            int health_checks = 0;
        };

        void on_request(const raw_value& event)
        {
            nom::ctx(this).trace.push_back("client:request");
            nom::relay<Traits>::pass_request(this, event);
            nom::ctx(this).trace.push_back("client:return");
        }

        void on_response(const receipt& event)
        {
            auto& shared = nom::ctx(this);
            shared.trace.push_back("client:response");
            shared.received = event.value;
        }
    };

    template <typename Traits> struct normalizer
    {
        void on_request(const raw_value& event)
        {
            auto& trace = nom::ctx(this).trace;
            if (event.value < 0)
            {
                trace.push_back("normalizer:consume");
                return; // Handling an event does not automatically forward it.
            }
            trace.push_back("normalizer:transform");
            nom::relay<Traits>::pass_request(this, normalized_value{event.value * 10});
        }

        void on_response(const receipt& event)
        {
            nom::ctx(this).trace.push_back("normalizer:response");
            nom::relay<Traits>::pass_response(this, event);
        }
    };

    // This node belongs to the assembly but participates in a different event route.
    template <typename Traits> struct health_monitor
    {
        void on_request(health_check)
        {
            ++nom::ctx(this).health_checks;
        }
    };

    template <typename Traits> struct endpoint
    {
        void on_request(const normalized_value& event)
        {
            nom::ctx(this).trace.push_back("endpoint:request");
            nom::relay<Traits>::pass_response(this, receipt{event.value});
        }

        void on_response(const receipt& event)
        {
            nom::ctx(this).trace.push_back("endpoint:response");
            nom::relay<Traits>::pass_response(this, event);
        }
    };

    struct routing_assembly : nom::assembly<routing_assembly, client, normalizer, health_monitor, endpoint>
    {
    };

    template <typename Traits> struct transport_forwarder
    {
        void on_request(int& count, std::string& text)
        {
            ++count;
            text += "forwarder ";
            nom::relay<Traits>::pass_request(this, count, text);
        }

        void on_request(std::unique_ptr<int>&& value)
        {
            nom::relay<Traits>::pass_request(this, std::move(value));
        }

        void on_response(std::unique_ptr<int>&& value)
        {
            nom::ctx(this).owned = std::move(value);
        }
    };

    template <typename Traits> struct transport_receiver
    {
        struct context_mixin
        {
            int* observed_count = nullptr;
            std::string* observed_text = nullptr;
            std::unique_ptr<int> owned;
            std::vector<std::string> categories;
        };

        void on_request(int& count, std::string& text)
        {
            auto& shared = nom::ctx(this);
            shared.observed_count = &count;
            shared.observed_text = &text;
            ++count;
            text += "receiver";
        }

        void on_request(std::unique_ptr<int>&& value)
        {
            nom::ctx(this).owned = std::move(value);
        }

        void on_response(std::unique_ptr<int>&& value)
        {
            nom::relay<Traits>::pass_response(this, std::move(value));
        }

        void on_request(std::string&)
        {
            nom::ctx(this).categories.push_back("mutable lvalue");
        }

        void on_request(const std::string&)
        {
            nom::ctx(this).categories.push_back("const lvalue");
        }

        void on_request(std::string&&)
        {
            nom::ctx(this).categories.push_back("rvalue");
        }
    };

    struct transport_assembly : nom::assembly<transport_assembly, transport_forwarder, transport_receiver>
    {
    };

    struct empty_traits : nom::assembly<empty_traits>
    {
    };

    struct optional_notice
    {
        bool consume = false;
    };

    struct unobserved_request
    {
    };

    struct unobserved_response
    {
    };

    struct emit_unobserved
    {
    };

    struct emit_move_only
    {
        std::unique_ptr<int>& value;
    };

    struct convertible_notice
    {
        operator unobserved_request() const
        {
            return {};
        }

        operator unobserved_response() const
        {
            return {};
        }
    };

    template <typename Traits> struct first_observer
    {
        struct context_mixin
        {
            std::vector<std::string> trace;
        };

        void on_request(const optional_notice& event)
        {
            if (event.consume)
            {
                nom::ctx(this).trace.push_back("first:consume request");
                return;
            }
            nom::ctx(this).trace.push_back("first:request");
            nom::relay<Traits>::pass_request(this, event);
            nom::ctx(this).trace.push_back("first:return");
        }

        void on_response(const optional_notice& event)
        {
            nom::ctx(this).trace.push_back("first:response");
            nom::relay<Traits>::pass_response(this, event);
        }

        void on_response(emit_unobserved)
        {
            nom::ctx(this).trace.push_back("first:emit response");
            nom::relay<Traits>::pass_response(this, unobserved_response{});
        }

        void on_response(emit_move_only event)
        {
            nom::ctx(this).trace.push_back("first:emit response");
            nom::relay<Traits>::pass_response(this, std::move(event.value));
        }
    };

    template <typename Traits> struct passive_node
    {
    };

    template <typename Traits> struct last_observer
    {
        void on_request(const optional_notice& event)
        {
            nom::ctx(this).trace.push_back("last:request");
            nom::relay<Traits>::pass_request(this, event);
        }

        void on_request(emit_unobserved)
        {
            nom::ctx(this).trace.push_back("last:emit request");
            nom::relay<Traits>::pass_request(this, unobserved_request{});
        }

        void on_request(emit_move_only event)
        {
            nom::ctx(this).trace.push_back("last:emit request");
            nom::relay<Traits>::pass_request(this, std::move(event.value));
        }

        void on_response(const optional_notice& event)
        {
            if (event.consume)
            {
                nom::ctx(this).trace.push_back("last:consume response");
                return;
            }
            nom::ctx(this).trace.push_back("last:response");
            nom::relay<Traits>::pass_response(this, event);
        }
    };

    struct optional_assembly : nom::assembly<optional_assembly, first_observer, passive_node, last_observer>
    {
        struct custom_traits : nom::event_suppressors
        {
            using suppressed_requests = nom::type_list<optional_notice, unobserved_request, std::unique_ptr<int>>;
            using suppressed_responses = nom::type_list<optional_notice, unobserved_response, std::unique_ptr<int>>;
        };
    };

    // No application nodes are needed to suppress an optional event.
    struct request_only_assembly : nom::assembly<request_only_assembly>
    {
        struct custom_traits : nom::event_suppressors
        {
            using suppressed_requests = nom::type_list<unobserved_request>;
        };
    };

    struct response_only_assembly : nom::assembly<response_only_assembly>
    {
        struct custom_traits : nom::event_suppressors
        {
            using suppressed_responses = nom::type_list<unobserved_response>;
        };
    };

    struct unrelated_custom_assembly : nom::assembly<unrelated_custom_assembly, passive_node>
    {
        struct custom_traits
        {
            using suppressed_requests = nom::type_list<unobserved_request>;
            using suppressed_responses = nom::type_list<unobserved_response>;
        };
    };
} // namespace

TEST_CASE("Public relay constraints distinguish request and response routes", "[relay]")
{
    using relay = nom::relay<routing_assembly>;

    STATIC_REQUIRE(accepts_request<relay, raw_value>);
    STATIC_REQUIRE(accepts_request<relay, normalized_value>);
    STATIC_REQUIRE(accepts_request<relay, health_check>);
    STATIC_REQUIRE(accepts_response<relay, receipt>);

    STATIC_REQUIRE_FALSE(accepts_request<relay, receipt>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, raw_value>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, std::string>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, std::string>);
    STATIC_REQUIRE_FALSE(accepts_request<relay>);
    STATIC_REQUIRE_FALSE(accepts_response<relay>);
}

TEST_CASE("Forwarding constraints consider only nodes beyond the source in the route direction", "[relay]")
{
    using relay = nom::relay<routing_assembly>;

    STATIC_REQUIRE(forwards_request<relay, client<routing_assembly>, raw_value>);
    STATIC_REQUIRE(forwards_request<relay, normalizer<routing_assembly>, normalized_value>);
    STATIC_REQUIRE(forwards_request<relay, health_monitor<routing_assembly>, normalized_value>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, normalizer<routing_assembly>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, endpoint<routing_assembly>, normalized_value>);

    STATIC_REQUIRE(forwards_response<relay, endpoint<routing_assembly>, receipt>);
    STATIC_REQUIRE(forwards_response<relay, health_monitor<routing_assembly>, receipt>);
    STATIC_REQUIRE(forwards_response<relay, normalizer<routing_assembly>, receipt>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, client<routing_assembly>, receipt>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, endpoint<routing_assembly>, raw_value>);
}

TEST_CASE("Invalid forwarding sources and empty assemblies fail constraints without hard errors", "[relay]")
{
    using relay = nom::relay<routing_assembly>;
    using foreign_node = transport_receiver<transport_assembly>;
    using empty_relay = nom::relay<empty_traits>;

    STATIC_REQUIRE_FALSE(forwards_request<relay, foreign_node, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, foreign_node, receipt>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, const client<routing_assembly>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, const endpoint<routing_assembly>, receipt>);

    STATIC_REQUIRE_FALSE(accepts_request<empty_relay, raw_value>);
    STATIC_REQUIRE_FALSE(accepts_response<empty_relay, receipt>);
    STATIC_REQUIRE_FALSE(forwards_request<empty_relay, client<routing_assembly>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<empty_relay, endpoint<routing_assembly>, receipt>);
}

TEST_CASE("Route constraints preserve argument constness and value categories", "[relay]")
{
    using relay = nom::relay<transport_assembly>;

    STATIC_REQUIRE(accepts_request<relay, std::string&>);
    STATIC_REQUIRE(accepts_request<relay, const std::string&>);
    STATIC_REQUIRE(accepts_request<relay, std::string>);
    STATIC_REQUIRE(accepts_request<relay, int&, std::string&>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, const int&, std::string&>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, int&, std::string>);

    STATIC_REQUIRE(accepts_request<relay, std::unique_ptr<int>>);
    STATIC_REQUIRE(accepts_response<relay, std::unique_ptr<int>>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, std::unique_ptr<int>&>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, std::unique_ptr<int>&>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, const std::unique_ptr<int>>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, const std::unique_ptr<int>>);

    STATIC_REQUIRE(forwards_request<relay, transport_forwarder<transport_assembly>, std::unique_ptr<int>>);
    STATIC_REQUIRE(forwards_response<relay, transport_receiver<transport_assembly>, std::unique_ptr<int>>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, transport_forwarder<transport_assembly>, std::unique_ptr<int>&>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, transport_receiver<transport_assembly>, std::unique_ptr<int>&>);
}

TEST_CASE("Requests travel forward and responses return before request returns", "[relay]")
{
    nom::relay<routing_assembly> app;
    app.request(raw_value{7});

    CHECK(app.ctx().received == 70);
    CHECK(app.ctx().health_checks == 0);
    CHECK(app.ctx().trace == std::vector<std::string>{"client:request",
                                                      "normalizer:transform",
                                                      "endpoint:request",
                                                      "normalizer:response",
                                                      "client:response",
                                                      "client:return"});
}

TEST_CASE("A handler can consume an event by omitting pass_request", "[relay]")
{
    nom::relay<routing_assembly> app;
    app.request(raw_value{-1});

    CHECK(app.ctx().received == 0);
    CHECK(app.ctx().trace == std::vector<std::string>{"client:request", "normalizer:consume", "client:return"});
}

TEST_CASE("A new response enters at the last matching node and travels backward", "[relay]")
{
    nom::relay<routing_assembly> app;
    app.respond(receipt{23});

    CHECK(app.ctx().received == 23);
    CHECK(app.ctx().trace == std::vector<std::string>{"endpoint:response", "normalizer:response", "client:response"});
}

TEST_CASE("The event signature selects a route and skips unrelated nodes", "[relay]")
{
    nom::relay<routing_assembly> app;
    app.request(health_check{});

    CHECK(std::as_const(app).ctx().health_checks == 1);
    CHECK(app.ctx().trace.empty());
}

TEST_CASE("Variadic requests preserve the identity and mutability of lvalue arguments", "[relay]")
{
    nom::relay<transport_assembly> app;
    int count = 0;
    std::string text;
    app.request(count, text);

    CHECK(count == 2);
    CHECK(text == "forwarder receiver");
    CHECK(app.ctx().observed_count == &count);
    CHECK(app.ctx().observed_text == &text);
}

TEST_CASE("A move-only rvalue can cross a forwarding node and transfer ownership", "[relay]")
{
    using relay = nom::relay<transport_assembly>;
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<relay>);
    STATIC_REQUIRE(std::is_move_constructible_v<relay>);

    nom::relay<transport_assembly> app;
    auto value = std::make_unique<int>(42);
    const auto* original = value.get();
    app.request(std::move(value));

    CHECK_FALSE(value);
    REQUIRE(app.ctx().owned);
    CHECK(app.ctx().owned.get() == original);
    CHECK(*app.ctx().owned == 42);
}

TEST_CASE("Handler overload selection retains constness and value category", "[relay]")
{
    nom::relay<transport_assembly> app;
    std::string mutable_text = "mutable";
    const std::string immutable_text = "immutable";
    app.request(mutable_text);
    app.request(immutable_text);
    app.request(std::string{"temporary"});

    CHECK(app.ctx().categories == std::vector<std::string>{"mutable lvalue", "const lvalue", "rvalue"});
}

TEST_CASE("Move-only responses preserve ownership transfer along the reverse route", "[relay]")
{
    nom::relay<transport_assembly> app;
    auto value = std::make_unique<int>(73);
    const auto* original = value.get();
    app.respond(std::move(value));

    CHECK_FALSE(value);
    REQUIRE(app.ctx().owned);
    CHECK(app.ctx().owned.get() == original);
    CHECK(*app.ctx().owned == 73);
}

TEST_CASE("Suppression is configured independently for each direction, including empty assemblies", "[relay]")
{
    using request_relay = nom::relay<request_only_assembly>;
    using response_relay = nom::relay<response_only_assembly>;

    STATIC_REQUIRE(accepts_request<request_relay, unobserved_request>);
    STATIC_REQUIRE_FALSE(accepts_response<request_relay, unobserved_request>);
    STATIC_REQUIRE(accepts_response<response_relay, unobserved_response>);
    STATIC_REQUIRE_FALSE(accepts_request<response_relay, unobserved_response>);

    request_relay requests;
    response_relay responses;
    requests.request(unobserved_request{});
    responses.respond(unobserved_response{});

    // Matching member names alone do not opt unrelated traits into suppression.
    using unrelated_relay = nom::relay<unrelated_custom_assembly>;
    STATIC_REQUIRE_FALSE(accepts_request<unrelated_relay, unobserved_request>);
    STATIC_REQUIRE_FALSE(accepts_response<unrelated_relay, unobserved_response>);
}

TEST_CASE("Listed events can reach a suppressor directly or after the final application node", "[relay]")
{
    using relay = nom::relay<optional_assembly>;
    STATIC_REQUIRE(accepts_request<relay, unobserved_request>);
    STATIC_REQUIRE(accepts_response<relay, unobserved_response>);
    STATIC_REQUIRE(forwards_request<relay, last_observer<optional_assembly>, unobserved_request>);
    STATIC_REQUIRE(forwards_response<relay, first_observer<optional_assembly>, unobserved_response>);

    relay app;
    app.request(unobserved_request{});
    app.respond(unobserved_response{});
    CHECK(app.ctx().trace.empty());

    app.request(emit_unobserved{});
    app.respond(emit_unobserved{});
    CHECK(app.ctx().trace == std::vector<std::string>{"last:emit request", "first:emit response"});
}

TEST_CASE("Application handlers precede suppressors and may forward all the way to either end", "[relay]")
{
    nom::relay<optional_assembly> app;
    app.request(optional_notice{});
    CHECK(app.ctx().trace == std::vector<std::string>{"first:request", "last:request", "first:return"});

    app.ctx().trace.clear();
    app.respond(optional_notice{});
    CHECK(app.ctx().trace == std::vector<std::string>{"last:response", "first:response"});
}

TEST_CASE("Suppression does not make forwarding automatic", "[relay]")
{
    nom::relay<optional_assembly> app;
    app.request(optional_notice{true});
    CHECK(app.ctx().trace == std::vector<std::string>{"first:consume request"});

    app.ctx().trace.clear();
    app.respond(optional_notice{true});
    CHECK(app.ctx().trace == std::vector<std::string>{"last:consume response"});
}

TEST_CASE("Suppressors accept one listed event, not conversions or arbitrary argument packs", "[relay]")
{
    using relay = nom::relay<optional_assembly>;
    using first_node = first_observer<optional_assembly>;
    using last_node = last_observer<optional_assembly>;

    STATIC_REQUIRE_FALSE(accepts_request<relay, unobserved_response>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, unobserved_request>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, convertible_notice>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, convertible_notice>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, int>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, int>);
    STATIC_REQUIRE_FALSE(accepts_request<relay>);
    STATIC_REQUIRE_FALSE(accepts_response<relay>);
    STATIC_REQUIRE_FALSE(accepts_request<relay, unobserved_request, int>);
    STATIC_REQUIRE_FALSE(accepts_response<relay, unobserved_response, int>);

    STATIC_REQUIRE_FALSE(forwards_request<relay, last_node, unobserved_response>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, first_node, unobserved_request>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, last_node>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, first_node>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, last_node, unobserved_request, int>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, first_node, unobserved_response, int>);
}

TEST_CASE("Suppression never makes an invalid forwarding source valid", "[relay]")
{
    using relay = nom::relay<optional_assembly>;
    using foreign_node = client<routing_assembly>;

    STATIC_REQUIRE_FALSE(forwards_request<relay, foreign_node, unobserved_request>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, foreign_node, unobserved_response>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, const last_observer<optional_assembly>, unobserved_request>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, const first_observer<optional_assembly>, unobserved_response>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, int, unobserved_request>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, int, unobserved_response>);
    STATIC_REQUIRE_FALSE(forwards_request<nom::relay<request_only_assembly>, foreign_node, unobserved_request>);
    STATIC_REQUIRE_FALSE(forwards_response<nom::relay<response_only_assembly>, foreign_node, unobserved_response>);
}

TEST_CASE("Suppressing a move-only event accepts every cvref category without taking ownership", "[relay]")
{
    using relay = nom::relay<optional_assembly>;
    STATIC_REQUIRE(accepts_request<relay, std::unique_ptr<int>&>);
    STATIC_REQUIRE(accepts_request<relay, const std::unique_ptr<int>&>);
    STATIC_REQUIRE(accepts_request<relay, std::unique_ptr<int>>);
    STATIC_REQUIRE(accepts_request<relay, const std::unique_ptr<int>>);
    STATIC_REQUIRE(accepts_response<relay, std::unique_ptr<int>&>);
    STATIC_REQUIRE(accepts_response<relay, const std::unique_ptr<int>&>);
    STATIC_REQUIRE(accepts_response<relay, std::unique_ptr<int>>);
    STATIC_REQUIRE(accepts_response<relay, const std::unique_ptr<int>>);

    relay app;
    auto value = std::make_unique<int>(42);
    const int* original = value.get();
    app.request(value);
    app.request(std::as_const(value));
    app.request(std::move(value));
    app.request(std::move(std::as_const(value)));
    app.respond(value);
    app.respond(std::as_const(value));
    app.respond(std::move(value));
    app.respond(std::move(std::as_const(value)));

    CHECK(app.ctx().trace.empty());
    app.request(emit_move_only{value});
    app.respond(emit_move_only{value});
    REQUIRE(value.get() == original);
    CHECK(*value == 42);
    CHECK(app.ctx().trace == std::vector<std::string>{"last:emit request", "first:emit response"});
}
