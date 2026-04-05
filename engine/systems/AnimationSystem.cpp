#include "AnimationSystem.hpp"
#include "../ecs/World.hpp"
#include "../components/Sprite.hpp"
#include "../components/AnimationState.hpp"

namespace hiromi {

void AnimationSystem::update(World& world, float dt) {
    world.query<Sprite, AnimationState>().each(
        [&](EntityId /*e*/, Sprite& sprite, AnimationState& anim) {
            if (!anim.playing || anim.frame_count <= 1) return;

            anim.elapsed += dt;
            const float frame_duration = 1.0f / anim.fps;

            while (anim.elapsed >= frame_duration) {
                anim.elapsed -= frame_duration;
                anim.current_frame++;

                if (anim.current_frame >= anim.frame_count) {
                    if (anim.looping) {
                        anim.current_frame = 0;
                    } else {
                        anim.current_frame = anim.frame_count - 1;
                        anim.playing       = false;
                        break;
                    }
                }
            }

            // Update sprite source rectangle (horizontal sprite sheet).
            if (anim.frame_pixel_width > 0.0f) {
                sprite.src_x = anim.current_frame * anim.frame_pixel_width;
                sprite.src_w = anim.frame_pixel_width;
                if (anim.frame_pixel_height > 0.0f) {
                    sprite.src_h = anim.frame_pixel_height;
                }
            }
        });
}

} // namespace hiromi
