uniform vec3 color;
varying float shade;

void main(void)
{
    vec2 v = (gl_PointCoord - vec2(0.5)) * 2.0;
    float alpha = shade * (1.0 - min(dot(v, v), 1.0));
    gl_FragColor = vec4(color, alpha);
}
