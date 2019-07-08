#version 120

attribute float magnitude;

uniform vec3 viewportSize;
uniform float limitingMagnitude;
uniform float saturationMagnitude;
//uniform float glareBrightness;
//uniform float sigma2;
//uniform float glareSigma2;
//uniform float exposure;

varying vec2 pointCenter;
varying vec4 starColor;
varying float brightness;

const float glareBrightness = 0.003f;
const float sigma2 = 0.35f;
const float glareSigma2 = 0.015f; // fixme

const float glareFalloff = 1.0f / 15.0f;

const float exposure = 1.0f;

const float thresholdBrightness = 1.0 / 512.0;

void main()
{
    vec4 projectedPosition = ftransform();

    // Perspective projection to get the star position in normalized device coordinates
    vec2 ndcPosition = projectedPosition.xy / projectedPosition.w;

    float b = pow(2.512, (limitingMagnitude - saturationMagnitude) * (saturationMagnitude - magnitude));

    // Calculate the minimum size of a point large enough to contain both the glare function and PSF; these
    // functions are both infinite in extent, but we can clip them when they fall off to an indiscernable level
    float r2  = -log(thresholdBrightness / (exposure * b)) * 2.0 * sigma2;
//    float rG2 = -log(thresholdBrightness / (glareBrightness * b)) * 2.0 * glareSigma2;
   float rG2 = (exposure * glareBrightness * b / thresholdBrightness - 1.0) / glareFalloff;

    // Convert to viewport coordinates (the same coordinate system as the gl_FragCoord register)
    pointCenter  = (ndcPosition + 1.0) * 0.5 * viewportSize.xy;
    starColor    = gl_Color;
    brightness   = b;
    gl_PointSize = 2.0 * sqrt(max(r2, rG2));
    gl_Position  = projectedPosition;
}
