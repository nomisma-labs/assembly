// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>

import std;

import nom.assembly;

namespace
{
    using nom::delegate;
    using nom::delegate_ref;
    using nom::events_delegate_t;

    struct connected
    {
    };

    struct message
    {
        std::string text;
    };

    struct recorder
    {
        void operator()(connected)
        {
            ++connections;
        }

        void operator()(const message& event)
        {
            messages.push_back(event.text);
        }

        int connections = 0;
        std::vector<std::string> messages;
    };

    // The producer declares the events it emits without knowing its consumer.
    struct event_source
    {
        using callbacks = events_delegate_t<connected, const message&>;

        void replay(const std::string& text) const
        {
            receive(connected{});
            receive(message{text});
        }

        callbacks receive;
    };

    struct accumulator
    {
        int operator()(int amount)
        {
            total += amount;
            return total;
        }

        int total = 0;
    };

    struct payload_store
    {
        payload_store() = default;
        payload_store(const payload_store&) = delete;
        payload_store& operator=(const payload_store&) = delete;

        int& operator()(std::unique_ptr<int> value) noexcept
        {
            payload = std::move(value);
            return *payload;
        }

        std::unique_ptr<int> payload;
    };
} // namespace

TEST_CASE("delegate combines concrete callables into one overload set", "[delegate]")
{
    // Deduction keeps the lambda types; no std::function or virtual interface is required.
    const delegate describe([](int count) { return std::to_string(count) + " messages"; },
                            [](const std::string& text) { return "message: " + text; });

    const std::variant<int, std::string> event = std::string{"ready"};
    REQUIRE(describe(3) == "3 messages");
    REQUIRE(std::visit(describe, event) == "message: ready");

    // Concrete delegates can also own move-only callables.
    delegate next([count = std::make_unique<int>(0)]() mutable { return ++*count; });
    STATIC_REQUIRE_FALSE(std::copy_constructible<decltype(next)>);
    REQUIRE(next() == 1);
    REQUIRE(next() == 2);
}

TEST_CASE("delegate ownership is chosen when the handler is bound", "[delegate]")
{
    accumulator handler;
    delegate owned(handler);
    delegate borrowed(std::ref(handler));

    REQUIRE(owned(4) == 4);
    REQUIRE(handler.total == 0);

    REQUIRE(borrowed(7) == 7);
    REQUIRE(handler.total == 7);
    REQUIRE(owned(1) == 5);
}

TEST_CASE("an event interface binds and rebinds a complete handler at runtime", "[delegate]")
{
    event_source source;

    SECTION("an unbound event signature fails explicitly")
    {
        REQUIRE_THROWS_AS(source.receive(connected{}), std::bad_function_call);
    }

    SECTION("std::ref sends every event to the same live handler")
    {
        recorder first;
        recorder second;

        source.receive = std::ref(first);
        source.replay("first session");

        source.receive = std::ref(second);
        source.replay("second session");

        REQUIRE(first.connections == 1);
        REQUIRE(first.messages == std::vector<std::string>{"first session"});
        REQUIRE(second.connections == 1);
        REQUIRE(second.messages == std::vector<std::string>{"second session"});
    }
}

TEST_CASE("separate lambdas can implement the same named event interface", "[delegate]")
{
    int connections = 0;
    const message* received = nullptr;

    event_source source;
    source.receive = delegate([&](connected) { ++connections; }, [&](const message& event) { received = &event; });

    const message event{"borrowed payload"};
    source.receive(connected{});
    source.receive(event);

    REQUIRE(connections == 1);
    REQUIRE(received == &event);
}

TEST_CASE("explicit delegate signatures support arguments and return values beyond events", "[delegate]")
{
    using service = delegate<std::function<int(int, int)>, std::function<std::string(const std::string&)>>;

    // This is an ordinary overload set, so each signature can have its own result type.
    const service callbacks([](int lhs, int rhs) { return lhs + rhs; },
                            [](const std::string& name) { return "hello " + name; });

    REQUIRE(callbacks(2, 3) == 5);
    REQUIRE(callbacks(std::string{"Ada"}) == "hello Ada");
}

TEST_CASE("delegate_ref borrows a concrete handler and preserves forwarding and reference results", "[delegate]")
{
    payload_store handler;
    const delegate_ref receive(handler);
    const delegate_ref via_wrapper(std::ref(handler));

    STATIC_REQUIRE(std::same_as<decltype(via_wrapper), const delegate_ref<payload_store>>);
    STATIC_REQUIRE(std::is_nothrow_invocable_v<decltype(receive), std::unique_ptr<int>>);
    STATIC_REQUIRE_FALSE(std::invocable<decltype(receive), std::string>);

    auto payload = std::make_unique<int>(9);
    int& stored = receive(std::move(payload));
    REQUIRE(payload == nullptr);
    REQUIRE(&stored == handler.payload.get());

    stored = 12;
    REQUIRE(*handler.payload == 12);
    REQUIRE(via_wrapper(std::make_unique<int>(17)) == 17);
}
