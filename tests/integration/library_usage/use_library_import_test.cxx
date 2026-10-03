// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

// A module does not make the std headers it includes visible to its importers: include what
// you use (here, std::string's operator==), before the imports.
#include <string>

import nom.example;

int main()
{
    nom::example::counter clicks;
    clicks.increment();
    const nom::example::settings defaults;
    const bool ok = nom::example::join("-", defaults.name, clicks.value()) == "world-1";
    return ok ? 0 : 1;
}
