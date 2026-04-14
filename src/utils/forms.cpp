/// @file forms.cpp
/// Form wrapper utilities for Bedrock Edition UI forms.

#include "primebds/utils/forms.h"

#include <nlohmann/json.hpp>

namespace primebds::utils {

    // --- ActionFormBuilder ---

    ActionFormBuilder &ActionFormBuilder::title(const std::string &t) {
        title_ = t;
        return *this;
    }

    ActionFormBuilder &ActionFormBuilder::body(const std::string &b) {
        body_ = b;
        return *this;
    }

    ActionFormBuilder &ActionFormBuilder::button(const std::string &text, const std::string &icon) {
        buttons_.push_back({text, icon});
        return *this;
    }

    void ActionFormBuilder::show(endstone::Player &player,
                                 std::function<void(std::optional<int>)> callback) {
        endstone::ActionForm form;
        form.setTitle(title_);
        form.setContent(body_);

        for (auto &btn : buttons_)
            form.addButton(btn.text, btn.icon.empty() ? "" : btn.icon);

        auto cb = std::move(callback);
        form.setOnSubmit([cb](endstone::Player *, int selection) {
        if (cb)
            cb(selection); });
        form.setOnClose([cb](endstone::Player *) {
        if (cb)
            cb(std::nullopt); });

        player.sendForm(form);
    }

    // --- ModalFormBuilder ---

    ModalFormBuilder &ModalFormBuilder::title(const std::string &t) {
        title_ = t;
        return *this;
    }

    ModalFormBuilder &ModalFormBuilder::dropdown(const std::string &label,
                                                 const std::vector<std::string> &options,
                                                 int default_index) {
        FormElement el;
        el.type = "dropdown";
        el.label = label;
        el.options = options;
        el.default_int = default_index;
        elements_.push_back(el);
        return *this;
    }

    ModalFormBuilder &ModalFormBuilder::slider(const std::string &label, float min, float max,
                                               float step, float default_val) {
        FormElement el;
        el.type = "slider";
        el.label = label;
        el.min = min;
        el.max = max;
        el.step = step;
        el.default_float = default_val;
        elements_.push_back(el);
        return *this;
    }

    ModalFormBuilder &ModalFormBuilder::textInput(const std::string &label,
                                                  const std::string &placeholder,
                                                  const std::string &default_val) {
        FormElement el;
        el.type = "text_input";
        el.label = label;
        el.placeholder = placeholder;
        el.default_string = default_val;
        elements_.push_back(el);
        return *this;
    }

    ModalFormBuilder &ModalFormBuilder::toggle(const std::string &label, bool default_val) {
        FormElement el;
        el.type = "toggle";
        el.label = label;
        el.default_bool = default_val;
        elements_.push_back(el);
        return *this;
    }

    void ModalFormBuilder::show(endstone::Player &player,
                                std::function<void(std::optional<std::vector<std::variant<int, float, std::string, bool>>>)> callback) {
        endstone::ModalForm form;
        form.setTitle(title_);

        for (auto &el : elements_) {
            if (el.type == "dropdown")
                form.addControl(endstone::Dropdown(el.label, el.options, el.default_int));
            else if (el.type == "slider")
                form.addControl(endstone::Slider(el.label, el.min, el.max, el.step, el.default_float));
            else if (el.type == "text_input")
                form.addControl(endstone::TextInput(el.label, el.placeholder, el.default_string));
            else if (el.type == "toggle")
                form.addControl(endstone::Toggle(el.label, el.default_bool));
        }

        auto cb = std::move(callback);
        form.setOnSubmit([cb](endstone::Player *, const std::string &data) {
        if (!cb)
            return;
        try {
            auto arr = nlohmann::json::parse(data);
            std::vector<std::variant<int, float, std::string, bool>> values;
            for (auto &v : arr) {
                if (v.is_number_integer())
                    values.emplace_back(v.get<int>());
                else if (v.is_number_float())
                    values.emplace_back(v.get<float>());
                else if (v.is_boolean())
                    values.emplace_back(v.get<bool>());
                else if (v.is_string())
                    values.emplace_back(v.get<std::string>());
            }
            cb(values);
        } catch (...) {
            cb(std::nullopt);
        } });
        form.setOnClose([cb](endstone::Player *) {
        if (cb)
            cb(std::nullopt); });

        player.sendForm(form);
    }

    // --- MessageFormBuilder ---

    MessageFormBuilder &MessageFormBuilder::title(const std::string &t) {
        title_ = t;
        return *this;
    }

    MessageFormBuilder &MessageFormBuilder::body(const std::string &b) {
        body_ = b;
        return *this;
    }

    MessageFormBuilder &MessageFormBuilder::button1(const std::string &text) {
        button1_ = text;
        return *this;
    }

    MessageFormBuilder &MessageFormBuilder::button2(const std::string &text) {
        button2_ = text;
        return *this;
    }

    void MessageFormBuilder::show(endstone::Player &player,
                                  std::function<void(std::optional<bool>)> callback) {
        endstone::MessageForm form;
        form.setTitle(title_);
        form.setContent(body_);
        form.setButton1(button1_);
        form.setButton2(button2_);

        auto cb = std::move(callback);
        form.setOnSubmit([cb](endstone::Player *, int selection) {
        if (cb)
            cb(selection == 0); });
        form.setOnClose([cb](endstone::Player *) {
        if (cb)
            cb(std::nullopt); });

        player.sendForm(form);
    }

} // namespace primebds::utils
