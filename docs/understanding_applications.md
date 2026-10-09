# How to understand Assembly/Relay applications

Assembly/Relay is the core integration framework of the platform. As such, it exposes the
general structure of applications and serves as a good starting point for understanding what
they do.

The first thing you see in an Assembly/Relay app is the list of nodes organized into a logical
communication pipe. They communicate and collaborate via two mechanisms: event passing and
context interaction. If there is an option to choose between the two, event passing is
preferable because it is naturally more restrictive and reduces the scope of interaction.
The context serves two goals: it contains application-wide state and provides means for more
complex interaction, but this comes at the cost of additional complexity.

At a high level, just looking at the assembly structure might be enough to understand what it
does. However, if you want to dig deeper, you can ask the following five questions:

1. **What public services does the assembly context expose?**

   For each node in the assembly, list the public interface contributed by its `context_mixin`.
   Taken together, these methods and public members define the services available through
   `nom::ctx(...)`.

2. **Which events does each assembly node handle?**

   For every non-terminal node, list the events handled through `on_request(...)` and
   `on_response(...)`.

3. **Which assembly traits are active, and which components consume them?**

   List the traits declared on the assembly, explain which nodes read each trait, and describe
   the behavior each trait changes. A dedicated `override_traits` facility is not implemented
   yet.

4. **Which configuration sections are used by each node?**

   For every node in the assembly, identify which fields of the application configuration it
   reads directly.

5. **Which nodes are the original emitters of each event?**

   Identify the node that first creates or introduces each request/response event into the
   assembly, excluding nodes that only forward, delay, observe, or transform existing events.

See the [worked example](tests.md#read-an-application-through-five-inventories) for these five
questions applied to a small application.
