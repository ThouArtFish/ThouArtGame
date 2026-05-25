#version 460 core

struct PointLight {
    vec4 a, b;
};
struct RayLight {
    vec4 a;
    vec2 b;
};
struct FlashLight {
    vec4 a, b, c;
};
struct Scene {
    float ambience;
};
layout(std430, binding = 0) buffer readonly ssbo_0 {
    int point_lights_size;
    PointLight point_lights[];
};
layout(std430, binding = 1) buffer readonly ssbo_1 {
    int ray_lights_size;
    RayLight ray_lights[];
};
layout(std430, binding = 2) buffer readonly ssbo_2 {
    int flash_lights_size;
    FlashLight flash_lights[];
};
layout(std430, binding = 3) buffer readonly ssbo_3 {
    Scene scene;
};
out vec4 FragColour;
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
uniform vec3 camera_pos;
uniform vec3 colour;
uniform float spec_fac;
uniform float spec_exp;
uniform float opacity;
uniform int spec_tex_num;
uniform int diff_tex_num;
uniform sampler2D diff_texs[16];
uniform sampler2D spec_texs[16];
void main() {
    int i;
    vec3 final_shade = vec3(scene.ambience);
    vec4 obj_base = (diff_tex_num == 0 ? vec4(colour, 1.0f) : vec4(0.0f));
    for (i = 0; i < diff_tex_num; i++) {
        if (i == 0) 
	        obj_base = texture(diff_texs[i], TexCoords);
        else
            obj_base = mix(obj_base, texture(diff_texs[i], TexCoords), 0.5f);
    }
    vec3 spec_frag = vec3(spec_fac);
    if (spec_tex_num > 0)
        for (i = 0; i < spec_tex_num; i++) {
            if (i == 0) 
	            spec_frag *= texture(spec_texs[i], TexCoords).xyz;
            else
                spec_frag = mix(spec_frag, texture(spec_texs[i], TexCoords).xyz, 0.5f);
        }
    for (i = 0; i < point_lights_size; i++) {
    	vec3 light_dir = point_lights[i].a.xyz - FragPos;
	    float dist = length(light_dir);
        float atten = 1.0f / (1.0f + point_lights[i].a.w * dist + point_lights[i].b.w * dist * dist);
	    light_dir /= dist;
        final_shade += (point_lights[i].b.xyz * max(0, dot(Normal, light_dir)) + point_lights[i].b.xyz * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + light_dir)), 0.0), spec_exp)) * atten;
    }
    for (i = 0; i < ray_lights_size; i++) {
        vec3 colour = vec3(ray_lights[i].a.z, ray_lights[i].b);
        final_shade += colour * max(0, dot(Normal, ray_lights[i].a.xyz)) + colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + ray_lights[i].a.xyz)), 0.0), spec_exp);
    }
    for (i = 0; i < flash_lights_size; i++) {
        vec3 light_dir = flash_lights[i].a.xyz - FragPos;
        vec3 view_dir = normalize(camera_pos - FragPos);
	    float dist = length(light_dir);
        float atten = 1.0f / (1.0f + flash_lights[i].a.w * dist + flash_lights[i].b.w * dist * dist);
        light_dir /= dist;
        vec3 colour = flash_lights[i].c.xyz * min(1, max(0, (dot(-light_dir, flash_lights[i].b.xyz) - flash_lights[i].c.w) / (flash_lights[i].c.w - cos(acos(flash_lights[i].c.w) + 0.1396))));
        final_shade += (colour * max(0, dot(Normal, flash_lights[i].b.xyz)) + colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + flash_lights[i].b.xyz)), 0.0), spec_exp)) * atten;
    }
    FragColour = vec4(obj_base.xyz * final_shade, obj_base.z * opacity);
}