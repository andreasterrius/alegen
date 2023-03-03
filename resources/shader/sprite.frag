#version 330 core

in vec2 texCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D tex;

void main()
{
    if (useTexture && tex != 0)
    {
        finalColor = texture(tex, texCoord);
    }
    else
    {
        finalColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

    finalColor = fragColor;
}
