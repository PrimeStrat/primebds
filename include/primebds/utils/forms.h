/// @file forms.h
/// Form wrapper utilities for Bedrock Edition UI forms.

#pragma once

#include <endstone/endstone.hpp>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace primebds::utils
{

    struct FormButton
    {
        std::string text;
        std::string icon;
    };

    class ActionFormBuilder
    {
    public:
        ActionFormBuilder &title(const std::string &title);
        ActionFormBuilder &body(const std::string &body);
        ActionFormBuilder &button(const std::string &text, const std::string &icon = "");
        void show(endstone::Player &player,
                  std::function<void(std::optional<int>)> callback);

    private:
        std::string title_;
        std::string body_;
        std::vector<FormButton> buttons_;
    };

    class ModalFormBuilder
    {
    public:
        ModalFormBuilder &title(const std::string &title);
        ModalFormBuilder &dropdown(const std::string &label, const std::vector<std::string> &options,
                                   int default_index = 0);
        ModalFormBuilder &slider(const std::string &label, float min, float max,
                                 float step = 1.0f, float default_val = 0.0f);
        ModalFormBuilder &textInput(const std::string &label, const std::string &placeholder = "",
                                    const std::string &default_val = "");
        ModalFormBuilder &toggle(const std::string &label, bool default_val = false);
        void show(endstone::Player &player,
                  std::function<void(std::optional<std::vector<std::variant<int, float, std::string, bool>>>)> callback);

    private:
        std::string title_;
        struct FormElement
        {
            std::string type;
            std::string label;
            std::vector<std::string> options;
            float min = 0, max = 0, step = 1, default_float = 0;
            std::string placeholder, default_string;
            bool default_bool = false;
            int default_int = 0;
        };
        std::vector<FormElement> elements_;
    };

    class MessageFormBuilder
    {
    public:
        MessageFormBuilder &title(const std::string &title);
        MessageFormBuilder &body(const std::string &body);
        MessageFormBuilder &button1(const std::string &text);
        MessageFormBuilder &button2(const std::string &text);
        void show(endstone::Player &player,
                  std::function<void(std::optional<bool>)> callback);

    private:
        std::string title_;
        std::string body_;
        std::string button1_;
        std::string button2_;
    };

} // namespace primebds::utils
