
float gridSize = 100.0;

float gridCellSize = 0.025;

vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);
vec4 gridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

// switch the LOD when the number of pixels between 2 adjacent cell lines is less than this:
const float gridMinPixelsBetweenCells = 2.0;

float log10(float x) {
    return log(x) / log(10.0);
}

float clampf(float x) {
    return clamp(x, 0.0, 1.0);
}

float clampv(vec2 x) {
    return clamp(x, vec2(0.0), vec2(1.0));
}

float maxv(vec2 v) {
    return max(v.x, v.y);
}
