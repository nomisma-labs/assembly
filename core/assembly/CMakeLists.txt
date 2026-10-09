# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_lib(assembly
    MODULES assembly.cppm assembly_impl.cppm override_traits.cppm relay.cppm delegate.cppm
    PUBLIC_DEPS nom::metal)

if(nom_install)
    nom_install_package(TARGETS assembly DEPENDENCIES nom_metal)
endif()
