# TODO

## RELAY-ROOT-REQUEST — Start a request from the beginning of an assembly from any node

**Status:** Deferred; proposal only. API naming remains open.

### Problem

A node such as `app_terminator` can originate an application-wide `service_command::stop`
after a signal, timeout or feed failure. Using `relay<Traits>::pass_request(this, stop)`
only searches nodes after the terminator, so earlier nodes with stop handlers are missed.
The request should enter through the same route as `relay::request(...)`, starting at
the first node and proceeding in declaration order through `on_request(...)` handlers.
Nodes currently have no public entry point for doing this without an owning relay reference.

This should preserve normal selective routing: nodes without a matching handler are
skipped, which is expected. Handling nodes forward when subsequent handlers should receive
the event; the final consumer or a configured suppressor ends the chain. This is not a
proposal for automatic broadcast.

Sending both a request and a response is unsuitable: responses travel in reverse order
and invoke `on_response(...)`, while stop handlers are defined as `on_request(...)`.

### Possible workarounds

**Place the terminator before every other stop-handling node.** This is why the current feeder
works: `app_terminator` is the first node that handles `stop`, so its `pass_request(this, stop)`
reaches all other stop handlers. Nodes without such a handler, such as `asio_service`, may
precede it. This is a composition workaround: adding or moving a stop handler before the
terminator silently excludes it from shutdown initiated by the terminator. It does not
provide a general way for any node to originate a request from the beginning.

**Use node 0 as a forwarding anchor.** A node belonging to a live relay can recover its
assembly body and forward from the first node:

```cpp
auto& body = static_cast<typename Traits::body&>(*this);
nom::relay<Traits>::pass_request(&body.template get<0>(), command);
```

This reaches nodes before the originator, but `pass_request` excludes the anchor itself.
It works when node 0 has no relevant request handler, as with an I/O-context service.
It is a poor general solution because correctness depends on node order and the role of
node 0; adding a stop handler there silently misses it. The problem is the skipped anchor,
not the normal absence of handlers on unrelated nodes.

### Preferred solution

Add a node-based relay entry point, provisionally:

```cpp
nom::relay<Traits>::request_from_start(this, command);
```

Inside the relay, recover the containing body exactly as `pass_request` already does,
then select the first matching handler from the full node list, as `request` does.
No stored parent pointer, application callback or additional ownership is required.
Preserve synchronous dispatch, argument forwarding, source-node constraints and existing
suppression rules. The source must belong to a live relay, as for `pass_request`.

Separate initiating shutdown from handling a stop request. The originator records the
first exit result and dispatches from the beginning once. When stop reaches that node,
its `on_request` handler cleans up local resources and forwards normally; it must not
restart dispatch or consume the request merely because the exit result is already set.

When implementing, verify origins at the first, middle and last node; delivery to node 0
when it has a handler; skipped non-handlers; normal forwarding/suppression; and terminator
re-entry without recursive dispatch or lost downstream delivery.

### Alternative: inherit the assembly storage in relay

Consider making `relay<Assembly>` inherit the node/context storage instead of owning the
`body assembly_` member. `Assembly` itself is currently a type descriptor; deriving from
that descriptor alone would not make nodes base subobjects of the relay. The candidate
must include `Assembly::body` and the request/response suppressors in its base hierarchy.

A node could then recover its containing relay through the inheritance chain, possibly
through a helper, and call its normal `request(...)` entry point. This could provide
root-originated requests without storing an owner pointer, while preserving the same
selective routing and synchronous dispatch.

**Status:** Deferred design option; compilation feasibility has not been verified.
Investigate possible recursive template instantiation and incomplete-type constraints
when node definitions, final assembly traits and `relay<Traits>` depend on one another.
A compile failure would need a minimal reproducer before attributing it to inheritance
itself; separating storage declarations from dispatch definitions may be relevant.
Also check cast accessibility, ambiguous inherited members, suppressor placement,
construction/context lifetime and compatibility with existing `ctx` and forwarding calls.

Inheritance alone does not solve stop-handler re-entry: initiating a root request and
handling that request must remain separate, as described above. Use the same routing
and re-entry checks when comparing this option with `request_from_start`. Keep it here
as an alternative regardless of the outcome of an initial compilation experiment.

Relevant code and contract: [relay.cppm](core/assembly/relay.cppm),
[assembly_impl.cppm](core/assembly/assembly_impl.cppm),
[event routing](docs/assembly/framework.md#following-an-event).
