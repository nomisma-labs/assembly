# Assembly, relay and delegate

An assembly describes an application as an ordered collection of nodes. Each node owns a
responsibility: validate a command, adapt a transport, maintain shared state, or consume a result.
The assembly definition makes those responsibilities and their communication order visible in one place.

Start with [How to understand Assembly/Relay applications](understanding_applications.md) when
reading an application. This guide explains the framework contracts behind that reading method.

Nodes collaborate through events and a shared context. Prefer events when they express the
interaction: the event signature and the node's position limit who can participate. Context is
useful for services and state with application-wide scope, such as a clock, connection state or
an order view. Its public interface is the union of the services contributed by the nodes.

The library has three main facilities:

| Facility | Responsibility | Module source |
|---|---|---|
| `assembly<Traits, Nodes...>` | Describe nodes and compose their context and storage | [`assembly_impl.cppm`](../../src/assembly/assembly_impl.cppm) |
| `relay<Assembly>` | Own an assembly and route typed calls through it | [`relay.cppm`](../../src/assembly/relay.cppm) |
| `delegate` | Connect a producer to callable handlers | [`delegate.cppm`](../../src/assembly/delegate.cppm) |

Supporting definitions include `type_list` for compile-time type lists,
[`apply_custom_traits_t`](../../src/assembly/custom_traits.cppm) for applying custom traits,
and `event_suppressors` for events without a final consumer. `events_delegate_t` and `delegate_ref`
provide additional delegate forms.

All public names are in `nom`: for example, `nom::assembly<Traits, Nodes...>` and
`nom::relay<Assembly>`. Import `nom.assembly`; its
[primary interface](../../src/assembly/assembly.cppm) re-exports the partitions above.
The implementation uses Metal **v2.1.4** headers, pinned in `external/metal`, for
compile-time type lists. The exported alias `nom::type_list<Types...>` lets applications
declare these lists without including Metal headers. Applications needing Metal algorithms
can include their headers explicitly.
The library has no dependency on configuration, logging, networking
or trading libraries. `complex_relay` is a teaching scenario built with these facilities.

## Composing an application

Each node is a class template taking the final assembly as its `Traits` parameter. The application
declares an `Assembly` type derived from `assembly<Assembly, Nodes...>`, with types or constants
that nodes consume as policies.
Nodes can read these members directly or use `apply_custom_traits_t` to apply custom traits
for their own default traits, as described below.

`Assembly::body` owns the node subobjects and exposes `get<I>()` in declared node order. A node may declare
`context_mixin`; the assembly combines those mixins into `body::context`. A passive node can
contribute only a context service, without handling any events. A relay owns an internal body
that derives from `Assembly::body` and adds the relay's service nodes. It exposes the shared
context through `ctx()` and names its assembly type through `relay<Assembly>::assembly`.

Construction shares the same initialization argument pack as **const references** with every
context mixin and node. A component accepts the complete pack if it has a compatible constructor;
otherwise it must be default constructible. For a nonempty pack, at least one node or context mixin must
accept the **entire pack** as const references. The `accepts_init_arguments` constraint rejects
arguments that every component would ignore, even if all components can default-construct.
This check spans nodes and mixins together; either group may supply the accepting constructor.
Without arguments, ordinary default construction remains available, including for an empty assembly.

These constraints apply to both the body and relay. A component without a default constructor
makes its body and relay non-default-constructible. Initialization arguments can carry configuration
data or handles to shared services. Construction does not select individual arguments or transfer
move-only ownership. Retained references must outlive their consumers.

All context mixins are constructed before any node, so a node constructor can acquire a shared
service. The free `ctx` helper accesses this context directly without calling a member on the
unfinished body. Mixins and nodes each follow declared node order; a mixin constructor must not call a
service whose construction has not completed.

From a node, `nom::ctx(this)` obtains the same context exposed by the relay. This form deduces
the assembly from `Node<Traits>` or a separate `Mixin<Traits>` template. A nested mixin supplies
the assembly explicitly with `nom::ctx<Traits>(this)`, using its enclosing node's template
parameter:

```cpp
template<typename Traits>
struct publisher_node
{
    struct context_mixin
    {
        void publish(int price)
        {
            nom::ctx<Traits>(this).quote = price;
        }
    };
};
```

Here another mixin contributes `quote`. Explicit lookup also works from nodes and separate
mixin templates. Both forms return a reference, preserve the source's constness and are
`noexcept`. Their constraints reject source types that cannot be cast to the specified
assembly's context or body. The source must still belong to that live assembly.

Mixins access the combined context directly; nodes reach it through the body. The overloads
share an internal helper that preserves constness and performs the casts without calling a
member on an unfinished object.
During construction, a mixin may use earlier, already constructed services; node constructors
can use the completed context.

## Applying custom traits

A node can define its defaults independently of an assembly. The alias
`nom::apply_custom_traits_t<Traits, NodeTraits>` selects `Traits::custom_traits` when
that type inherits from `NodeTraits` (or is the same class type). Otherwise it selects
`NodeTraits`, including when no `custom_traits` type exists. Matching member names alone
do not make a type applicable.

```cpp
struct risk_traits
{
    static constexpr int lot_size = 1;
};

template<typename Traits>
struct risk_check
{
    using risk_traits = nom::apply_custom_traits_t<Traits, ::risk_traits>;
    // Handlers read risk_traits::lot_size.
};

struct round_lot_assembly : nom::assembly<round_lot_assembly, risk_check>
{
    struct custom_traits : risk_traits
    {
        static constexpr int lot_size = 10;
    };
};
```

An assembly without custom traits uses `risk_traits::lot_size`, which is `1`. Here the
node selects `round_lot_assembly::custom_traits`, whose value is `10`. Unchanged constants
and type aliases are inherited from the defaults. The nested type can also be an alias to a
separately defined custom traits type.

One custom traits type may inherit several nodes' default trait types. Each node selects it only
for a trait family it inherits; unrelated families keep their own defaults. Selection returns
the whole custom traits type, without merging or filtering its members. Use public bases and resolve
any conflicting member names; selection uses `std::is_base_of_v` and does not
remove ordinary C++ access restrictions or ambiguities. Define custom traits before instantiating
nodes or context mixins that select it.

Nodes still receive the assembly itself as `Traits`, so relay routing and context lookup keep
the same owner type. Selecting a policy adds no runtime storage or dispatch. The
[lot-size example](tests.md#3-which-assembly-traits-change-behavior-and-who-reads-them) demonstrates
the resulting application behavior.

## Following an event

Requests travel towards later nodes; responses travel towards earlier nodes. These directions
describe the declared node order, not a requirement to pair every request with one response.

| Call | Where matching begins |
|---|---|
| `pipe.request(args...)` | First node, moving forwards |
| `pipe.respond(args...)` | Last node, moving backwards |
| `relay<Traits>::pass_request(this, args...)` | After the current node, moving forwards |
| `relay<Traits>::pass_response(this, args...)` | Before the current node, moving backwards |

At each step the relay calls the first reachable node whose `on_request(...)` or
`on_response(...)` accepts the arguments. Unrelated nodes are skipped. A handler decides what
happens next: it may forward, change the event type, create another event, or simply return.
Returning consumes the event. An observer must explicitly forward if later handlers should
also see it. A catch-all handler participates in this same search and can intercept many event
types, so its position matters.

For application-wide events without a final consumer, [event suppression](event_suppression.md)
supplies terminal request and response handlers through assembly custom traits. Application
handlers are visited first; unlisted events still require a reachable handler.

Each routing entry point calls its selected handler directly, without an intermediate dispatch
function or its extra debug stack frame. Calls run synchronously on the caller's thread.
Work after a forwarding call executes when the downstream call returns, just like nested
function calls. A transport callback can introduce a
response later, but the relay supplies no queue, scheduler, locking or ownership for delayed
events. A node that retains an event must arrange its lifetime.

Matching uses the exact forwarded argument categories, including constness and lvalue/rvalue
references. Multiple arguments and move-only events are supported. Application handlers use
normal C++ overload resolution and conversions; their matching is not restricted to identical
type names. Suppressors match a single event by its exact type without cv/ref qualifiers.
Handler return values are discarded. The four routing entry points are constrained by named
concepts: `handles_request`, `handles_response`, `handles_request_after` and
`handles_response_after`. With no reachable application handler or configured suppressor,
the call fails the corresponding constraint, so a dependent `requires` expression can test
whether a call is supported. Forwarding also
requires an `assembly_node`: the source type must belong to the assembly before the relay
searches the remaining nodes. These checks use the same forwarded argument categories as
the actual call. They check the next handler, not the bodies of handlers farther along a route.

### Exceptions during event processing (are meaningless)

Avoid exceptions in node logic and prefer internal code and library APIs that report expected
failures without throwing. If an operation uses exceptions as part of normal behavior, catch
them inside the node, around that operation, and translate the failure into an event or
application state. Do not let these exceptions escape into the relay pipe.

Relay permits exceptions and does not require `noexcept` handlers, preserving compatibility
with existing code. Exceptions have no recovery meaning in the event flow.

A `try/catch` around `pass_request` or `pass_response` can catch synchronous downstream
exceptions, but cannot undo effects already performed or protect work deferred to a later
callback or queue iteration. Event processing provides no rollback; unwinding the C++ stack
does not reverse the logical event flow.

Treat an exception escaping event processing as fatal. Catch it at the calling application
boundary, typically an `epoll`-driven event loop: wrap the **whole loop** in `try/catch`, log
the failure, and call `std::terminate()`. Do not catch each iteration and resume processing
with potentially inconsistent state. This is the application's responsibility; relay lets
escaping exceptions propagate.

## Connecting a callback boundary

Use `delegate` when a component emits several typed callbacks through one interface. A transport
can declare connection and data callbacks while a node adapts those callbacks into relay events.
This keeps the transport independent of the application's assembly definition.

| Form | Storage and intended use |
|---|---|
| `delegate(lambda_a, lambda_b)` | Owns concrete callable objects and combines their overloads; suitable for local adapters and visitors |
| `events_delegate_t<connected, const message&>` | A named interface containing one `std::function<void(Event)>` per event type; supports runtime binding and rebinding |
| `delegate(std::ref(handler))` | Borrows a handler through a reference wrapper |
| `delegate_ref(handler)` | Borrows a handler while retaining its concrete type; forwards arguments and preserves reference results |

For callback signatures with several arguments or non-void results, use explicit bases such as
`delegate<std::function<int(int, int)>, std::function<void(const message&)>>`.

Include `<functional>` before `import nom.assembly;` when using these `std::function`-based
interfaces, including `events_delegate_t`.

Binding a handler by value stores a copy. Adapting one handler to several `std::function` bases
creates a separate copy for each signature; use `std::ref(handler)` when all callbacks must
share one handler's state. Every borrowed handler must outlive its delegates. A const
`delegate_ref` can call a mutable borrowed handler; constness applies to the wrapper, as with a
reference. An unbound erased signature throws `std::bad_function_call` when called.

Concrete delegates can own move-only callables. The erased event interface follows
`std::function`'s copyability requirements. Event references are preserved when explicitly
included in the signature; the delegate does not extend an event's lifetime.

## Composition boundaries

Node and context storage uses inheritance. Concrete node types must be distinct, and repeated
context-mixin types or clashing public service names can create ambiguous bases or access.
Use distinct node or mixin types for distinct roles.

`ctx(part)` requires a node or mixin belonging to the specified live assembly;
`pass_request`/`pass_response` require a node owned by the corresponding relay. A separately
constructed `Assembly::body` lacks the relay's service nodes. Establish a stable owner address
before installing callbacks that
capture nodes; moving a wired assembly does not retarget existing pointers or references.

Continue with the [executable learning guide](tests.md), which applies this model to a complete
small application and identifies its services, events, policies, configuration and emitters.
