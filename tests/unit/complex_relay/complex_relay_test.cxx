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
        int initial_cash = 500;
        int initial_quote = 100;
        std::string venue = "paper";
    };

    struct submit_order
    {
        int quantity;
    };

    struct quote_update
    {
        int price;
    };

    struct order_request
    {
        int id;
        int quantity;
    };

    struct approved_order
    {
        int id;
        int quantity;
        int notional;
    };

    struct execution_report
    {
        int id;
        bool filled;
        int notional;
        std::string detail;
    };

    template <typename Traits> struct order_client
    {
        struct context_mixin
        {
            int next_order_id = 1;
            std::vector<execution_report> reports;
            std::vector<std::string> trace;
        };

        void on_request(const submit_order& command)
        {
            auto& shared = nom::ctx(this);
            shared.trace.push_back("client:submit");
            // The client originates the order event; submit_order is an external command.
            const order_request pending{shared.next_order_id++, command.quantity};
            nom::relay<Traits>::pass_request(this, pending);
        }

        void on_response(const execution_report& event)
        {
            auto& shared = nom::ctx(this);
            shared.trace.push_back("client:report");
            shared.reports.push_back(event);
        }
    };

    struct market_state
    {
        explicit market_state(const app_config& config) : price_(config.initial_quote)
        {
        }

        void update_quote(int price)
        {
            price_ = price;
        }

        int mark_price() const
        {
            return price_;
        }

    private:
        int price_;
    };

    template <typename Traits> struct market_data
    {
        using context_mixin = market_state;

        void on_request(const quote_update& event)
        {
            nom::ctx(this).update_quote(event.price);
        }
    };

    struct account_state
    {
        explicit account_state(const app_config& config) : cash_(config.initial_cash)
        {
        }

        bool reserve(int notional)
        {
            if (notional > cash_)
                return false;
            cash_ -= notional;
            return true;
        }

        int available_cash() const
        {
            return cash_;
        }

    private:
        int cash_;
    };

    template <typename Traits> struct risk_check
    {
        using context_mixin = account_state;

        void on_request(const order_request& event)
        {
            auto& shared = nom::ctx(this);
            shared.trace.push_back("risk:check");
            if (event.quantity <= 0 || event.quantity % Traits::lot_size != 0)
            {
                reject(event.id, "invalid lot");
                return;
            }

            // The quote and account are shared services; the decision travels as an event.
            const int notional = event.quantity * shared.mark_price();
            if (!shared.reserve(notional))
            {
                reject(event.id, "insufficient cash");
                return;
            }
            nom::relay<Traits>::pass_request(this, approved_order{event.id, event.quantity, notional});
        }

        void on_response(const execution_report& event)
        {
            nom::ctx(this).trace.push_back("risk:report");
            nom::relay<Traits>::pass_response(this, event);
        }

    private:
        void reject(int id, const std::string& reason)
        {
            // Risk originates a rejection; it never reaches the execution node.
            nom::relay<Traits>::pass_response(this, execution_report{id, false, 0, reason});
        }
    };

    template <typename Traits> struct paper_execution
    {
        explicit paper_execution(const app_config& config) : venue_(config.venue)
        {
        }

        void on_request(const approved_order& event)
        {
            nom::ctx(this).trace.push_back("execution:fill");
            // This terminal node originates the fill, beginning the response path.
            nom::relay<Traits>::pass_response(this, execution_report{event.id, true, event.notional, venue_});
        }

    private:
        std::string venue_;
    };

    // Requests: client -> [market_data skipped] -> risk -> execution.
    // Reports: execution -> risk -> [market_data skipped] -> client.
    struct order_recipe : nom::assembly<order_recipe, order_client, market_data, risk_check, paper_execution>
    {
        static constexpr int lot_size = 1;
    };

    // Recipes are their own traits. The same risk node reads a different policy here.
    struct round_lot_recipe : nom::assembly<round_lot_recipe, order_client, market_data, risk_check, paper_execution>
    {
        static constexpr int lot_size = 10;
    };

    static_assert(!std::is_default_constructible_v<nom::relay<order_recipe>>);
    static_assert(!std::is_constructible_v<nom::relay<order_recipe>, int>);
    static_assert(std::is_constructible_v<nom::relay<order_recipe>, const app_config&>);
} // namespace

TEST_CASE("An order reserves shared cash and returns a fill through the reverse route", "[complex_relay]")
{
    nom::relay<order_recipe> app(app_config{});
    app.request(submit_order{2});

    REQUIRE(app.ctx().reports.size() == 1);
    CHECK(app.ctx().reports.front().id == 1);
    CHECK(app.ctx().reports.front().filled);
    CHECK(app.ctx().reports.front().notional == 200);
    CHECK(app.ctx().available_cash() == 300);
    CHECK(app.ctx().trace ==
          std::vector<std::string>{"client:submit", "risk:check", "execution:fill", "risk:report", "client:report"});
}

TEST_CASE("Risk originates a rejection and consumes an unaffordable order", "[complex_relay]")
{
    nom::relay<order_recipe> app(app_config{});
    app.request(submit_order{6});

    REQUIRE(app.ctx().reports.size() == 1);
    CHECK_FALSE(app.ctx().reports.front().filled);
    CHECK(app.ctx().reports.front().detail == "insufficient cash");
    CHECK(app.ctx().available_cash() == 500);
    CHECK(app.ctx().trace == std::vector<std::string>{"client:submit", "risk:check", "client:report"});
}

TEST_CASE("A fixed callback interface admits external events into the static assembly", "[complex_relay]")
{
    nom::relay<order_recipe> app(app_config{1000, 120, "test venue"});

    // An input adapter can store this fixed interface without knowing the recipe type.
    // The callback borrows app: its owner must keep the relay alive while callbacks can run.
    using input_events = nom::events_delegate_t<const submit_order&, const quote_update&>;
    input_events ingress([&app](const auto& event) { app.request(event); });
    ingress(quote_update{80});
    ingress(submit_order{4});

    const auto& services = std::as_const(app).ctx();
    CHECK(services.mark_price() == 80);
    CHECK(services.available_cash() == 680);
    REQUIRE(services.reports.size() == 1);
    CHECK(services.reports.front().notional == 320);
    CHECK(services.reports.front().detail == "test venue");
}

TEST_CASE("Changing a recipe trait changes the policy consumed by its risk node", "[complex_relay]")
{
    nom::relay<round_lot_recipe> app(app_config{2000, 100, "round lots"});
    app.request(submit_order{3});
    app.request(submit_order{10});

    REQUIRE(app.ctx().reports.size() == 2);
    CHECK(app.ctx().reports[0].id == 1);
    CHECK_FALSE(app.ctx().reports[0].filled);
    CHECK(app.ctx().reports[0].detail == "invalid lot");
    CHECK(app.ctx().reports[1].id == 2);
    CHECK(app.ctx().reports[1].filled);
    CHECK(app.ctx().reports[1].notional == 1000);
    CHECK(app.ctx().available_cash() == 1000);
}
