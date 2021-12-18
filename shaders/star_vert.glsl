attribute vec3 in_Position;
attribute float in_PointSize;
attribute vec4 in_Color;

uniform vec4 viewport;
uniform float limitingMagnitude;
uniform float pointScale;

const float sigma2 = 0.35;
const float glareFalloff = 1.0 / 15.0;
const float glareBrightness = 0.003;
const float thresholdBrightness = 1.0 / 255.0;

varying vec2 pointCenter;
varying vec4 color;
varying float brightness;
varying float exposure;

void main()
{
    // position
    set_vp(vec4(in_Position, 1.0));
    // Perspective projection to get the star position in normalized device coordinates
    vec2 devicePosition = gl_Position.xy / gl_Position.w;
    // Convert to viewport coordinates (the same coordinate system as the gl_FragCoord register)
    pointCenter = (devicePosition * 0.5 + vec2(0.5, 0.5)) * viewport.zw + viewport.xy;

    // color
    color = in_Color;

    // brightness and exposure
    float logMVisThreshold = log(thresholdBrightness) / log(2.512);
#if 1
    float saturationMagnitude = limitingMagnitude - 4.5/* + logMVisThreshold*/;
#else
    float t = max(0.0, limitingMagnitude - 8.0) / 7.0;
    float saturationMagnitude = limitingMagnitude - 4.5 - pow(t, 1.1)*2.0;
#endif
    float magScale = logMVisThreshold / (saturationMagnitude - limitingMagnitude);
    float e = pow(2.512, magScale * saturationMagnitude);
    exposure = e;

    // limit size
    float PS = 256.0 * 256.0 * 4.0;
    float X = (PS * glareFalloff + 1.0f) * thresholdBrightness / glareBrightness;
    float GMagnitude = saturationMagnitude - log(X) / log(2.512f) / magScale;

    float appMag = max(in_PointSize, GMagnitude);

    float b = pow(2.512, -appMag * magScale);
    brightness = b;

    // point size
    float r2 = -log(thresholdBrightness / (e * b)) * 2.0 * sigma2;
    float rGlare2 = (e * glareBrightness * b / thresholdBrightness - 1.0) / glareFalloff;
    gl_PointSize = 2.0 * pointScale * sqrt(max(r2, rGlare2));
}
