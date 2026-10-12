# Event suppression

Some application-wide events have no final consumer: nodes along a route may react to them
and forward them, but none ends their route. Configure these events through assembly traits
instead of adding utility terminator nodes. The relay supplies a separate suppressor at the
end of each route:

```text
request:  first node -> ... -> last node  -> request suppressor
response: last node  -> ... -> first node -> response suppressor
```

```cpp
import nom.assembly;

struct app_command {};

template<typename Traits>
struct processing_node
{
    void on_request(const app_command& command)
    {
        nom::relay<Traits>::pass_request(this, command);
    }
};

struct app_assembly : nom::assembly<app_assembly, processing_node>
{
    struct custom_traits : nom::event_suppressors
    {
        using suppressed_requests = nom::type_list<app_command>;
        using suppressed_responses = nom::type_list<app_command>;
    };
};

int main()
{
    nom::relay<app_assembly> pipe;
    pipe.request(app_command{}); // processing_node forwards to the request suppressor.
    pipe.respond(app_command{}); // No application handler; the response suppressor consumes it.
}
```

Both lists default to empty and can be customized independently. Suppression accepts a single
event whose type appears in the corresponding list, ignoring cv/ref qualifiers. Unlisted
events and other argument packs still require a reachable handler.

Application handlers have precedence and keep their usual forwarding behavior. Returning from
a handler consumes the event; forwarding continues the search towards the suppressor, whose
handler does nothing. Both suppressors are bases of the relay's internal body and use the same
dispatch path as application nodes. Assembly supplies the traits; relay owns this termination
behavior.

Suppression traits must be inheritable. The relay wraps them in a named derived class to keep
event lists out of suppressor template arguments and limit debug-symbol growth.
