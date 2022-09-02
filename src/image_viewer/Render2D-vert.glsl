//****************************************************************************
// Copyright © 2021 Jan Erik Breimo. All rights reserved.
// Created by Jan Erik Breimo on 2021-10-09.
//
// This file is distributed under the BSD License.
// License text is included with the source distribution.
//****************************************************************************
#version 100

attribute vec2 a_position;
attribute vec2 a_texture_coord;

uniform mat3 u_transformation;
uniform highp float u_z;

varying highp vec2 v_texture_coord;

void main()
{
    vec3 p = u_transformation * vec3(a_position, 1.0);
    gl_Position = vec4(p.xy, u_z, 1.0);
    v_texture_coord = a_texture_coord;
}
