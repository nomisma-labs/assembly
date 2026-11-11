# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: MIT

nom_lib(assembly IMPORT_STD
    MODULES assembly.cppm assembly_impl.cppm custom_traits.cppm relay.cppm delegate.cppm
    PUBLIC_DEPS nom::metal)

nom_install_package(TARGETS assembly DEPENDENCIES nom_metal)
