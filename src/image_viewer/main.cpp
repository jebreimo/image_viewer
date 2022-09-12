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

    struct View
    {
        Xyz::Vector2F center;
        int scale = 0;
    };
}

class ImageViewer : public Tungsten::EventLoop
{
public:
    void set_image(yimage::Image image)
    {
        img_ = std::move(image);
    }

    void set_view(const View& view)
    {
        view_ = view;
    }

    void on_startup(Tungsten::SdlApplication& app) final
    {
        if (img_.size() == 0)
            throw std::runtime_error("The image is empty!");

        img_asp_ratio_ = double(img_.width()) / double(img_.height());
        float square[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                          1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
                          1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                          -1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        short indexes[] = {0, 2, 3, 0, 1, 2};

        vertex_array_ = Tungsten::generate_vertex_array();
        Tungsten::bind_vertex_array(vertex_array_);

        buffers_ = Tungsten::generate_buffers(2);
        Tungsten::bind_buffer(GL_ARRAY_BUFFER, buffers_[0]);
        Tungsten::set_buffer_data(GL_ARRAY_BUFFER, sizeof(square),
                                  square, GL_STATIC_DRAW);
        Tungsten::bind_buffer(GL_ELEMENT_ARRAY_BUFFER, buffers_[1]);
        Tungsten::set_buffer_data(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes),
                                  indexes, GL_STATIC_DRAW);

        texture_ = Tungsten::generate_texture();
        Tungsten::bind_texture(GL_TEXTURE_2D, texture_);

        auto [format, type] = get_ogl_pixel_type(img_.pixel_type());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        Tungsten::set_texture_image_2d(GL_TEXTURE_2D, 0, GL_RGBA,
                                       int(img_.width()), int(img_.height()),
                                       format, type,
                                       img_.data());
        img_ = {};

        Tungsten::set_texture_min_filter(GL_TEXTURE_2D, GL_LINEAR);
        Tungsten::set_texture_mag_filter(GL_TEXTURE_2D, GL_LINEAR);
        Tungsten::set_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Tungsten::set_texture_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        program_.setup();
        Tungsten::define_vertex_attribute_pointer(
            program_.position, 3, GL_FLOAT, false, 5 * sizeof(float), 0);
        Tungsten::enable_vertex_attribute(program_.position);
        Tungsten::define_vertex_attribute_pointer(
            program_.textureCoord, 2, GL_FLOAT, true, 5 * sizeof(float),
            3 * sizeof(float));
        Tungsten::enable_vertex_attribute(program_.textureCoord);
    }

    bool on_event(Tungsten::SdlApplication& app, const SDL_Event& event) override
    {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
            return on_mouse_wheel(app, event);
        case SDL_MOUSEMOTION:
            return on_mouse_motion(app, event);
        case SDL_MOUSEBUTTONDOWN:
            return on_mouse_button_down(app, event);
        case SDL_MOUSEBUTTONUP:
            return on_mouse_button_up(app, event);
        default:
            return false;
        }
    }

    void on_draw(Tungsten::SdlApplication& app) final
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        program_.transformation.set(get_transformation(app));
        Tungsten::draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT);
    }
private:
    bool on_mouse_wheel(const Tungsten::SdlApplication& app,
                        const SDL_Event& event)
    {
        auto scale = compute_scale(app);
        auto offset = Xyz::divide(mouse_pos_, scale);
        auto pos = view_.center + offset;
        if (event.wheel.y > 0)
            ++view_.scale;
        else
            --view_.scale;
        scale = compute_scale(app);
        view_.center = pos - divide(mouse_pos_, scale);

        if (mouse_move_)
        {
            mouse_click_pos_ = mouse_pos_;
            mouse_down_center_ = view_.center;
        }
        return true;
    }

    bool on_mouse_motion(const Tungsten::SdlApplication& app,
                         const SDL_Event& event)
    {
        auto [w, h] = app.window_size();
        mouse_pos_ = {float(2 * event.motion.x) / float(w) - 1,
                      float(2 * (h - event.motion.y)) / float(h) - 1};

        if (mouse_move_)
        {
            auto delta = divide(mouse_click_pos_ - mouse_pos_,
                                  compute_scale(app))
                         + mouse_down_center_;
            view_.center = delta;
        }

        return true;
    }

    bool on_mouse_button_down(const Tungsten::SdlApplication& app,
                              const SDL_Event& event)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            mouse_move_ = true;
            mouse_click_pos_ = mouse_pos_;
            mouse_down_center_ = view_.center;
        }
        return true;
    }

    bool on_mouse_button_up(const Tungsten::SdlApplication& app,
                            const SDL_Event& event)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
            mouse_move_ = false;
        return true;
    }

    [[nodiscard]]
    Xyz::Vector2F compute_scale(const Tungsten::SdlApplication& app) const
    {
        auto [w, h] = app.window_size();
        auto win_asp_ratio = double(w) / double(h);
        Xyz::Vector2F scale_vec;
        if (img_asp_ratio_ > win_asp_ratio)
            scale_vec = {1.0f, float(win_asp_ratio / img_asp_ratio_)};
        else
            scale_vec = {float(img_asp_ratio_) / float(win_asp_ratio), 1.0f};
        return pow(1.25f, float(view_.scale)) * scale_vec;
    }

    [[nodiscard]]
    Xyz::Matrix3F get_transformation(const Tungsten::SdlApplication& app) const
    {
        return Xyz::scale3(compute_scale(app))
               * Xyz::translate3(-view_.center);
    }

    yimage::Image img_;
    double img_asp_ratio_ = {};
    std::vector<Tungsten::BufferHandle> buffers_;
    Tungsten::VertexArrayHandle vertex_array_;
    Tungsten::TextureHandle texture_;
    Render2DShaderProgram program_;
    View view_;
    Xyz::Vector2F mouse_down_center_;
    Xyz::Vector2F mouse_pos_;
    Xyz::Vector2F mouse_click_pos_;
    bool mouse_move_ = {};
};

int main(int argc, char* argv[])
{
    try
    {
        argos::ArgumentParser parser(argv[0]);
        parser.add(argos::Argument("IMAGE")
                       .help("An image file (PNG or JPEG)."));
        Tungsten::SdlApplication::add_command_line_options(parser);
        auto args = parser.parse(argc, argv);
        auto event_loop = std::make_unique<ImageViewer>();
        event_loop->set_image(yimage::read_image(args.value("IMAGE").as_string()));
        Tungsten::SdlApplication app("ShowPng", std::move(event_loop));
        app.read_command_line_options(args);
        app.run();
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << "\n";
        return 1;
    }
    return 0;
}
