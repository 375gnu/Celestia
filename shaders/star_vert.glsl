attribute vec3 in_Position;
attribute float in_PointSize;
attribute vec4 in_Color;

uniform float limitingMagnitude;
uniform float pointScale;

varying vec4 color;

const float baseSize = 1.4;
const float saturationDiff = 6.5;

void main()
{
    float saturationMagnitude = limitingMagnitude - saturationDiff;

    float magnitude = in_PointSize;

    float m = min(magnitude, saturationMagnitude);
    float s = limitingMagnitude - m;
    s += sqrt(max(0.0, saturationMagnitude - magnitude));

    color = in_Color;
    set_vp(vec4(in_Position, 1.0));
    gl_PointSize = pointScale * pow(baseSize, s);
}
