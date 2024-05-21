@ctype vec3 hmm_vec3
@ctype mat4 hmm_mat4

@vs vs
in vec3 position;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(position.x, position.y, position.z, 1.0);
}
@end

@fs fs
out vec4 FragColor;

uniform fs_params {
    vec3 color; 
};

void main() {
    FragColor = vec4(color, 0.5f);
}
@end

@program simple vs fs
