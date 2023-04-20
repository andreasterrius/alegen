#version 330 core

in vec2 texCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D tex;

void main()
{

    ivec2 textureSize = textureSize(tex, 0);
    if (textureSize.x == 1 && textureSize.y == 1)
    {
        finalColor = fragColor;
    }
    else
    {
        // finalColor = vec4(fragColor.x, fragColor.y, fragColor.z, 1.0);
         finalColor = vec4(vec3(fragColor.x, fragColor.y, fragColor.z), texture(tex, texCoord).r);
        // finalColor = fragColor;
    }
}
