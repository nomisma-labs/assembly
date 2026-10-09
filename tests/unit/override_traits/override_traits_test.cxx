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

    // Matching member names do not opt a component into these overrides: inheritance does.
    struct unrelated_override_traits
    {
        struct override_traits
        {
            static constexpr int batch_size = 99;
        };
    };

    struct scalar_override_traits
    {
        using override_traits = int;
    };

    struct same_type_override_traits
    {
        using override_traits = batching_traits;
    };

    struct custom_batching_traits
    {
        struct override_traits : batching_traits
        {
            using counter_type = long;
            static constexpr int batch_size = 4;
        };
    };

    struct inherited_override_traits : custom_batching_traits
    {
    };

    struct shared_override_traits
    {
        struct override_traits : batching_traits, storage_traits
        {
            static constexpr int batch_size = 16;
            static constexpr int capacity = storage_traits::capacity * 8;
        };
    };

    struct private_override_traits
    {
        struct override_traits : private batching_traits
        {
        };
    };

    struct left_batching_traits : batching_traits
    {
    };

    struct right_batching_traits : batching_traits
    {
    };

    struct ambiguous_override_traits
    {
        struct override_traits : left_batching_traits, right_batching_traits
        {
        };
    };

    template <typename Traits> struct batching_node
    {
        using effective_traits = nom::use_override_if_defined_t<Traits, batching_traits>;

        struct context_mixin
        {
            typename effective_traits::counter_type processed = 0;
        };

        void on_request(int& batch_size)
        {
            batch_size = effective_traits::batch_size;
            nom::ctx(this).processed += batch_size;
        }
    };

    struct default_assembly : nom::assembly<default_assembly, batching_node>
    {
    };

    struct custom_assembly : nom::assembly<custom_assembly, batching_node>
    {
        struct override_traits : batching_traits
        {
            using counter_type = long;
            static constexpr int batch_size = 4;
        };
    };
} // namespace

TEST_CASE("missing or unrelated overrides preserve the component's default traits", "[override_traits]")
{
    using selected_traits = nom::use_override_if_defined_t<unrelated_override_traits, batching_traits>;

    STATIC_REQUIRE(std::is_same_v<nom::use_override_if_defined_t<default_traits, batching_traits>, batching_traits>);
    STATIC_REQUIRE(std::is_same_v<selected_traits, batching_traits>);
    STATIC_REQUIRE(selected_traits::batch_size != unrelated_override_traits::override_traits::batch_size);
    STATIC_REQUIRE(
        std::is_same_v<nom::use_override_if_defined_t<scalar_override_traits, batching_traits>, batching_traits>);
}

TEST_CASE("a derived override replaces selected members and inherits the remaining defaults", "[override_traits]")
{
    using selected_traits = nom::use_override_if_defined_t<custom_batching_traits, batching_traits>;

    STATIC_REQUIRE(std::is_same_v<selected_traits, custom_batching_traits::override_traits>);
    STATIC_REQUIRE(selected_traits::batch_size == 4);
    STATIC_REQUIRE(std::is_same_v<selected_traits::counter_type, long>);
    STATIC_REQUIRE_FALSE(selected_traits::trace_batches);
}

TEST_CASE("one override can customize multiple trait families while other families keep their defaults",
          "[override_traits]")
{
    using selected_batching_traits = nom::use_override_if_defined_t<shared_override_traits, batching_traits>;
    using selected_storage_traits = nom::use_override_if_defined_t<shared_override_traits, storage_traits>;
    using selected_retry_traits = nom::use_override_if_defined_t<shared_override_traits, retry_traits>;

    STATIC_REQUIRE(std::is_same_v<selected_batching_traits, shared_override_traits::override_traits>);
    STATIC_REQUIRE(std::is_same_v<selected_storage_traits, shared_override_traits::override_traits>);
    STATIC_REQUIRE(selected_batching_traits::batch_size == 16);
    STATIC_REQUIRE(selected_storage_traits::capacity == 64);
    STATIC_REQUIRE(std::is_same_v<selected_retry_traits, retry_traits>);
    STATIC_REQUIRE(selected_retry_traits::retry_limit == 3);
}

TEST_CASE("the override may name the default type or be inherited from another traits definition", "[override_traits]")
{
    STATIC_REQUIRE(
        std::is_same_v<nom::use_override_if_defined_t<same_type_override_traits, batching_traits>, batching_traits>);
    STATIC_REQUIRE(std::is_same_v<nom::use_override_if_defined_t<inherited_override_traits, batching_traits>,
                                  custom_batching_traits::override_traits>);
}

TEST_CASE("override selection follows the base relationship rather than public convertibility", "[override_traits]")
{
    // Selection does not promise that inherited members are accessible or unambiguous.
    STATIC_REQUIRE(std::is_same_v<nom::use_override_if_defined_t<private_override_traits, batching_traits>,
                                  private_override_traits::override_traits>);
    STATIC_REQUIRE(std::is_same_v<nom::use_override_if_defined_t<ambiguous_override_traits, batching_traits>,
                                  ambiguous_override_traits::override_traits>);
}

TEST_CASE("an assembly override controls both its node behavior and its context member type", "[override_traits]")
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
