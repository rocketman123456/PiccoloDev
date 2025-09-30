#pragma once

#include <cstdint>
#include <vector>

namespace Piccolo
{
    class RenderEntity
    {
    public:
        uint32_t m_instance_id {0};

        // mesh
        size_t m_mesh_asset_id {0};

        // material
        size_t m_material_asset_id {0};
    };
} // namespace Piccolo
