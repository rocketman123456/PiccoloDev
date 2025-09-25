#pragma once
#include "runtime/function/controller/character_controller.h"

#include "runtime/core/math/vector3.h"
#include "runtime/resource/res_type/components/rigid_body.h"
#include "runtime/resource/res_type/data/basic_shape.h"

namespace Piccolo
{
    class NPCCharacterController : public Controller
    {
    public:
        NPCCharacterController(const Capsule& capsule);
        ~NPCCharacterController() = default;

        Vector3 move(const Vector3& current_position, const Vector3& displacement) override;

    private:
        Capsule        m_capsule;
        RigidBodyShape m_rigidbody_shape;
    };
} // namespace Piccolo
