#pragma once
#include "runtime/resource/res_type/data/animation_clip.h"
#include "runtime/resource/res_type/data/animation_skeleton_node_map.h"
#include "runtime/resource/res_type/data/blend_state.h"

namespace Piccolo
{
    class AnimationPose
    {
        static void extractFromClip(std::vector<Transform>& bones, const AnimationClip& clip, float ratio);

    public:
        std::vector<Transform> bone_poses;
        bool                   reorder {false};
        std::vector<int>       bone_indexs;
        BoneBlendWeight        weight;

        AnimationPose() = default;

        AnimationPose(const AnimationClip& clip, float ratio, const AnimSkelMap& animSkelMap);
        AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight, float ratio);
        AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight, float ratio, const AnimSkelMap& animSkelMap);
        void blend(const AnimationPose& pose);
    };
} // namespace Piccolo
