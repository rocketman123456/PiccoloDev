#include "runtime/function/animation/pose.h"

using namespace Piccolo;

AnimationPose::AnimationPose(const AnimationClip& clip, float ratio, const AnimSkelMap& animSkelMap)
{
    bone_indexs = animSkelMap.convert;
    reorder     = true;
    extractFromClip(bone_poses, clip, ratio);
    weight.blend_weight.resize(bone_poses.size());
    for (auto& weight : weight.blend_weight)
    {
        weight = 1.f;
    }
}
AnimationPose::AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight_, float ratio)
{
    weight  = weight_;
    reorder = false;
    extractFromClip(bone_poses, clip, ratio);
}
AnimationPose::AnimationPose(const AnimationClip& clip, const BoneBlendWeight& weight_, float ratio, const AnimSkelMap& animSkelMap)
{
    weight      = weight_;
    bone_indexs = animSkelMap.convert;
    reorder     = true;
    extractFromClip(bone_poses, clip, ratio);
}

void AnimationPose::extractFromClip(std::vector<Transform>& bones, const AnimationClip& clip, float ratio)
{
    bones.resize(clip.node_count);

    float exact_frame        = ratio * (clip.total_frame - 1);
    int   current_frame_low  = floor(exact_frame);
    int   current_frame_high = ceil(exact_frame);
    float lerp_ratio         = exact_frame - current_frame_low;
    for (int i = 0; i < clip.node_count; i++)
    {
        const AnimationChannel& channel = clip.node_channels[i];

        bones[i].m_position = Vector3::lerp(channel.position_keys[current_frame_low], channel.position_keys[current_frame_high], lerp_ratio);
        bones[i].m_scale    = Vector3::lerp(channel.scaling_keys[current_frame_low], channel.scaling_keys[current_frame_high], lerp_ratio);
        bones[i].m_rotation = Quaternion::nLerp(lerp_ratio, channel.rotation_keys[current_frame_low], channel.rotation_keys[current_frame_high], true);
    }
}

void AnimationPose::blend(const AnimationPose& pose)
{
    // Loop each bone
    for (int i = 0; i < bone_poses.size(); i++)
    {
        auto&       bone_trans_one = bone_poses[i];
        const auto& bone_trans_two = pose.bone_poses[i];

        const float& weight_one = weight.blend_weight[i];
        const float& weight_two = pose.weight.blend_weight[i];
        float        sum_weight = weight_one + weight_two;

        if (sum_weight != 0)
        {
            float cur_weight          = weight_two / sum_weight;
            weight.blend_weight[i]    = sum_weight;
            bone_trans_one.m_position = Vector3::lerp(bone_trans_one.m_position, bone_trans_two.m_position, cur_weight);
            bone_trans_one.m_scale    = Vector3::lerp(bone_trans_one.m_scale, bone_trans_two.m_scale, cur_weight);
            bone_trans_one.m_rotation = Quaternion::sLerp(cur_weight, bone_trans_one.m_rotation, bone_trans_two.m_rotation, true);
        }
    }
}
