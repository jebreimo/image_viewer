//****************************************************************************
// Copyright © 2021 Jan Erik Breimo. All rights reserved.
// Created by Jan Erik Breimo on 2021-10-09.
//
// This file is distributed under the BSD License.
// License text is included with the source distribution.
//****************************************************************************
#version 100

varying highp vec2 v_texture_coord;

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = texture2D(u_texture, v_texture_coord);
}
