#version 120

varying vec2 pointCenter;
varying vec4 color;
varying float brightness;

//uniform float sigma2;
const float sigma2 = 0.35f;
//uniform float glareFalloff;
const float glareFalloff = 1.0f / 15.0f;
//uniform float glareBrightness;
const float glareBrightness = 0.003f;

uniform float diffSpikeBrightness;
uniform float exposure;


vec3 linearToSRGB(vec3 c)
{
    return c;
}

void main()
{
    vec2 offset = gl_FragCoord.xy - pointCenter;
    float r2 = dot(offset, offset);
    float b = exp(-r2 / (2.0 * sigma2));
    float spikes = (max(0.0, 1.0 - abs(offset.x + offset.y)) + max(0.0, 1.0 - abs(offset.x - offset.y))) * diffSpikeBrightness;
    b += glareBrightness / (glareFalloff * pow(r2, 1.5) + 1.0) * (spikes + 0.5);
    gl_FragColor = vec4(linearToSRGB(b * exposure * color.rgb * brightness), 1.0);
}
