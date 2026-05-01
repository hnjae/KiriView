// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#version 440

layout(location = 0) in vec2 texCoord;

layout(std140, binding = 0) uniform buf
{
    mat4 matrix;
    vec4 targetRect;
    vec4 opacity;
} ubuf;

layout(binding = 1) uniform sampler2D imageTexture;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(imageTexture, texCoord) * ubuf.opacity.x;
}
