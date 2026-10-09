// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

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

    struct routing_recipe : nom::assembly<routing_recipe, client, normalizer, health_monitor, endpoint>
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

    struct transport_recipe : nom::assembly<transport_recipe, transport_forwarder, transport_receiver>
    {
    };

    struct empty_traits : nom::assembly<empty_traits>
    {
    };
} // namespace

TEST_CASE("Public relay constraints distinguish request and response routes", "[relay]")
{
    using relay = nom::relay<routing_recipe>;

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
    using relay = nom::relay<routing_recipe>;

    STATIC_REQUIRE(forwards_request<relay, client<routing_recipe>, raw_value>);
    STATIC_REQUIRE(forwards_request<relay, normalizer<routing_recipe>, normalized_value>);
    STATIC_REQUIRE(forwards_request<relay, health_monitor<routing_recipe>, normalized_value>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, normalizer<routing_recipe>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, endpoint<routing_recipe>, normalized_value>);

    STATIC_REQUIRE(forwards_response<relay, endpoint<routing_recipe>, receipt>);
    STATIC_REQUIRE(forwards_response<relay, health_monitor<routing_recipe>, receipt>);
    STATIC_REQUIRE(forwards_response<relay, normalizer<routing_recipe>, receipt>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, client<routing_recipe>, receipt>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, endpoint<routing_recipe>, raw_value>);
}

TEST_CASE("Invalid forwarding sources and empty assemblies fail constraints without hard errors", "[relay]")
{
    using relay = nom::relay<routing_recipe>;
    using foreign_node = transport_receiver<transport_recipe>;
    using empty_relay = nom::relay<empty_traits>;

    STATIC_REQUIRE_FALSE(forwards_request<relay, foreign_node, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, foreign_node, receipt>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, const client<routing_recipe>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, const endpoint<routing_recipe>, receipt>);

    STATIC_REQUIRE_FALSE(accepts_request<empty_relay, raw_value>);
    STATIC_REQUIRE_FALSE(accepts_response<empty_relay, receipt>);
    STATIC_REQUIRE_FALSE(forwards_request<empty_relay, client<routing_recipe>, raw_value>);
    STATIC_REQUIRE_FALSE(forwards_response<empty_relay, endpoint<routing_recipe>, receipt>);
}

TEST_CASE("Route constraints preserve argument constness and value categories", "[relay]")
{
    using relay = nom::relay<transport_recipe>;

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

    STATIC_REQUIRE(forwards_request<relay, transport_forwarder<transport_recipe>, std::unique_ptr<int>>);
    STATIC_REQUIRE(forwards_response<relay, transport_receiver<transport_recipe>, std::unique_ptr<int>>);
    STATIC_REQUIRE_FALSE(forwards_request<relay, transport_forwarder<transport_recipe>, std::unique_ptr<int>&>);
    STATIC_REQUIRE_FALSE(forwards_response<relay, transport_receiver<transport_recipe>, std::unique_ptr<int>&>);
}

TEST_CASE("Requests travel forward and responses return before request returns", "[relay]")
{
    nom::relay<routing_recipe> app;
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
    nom::relay<routing_recipe> app;
    app.request(raw_value{-1});

    CHECK(app.ctx().received == 0);
    CHECK(app.ctx().trace == std::vector<std::string>{"client:request", "normalizer:consume", "client:return"});
}

TEST_CASE("A new response enters at the last matching node and travels backward", "[relay]")
{
    nom::relay<routing_recipe> app;
    app.respond(receipt{23});

    CHECK(app.ctx().received == 23);
    CHECK(app.ctx().trace == std::vector<std::string>{"endpoint:response", "normalizer:response", "client:response"});
}

TEST_CASE("The event signature selects a route and skips unrelated nodes", "[relay]")
{
    nom::relay<routing_recipe> app;
    app.request(health_check{});

    CHECK(std::as_const(app).ctx().health_checks == 1);
    CHECK(app.ctx().trace.empty());
}

TEST_CASE("Variadic requests preserve the identity and mutability of lvalue arguments", "[relay]")
{
    nom::relay<transport_recipe> app;
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
    nom::relay<transport_recipe> app;
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
    nom::relay<transport_recipe> app;
    std::string mutable_text = "mutable";
    const std::string immutable_text = "immutable";
    app.request(mutable_text);
    app.request(immutable_text);
    app.request(std::string{"temporary"});

    CHECK(app.ctx().categories == std::vector<std::string>{"mutable lvalue", "const lvalue", "rvalue"});
}

TEST_CASE("Move-only responses preserve ownership transfer along the reverse route", "[relay]")
{
    nom::relay<transport_recipe> app;
    auto value = std::make_unique<int>(73);
    const auto* original = value.get();
    app.respond(std::move(value));

    CHECK_FALSE(value);
    REQUIRE(app.ctx().owned);
    CHECK(app.ctx().owned.get() == original);
    CHECK(*app.ctx().owned == 73);
}
