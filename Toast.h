#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <cmath>
#include "imgui.h"

namespace Custom {
    namespace Toast {
        struct ToastData;
        static std::vector<std::unique_ptr<ToastData>> toasts;
        static float toast_width = 280.0f;
        static float toast_margin = 20.0f;
        static float toast_spacing = 12.0f;
        static float animation_duration = 1.5f;
        static int max_visible_toasts = 5;

        struct StackParams {
            float offset_y;
            float scale;
            float alpha;
            float shadow;
        };
        
        struct ToastPhysics {
            ImVec2 velocity = ImVec2(0, 0);
            ImVec2 acceleration = ImVec2(0, 0);
        };

        struct ToastData {
            std::string title;
            std::string content;
            float duration;
            float timer;
            ImVec2 target_pos;
            ImVec2 current_pos;
            ImVec2 start_pos;
            float animation_progress;
            bool active;
            float height;
            bool is_error;
            bool is_exiting;
            ToastPhysics physics;
            float scale;
            float alpha;
            float shadow;
            float exit_progress;

            ToastData(const std::string& t, const std::string& c, float d, bool err) : 
                title(t), content(c), duration(d), timer(0.0f), animation_progress(0.0f), 
                active(true), is_error(err), is_exiting(false), scale(1.0f), alpha(1.0f), 
                shadow(0.0f), exit_progress(0.0f) {
                ImVec2 title_size = ImGui::CalcTextSize(title.c_str());
                ImVec2 content_size = ImGui::CalcTextSize(content.c_str());
                height = 50.0f + content_size.y;
                current_pos = ImVec2(0, 0);
                start_pos = ImVec2(0, 0);
            }
        };

        // 缓动函数
        namespace Easing {
            float EaseOutQuad(float t) { return 1 - (1 - t) * (1 - t); }
            float EaseOutBack(float t) { 
                const float c1 = 1.70158f; 
                const float c3 = c1 + 1;
                return 1 + c3 * pow(t - 1, 3) + c1 * pow(t - 1, 2); 
            }
        }

        // 工具函数
        inline float ImLength(const ImVec2& vec) { 
            return sqrtf(vec.x * vec.x + vec.y * vec.y); 
        }
        
        template<typename T>
        T CustomLerp(const T& a, const T& b, float t, float (*easing)(float) = Easing::EaseOutQuad) {
            return a + (b - a) * easing(t);
        }

        StackParams CalculateStackParams(int index, int total_count) {
            StackParams params;
            int depth = total_count - index + 1;
            params.offset_y = (depth - 1) * 6.0f;
            params.scale = 1.0f - (depth - 1) * 0.04f;
            params.scale = ImClamp(params.scale, 0.85f, 1.0f);
            params.alpha = 1.0f - (depth - 1) * 0.15f;
            params.alpha = ImClamp(params.alpha, 0.6f, 1.0f);
            params.shadow = 0.3f - (depth - 1) * 0.08f;
            params.shadow = ImClamp(params.shadow, 0.1f, 0.3f);
            return params;
        }

        void UpdateToastPhysics(std::unique_ptr<ToastData>& toast, const StackParams& params, float delta_time) {
            ImVec2 target_pos = toast->target_pos;
            ImVec2 displacement = target_pos - toast->current_pos;
            float distance = ImLength(displacement);
            
            if (distance > 0.1f) {
                toast->physics.acceleration = displacement * 25.0f - toast->physics.velocity * 3.5f;
                toast->physics.velocity += toast->physics.acceleration * delta_time;
                float max_speed = 800.0f;
                float current_speed = ImLength(toast->physics.velocity);
                if (current_speed > max_speed) {
                    toast->physics.velocity = toast->physics.velocity * (max_speed / current_speed);
                }
                toast->current_pos += toast->physics.velocity * delta_time;
            } else {
                toast->physics.velocity = toast->physics.velocity * (1.0f - delta_time * 8.0f);
                toast->current_pos = target_pos;
            }
            toast->scale = CustomLerp(toast->scale, params.scale, delta_time * 8.0f, Easing::EaseOutQuad);
            toast->alpha = CustomLerp(toast->alpha, params.alpha, delta_time * 6.0f, Easing::EaseOutQuad);
            toast->shadow = CustomLerp(toast->shadow, params.shadow, delta_time * 5.0f, Easing::EaseOutQuad);
        }

        void Show(const std::string& title, const std::string& content, float duration = 5.0f, bool is_error = false) {
            auto toast = std::make_unique<ToastData>(title, content, duration, is_error);
            ImVec2 screen_size = ImGui::GetIO().DisplaySize;
            toast->start_pos = ImVec2(screen_size.x + toast_width, toast_margin);
            toast->current_pos = toast->start_pos;
            toast->target_pos = ImVec2(screen_size.x - toast_width - toast_margin - 35.0f, toast_margin);
            toasts.insert(toasts.begin(), std::move(toast));
        }

        void UpdateAndRender() {
            float delta_time = ImGui::GetIO().DeltaTime;
            ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
            ImVec2 screen_size = ImGui::GetIO().DisplaySize;
            int visible_count = 0;

            for (auto it = toasts.begin(); it != toasts.end();) {
                auto& toast = *it;
                if (toast->is_exiting) {
                    toast->exit_progress += delta_time / animation_duration;
                    if (toast->exit_progress >= 1.0f) {
                        it = toasts.erase(it);
                        continue;
                    }
                } else {
                    toast->animation_progress += delta_time / animation_duration;
                    if (toast->animation_progress >= 1.0f) {
                        toast->animation_progress = 1.0f;
                        toast->timer += delta_time;
                        if (toast->timer >= toast->duration) {
                            toast->is_exiting = true;
                            toast->exit_progress = 0.0f;
                            toast->target_pos = ImVec2(screen_size.x + toast_width, toast->current_pos.y);
                        }
                    }
                }
                
                if ((!toast->is_exiting && toast->animation_progress > 0.1f) || 
                    (toast->is_exiting && toast->exit_progress < 0.9f)) {
                    visible_count++;
                }
                ++it;
            }
            
            float current_y = toast_margin;
            int current_index = 0;
            for (auto& toast : toasts) {
                if ((toast->is_exiting && toast->exit_progress >= 0.9f) || 
                    (!toast->is_exiting && toast->animation_progress < 0.1f)) {
                    continue;
                }
                current_index++;
                StackParams params = CalculateStackParams(current_index, ImMin(visible_count, max_visible_toasts));
                if (!toast->is_exiting) {
                    toast->target_pos = ImVec2(screen_size.x - toast_width - toast_margin - 35.0f, current_y + params.offset_y);
                } else {
                    params.scale *= (1.0f - toast->exit_progress);
                    params.alpha *= (1.0f - toast->exit_progress);
                    params.shadow *= (1.0f - toast->exit_progress);
                }
                float toast_height_with_scale = toast->height * params.scale;
                UpdateToastPhysics(toast, params, delta_time);
                current_y += toast_height_with_scale + toast_spacing;
            }

            for (int i = toasts.size() - 1; i >= 0; --i) {
                auto& toast = toasts[i];
                float overall_progress;
                if (toast->is_exiting) {
                    overall_progress = 1.0f - toast->exit_progress;
                } else {
                    overall_progress = toast->animation_progress;
                }
                
                if (overall_progress < 0.01f) continue;

                float eased_progress;
                if (toast->is_exiting) {
                    eased_progress = CustomLerp(1.0f, 0.0f, toast->exit_progress, Easing::EaseOutBack);
                } else {
                    eased_progress = CustomLerp(0.0f, 1.0f, toast->animation_progress, Easing::EaseOutBack);
                }
                
                float final_scale = toast->scale * eased_progress;
                float final_alpha = eased_progress * toast->alpha;
                float scaled_width = toast_width * final_scale;
                float scaled_height = toast->height * final_scale;
                ImVec2 draw_pos = toast->current_pos;

                if (toast->shadow > 0.01f && final_alpha > 0.1f) {
                    ImVec4 shadow_color = ImVec4(0.0f, 0.0f, 0.0f, toast->shadow * final_alpha * 0.8f);
                    ImVec2 shadow_offset = ImVec2(2.0f, 4.0f);
                    draw_list->AddRectFilled(draw_pos + shadow_offset, draw_pos + ImVec2(scaled_width, scaled_height) + shadow_offset,ImGui::GetColorU32(shadow_color), 16.0f * final_scale);
                }

                if (final_alpha > 0.01f) {
                    ImVec4 bg_color = ImVec4(0.1f, 0.1f, 0.1f, 0.95f * final_alpha);
                    ImVec4 accent_color = toast->is_error ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(112/255.f, 109/255.f, 214/255.f, 1.f);
                    draw_list->AddRectFilled(draw_pos, draw_pos + ImVec2(scaled_width, scaled_height),ImGui::GetColorU32(bg_color), 16.0f * final_scale, ImDrawFlags_RoundCornersTop);
                    
                    ImVec4 title_color = toast->is_error ? accent_color : ImVec4(1.0f, 1.0f, 1.0f, final_alpha);
                    ImVec2 title_pos = draw_pos + ImVec2(12.0f * final_scale, 12.0f * final_scale);
                    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * final_scale,title_pos, ImGui::GetColorU32(title_color), toast->title.c_str());
                    
                    ImVec4 content_color = ImVec4(0.8f, 0.8f, 0.8f, final_alpha);
                    ImVec2 content_pos = draw_pos + ImVec2(12.0f * final_scale, 36.0f * final_scale);
                    draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.9f * final_scale,content_pos, ImGui::GetColorU32(content_color), toast->content.c_str());

                    if (!toast->is_exiting && toast->animation_progress >= 1.0f) {
                        float time_progress = 1.0f - (toast->timer / toast->duration);
                        ImVec2 progress_min = ImVec2(draw_pos.x, draw_pos.y + scaled_height - 3.0f * final_scale);
                        ImVec2 progress_max = ImVec2(draw_pos.x + scaled_width * time_progress, draw_pos.y + scaled_height);
                        draw_list->AddRectFilled(progress_min, progress_max, 
                                               ImGui::GetColorU32(ImVec4(accent_color.x, accent_color.y, accent_color.z, final_alpha)),
                                               8.0f * final_scale, ImDrawFlags_RoundCornersBottom);
                    }
                }
            }
        }

        void ClearAll() {
            for (auto& toast : toasts) {
                toast->is_exiting = true;
                toast->exit_progress = 0.0f;
                toast->target_pos = ImVec2(ImGui::GetIO().DisplaySize.x + toast_width, toast->current_pos.y);
            }
        }

        int GetVisibleCount() {
            int count = 0;
            for (auto& toast : toasts) {
                if ((!toast->is_exiting && toast->animation_progress > 0.1f) || 
                    (toast->is_exiting && toast->exit_progress < 0.9f)) {
                    count++;
                }
            }
            return count;
        }
    }
}