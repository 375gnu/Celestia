#version 120

uniform vec2 viewportSize;
uniform vec2 viewportCoord;

varying vec2 pointCenter;
varying vec4 color;
varying float brightness;

//uniform float Llim;
//uniform float Lsat;
uniform float magScale;

//uniform float sigma2;
const float sigma2 = 0.35f;
//uniform float glareFalloff;
const float glareFalloff = 1.0f / 15.0f;
//uniform float glareBrightness;
const float glareBrightness = 0.003f;

uniform float exposure;
uniform float thresholdBrightness;

void main()
{
    vec4 position = gl_Vertex;
    float appMag = position.z;
    position.z = sqrt(1.0 - dot(position.xy, position.xy)) * sign(gl_Color.a - 0.5);
    vec4 projectedPosition = gl_ModelViewProjectionMatrix * position;
    vec2 devicePosition = projectedPosition.xy / projectedPosition.w;
    pointCenter = (devicePosition * 0.5 + vec2(0.5, 0.5)) * viewportSize + viewportCoord;
    color = gl_Color;
    float b = pow(2.512, -appMag * magScale);
    float r2 = -log(thresholdBrightness / (exposure * b)) * 2.0 * sigma2;
    float rGlare2 = (exposure * glareBrightness * b / thresholdBrightness - 1.0) / glareFalloff;
    gl_PointSize = 2.0 * sqrt(max(r2, max(0.25, rGlare2)));

    brightness = b;
    gl_Position = projectedPosition;
}
