# Assembly, relay and delegate

An assembly describes an application as an ordered collection of nodes. Each node owns a
responsibility: validate a command, adapt a transport, maintain shared state, or consume a result.
The recipe makes those responsibilities and their communication order visible in one place.

Nodes collaborate through events and a shared context. Prefer events when they express the
interaction: the event signature and the node's position limit who can participate. Context is
useful for services and state with application-wide scope, such as a clock, connection state or
an order view. Its public interface is the union of the services contributed by the nodes.

The library separates three jobs:

| Facility | Responsibility | Module source |
|---|---|---|
| `assembly<Traits, Nodes...>` | Describe nodes and compose their context and storage | [`composition.cppm`](../core/assembly/composition.cppm) |
| `relay<Recipe>` | Own an assembly and route typed calls through it | [`relay.cppm`](../core/assembly/relay.cppm) |
| `delegate`, `events_delegate_t`, `delegate_ref` | Connect a producer to callable handlers | [`delegate.cppm`](../core/assembly/delegate.cppm) |

All public names are in `nom`: for example, `nom::assembly<Traits, Nodes...>` and
`nom::relay<Recipe>`. Import `nom.assembly`; its
[primary interface](../core/assembly/assembly.cppm) re-exports the three partitions above.
The implementation uses the shared foundation's Metal **v2.1.4** headers for
compile-time type lists. It has no dependency on configuration, logging, networking
or trading libraries. `complex_relay` is a teaching scenario built with these facilities.

## Composing an application

Each node is a class template taking the final recipe as its parameter. The recipe derives from
`assembly<Recipe, Nodes...>` and can declare types or constants that nodes consume as policies.
Traits are ordinary C++ members; there is no special registry of overrides.

`Recipe::body` owns the node subobjects and exposes `get<I>()` in recipe order. A node may declare
`context_mixin`; the assembly combines those mixins into `body::context`. A passive node can
contribute only a context service, without handling any events. `relay<Recipe>` owns the body and
exposes its context through `ctx()`.

Construction shares the same argument pack as **const references** with every context mixin and
node. A component accepts the complete pack if it has a compatible constructor; otherwise it
must be default constructible. Constructor constraints reflect this: a configuration-only
component makes its body and relay non-default-constructible. Different components can read
different sections of one configuration. Construction does not select individual arguments or
transfer move-only ownership. Retained references must outlive their consumers.

All context mixins are constructed before any node, so a node constructor can acquire a shared
service. The free `ctx` helper accesses this context directly without calling a member on the
unfinished body. Mixins and nodes each follow recipe order; a mixin constructor must not call a
service whose construction has not completed.

From a node, `nom::ctx(this)` obtains the same context exposed by the relay. Const
nodes obtain a const context. A mixin that itself needs this lookup should be a separate
`Mixin<Recipe>` template, contributed through `using context_mixin = Mixin<Recipe>`. A nested
non-template mixin can expose services but does not carry enough information for that lookup.

## Following an event

Requests travel towards later nodes; responses travel towards earlier nodes. These directions
describe the recipe order, not a requirement to pair every request with one response.

| Call | Where matching begins |
|---|---|
| `pipe.request(args...)` | First node, moving forwards |
| `pipe.respond(args...)` | Last node, moving backwards |
| `relay<Recipe>::pass_request(this, args...)` | After the current node, moving forwards |
| `relay<Recipe>::pass_response(this, args...)` | Before the current node, moving backwards |

At each step the relay calls the first reachable node whose `on_request(...)` or
`on_response(...)` accepts the arguments. Unrelated nodes are skipped. A handler decides what
happens next: it may forward, change the event type, create another event, or simply return.
Returning consumes the event. An observer must explicitly forward if later handlers should
also see it. A catch-all handler participates in this same search and can intercept many event
types, so its position matters.

Calls run synchronously on the caller's thread. Work after a forwarding call executes when the
downstream call returns, just like nested function calls. A transport callback can introduce a
response later, but the relay supplies no queue, scheduler, locking or ownership for delayed
events. A node that retains an event must arrange its lifetime.

Matching uses the exact forwarded argument categories, including constness and lvalue/rvalue
references. Multiple arguments and move-only events are supported. Normal C++ overload
resolution and conversions still apply; matching is not restricted to identical type names.
Handler return values are discarded. A missing reachable handler is a compile-time error.

## Connecting a callback boundary

Use `delegate` when a component emits several typed callbacks through one interface. A transport
can declare connection and data callbacks while a node adapts those callbacks into relay events.
This keeps the transport independent of the application recipe.

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

`ctx(node)` and `pass_request`/`pass_response` require a node belonging to the corresponding live
assembly. A separately constructed node does not satisfy that contract. Establish a stable
owner address before installing callbacks that capture nodes; moving a wired assembly does not
retarget existing pointers or references.

## Migrating code from trading-platform

The extraction preserves composition and routing while placing the API directly in `nom` and
using snake_case names. The old `core` spellings are not compatibility aliases.

| Previous API | Assembly repository API |
|---|---|
| `core::Assembly<Traits, Nodes...>` | `nom::assembly<Traits, Nodes...>` |
| `core::Relay<Recipe>` | `nom::relay<Recipe>` |
| `core::ctx(node)` | `nom::ctx(node)` |
| `ContextMixin` | `context_mixin` |
| Assembly `Traits`, `Nodes`, `Body`; body `Context` | `traits`, `nodes`, `body`; body `context` |
| Relay `Assembly`, `Nodes`, `RNodes`, `Context` | `assembly_type`, `nodes`, `reverse_nodes`, `context` |
| `onRequest`, `onResponse` | `on_request`, `on_response` |
| `passRequest`, `passResponse` | `pass_request`, `pass_response` |
| `core::DGate`, `core::DGateRef` | `nom::delegate`, `nom::delegate_ref` |
| `core::EventsDGate<Events...>::Type` | `nom::events_delegate<Events...>::type`, or `nom::events_delegate_t<Events...>` |
| Framework `#include` directives | `import nom.assembly;` |

`request`, `respond`, `ctx`, `get<I>()` and `size` retain their names. Link consumers to the
CMake target `nom::assembly` and import `nom.assembly`.

Two behavior corrections accompany the extraction: relay handler detection now uses the same
forwarding as invocation, and `delegate_ref` preserves reference results instead of copying them.
The borrowed wrapper also supports const invocation and propagates the handler's `noexcept`
contract. Missing routes report a focused assertion instead of additionally indexing beyond
the candidate list. Const bodies now provide `get<I>()`.

Continue with the [executable learning guide](tests.md), which applies this model to a complete
small application and identifies its services, events, policies, configuration and emitters.
