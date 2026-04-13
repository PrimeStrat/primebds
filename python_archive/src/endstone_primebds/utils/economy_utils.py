from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from endstone_primebds.primebds import PrimeBDS

def get_eco_link(self: "PrimeBDS"):
    umoney = self.server.plugin_manager.get_plugin('umoney')
    if umoney:
        return umoney