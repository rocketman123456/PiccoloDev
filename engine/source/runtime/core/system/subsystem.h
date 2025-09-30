#pragma once
#include "runtime/core/base/macro.h"
#include "runtime/core/utils/noncopyable.h"
#include "runtime/core/utils/utils.h"

#include <string>

namespace Piccolo
{
    class SubsystemTickData
    {
    public:
        ApplicationTickData tick_data;
    };

    class ISubsystem : NonCopyable
    {
    public:
        explicit ISubsystem(const std::string& name)
            : m_name(name)
        {}

        virtual ~ISubsystem() = default;

        // Get subsystem name.
        inline const auto& getName() const { return m_name; }
        inline const auto& getHash() const { return m_hash; }

        inline bool initialize(const std::string& hash)
        {
            m_hash           = hash;
            bool initialized = onInit();
            if (initialized)
            {
                LOG_DEBUG("Init subsystem: '{0}'.", m_name);
            }
            else
            {
                m_hash.clear();
            }
            return initialized;
        }

        inline bool tick(const SubsystemTickData& tickData)
        {
            // check(!m_hash.empty());
            return onTick(tickData);
        }

        inline void release()
        {
            onRelease();
            LOG_DEBUG("Release subsystem: '{0}'.", m_name);
        }

        virtual void registerCheck() {}
        virtual void beforeRelease() {}

    protected:
        virtual bool onInit()                                  = 0;
        virtual bool onTick(const SubsystemTickData& tickData) = 0;
        virtual void onRelease()                               = 0;

    protected:
        std::string m_name;
        std::string m_hash {};
    };

    template<typename T>
    constexpr void checkIsBasedOnSubsystem()
    {
        static_assert(std::is_base_of<ISubsystem, T>::value, "This type doesn't based on ISubsystem.");
    }
} // namespace Piccolo
