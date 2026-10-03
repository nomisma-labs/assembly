// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

export module nom.assembly:delegate;

export namespace nom
{
    // An overload set that owns its callables. Use std::ref to borrow a handler;
    // use std::function bases when a component needs a fixed callback interface.
    template <typename... Callables> struct delegate : Callables...
    {
        using Callables::operator()...;

        delegate() = default;

        constexpr delegate(Callables... callables)
            requires(sizeof...(Callables) > 0)
            : Callables(std::move(callables))...
        {
        }

        // Adapt one handler to every signature. Each base stores its own copy
        // unless the handler is a reference wrapper.
        template <typename Callable>
            requires(sizeof...(Callables) > 0 && (std::constructible_from<Callables, Callable&> && ...))
        constexpr delegate(Callable callable) : Callables(callable)...
        {
        }
    };

    template <typename... Callables> delegate(Callables...) -> delegate<Callables...>;

    // Event parameter types are preserved: use const event& to borrow an event.
    // An unbound signature has the usual std::function bad_function_call behavior.
    template <typename... Events> struct events_delegate
    {
        using type = delegate<std::function<void(Events)>...>;
    };

    template <typename... Events> using events_delegate_t = typename events_delegate<Events...>::type;

    // A non-owning delegate with a concrete handler type. The handler must
    // outlive this wrapper and every copy of it.
    template <typename Callable> class delegate_ref
    {
    public:
        constexpr delegate_ref(Callable& callable) noexcept : ref_(callable)
        {
        }

        constexpr delegate_ref(const std::reference_wrapper<Callable>& ref) noexcept : ref_(ref)
        {
        }

        template <typename... Args>
            requires std::invocable<Callable&, Args...>
        constexpr decltype(auto) operator()(Args&&... args) const
            noexcept(std::is_nothrow_invocable_v<Callable&, Args...>)
        {
            return std::invoke(ref_.get(), std::forward<Args>(args)...);
        }

    private:
        const std::reference_wrapper<Callable> ref_;
    };
} // namespace nom
