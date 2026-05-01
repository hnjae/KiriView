// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#version 440

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(std140, binding = 0) uniform buf
{
    mat4 matrix;
    vec4 targetRect;
    vec4 opacity;
} ubuf;

layout(location = 0) out vec2 texCoord;

void main()
{
    vec2 position = ubuf.targetRect.xy + inPosition * ubuf.targetRect.zw;
    texCoord = inTexCoord;
    gl_Position = ubuf.matrix * vec4(position, 0.0, 1.0);
}
