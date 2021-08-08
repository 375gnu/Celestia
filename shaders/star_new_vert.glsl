#ifndef GL_ES
#define highp
#define mediump
#define lowp
#endif

attribute vec3 in_Position;
attribute float in_PointSize;
attribute vec4 in_Color;

uniform vec2 viewportSize;
uniform vec2 viewportCoord;
uniform float limitingMagnitude;

const float sigma2 = 0.35;
const float glareFalloff = 1.0 / 15.0;
const float glareBrightness = 0.003;

varying vec2 pointCenter;
varying lowp vec4 color;
varying float brightness;
varying float exposure;

void main()
{
    const float thresholdBrightness = 1.0 / 255.0;
    float logMVisThreshold = log(thresholdBrightness) / log(2.512);
    float saturationMagnitude = limitingMagnitude - 4.5/* + logMVisThreshold */;
    float t = max(0.0, limitingMagnitude - 8.0) / 7.0;
    ///float saturationMagnitude = limitingMagnitude - 4.5 - pow(t, 1.1)*2.0;
    float magScale = (logMVisThreshold) / (saturationMagnitude - limitingMagnitude);

    float e = pow(2.512, magScale * saturationMagnitude);

    ///float magnitude = max(in_PointSize, saturationMagnitude);
    float magnitude = in_PointSize;

    float b = pow(2.512, -magnitude * magScale);

    ///float b = pow(2.512, (limitingMagnitude - saturationMagnitude) * (saturationMagnitude - magnitude));
/*
    // Calculate the minimum size of a point large enough to contain both the
    // glare function and PSF; these functions are both infinite in extent, but
    // we can clip them when they fall off to an indiscernable level
    float r2 = -log(thresholdBrightness / (e * b)) * 2.0 * sigma2;
    ///float r2 = -log(thresholdBrightness / brightness) * 2.0 * sigma2;
    float rGlare2 = (e * glareBrightness * b / thresholdBrightness - 1.0) / glareFalloff;
    ///float rGlare2 = -log(thresholdBrightness / (glareBrightness * b)) * 2.0 * glareFalloff;
    gl_PointSize = 2.0 * sqrt(max(r2, rGlare2));
*/

    float s;
    float baseSize = 1.4;
    if (magnitude >= saturationMagnitude)
        s = pow(baseSize, limitingMagnitude - magnitude);
    else
        s = pow(baseSize, limitingMagnitude - saturationMagnitude + sqrt(sqrt(saturationMagnitude - magnitude)));
    gl_PointSize = s;

    vec4 position = vec4(in_Position, 1.0);
    vec4 projectedPosition = MVPMatrix * position;
    // Perspective projection to get the star position in normalized device coordinates
    vec2 devicePosition = projectedPosition.xy / projectedPosition.w;
    // Convert to viewport coordinates (the same coordinate system as the gl_FragCoord register)
    pointCenter = (devicePosition * 0.5 + vec2(0.5, 0.5)) * viewportSize + viewportCoord;

    color = in_Color;

    brightness = b;
    exposure = e;
    gl_Position = projectedPosition;
}
