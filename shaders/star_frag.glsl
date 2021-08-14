varying vec4 color;
varying vec2 pointCenter;
varying float brightness;
varying float exposure;

const float sigma2 = 0.35;
const float glareFalloff = 1.0 / 15.0;
const float glareBrightness = 0.003;
const float diffSpikeBrightness = 0.9;

vec3 linearToSRGB(vec3 c)
{
    vec3 linear = 12.92 * c;
    vec3 nonlinear = (1.0 + 0.055) * pow(c, vec3(1.0 / 2.4)) - vec3(0.055);
    return mix(linear, nonlinear, step(vec3(0.0031308), c));
}

void main(void)
{
    // gl_FragCoord contains the position of the pixel center in viewport
    // coordinates. Compute the distance to the center of the star.
    vec2 offset = gl_FragCoord.xy - pointCenter;
    float r2 = dot(offset, offset);
    float b = exp(-r2 / (2.0 * sigma2));
#if 0
    b += glareBrightness / (glareFalloff * pow(r2, 1.5) + 1.0) * 0.5;
#else
    float spikes = (max(0.0, 1.0 - abs(offset.x + offset.y)) + max(0.0, 1.0 - abs(offset.x - offset.y))) * diffSpikeBrightness;
    b += glareBrightness / (glareFalloff * pow(r2, 1.5) + 1.0) * (spikes + 0.5);
#endif
    gl_FragColor = vec4(linearToSRGB(b * exposure * color.rgb * brightness), 1.0);
}
