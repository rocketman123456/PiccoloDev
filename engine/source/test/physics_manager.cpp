#include "physics_manager.h"
#include "terrain_system.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <iostream>

using namespace std;

// ObjectLayerPairFilterImpl 实现
bool ObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const
{
    switch (inObject1)
    {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
    }
}

// BPLayerInterfaceImpl 实现
BPLayerInterfaceImpl::BPLayerInterfaceImpl()
{
    mObjectToBroadPhase[Layers::NON_MOVING] = BPLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BPLayers::MOVING;
}

uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
{
    return BPLayers::NUM_LAYERS;
}

BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer inLayer) const
{
    JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
{
    switch ((BroadPhaseLayer::Type)inLayer)
    {
        case (BroadPhaseLayer::Type)BPLayers::NON_MOVING:
            return "NON_MOVING";
        case (BroadPhaseLayer::Type)BPLayers::MOVING:
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
    }
}
#endif

// ObjectVsBroadPhaseLayerFilterImpl 实现
bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const
{
    switch (inLayer1)
    {
        case Layers::NON_MOVING:
            return inLayer2 == BPLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
    }
}

// MyContactListener 实现
ValidateResult MyContactListener::OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult)
{
    return ValidateResult::AcceptAllContactsForThisBodyPair;
}

void MyContactListener::OnContactAdded(const Body&, const Body&, const ContactManifold&, ContactSettings&) {}
void MyContactListener::OnContactPersisted(const Body&, const Body&, const ContactManifold&, ContactSettings&) {}
void MyContactListener::OnContactRemoved(const SubShapeIDPair&) {}

void MyBodyActivationListener::OnBodyActivated(const BodyID&, uint64) {}
void MyBodyActivationListener::OnBodyDeactivated(const BodyID&, uint64) {}

// PhysicsManager 实现
PhysicsManager::PhysicsManager()
    : m_physics_system(nullptr)
    , m_temp_allocator(nullptr)
    , m_job_system(nullptr)
    , m_character(nullptr)
    , m_last_character_grid_pos(-999, -999)
{
}

PhysicsManager::~PhysicsManager()
{
    Shutdown();
}

bool PhysicsManager::Initialize()
{
    // 调用不带地形系统的版本，使用默认位置
    return Initialize(nullptr);
}

bool PhysicsManager::Initialize(TerrainSystem* terrain_system)
{
    // 注册所有 Jolt 的类型
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    // 创建临时内存分配器
    m_temp_allocator = new TempAllocatorImpl(160 * 1024 * 1024);

    // 创建作业系统
    m_job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    // 设置层接口
    BPLayerInterfaceImpl* broad_phase_layer_interface = new BPLayerInterfaceImpl();
    ObjectVsBroadPhaseLayerFilterImpl* object_vs_broad_phase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
    ObjectLayerPairFilterImpl* object_layer_pair_filter = new ObjectLayerPairFilterImpl();

    // 创建物理系统
    const uint cMaxBodies = 1024;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 1024;
    const uint cMaxContactConstraints = 1024;

    m_physics_system = new PhysicsSystem();
    m_physics_system->Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        *broad_phase_layer_interface,
        *object_vs_broad_phase_layer_filter,
        *object_layer_pair_filter
    );

    // 设置联系监听器
    MyContactListener* contact_listener = new MyContactListener();
    m_physics_system->SetContactListener(contact_listener);

    // 设置身体激活监听器
    MyBodyActivationListener* body_activation_listener = new MyBodyActivationListener();
    m_physics_system->SetBodyActivationListener(body_activation_listener);

    m_active_physics_bodies.clear();
    m_last_character_grid_pos = {-999, -999};

    // 计算角色初始位置
    RVec3 character_position(0, 8, 0); // 默认位置
    if (terrain_system)
    {
        // 根据地形高度设置角色位置
        float terrain_height = terrain_system->GetTerrainHeightAtWorldPos(0.0f, 0.0f);
        character_position = RVec3(0, terrain_height + m_character_config.half_height + m_character_config.terrain_height_offset, 0);
        cout << "角色初始位置设置为地形高度: " << terrain_height << " + 角色半高度: " << m_character_config.half_height << " + 偏移: " << m_character_config.terrain_height_offset << " = " << character_position.GetY() << endl;
    }
    else
    {
        cout << "未提供地形系统，使用默认角色位置: " << character_position.GetY() << endl;
    }

    // 创建角色控制器
    Ref<CharacterSettings> character_settings = new CharacterSettings();
    character_settings->mMaxSlopeAngle = DegreesToRadians(m_character_config.max_slope_angle);
    character_settings->mLayer = Layers::MOVING;
    character_settings->mShape = new CapsuleShape(m_character_config.half_height, m_character_config.radius);
    character_settings->mFriction = m_character_config.friction;
    character_settings->mSupportingVolume = Plane(Vec3::sAxisY(), -m_character_config.half_height);

    m_character = new Character(character_settings, character_position, Quat::sIdentity(), 0, m_physics_system);
    m_character->AddToPhysicsSystem(EActivation::Activate);

    return true;
}

void PhysicsManager::Update(float delta_time)
{
    if (!m_physics_system || !m_character)
        return;

    // 更新物理世界
    const int cCollisionSteps = 1;
    m_physics_system->Update(delta_time, cCollisionSteps, m_temp_allocator, m_job_system);

    // 更新角色位置（在物理更新之后）
    m_character->PostSimulation(0.55f);
}

void PhysicsManager::Shutdown()
{
    if (m_character)
    {
        m_character->RemoveFromPhysicsSystem();
        delete m_character;
        m_character = nullptr;
    }

    if (m_physics_system)
    {
        delete m_physics_system;
        m_physics_system = nullptr;
    }

    if (m_job_system)
    {
        delete m_job_system;
        m_job_system = nullptr;
    }

    if (m_temp_allocator)
    {
        delete m_temp_allocator;
        m_temp_allocator = nullptr;
    }

    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}

void PhysicsManager::SetCharacterPosition(const RVec3& position)
{
    if (m_character)
    {
        m_character->SetPosition(position);
    }
}

void PhysicsManager::SetCharacterPositionOnTerrain(float world_x, float world_z, TerrainSystem* terrain_system)
{
    if (m_character && terrain_system)
    {
        float terrain_height = terrain_system->GetTerrainHeightAtWorldPos(world_x, world_z);
        RVec3 position(world_x, terrain_height + m_character_config.half_height + m_character_config.terrain_height_offset, world_z);
        m_character->SetPosition(position);
        cout << "角色位置设置到地形: (" << world_x << ", " << world_z << ") 高度: " << terrain_height << " 最终Y: " << position.GetY() << endl;
    }
}

RVec3 PhysicsManager::GetCharacterPosition() const
{
    if (m_character)
    {
        return m_character->GetPosition();
    }
    return RVec3::sZero();
}

void PhysicsManager::SetCharacterVelocity(const Vec3& velocity)
{
    if (m_character)
    {
        m_character->SetLinearVelocity(velocity);
    }
}

Vec3 PhysicsManager::GetCharacterVelocity() const
{
    if (m_character)
    {
        return m_character->GetLinearVelocity();
    }
    return Vec3::sZero();
}

Character::EGroundState PhysicsManager::GetCharacterGroundState() const
{
    if (m_character)
    {
        return m_character->GetGroundState();
    }
    return Character::EGroundState::InAir;
}

void PhysicsManager::UpdateDynamicGroundPhysics(const std::map<std::pair<int, int>, float>& terrain_data, const std::pair<int, int>& character_grid_pos)
{
    if (!m_character || !m_physics_system)
        return;

    // 检查是否需要更新
    if (character_grid_pos == m_last_character_grid_pos)
        return; // 角色没有移动到新的网格位置，不需要更新

    BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    // 计算新的区域
    std::set<std::pair<int, int>> new_region;
    for (int dx = -m_terrain_config.physics_radius; dx <= m_terrain_config.physics_radius; ++dx)
    {
        for (int dz = -m_terrain_config.physics_radius; dz <= m_terrain_config.physics_radius; ++dz)
        {
            int grid_x = character_grid_pos.first + dx;
            int grid_z = character_grid_pos.second + dz;

            // 检查地形数据是否存在
            if (terrain_data.find({grid_x, grid_z}) != terrain_data.end())
            {
                new_region.insert({grid_x, grid_z});
            }
        }
    }

    // 移除不再需要的地面物理体
    auto it = m_active_physics_bodies.begin();
    while (it != m_active_physics_bodies.end())
    {
        if (new_region.find(it->first) == new_region.end())
        {
            RemoveGroundPhysicsBody(it->second, body_interface);
            it = m_active_physics_bodies.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 移除不再需要的侧墙物理体
    auto side_it = m_active_side_wall_bodies.begin();
    while (side_it != m_active_side_wall_bodies.end())
    {
        // 从复合键中提取网格位置
        int grid_x = side_it->first.first / 1000;
        int grid_z = side_it->first.second;
        std::pair<int, int> grid_pos = {grid_x, grid_z};
        
        if (new_region.find(grid_pos) == new_region.end())
        {
            RemoveSideWallPhysicsBody(side_it->second, body_interface);
            side_it = m_active_side_wall_bodies.erase(side_it);
        }
        else
        {
            ++side_it;
        }
    }

    // 添加新的地面物理体
    for (const auto& grid_pos : new_region)
    {
        if (m_active_physics_bodies.find(grid_pos) == m_active_physics_bodies.end())
        {
            BodyID new_body = CreateGroundPhysicsBody(grid_pos.first, grid_pos.second, body_interface, terrain_data);
            if (!new_body.IsInvalid())
            {
                m_active_physics_bodies[grid_pos] = new_body;
            }
        }
    }

    // 添加新的侧墙物理体
    for (const auto& grid_pos : new_region)
    {
        // 检查四个方向的侧墙
        for (int side = 0; side < 4; ++side)
        {
            std::pair<int, int> side_key = {grid_pos.first * 1000 + side, grid_pos.second}; // 使用复合键
            if (m_active_side_wall_bodies.find(side_key) == m_active_side_wall_bodies.end())
            {
                BodyID side_body = CreateSideWallPhysicsBody(grid_pos.first, grid_pos.second, side, body_interface, terrain_data);
                if (!side_body.IsInvalid())
                {
                    m_active_side_wall_bodies[side_key] = side_body;
                }
            }
        }
    }

    m_last_character_grid_pos = character_grid_pos;
    cout << "动态地面物理更新完成，地面物理体: " << m_active_physics_bodies.size() 
         << ", 侧墙物理体: " << m_active_side_wall_bodies.size() << endl;
}

std::pair<int, int> PhysicsManager::WorldToGrid(float world_x, float world_z) const
{
    int grid_x = (int)floor(world_x + m_terrain_config.grid_size * 0.5f);
    int grid_z = (int)floor(world_z + m_terrain_config.grid_size * 0.5f);
    return {grid_x, grid_z};
}

bool PhysicsManager::IsValidGridPos(int grid_x, int grid_z) const
{
    return grid_x >= 0 && grid_x < m_terrain_config.grid_size && 
           grid_z >= 0 && grid_z < m_terrain_config.grid_size;
}

BodyID PhysicsManager::CreateGroundPhysicsBody(int grid_x, int grid_z, BodyInterface& body_interface, const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 检查地形数据是否存在
    auto terrain_it = terrain_data.find({grid_x, grid_z});
    if (terrain_it == terrain_data.end())
        return BodyID(); // 地形数据不存在，不创建物理体

    float height = terrain_it->second;

    // 计算世界位置
    float world_x = (grid_x + 0.5f - m_terrain_config.grid_size / 2.0f) * m_terrain_config.tile_size;
    float world_z = (grid_z + 0.5f - m_terrain_config.grid_size / 2.0f) * m_terrain_config.tile_size;

    // 创建物理体
    float half_tile_size = m_terrain_config.tile_size / 2.0f;
    float half_height = 0.5f;

    BoxShapeSettings box_shape_settings(Vec3(half_tile_size, half_height, half_tile_size));
    ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
    ShapeRefC box_shape = box_shape_result.Get();

    BodyCreationSettings tile_settings(
        box_shape,
        RVec3(world_x, height - half_height, world_z),
        Quat::sIdentity(),
        EMotionType::Static,
        Layers::NON_MOVING
    );

    Body* tile = body_interface.CreateBody(tile_settings);
    body_interface.AddBody(tile->GetID(), EActivation::DontActivate);

    return tile->GetID();
}

void PhysicsManager::RemoveGroundPhysicsBody(BodyID body_id, BodyInterface& body_interface)
{
    if (body_id.IsInvalid())
        return;

    body_interface.RemoveBody(body_id);
    body_interface.DestroyBody(body_id);
}

BodyID PhysicsManager::CreateSideWallPhysicsBody(int grid_x, int grid_z, int side, BodyInterface& body_interface, const std::map<std::pair<int, int>, float>& terrain_data)
{
    // 检查地形数据是否存在
    auto terrain_it = terrain_data.find({grid_x, grid_z});
    if (terrain_it == terrain_data.end())
        return BodyID(); // 地形数据不存在，不创建物理体

    float height = terrain_it->second;
    float height_bottom = m_terrain_config.base_height;
    
    // 如果高度差太小，不需要创建侧墙碰撞体
    if (height - height_bottom < 0.1f)
        return BodyID();

    // 计算世界位置
    float world_x = (grid_x + 0.5f - m_terrain_config.grid_size / 2.0f) * m_terrain_config.tile_size;
    float world_z = (grid_z + 0.5f - m_terrain_config.grid_size / 2.0f) * m_terrain_config.tile_size;

    // 检查相邻地形的高度，决定是否需要侧墙
    float adjacent_height = height_bottom;
    float wall_center_x = world_x;
    float wall_center_z = world_z;
    float wall_center_y = (height + height_bottom) / 2.0f;
    
    // 根据侧墙方向调整位置和检查相邻高度
    switch (side)
    {
        case 0: // 左面 (-X)
        {
            auto left_it = terrain_data.find({grid_x - 1, grid_z});
            adjacent_height = (left_it != terrain_data.end()) ? left_it->second : height_bottom;
            wall_center_x = world_x - m_terrain_config.tile_size / 2.0f;
            break;
        }
        case 1: // 右面 (+X)
        {
            auto right_it = terrain_data.find({grid_x + 1, grid_z});
            adjacent_height = (right_it != terrain_data.end()) ? right_it->second : height_bottom;
            wall_center_x = world_x + m_terrain_config.tile_size / 2.0f;
            break;
        }
        case 2: // 前面 (-Z)
        {
            auto front_it = terrain_data.find({grid_x, grid_z - 1});
            adjacent_height = (front_it != terrain_data.end()) ? front_it->second : height_bottom;
            wall_center_z = world_z - m_terrain_config.tile_size / 2.0f;
            break;
        }
        case 3: // 后面 (+Z)
        {
            auto back_it = terrain_data.find({grid_x, grid_z + 1});
            adjacent_height = (back_it != terrain_data.end()) ? back_it->second : height_bottom;
            wall_center_z = world_z + m_terrain_config.tile_size / 2.0f;
            break;
        }
    }

    // 如果高度差太小，不需要创建侧墙碰撞体
    if (abs(height - adjacent_height) < 0.1f)
        return BodyID();

    // 创建侧墙碰撞体
    float wall_thickness = 0.1f; // 侧墙厚度
    float wall_height = abs(height - adjacent_height);
    float wall_width = m_terrain_config.tile_size;
    
    Vec3 wall_half_extents;
    if (side == 0 || side == 1) // 左右面
    {
        wall_half_extents = Vec3(wall_thickness / 2.0f, wall_height / 2.0f, wall_width / 2.0f);
    }
    else // 前后面
    {
        wall_half_extents = Vec3(wall_width / 2.0f, wall_height / 2.0f, wall_thickness / 2.0f);
    }

    BoxShapeSettings wall_shape_settings(wall_half_extents);
    ShapeSettings::ShapeResult wall_shape_result = wall_shape_settings.Create();
    ShapeRefC wall_shape = wall_shape_result.Get();

    BodyCreationSettings wall_settings(
        wall_shape,
        RVec3(wall_center_x, wall_center_y, wall_center_z),
        Quat::sIdentity(),
        EMotionType::Static,
        Layers::NON_MOVING
    );

    Body* wall = body_interface.CreateBody(wall_settings);
    body_interface.AddBody(wall->GetID(), EActivation::DontActivate);

    return wall->GetID();
}

void PhysicsManager::RemoveSideWallPhysicsBody(BodyID body_id, BodyInterface& body_interface)
{
    if (body_id.IsInvalid())
        return;

    body_interface.RemoveBody(body_id);
    body_interface.DestroyBody(body_id);
}
