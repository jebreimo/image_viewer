//****************************************************************************
// Copyright © 2022 Jan Erik Breimo. All rights reserved.
// Created by Jan Erik Breimo on 2022-08-18.
//
// This file is distributed under the BSD License.
// License text is included with the source distribution.
//****************************************************************************
#include <iostream>
#include <Argos/Argos.hpp>
#include <Tungsten/SdlApplication.hpp>
#include <Yimage/Yimage.hpp>
#include "Render2DShaderProgram.hpp"

namespace
{
    std::pair<int, int> get_ogl_pixel_type(yimage::PixelType type)
    {
        switch (type)
        {
        case yimage::PixelType::MONO_8:
            return {GL_RED, GL_UNSIGNED_BYTE};
        case yimage::PixelType::MONO_ALPHA_8:
            return {GL_RG, GL_UNSIGNED_BYTE};
        case yimage::PixelType::RGB_8:
            return {GL_RGB, GL_UNSIGNED_BYTE};
        case yimage::PixelType::RGBA_8:
            return {GL_RGBA, GL_UNSIGNED_BYTE};
        case yimage::PixelType::MONO_1:
        case yimage::PixelType::MONO_2:
        case yimage::PixelType::MONO_4:
        case yimage::PixelType::MONO_16:
        case yimage::PixelType::ALPHA_MONO_8:
        case yimage::PixelType::ALPHA_MONO_16:
        case yimage::PixelType::MONO_ALPHA_16:
        case yimage::PixelType::RGB_16:
        case yimage::PixelType::ARGB_8:
        case yimage::PixelType::ARGB_16:
        case yimage::PixelType::RGBA_16:
        default:
            break;
        }
        throw std::runtime_error("GLES has no corresponding pixel format: "
                                 + std::to_string(int(type)));
    }
}

class ImageViewer : public Tungsten::EventLoop
{
public:
    [[nodiscard]]
    const std::string& image_path() const
    {
        return m_image_path;
    }

    void set_image_path(const std::string& image_path)
    {
        m_image_path = image_path;
    }

    void on_startup(Tungsten::SdlApplication& app) final
    {
        float square[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                          1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
                          1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                          -1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        short indexes[] = {0, 2, 3, 0, 1, 2};

        m_vertex_array = Tungsten::generate_vertex_array();
        Tungsten::bind_vertex_array(m_vertex_array);

        m_buffers = Tungsten::generate_buffers(2);
        Tungsten::bind_buffer(GL_ARRAY_BUFFER, m_buffers[0]);
        Tungsten::set_buffer_data(GL_ARRAY_BUFFER, sizeof(square),
                                  square, GL_STATIC_DRAW);
        Tungsten::bind_buffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[1]);
        Tungsten::set_buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes),
                                  indexes, GL_STATIC_DRAW);

        m_texture = Tungsten::generate_texture();
        Tungsten::bind_texture(GL_TEXTURE_2D, m_texture);

        auto image = yimage::read_image(m_image_path);
        auto [format, type] = get_ogl_pixel_type(image.pixel_type());
        //glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        Tungsten::set_texture_image_2d(GL_TEXTURE_2D, 0, GL_RGBA,
                                       int(image.width()), int(image.height()),
                                       format, type,
                                       image.data());

        Tungsten::set_texture_min_filter(GL_TEXTURE_2D, GL_LINEAR);
        Tungsten::set_texture_mag_filter(GL_TEXTURE_2D, GL_LINEAR);
        Tungsten::set_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Tungsten::set_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        m_program.setup();
        Tungsten::define_vertex_attribute_pointer(
            m_program.position, 3, GL_FLOAT, false, 5 * sizeof(float), 0);
        Tungsten::enable_vertex_attribute(m_program.position);
        Tungsten::define_vertex_attribute_pointer(
            m_program.textureCoord, 2, GL_FLOAT, true, 5 * sizeof(float),
            3 * sizeof(float));
        Tungsten::enable_vertex_attribute(m_program.textureCoord);
    }

    bool on_event(Tungsten::SdlApplication& app, const SDL_Event& event) override
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            return true;
        default:
            return false;
        }
    }

    void on_draw(Tungsten::SdlApplication& app) final
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto [w, h] = app.window_size();
        m_program.transformation.set(Xyz::scale3(
            std::min(float(h) / float(w), 1.0f),
            std::min(float(w) / float(h), 1.0f)));
        Tungsten::draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT);
    }
private:
    std::string m_image_path;
    std::vector<Tungsten::BufferHandle> m_buffers;
    Tungsten::VertexArrayHandle m_vertex_array;
    Tungsten::TextureHandle m_texture;
    Render2DShaderProgram m_program;
};

int main(int argc, char* argv[])
{
    try
    {
        argos::ArgumentParser parser(argv[0]);
        parser.add(argos::Argument("IMAGE")
                       .help("An image file (PNG or JPEG) that the program will display."));
        Tungsten::SdlApplication::add_command_line_options(parser);
        auto args = parser.parse(argc, argv);
        auto event_loop = std::make_unique<ImageViewer>();
        event_loop->set_image_path(args.value("IMAGE").as_string());
        Tungsten::SdlApplication app("ShowPng", std::move(event_loop));
        app.read_command_line_options(args);
        app.run();
    }
    catch (Tungsten::TungstenException& ex)
    {
        std::cout << ex.what() << "\n";
        return 1;
    }
    return 0;
}
