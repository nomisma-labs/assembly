// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <functional> // events_delegate_t uses std::function.

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

    template <typename Traits> struct sensor
    {
        void on_request(int value)
        {
            nom::relay<Traits>::pass_response(this, reading{value * Traits::scale});
        }
    };

    struct recipe : nom::assembly<recipe, results, sensor>
    {
        static constexpr int scale = 2;
    };
} // namespace sample

int main()
{
    nom::relay<sample::recipe> app;
    auto handler = nom::delegate{[&app](int value) { app.request(value); }};
    const auto borrowed = nom::delegate_ref{handler};
    nom::events_delegate_t<int> callback = borrowed;
    callback(21);
    return app.ctx().latest == 42 ? 0 : 1;
}
