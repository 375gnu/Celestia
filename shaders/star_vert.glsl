attribute vec4 in_Position;
attribute vec4 in_Color;
attribute float in_PointSize;

//uniform float sizeFactor;
uniform float limitingMagnitude;
uniform float pointScale;

varying vec4 color;

const float sizeFactor = 1.4;

void main(void)
{
    float satMag = limitingMagnitude - 4.5;
    gl_PointSize = pointScale * 4.0 * max(1.0, pow(sizeFactor, satMag - in_PointSize));
    float alpha = min(1.0, 1.0 / max(1.0, (in_PointSize - satMag)));
    color = vec4(in_Color.rgb, alpha);
    set_vp(in_Position);
}
