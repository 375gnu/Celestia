varying vec2 texCoord;

uniform sampler2D tex;

const float gamma = 2.2;

void main(void)
{
    vec3 color = texture2D(tex, texCoord).rgb;
    gl_FragColor = vec4(pow(color, vec3(1.0 / gamma)), 1.0);
}
