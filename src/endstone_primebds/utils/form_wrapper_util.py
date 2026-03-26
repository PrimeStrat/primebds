
from endstone.form import (
    ActionForm,
    Dropdown,
    ModalForm,
    Slider,
    TextInput,
    Toggle,
    MessageForm,
)

from endstone import Player

import json

class ActionFormResponse:
    """
    Returns data about the player results from an action form.
    """

    def __init__(self, canceled: bool, selection: str):
        self.canceled = canceled
        self.selection = selection


class ActionFormData:
    """
    Builds a simple player form with buttons that let the player take action.

    *@example*

    action_form.py

    ```python
    from endstone._internal.endstone_python import Player
    from ..form_wrapper import (
        ActionFormData,
        ActionFormResponse,
    )

    def example_action_form(player: Player):
        form = ActionFormData()
        form.title("Test Form")
        form.body("This is a test form.")
        form.button("Test", "textures/ui/refresh_light")
        form.button("Test 2", "textures/ui/anvil_icon")
        form.button("Test 3", "textures/ui/refresh_light")

        def submit(player: Player, result: ActionFormResponse):
            if result.canceled:
                player.send_message("§cForm canceled.")
                return
            else:
                player.send_message(f"§aForm result: §f{result.selection}")
                return

        form.show(player).then(
            lambda player=Player, result=ActionFormResponse: submit(player, result)
        )

    ```

    """

    def __init__(self):
        self._form = ActionForm()

    def body(self, body_text: str):
        """
        Method that sets the body text for the modal form.

        :param body_text: The body text to set.
        """
        self._form.content = body_text
        return self

    def button(self, text: str, icon_path: str | None = None):
        """
        Adds a button to this form with an icon from a resource pack.

        :param text: The text to display on the button.
        """
        self._form.add_button(text, icon_path)
        return self

    def show(self, player: Player):
        """
        Creates and shows this modal popup form. Returns asynchronously when the player confirms or cancels the dialog.
        This function can't be called in read-only mode.

        :param player: Player to show this dialog to.
        :raises: This function can throw errors.
        """
        self._callback = None
        self._form.on_submit = lambda p, r: self.__form_submit(
            p, r, result=ActionFormResponse(False, r)
        )
        self._form.on_close = lambda p: self.__form_submit(
            p, 0, result=ActionFormResponse(True, None)
        )
        player.send_form(self._form)
        return self

    def then(self, callback):
        """
        Sets the callback to be called on form submission.

        :param callback: Callback function to be called on form submission.
        """
        self._callback = callback
        return self

    def title(self, title_text: str):
        """
        This builder method sets the title for the modal dialog.

        :param title_text: The title text to set.
        """
        self._form.title = title_text
        return self

    def __form_submit(self, player: Player, i, result):
        """
        Private method to handle form submission.

        :param player: Player who submitted the form.
        :param result: Result of the form submission.
        """
        if self._callback:
            self._callback(player, result)


class ModalFormResponse:
    """
    Returns data about the player results from a modal form.
    """

    def __init__(self, canceled: bool, formValues: list[bool | int | str] | None):
        self.canceled = canceled
        """
        Whether the form was canceled.
        """
        self.formValues = formValues
        """
        A list of the form values submitted by the player.
        """


class ModalFormData:
    """
    Used to create a fully customizable pop-up form for a player.

    *@example*

    modal_form.py

    ```python
    from endstone._internal.endstone_python import Player
    from ..form_wrapper import (
        ModalFormData,
        ModalFormResponse,
    )

    def example_modal_form(player: Player):
        form = ModalFormData()
        options = ["Option 1", "Option 2", "Option 3"]
        form.title("Test Form")
        form.dropdown("Test Dropdown", options)
        form.slider("Test Slider", 0, 100, 1)
        form.text_field("Test Text Field", "Enter text here")
        form.toggle("Test Toggle", False)
        form.submit_button("Booger")

        def submit(player: Player, response: ModalFormResponse):
            if response.canceled:
                player.send_message("§cForm canceled.")
                return
            else:
                player.send_message("§aDropdown: §f" + options[response.formValues[0]])
                player.send_message("§aSlider: §f" + str(response.formValues[1]))
                player.send_message("§aText Field: §f" + response.formValues[2])
                player.send_message("§aToggle: §f" + str(response.formValues[3]))
                return

        form.show(player).then(
            lambda player=Player, response=ModalFormResponse: submit(player, response)
        )

    ```

    """

    def __init__(self):
        self._form = ModalForm()
        self._callback = None

    def dropdown(self, label: str, options: list[str], default_value_index: int = 0):
        """
        Adds a dropdown with choices to the form.

        :param label: The label for the dropdown.
        :param options: The list of options for the dropdown.
        :param default_value_index: The default selected index.
        """
        self._form.add_control(Dropdown(label, options, default_value_index))
        return self

    def slider(
        self,
        label: str,
        minimum_value: float,
        maximum_value: float,
        value_step: float,
        default_value: float = 0,
    ):
        """
        Adds a numeric slider to the form.

        :param label: The label for the slider.
        :param minimum_value: The minimum value of the slider.
        :param maximum_value: The maximum value of the slider.
        :param value_step: The step value of the slider.
        :param default_value: The default value of the slider.
        """
        self._form.add_control(
            Slider(label, minimum_value, maximum_value, value_step, default_value)
        )
        return self

    def submit_button(self, submit_button_text: str):
        """
        Adds a submit button to the form.

        :param submit_button_text: The text for the submit button.
        """
        self._form.submit_button = submit_button_text
        return self

    def text_field(self, label: str, placeholder_text: str, default_value: str = ""):
        """
        Adds a textbox to the form.

        :param label: The label for the textbox.
        :param placeholder_text: The placeholder text for the textbox.
        :param default_value: The default value of the textbox.
        """
        self._form.add_control(TextInput(label, placeholder_text, default_value))
        return self

    def toggle(self, label: str, default_value: bool = False):
        """
        Adds a toggle checkbox button to the form.

        :param label: The label for the toggle.
        :param default_value: The default value of the toggle.
        """
        self._form.add_control(Toggle(label, default_value))
        return self

    def title(self, title_text: str):
        """
        This builder method sets the title for the modal dialog.

        :param title_text: The title text to set.
        """
        self._form.title = title_text
        return self

    def show(self, player: Player):
        """
        Creates and shows this modal popup form. Returns asynchronously when the player confirms or cancels the dialog.
        This function can't be called in read-only mode.

        :param player: Player to show this dialog to.
        :raises: This function can throw errors.
        """
        self._form.on_submit = lambda p, r: self.__form_submit(
            p, r, ModalFormResponse(False, json.loads(r))
        )
        self._form.on_close = lambda p: self.__form_submit(
            p, None, ModalFormResponse(True, None)
        )
        player.send_form(self._form)
        return self

    def then(self, callback):
        """
        Sets the callback to be called on form submission.

        :param callback: Callback function to be called on form submission.
        """
        self._callback = callback
        return self

    def __form_submit(
        self, player: Player, i: str | None, response: ModalFormResponse | None
    ):
        """
        Private method to handle form submission.

        :param player: Player who submitted the form.
        :param result: Result of the form submission.
        :param canceled: Whether the form was canceled.
        """
        if self._callback:
            self._callback(player, response)




class MessageFormResponse:
    def __init__(self, canceled: bool, selection: int | None):
        self.canceled = canceled
        self.selection = selection


class MessageFormData:
    """
    Builds a simple two-button modal dialog.

    Example:
        message_form.py

        ```python
    from endstone._internal.endstone_python import Player
    from ..form_wrapper import (
        MessageFormData,
        MessageFormResponse,
    )

    def example_message_form(player: Player):
        form = MessageFormData()
        form.title("Test Form")
        form.body("This is a test form.")
        form.button1("Okay")
        form.button2("No Thanks")

        def submit(player: Player, response: MessageFormResponse):
            if response.canceled:
                player.send_message("§cForm canceled.")
                return
            else:
                if response.selection == 0:
                    player.send_message("Okay")
                else:
                    player.send_message("No Thanks")
                return

        form.show(player).then(
            lambda player=Player, response=MessageFormResponse: submit(player, response)
        )

        ```
    """

    def __init__(self):
        self._form = MessageForm()
        self._callback = None

    def body(self, body_text: str):
        """
        Method that sets the body text for the modal form.

        :param body_text: The body text to set.
        """
        self._form.content = body_text
        return self

    def button1(self, text: str):
        """
        Method that sets the text for the first button of the dialog.

        :param text: The text for the first button.
        """
        self._form.button1 = text
        return self

    def button2(self, text: str):
        """
        This method sets the text for the second button on the dialog.

        :param text: The text for the second button.
        """
        self._form.button2 = text
        return self

    def title(self, title_text: str):
        """
        This builder method sets the title for the modal dialog.

        :param title_text: The title text to set.
        """
        self._form.title = title_text
        return self

    def show(self, player: Player):
        """
        Creates and shows this modal popup form. Returns asynchronously when the player confirms or cancels the dialog.
        This function can't be called in read-only mode.

        :param player: Player to show this dialog to.
        :raises: This function can throw errors.
        """
        self._form.on_submit = lambda p, r: self.__form_submit(
            p, r, MessageFormResponse(False, r)
        )
        self._form.on_close = lambda p: self.__form_submit(
            p, None, MessageFormResponse(True, None)
        )
        player.send_form(self._form)
        return self

    def then(self, callback):
        """
        Sets the callback to be called on form submission.

        :param callback: Callback function to be called on form submission.
        """
        self._callback = callback
        return self

    def __form_submit(
        self, player: Player, i: str | None, response: MessageFormResponse | None
    ):
        """
        Private method to handle form submission.

        :param player: Player who submitted the form.
        :param result: Result of the form submission.
        :param canceled: Whether the form was canceled.
        """
        if self._callback:
            self._callback(player, response)
