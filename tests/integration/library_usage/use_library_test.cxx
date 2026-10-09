// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <functional> // events_delegate_t uses std::function.
#include <type_traits>

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
        }
    };

    struct sensor_traits
    {
        static constexpr int scale = 1;
    };

    template <typename Traits> struct sensor
    {
        using sensor_policy = nom::use_override_if_defined_t<Traits, sensor_traits>;

        void on_request(int value)
        {
            nom::relay<Traits>::pass_response(this, reading{value * sensor_policy::scale});
        }
    };

    struct sensor_assembly : nom::assembly<sensor_assembly, results, sensor>
    {
        struct override_traits : sensor_traits
        {
            static constexpr int scale = 2;
        };
    };

    static_assert(std::is_same_v<sensor<sensor_assembly>::sensor_policy, sensor_assembly::override_traits>);
} // namespace sample

int main()
{
    nom::relay<sample::sensor_assembly> app;
    auto handler = nom::delegate{[&app](int value) { app.request(value); }};
    const auto borrowed = nom::delegate_ref{handler};
    nom::events_delegate_t<int> callback = borrowed;
    callback(21);
    return app.ctx().latest == 42 ? 0 : 1;
}
