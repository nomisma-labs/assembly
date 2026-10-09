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

    template <typename Traits> struct optional_config_node
    {
        optional_config_node() = default;

        explicit optional_config_node(const app_config& config) : initial_quote(config.initial_quote)
        {
        }

        void on_request(int& observed_quote) const
        {
            observed_quote = initial_quote;
        }

        int initial_quote = -1;
    };

    struct node_config_assembly : nom::assembly<node_config_assembly, optional_config_node, statistics_node>
    {
    };

    template <typename Traits> struct mixin_config_node
    {
        struct context_mixin
        {
            explicit context_mixin(const app_config& config) : initial_quote(config.initial_quote)
            {
            }

            int initial_quote;
        };

        bool ready = true;
    };

    struct mixin_config_assembly : nom::assembly<mixin_config_assembly, mixin_config_node>
    {
    };

    template <typename Traits> struct integer_context_node
    {
        struct context_mixin
        {
            context_mixin() = default;

            explicit context_mixin(const int& value) : seed(value)
            {
            }

            int seed = -1;
        };
    };

    struct split_config_assembly : nom::assembly<split_config_assembly, optional_config_node, integer_context_node>
    {
    };

    template <typename Traits> struct whole_pack_node
    {
        explicit whole_pack_node(const app_config& config, const int& increment)
            : initial_quote(config.initial_quote + increment)
        {
        }

        int initial_quote;
    };

    struct whole_pack_assembly
        : nom::assembly<whole_pack_assembly, whole_pack_node, optional_config_node, integer_context_node>
    {
    };

    template <typename Traits> struct nonconst_config_node
    {
        nonconst_config_node() = default;

        explicit nonconst_config_node(app_config&)
        {
        }

        explicit nonconst_config_node(app_config&&)
        {
        }
    };

    struct nonconst_config_assembly : nom::assembly<nonconst_config_assembly, nonconst_config_node>
    {
    };

    template <typename Traits> struct nested_publisher_node
    {
        struct context_mixin
        {
            explicit context_mixin(const app_config& config) : starting_quote(nom::ctx<Traits>(this).quote)
            {
                // The quote and statistics mixins have already been constructed.
                publish(starting_quote + 10);
                if (config.construction_log)
                    config.construction_log->push_back("nested publisher context");
            }

            void publish(int price)
            {
                auto& shared = nom::ctx<Traits>(this);
                shared.quote = price;
                ++shared.updates;
            }

            int latest() const
            {
                return nom::ctx<Traits>(this).quote;
            }

            int starting_quote;
        };

        explicit nested_publisher_node(const app_config& config) : initial_quote(nom::ctx<Traits>(this).quote)
        {
            if (config.construction_log)
                config.construction_log->push_back("nested publisher node");
        }

        int initial_quote;
    };

    struct nested_assembly : nom::assembly<nested_assembly, quote_node, statistics_node, nested_publisher_node>
    {
    };

    template <typename Assembly, typename Part>
    concept supports_explicit_context = requires(Part* part) { nom::ctx<Assembly>(part); };
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

TEST_CASE("Default-only assemblies reject unused construction arguments", "[assembly]")
{
    static_assert(std::is_default_constructible_v<statistics_recipe::body>);
    static_assert(std::is_default_constructible_v<nom::relay<statistics_recipe>>);
    static_assert(!std::is_constructible_v<statistics_recipe::body, const app_config&>);
    static_assert(!std::is_constructible_v<nom::relay<statistics_recipe>, const app_config&>);

    static_assert(std::is_default_constructible_v<empty_recipe::body>);
    static_assert(std::is_default_constructible_v<nom::relay<empty_recipe>>);
    static_assert(!std::is_constructible_v<empty_recipe::body, const app_config&>);
    static_assert(!std::is_constructible_v<nom::relay<empty_recipe>, const app_config&>);

    nom::relay<statistics_recipe> statistics;
    CHECK(statistics.ctx().updates == 0);
    nom::relay<empty_recipe> empty;
    CHECK(&empty.ctx() == &std::as_const(empty).ctx());
}

TEST_CASE("A node alone can consume configuration while every context mixin defaults", "[assembly]")
{
    static_assert(std::is_default_constructible_v<node_config_assembly::body>);
    static_assert(std::is_default_constructible_v<nom::relay<node_config_assembly>>);
    static_assert(std::is_constructible_v<node_config_assembly::body, const app_config&>);
    static_assert(std::is_constructible_v<nom::relay<node_config_assembly>, const app_config&>);
    static_assert(!std::is_constructible_v<node_config_assembly::body, const std::string&>);
    static_assert(!std::is_constructible_v<nom::relay<node_config_assembly>, const std::string&>);

    const app_config config{"test venue", 240};
    node_config_assembly::body app(config);
    CHECK(app.get<0>().initial_quote == 240);
    CHECK(app.get<1>().ready);
    CHECK(app.ctx().updates == 0);

    nom::relay<node_config_assembly> pipeline(config);
    int observed_quote = 0;
    pipeline.request(observed_quote);
    CHECK(observed_quote == 240);
    CHECK(pipeline.ctx().updates == 0);
}

TEST_CASE("A context mixin alone can consume configuration while every node defaults", "[assembly]")
{
    static_assert(!std::is_default_constructible_v<mixin_config_assembly::body>);
    static_assert(!std::is_default_constructible_v<nom::relay<mixin_config_assembly>>);
    static_assert(std::is_constructible_v<mixin_config_assembly::body, const app_config&>);
    static_assert(std::is_constructible_v<nom::relay<mixin_config_assembly>, const app_config&>);
    static_assert(!std::is_constructible_v<mixin_config_assembly::body, const std::string&>);
    static_assert(!std::is_constructible_v<nom::relay<mixin_config_assembly>, const std::string&>);

    const app_config config{"test venue", 240};
    mixin_config_assembly::body app(config);
    CHECK(app.ctx().initial_quote == 240);
    CHECK(app.get<0>().ready);
    nom::relay<mixin_config_assembly> pipeline(config);
    CHECK(pipeline.ctx().initial_quote == 240);
}

TEST_CASE("At least one component must accept the entire construction argument pack", "[assembly]")
{
    // Each argument has a consumer on its own; neither component accepts the pair.
    static_assert(std::is_constructible_v<split_config_assembly::body, const app_config&>);
    static_assert(std::is_constructible_v<nom::relay<split_config_assembly>, const app_config&>);
    static_assert(std::is_constructible_v<split_config_assembly::body, const int&>);
    static_assert(std::is_constructible_v<nom::relay<split_config_assembly>, const int&>);
    static_assert(!std::is_constructible_v<split_config_assembly::body, const app_config&, const int&>);
    static_assert(!std::is_constructible_v<nom::relay<split_config_assembly>, const app_config&, const int&>);

    // Adding a full-pack consumer makes construction valid; the partial matches still default.
    static_assert(std::is_constructible_v<whole_pack_assembly::body, const app_config&, const int&>);
    static_assert(std::is_constructible_v<nom::relay<whole_pack_assembly>, const app_config&, const int&>);
    whole_pack_assembly::body app(app_config{"test venue", 240}, 5);
    CHECK(app.get<0>().initial_quote == 245);
    CHECK(app.get<1>().initial_quote == -1);
    CHECK(app.ctx().seed == -1);
}

TEST_CASE("Mutable and rvalue constructors do not consume shared const construction arguments", "[assembly]")
{
    STATIC_REQUIRE(std::is_default_constructible_v<nonconst_config_assembly::body>);
    STATIC_REQUIRE(std::is_default_constructible_v<nom::relay<nonconst_config_assembly>>);
    using node = nonconst_config_node<nonconst_config_assembly>;
    static_assert(std::is_constructible_v<node, app_config&>);
    static_assert(std::is_constructible_v<node, app_config&&>);
    static_assert(!std::is_constructible_v<node, const app_config&>);

    static_assert(!std::is_constructible_v<nonconst_config_assembly::body, app_config&>);
    static_assert(!std::is_constructible_v<nom::relay<nonconst_config_assembly>, app_config&>);
    static_assert(!std::is_constructible_v<nonconst_config_assembly::body, app_config&&>);
    static_assert(!std::is_constructible_v<nom::relay<nonconst_config_assembly>, app_config&&>);
}

TEST_CASE("Copying a mutable body or relay preserves node and context state", "[assembly]")
{
    const app_config config{"test venue", 240};
    node_config_assembly::body original_body(config);
    original_body.get<0>().initial_quote = 310;
    original_body.ctx().updates = 7;

    node_config_assembly::body copied_body(original_body);
    CHECK(copied_body.get<0>().initial_quote == 310);
    CHECK(copied_body.ctx().updates == 7);
    copied_body.get<0>().initial_quote = 320;
    copied_body.ctx().updates = 8;
    CHECK(original_body.get<0>().initial_quote == 310);
    CHECK(original_body.ctx().updates == 7);

    nom::relay<node_config_assembly> original_relay(config);
    original_relay.ctx().updates = 7;
    nom::relay<node_config_assembly> copied_relay(original_relay);
    int observed_quote = 0;
    copied_relay.request(observed_quote);
    CHECK(observed_quote == 240);
    CHECK(copied_relay.ctx().updates == 7);
    copied_relay.ctx().updates = 8;
    CHECK(original_relay.ctx().updates == 7);
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

TEST_CASE("Nested context mixins access earlier services during construction", "[assembly]")
{
    std::vector<std::string> construction_log;
    nested_assembly::body app(app_config{"paper", 400, &construction_log});

    CHECK(construction_log ==
          std::vector<std::string>{"quote context", "nested publisher context", "quote node", "nested publisher node"});
    CHECK(app.ctx().starting_quote == 400);
    CHECK(app.ctx().quote == 410);
    CHECK(app.ctx().updates == 1);
    CHECK(app.get<0>().initial_quote() == 410);
    CHECK(app.get<2>().initial_quote == 410);

    app.ctx().publish(425);
    const auto& services = std::as_const(app).ctx();
    CHECK(services.latest() == 425);
    CHECK(services.updates == 2);
    CHECK(app.get<0>().latest_quote() == 425);
}

TEST_CASE("Explicit context lookup preserves identity and constness for nodes and nested mixins", "[assembly]")
{
    using context = nested_assembly::body::context;
    using mixin = nested_publisher_node<nested_assembly>::context_mixin;

    nested_assembly::body app(app_config{});
    auto& node = app.get<2>();
    auto& service = static_cast<mixin&>(app.ctx());
    const auto& read_only_node = std::as_const(node);
    const auto& read_only_service = std::as_const(service);

    static_assert(std::is_same_v<decltype(nom::ctx<nested_assembly>(&node)), context&>);
    static_assert(std::is_same_v<decltype(nom::ctx<nested_assembly>(&service)), context&>);
    static_assert(std::is_same_v<decltype(nom::ctx<nested_assembly>(&read_only_node)), const context&>);
    static_assert(std::is_same_v<decltype(nom::ctx<nested_assembly>(&read_only_service)), const context&>);
    static_assert(noexcept(nom::ctx<nested_assembly>(&node)));
    static_assert(noexcept(nom::ctx<nested_assembly>(&service)));
    static_assert(noexcept(nom::ctx<nested_assembly>(&read_only_node)));
    static_assert(noexcept(nom::ctx<nested_assembly>(&read_only_service)));

    CHECK(&nom::ctx<nested_assembly>(&node) == &app.ctx());
    CHECK(&nom::ctx<nested_assembly>(&service) == &app.ctx());
    CHECK(&nom::ctx<nested_assembly>(&read_only_node) == &std::as_const(app).ctx());
    CHECK(&nom::ctx<nested_assembly>(&read_only_service) == &std::as_const(app).ctx());
    CHECK(&nom::ctx<nested_assembly>(&node) == &nom::ctx(&node));
    CHECK(&nom::ctx<nested_assembly>(&read_only_node) == &nom::ctx(&read_only_node));

    static_assert(supports_explicit_context<nested_assembly, mixin>);
    static_assert(supports_explicit_context<nested_assembly, const mixin>);
    static_assert(!supports_explicit_context<nested_assembly, int>);
    static_assert(!supports_explicit_context<nested_assembly, void>);
    static_assert(!supports_explicit_context<nested_assembly, quote_node<market_recipe>>);
    static_assert(!supports_explicit_context<market_recipe, mixin>);
    static_assert(!supports_explicit_context<int, mixin>);
}
