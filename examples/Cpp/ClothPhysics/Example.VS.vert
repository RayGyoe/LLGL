// GLSL Vertex Shader "VS"
// Generated by XShaderCompiler
// 11/10/2019 20:18:28

#version 140

in vec4 position;
in vec4 normal;
in vec2 texCoord;

out vec4 xsv_NORMAL0;
out vec2 xsv_TEXCOORD0;

layout(std140, row_major) uniform SceneState
{
    mat4  wvpMatrix;
    mat4  wMatrix;
    vec4  gravity;
    uvec2 gridSize;
    uvec2 _pad0;
    float damping;
    float dTime;
    float dStiffness;
    float _pad1;
    vec4  lightVec;
};

void main()
{
    gl_Position = (position * wvpMatrix);
    xsv_NORMAL0 = (normal * wMatrix);
    xsv_TEXCOORD0 = texCoord;
}

