attribute vec3 in_Position;
attribute float in_PointSize;
attribute vec4 in_Color;

uniform float limitingMagnitude;
uniform float pointScale;

varying vec4 color;

const float baseSize = 1.4;
const float thresholdBrightness = 1.0 / 255.0;
const float saturationDiff = 4.5;

void main()
{
    float logMVisThreshold = log(thresholdBrightness) / log(2.512);
    float saturationMagnitude = limitingMagnitude - saturationDiff/* + logMVisThreshold */;
    float magScale = logMVisThreshold / saturationDiff;

    float magnitude = in_PointSize;

    float s = limitingMagnitude - magnitude;
#if 0
    if (magnitude >= saturationMagnitude)
        s = limitingMagnitude - magnitude;
    else
        s = limitingMagnitude - saturationMagnitude + sqrt(sqrt(saturationMagnitude - magnitude));
#endif
    color = in_Color;
    set_vp(vec4(in_Position, 1.0));
    gl_PointSize = pow(baseSize, s);
}
