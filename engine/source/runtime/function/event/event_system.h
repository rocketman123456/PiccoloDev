// EventBus.h
#pragma once
#include <algorithm>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Piccolo
{
    class EventBus
    {
    public:
        // A subscription handle that can be used to unsubscribe.
        struct Connection
        {
            EventBus*       bus  = nullptr;
            std::type_index type = typeid(void);
            uint64_t        id   = 0;

            void disconnect()
            {
                if (bus)
                    bus->unsubscribe(type, id);
                bus = nullptr;
            }

            ~Connection() { disconnect(); } // RAII

            Connection()                             = default;
            Connection(const Connection&)            = delete;
            Connection& operator=(const Connection&) = delete;

            Connection(EventBus* b, std::type_index t, uint64_t i)
                : bus(b)
                , type(t)
                , id(i)
            {}
            Connection(Connection&& o) noexcept
                : bus(o.bus)
                , type(o.type)
                , id(o.id)
            {
                o.bus = nullptr;
            }

            Connection& operator=(Connection&& o) noexcept
            {
                if (this != &o)
                {
                    disconnect();
                    bus   = o.bus;
                    type  = o.type;
                    id    = o.id;
                    o.bus = nullptr;
                }
                return *this;
            }
        };

        // Subscribe: returns a Connection. Handler: void(const T&).
        template<typename T, typename F>
        Connection subscribe(F&& handler, int priority = 0)
        {
            auto type = std::type_index(typeid(T));

            std::lock_guard<std::mutex> lock(m_mutex);

            auto&    vec = m_handlers[type];
            uint64_t id  = m_next_id++;

            vec.push_back({id, priority, [h = std::forward<F>(handler)](const void* e) { h(*static_cast<const T*>(e)); }});
            // Keep higher priority first
            std::stable_sort(vec.begin(), vec.end(), [](const Handler& a, const Handler& b) { return a.priority > b.priority; });
            return Connection(this, type, id);
        }

        // Publish immediately (synchronous).
        template<typename T>
        void publish(const T& ev)
        {
            auto type = std::type_index(typeid(T));

            std::vector<Handler> snapshot;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto                        it = m_handlers.find(type);
                if (it == m_handlers.end())
                    return;
                snapshot = it->second; // copy to avoid holding lock during callbacks
            }
            for (auto& h : snapshot)
            {
                h.fn(&ev);
            }
        }

        // Enqueue for later dispatch (thread-safe).
        template<typename T>
        void enqueue(const T& ev, int priority = 0)
        {
            auto type = std::type_index(typeid(T));

            std::lock_guard<std::mutex> lock(m_queue_mutex);

            m_queued.push_back({type, priority, std::make_shared<Model<T>>(ev)});
        }

        // Dispatch queued events (call once per frame on main thread).
        void dispatchQueued(size_t maxEvents = SIZE_MAX)
        {
            std::vector<QueuedEvent> local;
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                if (m_queued.empty())
                    return;
                // Higher priority first; stable for determinism
                std::stable_sort(m_queued.begin(), m_queued.end(), [](const QueuedEvent& a, const QueuedEvent& b) { return a.priority > b.priority; });
                if (maxEvents >= m_queued.size())
                {
                    local.swap(m_queued);
                }
                else
                {
                    local.assign(m_queued.begin(), m_queued.begin() + maxEvents);
                    m_queued.erase(m_queued.begin(), m_queued.begin() + maxEvents);
                }
            }
            for (auto& q : local)
            {
                // Snapshot handlers for this type
                std::vector<Handler> snapshot;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    auto                        it = m_handlers.find(q.type);
                    if (it == m_handlers.end())
                        continue;
                    snapshot = it->second;
                }
                for (auto& h : snapshot)
                {
                    h.fn(q.model->ptr());
                }
            }
        }

    private:
        struct Handler
        {
            uint64_t                         id;
            int                              priority;
            std::function<void(const void*)> fn;
        };

        struct Concept
        {
            virtual ~Concept()              = default;
            virtual const void* ptr() const = 0;
        };

        template<typename T>
        struct Model : Concept
        {
            T value;
            explicit Model(const T& v)
                : value(v)
            {}
            const void* ptr() const override { return &value; }
        };

        struct QueuedEvent
        {
            std::type_index          type;
            int                      priority;
            std::shared_ptr<Concept> model;
        };

        void unsubscribe(std::type_index type, uint64_t id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_handlers.find(type);
            if (it == m_handlers.end())
                return;
            auto& vec = it->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const Handler& h) { return h.id == id; }), vec.end());
            if (vec.empty())
                m_handlers.erase(it);
        }

        std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;

        std::mutex               m_mutex;
        std::atomic<uint64_t>    m_next_id {1};
        std::vector<QueuedEvent> m_queued;
        std::mutex               m_queue_mutex;
    };

    class EventSystem
    {
    public:
        EventSystem()  = default;
        ~EventSystem() = default;

        void initialize();
        void finalize();
        void tick(float dt);

        std::shared_ptr<EventBus> global_event_bus;
        std::shared_ptr<EventBus> physics_event_bus;
        std::shared_ptr<EventBus> render_event_bus;
        std::shared_ptr<EventBus> resource_event_bus;
    };
} // namespace Piccolo
