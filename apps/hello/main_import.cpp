// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

// The same app through `import nom.example;`. The headers the module can't carry (macros and the
// nlohmann mapping, see core/example/example.cppm) are still included; includes go before imports.
#include <example/macros.h>
#include <example/settings_json.h>

import nom.example;

int main(int argc, char* argv[])
{
    nom::example::settings greeting;
    if (argc > 1)
    {
        auto loaded = nom::example::load_settings(argv[1]);
        if (!loaded)
        {
            LPRINT("error: {}", loaded.error());
            return 1;
        }
        greeting = *loaded;
    }

    nom::example::counter greetings;
    for (int i = 0; i < greeting.repeat; ++i)
    {
        greetings.increment();
        LPRINT("{}", nom::example::join(" ", "hello,", greeting.name, greetings.value()));
    }
    LPRINT("settings as JSON: {}", nlohmann::json(greeting).dump());
    return 0;
}
