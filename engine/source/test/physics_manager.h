#pragma once

#include "jolt_character_test_common.h"

// 前向声明
class TerrainSystem;

// 对象层过滤器
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override;
};

// 宽相层接口
class BPLayerInterfaceImpl : public BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl();
    virtual uint GetNumBroadPhaseLayers() const override;
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override;
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override;
#endif

private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// 对象与宽相层过滤器
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override;
};

// 联系监听器
class MyContactListener : public ContactListener
{
public:
    virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override;
    virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override;
    virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override;
    virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override;
};

// 身体激活监听器
class MyBodyActivationListener : public BodyActivationListener
{
public:
    virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override;
    virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override;
};

// 物理管理器
class PhysicsManager
{
public:
    PhysicsManager();
    ~PhysicsManager();

    bool Initialize();
    bool Initialize(TerrainSystem* terrain_system);
    void Update(float delta_time);
    void Shutdown();

    // 角色相关
    Character* GetCharacter() const { return m_character; }
    void SetCharacterPosition(const RVec3& position);
    void SetCharacterPositionOnTerrain(float world_x, float world_z, TerrainSystem* terrain_system);
    RVec3 GetCharacterPosition() const;
    void SetCharacterVelocity(const Vec3& velocity);
    Vec3 GetCharacterVelocity() const;
    Character::EGroundState GetCharacterGroundState() const;

    // 地面物理体管理
    void UpdateDynamicGroundPhysics(const std::map<std::pair<int, int>, float>& terrain_data, const std::pair<int, int>& character_grid_pos);
    const std::map<std::pair<int, int>, BodyID>& GetActivePhysicsBodies() const { return m_active_physics_bodies; }
    const std::map<std::pair<int, int>, BodyID>& GetActiveSideWallBodies() const { return m_active_side_wall_bodies; }

    // 工具函数
    std::pair<int, int> WorldToGrid(float world_x, float world_z) const;
    bool IsValidGridPos(int grid_x, int grid_z) const;

private:
    PhysicsSystem* m_physics_system;
    TempAllocatorImpl* m_temp_allocator;
    JobSystemThreadPool* m_job_system;
    Character* m_character;
    
    // 动态地面物理系统
    std::map<std::pair<int, int>, BodyID> m_active_physics_bodies;
    std::map<std::pair<int, int>, BodyID> m_active_side_wall_bodies; // 侧墙碰撞体
    std::pair<int, int> m_last_character_grid_pos;
    
    // 配置
    TerrainConfig m_terrain_config;
    CharacterConfig m_character_config;

    // 内部方法
    BodyID CreateGroundPhysicsBody(int grid_x, int grid_z, BodyInterface& body_interface, const std::map<std::pair<int, int>, float>& terrain_data);
    void RemoveGroundPhysicsBody(BodyID body_id, BodyInterface& body_interface);
    BodyID CreateSideWallPhysicsBody(int grid_x, int grid_z, int side, BodyInterface& body_interface, const std::map<std::pair<int, int>, float>& terrain_data);
    void RemoveSideWallPhysicsBody(BodyID body_id, BodyInterface& body_interface);
};
