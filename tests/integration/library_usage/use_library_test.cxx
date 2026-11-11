// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

import std;

import nom.assembly;

namespace sample
{
    struct reading
    {
        int value;
    };

    template <typename Traits> struct results
    {
        struct context_mixin
        {
            int latest{};
        };

        void on_response(reading event)
        {
            nom::ctx(this).latest = event.value;
            nom::relay<Traits>::pass_response(this, event);
        }
    };

    struct sensor_traits
    {
        static constexpr int scale = 1;
    };

    template <typename Traits> struct sensor
    {
        using sensor_traits = nom::apply_custom_traits_t<Traits, ::sample::sensor_traits>;

        void on_request(int value)
        {
            nom::relay<Traits>::pass_response(this, reading{value * sensor_traits::scale});
            nom::relay<Traits>::pass_request(this, value);
        }
    };

    struct sensor_assembly : nom::assembly<sensor_assembly, results, sensor>
    {
        struct custom_traits : sensor_traits, nom::event_suppressors
        {
            static constexpr int scale = 2;
            using suppressed_requests = nom::type_list<int>;
            using suppressed_responses = nom::type_list<reading>;
        };
    };

    static_assert(std::is_same_v<sensor<sensor_assembly>::sensor_traits, sensor_assembly::custom_traits>);
    static_assert(std::is_same_v<nom::relay<sensor_assembly>::assembly, sensor_assembly>);
} // namespace sample

int main()
{
    nom::relay<sample::sensor_assembly> app;
    app.respond(sample::reading{7});
    if (app.ctx().latest != 7)
        return 1;

    auto handler = nom::delegate{[&app](int value) { app.request(value); }};
    const auto borrowed = nom::delegate_ref{handler};
    nom::events_delegate_t<int> callback = borrowed;
    callback(21);
    return app.ctx().latest == 42 ? 0 : 1;
}
