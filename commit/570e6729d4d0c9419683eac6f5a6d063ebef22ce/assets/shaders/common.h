
#define MK_UNIFORM_ARRAY_SIZE 16
#ifndef CPU_INCLUDE
uniform vec4 mk_uniform_array[MK_UNIFORM_ARRAY_SIZE];

vec2 mk_screen_size(){return mk_uniform_array[0].xy;}
float mk_time(){return mk_uniform_array[0].z;}
float mk_frame(){return mk_uniform_array[0].w;}
vec4 mk_audio_volume(){return mk_uniform_array[1];}
#endif 