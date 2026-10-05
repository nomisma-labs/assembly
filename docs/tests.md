# Learn the framework through tests

The four unit suites are small applications with assertions about their observable behavior.
Read them in this order, running a suite after reading its node definitions and recipe:

| Suite and source | What to look for |
|---|---|
| [`assembly_test.cxx`](../tests/unit/assembly/assembly_test.cxx) | A recipe versus its runtime body; configuration shared with compatible constructors; context available before nodes; services contributed by several mixins; const access |
| [`relay_test.cxx`](../tests/unit/relay/relay_test.cxx) | Requests moving forwards and responses backwards; unrelated nodes skipped; explicit consumption and transformation; call nesting; argument identity, ownership and value categories |
| [`delegate_test.cxx`](../tests/unit/delegate/delegate_test.cxx) | Concrete overloads versus a named callback interface; owned and borrowed handlers; runtime rebinding; reference results and move-only arguments |
| [`complex_relay_test.cxx`](../tests/unit/complex_relay/complex_relay_test.cxx) | A complete order workflow combining events, shared services, configuration, a recipe policy and an external callback interface |

Each suite imports `nom.assembly`, so its lessons also check the exported
API. Catch2 macros and standard-library declarations still come from headers. From the
repository root:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev -L unit
ctest --preset dev -R '^assembly_complex_relay$'
```

Each executable is at `build/dev/tests/unit/<suite>/assembly_<suite>`, where `<suite>` is
`assembly`, `relay`, `delegate` or `complex_relay`. Catch2's `--list-tests` shows each lesson;
`-s` prints successful assertions. For example:

```bash
build/dev/tests/unit/relay/assembly_relay --list-tests
build/dev/tests/unit/complex_relay/assembly_complex_relay -s
```

## Read an application through five inventories

Start with the recipe, then connect its public services, event handlers, policies, configuration
reads and event origins. This gives a checkable account of what each node contributes. The
`complex_relay` suite provides a worked example: a small paper-order workflow, with no external
services required.

Its node order is:

```text
order_client -> market_data -> risk_check -> paper_execution
```

The order path starts at the client, skips the market-data node, passes through risk checking,
and reaches paper execution. Reports return towards the client. Quote updates take a shorter
path and finish at `market_data`.

### 1. Which services can a node reach through context?

| Contributing node | Public context interface | Consumers |
|---|---|---|
| `order_client` | `next_order_id`, `reports`, `trace` | The client allocates identifiers and stores final reports; participating nodes append a trace |
| `market_data` via `market_state` | `mark_price() const`, `update_quote(int)` | Market data updates the quote; risk reads it when calculating notional |
| `risk_check` via `account_state` | `reserve(int)`, `available_cash() const` | Risk reserves cash; the caller and tests inspect the remaining balance |
| `paper_execution` | No mixin | It keeps its configured venue privately |

These members are available on the same object returned by `app.ctx()` and by
`nom::ctx(this)` inside a node. The account and quote services retain their own private
state. The event expresses the risk decision; a downstream node does not need to repeat the
context interaction that produced it.

### 2. Which event signatures select each node?

| Node | `on_request(...)` | `on_response(...)` |
|---|---|---|
| `order_client` | `const submit_order&`: allocate an identifier and send `order_request` onwards | `const execution_report&`: record the result and stop |
| `market_data` | `const quote_update&`: update shared state and stop | None |
| `risk_check` | `const order_request&`: reject invalid or unaffordable orders; otherwise reserve cash and send `approved_order` onwards | `const execution_report&`: append an observation and forward towards the client |
| `paper_execution` | `const approved_order&`: create a fill report travelling backwards | None |

The market-data node does not need forwarding overloads for order traffic: the relay skips it.
A rejection starts *before* the risk node on the reverse route, so it never calls risk's own
`on_response`. A fill originates farther along the recipe and does visit that response handler.

### 3. Which recipe policies change behavior, and who reads them?

`order_recipe::lot_size` is `1`; `round_lot_recipe::lot_size` is `10`. Both recipes compose the
same node templates. Only `risk_check` reads this value, requiring a positive quantity divisible
by the lot size. With the second recipe, quantity `3` is rejected while `10` is accepted when
cash permits. This is a compile-time policy chosen by the recipe, independent of runtime
configuration. No framework-specific override mechanism is required.

### 4. Which configuration values does each component read?

The scenario deliberately uses a small flat `app_config`. Each consumer reads its own field:

| Component | Direct configuration reads |
|---|---|
| `order_client` and its mixin | None; default initial state |
| `market_data` node | None |
| Its `market_state` mixin | `initial_quote` |
| `risk_check` node | None |
| Its `account_state` mixin | `initial_cash` |
| `paper_execution` node | `venue` |

The relay constructor passes the same configuration to compatible constructors. The application
does not manually construct or inject each service. Quote updates subsequently change the
market state through an event; they do not modify the original configuration.

### 5. Where does each event enter or originate?

| Event | Origin and subsequent handling |
|---|---|
| `submit_order` | The external caller introduces a command through `request`, or through the `input_events` callback adapter |
| `quote_update` | The external caller introduces a new quote through the same adapter; `market_data` consumes it |
| `order_request` | Internal representation: `order_client` assigns an identifier to the existing external submission |
| `approved_order` | `risk_check` transforms the existing order after validation and reservation; this remains the same order, not an independent submission |
| Rejected `execution_report` | `risk_check::reject` creates the rejection outcome |
| Filled `execution_report` | `paper_execution` creates the fill outcome; risk observes and forwards it, and the client records it |

Original occurrences here are external submissions and quotes, risk rejections, and execution
fills. `order_request` and `approved_order` represent stages of the existing submission; their
transformations are not additional original submissions. The callback adapter introduces
external events, and risk forwards fills; neither becomes their original emitter.

## Use the assertions as design examples

The fill test submits quantity `2` at price `100` against cash `500`. Its assertions verify a
report for notional `200`, cash `300`, and the route:

```text
client:submit -> risk:check -> execution:fill -> risk:report -> client:report
```

The rejection test submits quantity `6` against the same cash. It verifies unchanged cash and
the shorter trace `client:submit -> risk:check -> client:report`. The absence of execution from
that trace explains what consuming the request means.

The callback test binds a fixed `events_delegate_t<const submit_order&, const quote_update&>` to
a relay. A quote update changes the next order's notional; configuration determines the starting
cash and reported venue. The producer needs only the callback interface. Its adapter captures
the relay by reference, making the ownership relationship visible in the test.

The recipe-policy test changes the lot size while keeping the node templates unchanged. Compare
it with the construction and context examples in `assembly_test`, then with the exact argument
forwarding examples in `relay_test`. Together they distinguish application policy from routing
and storage mechanics.

The suites keep framework behavior separate from infrastructure: there are no network calls,
wall-clock dependencies or filesystem fixtures in the teaching scenarios. Their traces and
state assertions describe the contract at the call boundary. Build and package integration
checks are described separately in [build.md](kit/build.md). Both source and installed-package
consumers import the same public module.

See [framework.md](framework.md) for API contracts, ownership rules and the migration table.
