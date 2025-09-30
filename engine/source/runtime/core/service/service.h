#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/core/utils/noncopyable.h"

namespace Piccolo
{
    class IService : NonCopyable
    {
    public:
        explicit IService(const std::string& name)
            : m_name(name)
        {}

        virtual ~IService() = default;

        void initialize();
        void clear();

    protected:
        std::string m_name;
        std::string m_hash {};
    };
} // namespace Piccolo