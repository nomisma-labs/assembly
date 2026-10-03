// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <example/counter.h>
#include <example/join.h>
#include <example/settings.h>

int main()
{
    nom::example::counter clicks;
    clicks.increment();
    const nom::example::settings defaults;
    const bool ok = nom::example::join("-", defaults.name, clicks.value()) == "world-1";
    return ok ? 0 : 1;
}
