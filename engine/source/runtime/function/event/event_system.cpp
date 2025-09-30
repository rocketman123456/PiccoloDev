#include "runtime/function/event/event_system.h"
#include "runtime/function/event/event_def.h"

namespace Piccolo
{
    void EventSystem::initialize()
    {
        global_event_bus   = std::make_shared<EventBus>();
        physics_event_bus  = std::make_shared<EventBus>();
        render_event_bus   = std::make_shared<EventBus>();
        resource_event_bus = std::make_shared<EventBus>();
    }

    void EventSystem::clear()
    {
        global_event_bus.reset();
        physics_event_bus.reset();
        render_event_bus.reset();
        resource_event_bus.reset();
    }

    void EventSystem::tick(float dt)
    {
        // drain queued events deterministically
        global_event_bus->dispatchQueued();
        physics_event_bus->dispatchQueued();
        render_event_bus->dispatchQueued();
        resource_event_bus->dispatchQueued();
    }

    void event_test()
    {
        // Use the bus:
        EventBus bus;

        // auto c1 = bus.subscribe<PlayerDamaged>([](const PlayerDamaged& e) {
        //     // update health UI, play hurt sound
        // });

        // auto c2 = bus.subscribe<KeyPressed>(
        //     [](const KeyPressed& e) {
        //         if (e.key == /*Space*/)
        //         { /* jump */
        //         }
        //     },
        //     /*priority=*/10
        // );

        // Immediate:
        // bus.publish(PlayerDamaged {42, 15});

        // Deferred (from input thread or physics thread):
        // bus.enqueue(KeyPressed {0, false});
    }
} // namespace Piccolo
