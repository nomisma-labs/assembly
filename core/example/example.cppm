// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

// Wrapper module: the headers stay the source and this unit re-exports their API, so code can
// use either `#include <example/...>` or `import nom.example;`. Not in the module: macros.h
// (modules can't export macros) and settings_json.h (nlohmann, see settings.h).
module;

#include <example/counter.h>
#include <example/join.h>
#include <example/settings.h>

export module nom.example;

export namespace nom::example
{
    using nom::example::counter;
    using nom::example::join;
    using nom::example::load_settings;
    using nom::example::settings;
} // namespace nom::example
