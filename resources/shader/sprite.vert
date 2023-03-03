#version 330 core
layout(location = 0) in vec2 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec4 color;

out vec4 fragColor;
out vec2 texCoord;

void main(){
    fragColor = color;
    texCoord = position.xy;
    gl_Position = projection * view * model * vec4(position.x, position.y, 0.0, 1.0);
}

