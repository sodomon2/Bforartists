# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>
from bpy.types import Panel


class CollectionButtonsPanel:
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "collection"

    @classmethod
    def poll(cls, context):
        return context.collection != context.scene.collection


def lineart_make_line_type_entry(col, line_type, text_disp, expand, search_from):
    col.prop(line_type, "use", text=text_disp)
    if line_type.use and expand:
        col.prop_search(line_type, "layer", search_from,
                        "layers", icon='GREASEPENCIL')
        col.prop_search(line_type, "material",  search_from,
                        "materials", icon='SHADING_TEXTURE')


class COLLECTION_PT_collection_flags(CollectionButtonsPanel, Panel):
    bl_label = "Restrictions"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False

        collection = context.collection
        vl = context.view_layer
        vlc = vl.active_layer_collection

        col = layout.column(align=True)
        col.prop(collection, "hide_select", text="Selectable", toggle=False, invert_checkbox=True)
        col.prop(collection, "hide_render", toggle=False)

        col = layout.column(align=True)
        col.prop(vlc, "holdout", toggle=False)
        col.prop(vlc, "indirect_only", toggle=False)


class COLLECTION_PT_instancing(CollectionButtonsPanel, Panel):
    bl_label = "Instancing"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        collection = context.collection

        row = layout.row()
        row.prop(collection, "instance_offset")


class COLLECTION_PT_lineart_collection(CollectionButtonsPanel, Panel):
    bl_label = "Line Art"
    bl_order = 10

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        collection = context.collection

        row = layout.row()
        row.prop(collection, "lineart_usage")

        split = layout.split()
        col = split.column()
        col.use_property_split = False
        col.prop(collection, "lineart_use_intersection_mask", text="Collection Mask")
        col = split.column()
        if collection.lineart_use_intersection_mask:
            col.label(icon='DISCLOSURE_TRI_DOWN')
        else:
            col.label(icon='DISCLOSURE_TRI_RIGHT')

        if collection.lineart_use_intersection_mask:
            split = layout.split(factor = 0.2)
            split.use_property_split = False
            col = split.column()
            row = col.row()
            row.separator()
            row.label(text = "Masks")

            col = split.column()
            row = col.row(align = True)
            for i in range(8):
                row.prop(collection, "lineart_intersection_mask", index=i, text="", toggle=True)


classes = (
    COLLECTION_PT_collection_flags,
    COLLECTION_PT_instancing,
    COLLECTION_PT_lineart_collection,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
