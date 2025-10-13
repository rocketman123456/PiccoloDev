#include "physics_manager.h"
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

void MyContactListener::OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
{
    // 可以在这里添加联系处理逻辑
}

void MyContactListener::OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings)
{
    // 可以在这里添加持续联系处理逻辑
}

void MyContactListener::OnContactRemoved(const SubShapeIDPair& inSubShapePair)
{
    // 可以在这里添加联系移除处理逻辑
}

// MyBodyActivationListener 实现
void MyBodyActivationListener::OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData)
{
    // 可以在这里添加身体激活处理逻辑
}

void MyBodyActivationListener::OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData)
{
    // 可以在这里添加身体停用处理逻辑
}

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

    // 初始化动态地面物理系统
    cout << "初始化动态地面物理系统..." << endl;
    m_active_physics_bodies.clear();
    m_last_character_grid_pos = {-999, -999}; // 重置为无效位置，强制初始更新

    // 创建角色控制器
    Ref<CharacterSettings> character_settings = new CharacterSettings();
    character_settings->mMaxSlopeAngle = DegreesToRadians(m_character_config.max_slope_angle);
    character_settings->mLayer = Layers::MOVING;
    character_settings->mShape = new CapsuleShape(m_character_config.half_height, m_character_config.radius);
    character_settings->mFriction = m_character_config.friction;
    character_settings->mSupportingVolume = Plane(Vec3::sAxisY(), -m_character_config.half_height);

    m_character = new Character(character_settings, RVec3(0, 8, 0), Quat::sIdentity(), 0, m_physics_system);
    m_character->AddToPhysicsSystem(EActivation::Activate);

    cout << "Jolt Physics 初始化成功！" << endl;
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

    cout << "角色移动到网格位置: (" << character_grid_pos.first << ", " << character_grid_pos.second << ")" << endl;

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

    // 移除不再需要的物理体
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

    // 添加新的物理体
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

    m_last_character_grid_pos = character_grid_pos;
    cout << "动态地面物理更新完成，当前激活物理体数量: " << m_active_physics_bodies.size() << endl;
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
