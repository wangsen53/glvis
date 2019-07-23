// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443271. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the GLVis visualization tool and library. For more
// information and source code availability see http://glvis.org.
//
// GLVis is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#include <vector>

#include "font.hpp"
#include "aux_vis.hpp"

struct vert_tex2d {
    float x, y;
    float u, v;

    vert_tex2d(float x, float y, float u, float v)
        : x(x), y(y), u(u), v(v) { }
};


const int font_scale = 72;
bool GlVisFont::LoadFont(const std::string& path, int font_size) {
    if (!init) {
        return false;
    }
    if (font_init) {
        glDeleteTextures(1, &font_tex);
        FT_Done_Face(face);
        font_init = false;
    }
    if (FT_New_Face(library, path.c_str(), 0, &face)) {
        cout << "GlVisFont::LoadFont(): Cannot open font file: " << path << endl;
        return false;
    }
    int ppi_w, ppi_h;
    GetAppWindow()->getDpi(ppi_w, ppi_h);
    if (FT_Set_Char_Size(face, 0, font_size*font_scale, ppi_w, ppi_h)) {
        cout << "GlVisFont::LoadFont(): Cannot set font height: " << font_size << " pts"
             << endl;
        FT_Done_Face(face);
        return false;
    }

    //generate atlas
    int w = 0, h = 0;
    for (int c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            cout << "GlVisFont::LoadFont(): Cannot load glyph: " << (char) c << endl;
            continue;
        }
        w += face->glyph->bitmap.width + 2;
        if (h < face->glyph->bitmap.rows) {
            h = face->glyph->bitmap.rows;
        }
    }
    tex_w = w;
    tex_h = h + 2;

    glGenTextures(1, &font_tex);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    std::vector<uint8_t> zeros(tex_w * tex_h, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, alpha_channel, tex_w, tex_h, 0, alpha_channel, GL_UNSIGNED_BYTE, zeros.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int x = 0;
    for (int c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            cout << "GlVisFont::LoadFont(): Cannot load glyph: " << (char) c << endl;
            continue;
        }
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        x + 1, 1,
                        face->glyph->bitmap.width,
                        face->glyph->bitmap.rows,
                        alpha_channel,
                        GL_UNSIGNED_BYTE,
                        face->glyph->bitmap.buffer);
        font_chars[c] = {
            (unsigned)(face->glyph->bitmap.width + 2),
            (unsigned)(face->glyph->bitmap.rows + 2),
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,
            (int)(face->glyph->advance.x >> 6),
            (int)(face->glyph->advance.y >> 6),
            (float) x / w
        };
        x += face->glyph->bitmap.width + 2;
    }
    font_init = true;
    //glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    return true;
}

void GlVisFont::getObjectSize(const std::string& text, int& w, int& h) {
    float x = 0.f;
    w = 0.f, h = 0.f;
    for (char c : text) {
        glyph g = GetTexChar(c);
        float cur_x = x + g.bear_x;
        x += g.adv_x;
        if (!g.w || !g.h) {
            continue;
        }
        w = (int)(cur_x + g.w);
        h = std::max(h, (int)g.h);
    }
}
