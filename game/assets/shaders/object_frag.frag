#version 460 core

struct Light {
    vec4 a;
    vec4 b;
};

layout(std430, binding = 0) buffer readonly scene_lights {
    int lights_size;
    Light lights[];
};

out vec4 FragColour;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform float ambience = 0.0f;
uniform float spec_mod;
uniform float spec_exp;
uniform float opacity;
uniform bool spec_map;
uniform sampler2D diffuse1;
uniform sampler2D specular1;

void main() {   
    vec4 tex_frag = texture(diffuse1, TexCoords);
    if (tex_frag.a < 0.1f)
        discard;
    vec3 frag_base = tex_frag.rgb;
    vec3 spec_frag = (spec_map ? texture(specular1, TexCoords).xyz * spec_mod : vec3(spec_mod));
    vec3 final_shade = vec3(ambience);
    for (int i = 0; i < lights_size; i++) {
        vec3 frag_to_light = (lights[i].b.w < 0.0f ? lights[i].a.xyz : lights[i].a.xyz - FragPos);
        float size = length(frag_to_light);
        frag_to_light = frag_to_light / size;
        float dir_dot = dot(Normal, frag_to_light);
        vec3 light_col;
        if (lights[i].b.w > 0)
		light_col = vec3(1) * min(1, max(0, (dot(-frag_to_light, lights[i].b.xyz) - lights[i].b.w) / (lights[i].b.w - cos(acos(lights[i].b.w) + 0.1396))));
        else
            light_col = lights[i].b.xyz;
        float atten = (lights[i].b.w < 0.0f ? 1.0f : 1.0f / (1.0f + lights[i].a.w * size * size));
        final_shade += light_col * atten * max(dir_dot, 0.0f);
        if (spec_mod > 0.0f)
            final_shade += light_col * spec_frag * atten * pow(max(dot(frag_to_light, reflect(-frag_to_light, Normal)), 0), spec_exp);
    }
    FragColour = vec4(frag_base * final_shade, opacity * tex_frag.a);
}