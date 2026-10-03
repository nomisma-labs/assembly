// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

import nom.assembly;

namespace
{
    struct app_config
    {
        std::string venue = "paper";
        int initial_quote = 100;
        std::vector<std::string>* construction_log = nullptr;
    };

    template <typename Traits> struct quote_context
    {
        explicit quote_context(const app_config& config) : venue(config.venue), quote(config.initial_quote)
        {
            if (config.construction_log)
                config.construction_log->push_back("quote context");
        }

        std::string venue;
        int quote;
    };

    template <typename Traits> struct quote_node
    {
        using context_mixin = quote_context<Traits>;

        explicit quote_node(const app_config& config) : initial_quote_(nom::ctx(this).quote)
        {
            if (config.construction_log)
                config.construction_log->push_back("quote node");
        }

        int initial_quote() const
        {
            return initial_quote_;
        }

        int latest_quote() const
        {
            return nom::ctx(this).quote;
        }

    private:
        int initial_quote_;
    };

    // Neither this node nor its mixin accepts app_config; both use their default constructor.
    template <typename Traits> struct statistics_node
    {
        struct context_mixin
        {
            int updates = 0;
        };

        bool ready = true;
    };

    // A templated mixin can find the complete context, just as a templated node can.
    template <typename Traits> struct publisher_context
    {
        void publish(int price)
        {
            auto& shared = nom::ctx(this);
            shared.quote = price;
            ++shared.updates;
        }

        int latest() const
        {
            return nom::ctx(this).quote;
        }
    };

    template <typename Traits> struct publisher_node
    {
        using context_mixin = publisher_context<Traits>;
    };

    struct market_recipe : nom::assembly<market_recipe, quote_node, statistics_node, publisher_node>
    {
    };

    struct empty_recipe : nom::assembly<empty_recipe>
    {
    };

    struct statistics_recipe : nom::assembly<statistics_recipe, statistics_node>
    {
    };
} // namespace

TEST_CASE("A recipe describes node types; its body owns runtime state", "[assembly]")
{
    static_assert(std::is_same_v<market_recipe::traits, market_recipe>);
    static_assert(market_recipe::size == 3);
    static_assert(empty_recipe::size == 0);
    empty_recipe::body empty;
    CHECK(&empty.ctx() == &std::as_const(empty).ctx());

    statistics_recipe::body statistics;
    CHECK(statistics.get<0>().ready);
    CHECK(statistics.ctx().updates == 0);

    market_recipe::body first(app_config{});
    market_recipe::body second(app_config{});
    first.ctx().publish(120);
    CHECK(first.ctx().quote == 120);
    CHECK(second.ctx().quote == 100);
    CHECK(second.ctx().updates == 0);
}

TEST_CASE("Configuration is broadcast to every compatible node and context mixin", "[assembly]")
{
    const app_config config{"test venue", 240};
    market_recipe::body app(config);

    CHECK(app.ctx().venue == "test venue");
    CHECK(app.ctx().quote == 240);
    CHECK(app.get<0>().initial_quote() == 240);
    CHECK(app.get<1>().ready);
    CHECK(app.ctx().updates == 0);
}

TEST_CASE("The context is ready before node construction begins", "[assembly]")
{
    std::vector<std::string> construction_log;
    market_recipe::body app(app_config{"paper", 310, &construction_log});

    CHECK(construction_log == std::vector<std::string>{"quote context", "quote node"});
    CHECK(app.get<0>().initial_quote() == 310);
}

TEST_CASE("Node access and context lookup preserve constness", "[assembly]")
{
    market_recipe::body app(app_config{});
    auto& node = app.get<0>();
    const auto& read_only = std::as_const(app);
    static_assert(std::is_same_v<decltype(read_only.get<0>()), const quote_node<market_recipe>&>);
    static_assert(std::is_const_v<std::remove_reference_t<decltype(nom::ctx(&read_only.get<0>()))>>);

    CHECK(&nom::ctx(&node) == &app.ctx());
    CHECK(&nom::ctx(&read_only.get<0>()) == &read_only.ctx());
    app.ctx().publish(155);
    CHECK(read_only.get<0>().latest_quote() == 155);
}

TEST_CASE("Context mixins contribute services that can use other mixins", "[assembly]")
{
    market_recipe::body app(app_config{});
    app.ctx().publish(101);
    app.ctx().publish(102);

    const auto& services = std::as_const(app).ctx();
    CHECK(services.latest() == 102);
    CHECK(services.updates == 2);
    CHECK(app.get<0>().latest_quote() == services.latest());
}
