// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

import nom.assembly;

namespace
{
    struct batching_traits
    {
        using counter_type = int;
        static constexpr int batch_size = 1;
        static constexpr bool trace_batches = false;
    };

    struct storage_traits
    {
        static constexpr int capacity = 8;
    };

    struct retry_traits
    {
        static constexpr int retry_limit = 3;
    };

    struct default_traits
    {
    };

    // Matching member names do not opt a component into these customizations: inheritance does.
    struct unrelated_custom_traits
    {
        struct custom_traits
        {
            static constexpr int batch_size = 99;
        };
    };

    struct scalar_custom_traits
    {
        using custom_traits = int;
    };

    struct same_type_custom_traits
    {
        using custom_traits = batching_traits;
    };

    struct custom_batching_traits
    {
        struct custom_traits : batching_traits
        {
            using counter_type = long;
            static constexpr int batch_size = 4;
        };
    };

    struct inherited_custom_traits : custom_batching_traits
    {
    };

    struct shared_custom_traits
    {
        struct custom_traits : batching_traits, storage_traits
        {
            static constexpr int batch_size = 16;
            static constexpr int capacity = storage_traits::capacity * 8;
        };
    };

    struct private_custom_traits
    {
        struct custom_traits : private batching_traits
        {
        };
    };

    struct left_batching_traits : batching_traits
    {
    };

    struct right_batching_traits : batching_traits
    {
    };

    struct ambiguous_custom_traits
    {
        struct custom_traits : left_batching_traits, right_batching_traits
        {
        };
    };

    template <typename Traits> struct batching_node
    {
        using batching_traits = nom::apply_custom_traits_t<Traits, ::batching_traits>;

        struct context_mixin
        {
            typename batching_traits::counter_type processed = 0;
        };

        void on_request(int& batch_size)
        {
            batch_size = batching_traits::batch_size;
            nom::ctx(this).processed += batch_size;
        }
    };

    struct default_assembly : nom::assembly<default_assembly, batching_node>
    {
    };

    struct custom_assembly : nom::assembly<custom_assembly, batching_node>
    {
        struct custom_traits : batching_traits
        {
            using counter_type = long;
            static constexpr int batch_size = 4;
        };
    };
} // namespace

TEST_CASE("missing or unrelated custom traits preserve the component's default traits", "[custom_traits]")
{
    using batching_traits = nom::apply_custom_traits_t<unrelated_custom_traits, ::batching_traits>;

    STATIC_REQUIRE(std::is_same_v<nom::apply_custom_traits_t<default_traits, ::batching_traits>, ::batching_traits>);
    STATIC_REQUIRE(std::is_same_v<batching_traits, ::batching_traits>);
    STATIC_REQUIRE(batching_traits::batch_size != unrelated_custom_traits::custom_traits::batch_size);
    STATIC_REQUIRE(
        std::is_same_v<nom::apply_custom_traits_t<scalar_custom_traits, ::batching_traits>, ::batching_traits>);
}

TEST_CASE("a customization replaces selected members and inherits the remaining defaults", "[custom_traits]")
{
    using batching_traits = nom::apply_custom_traits_t<custom_batching_traits, ::batching_traits>;

    STATIC_REQUIRE(std::is_same_v<batching_traits, custom_batching_traits::custom_traits>);
    STATIC_REQUIRE(batching_traits::batch_size == 4);
    STATIC_REQUIRE(std::is_same_v<batching_traits::counter_type, long>);
    STATIC_REQUIRE_FALSE(batching_traits::trace_batches);
}

TEST_CASE("custom traits can customize multiple trait families while other families keep their defaults",
          "[custom_traits]")
{
    using batching_traits = nom::apply_custom_traits_t<shared_custom_traits, ::batching_traits>;
    using storage_traits = nom::apply_custom_traits_t<shared_custom_traits, ::storage_traits>;
    using retry_traits = nom::apply_custom_traits_t<shared_custom_traits, ::retry_traits>;

    STATIC_REQUIRE(std::is_same_v<batching_traits, shared_custom_traits::custom_traits>);
    STATIC_REQUIRE(std::is_same_v<storage_traits, shared_custom_traits::custom_traits>);
    STATIC_REQUIRE(batching_traits::batch_size == 16);
    STATIC_REQUIRE(storage_traits::capacity == 64);
    STATIC_REQUIRE(std::is_same_v<retry_traits, ::retry_traits>);
    STATIC_REQUIRE(retry_traits::retry_limit == 3);
}

TEST_CASE("custom traits may name the default type or be inherited from another traits definition", "[custom_traits]")
{
    STATIC_REQUIRE(
        std::is_same_v<nom::apply_custom_traits_t<same_type_custom_traits, ::batching_traits>, ::batching_traits>);
    STATIC_REQUIRE(std::is_same_v<nom::apply_custom_traits_t<inherited_custom_traits, ::batching_traits>,
                                  custom_batching_traits::custom_traits>);
}

TEST_CASE("custom trait selection follows the base relationship rather than public convertibility", "[custom_traits]")
{
    // Selection does not promise that inherited members are accessible or unambiguous.
    STATIC_REQUIRE(std::is_same_v<nom::apply_custom_traits_t<private_custom_traits, ::batching_traits>,
                                  private_custom_traits::custom_traits>);
    STATIC_REQUIRE(std::is_same_v<nom::apply_custom_traits_t<ambiguous_custom_traits, ::batching_traits>,
                                  ambiguous_custom_traits::custom_traits>);
}

TEST_CASE("an assembly customization controls both its node behavior and its context member type", "[custom_traits]")
{
    nom::relay<default_assembly> defaults;
    nom::relay<custom_assembly> customized;
    int default_batch_size = 0;
    int custom_batch_size = 0;

    STATIC_REQUIRE(std::is_same_v<decltype(defaults.ctx().processed), int>);
    STATIC_REQUIRE(std::is_same_v<decltype(customized.ctx().processed), long>);

    defaults.request(default_batch_size);
    customized.request(custom_batch_size);
    customized.request(custom_batch_size);

    REQUIRE(default_batch_size == 1);
    REQUIRE(defaults.ctx().processed == 1);
    REQUIRE(custom_batch_size == 4);
    REQUIRE(customized.ctx().processed == 8);
}
