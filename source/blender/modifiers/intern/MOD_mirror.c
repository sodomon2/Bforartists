/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2005 by the Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup modifiers
 */

#include "BLI_math.h"

#include "BLT_translation.h"

#include "DNA_defaults.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_screen_types.h"

#include "BKE_context.h"
#include "BKE_deform.h"
#include "BKE_lib_id.h"
#include "BKE_lib_query.h"
#include "BKE_mesh.h"
#include "BKE_mesh_mirror.h"
#include "BKE_modifier.h"
#include "BKE_screen.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "RNA_access.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "MEM_guardedalloc.h"

#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "MOD_modifiertypes.h"
#include "MOD_ui_common.h"

static void initData(ModifierData *md)
{
  MirrorModifierData *mmd = (MirrorModifierData *)md;

  BLI_assert(MEMCMP_STRUCT_AFTER_IS_ZERO(mmd, modifier));

  MEMCPY_STRUCT_AFTER(mmd, DNA_struct_default_get(MirrorModifierData), modifier);
}

static void foreachIDLink(ModifierData *md, Object *ob, IDWalkFunc walk, void *userData)
{
  MirrorModifierData *mmd = (MirrorModifierData *)md;

  walk(userData, ob, (ID **)&mmd->mirror_ob, IDWALK_CB_NOP);
}

static void updateDepsgraph(ModifierData *md, const ModifierUpdateDepsgraphContext *ctx)
{
  MirrorModifierData *mmd = (MirrorModifierData *)md;
  if (mmd->mirror_ob != NULL) {
    DEG_add_object_relation(ctx->node, mmd->mirror_ob, DEG_OB_COMP_TRANSFORM, "Mirror Modifier");
    DEG_add_modifier_to_transform_relation(ctx->node, "Mirror Modifier");
  }
}

static Mesh *mirrorModifier__doMirror(MirrorModifierData *mmd, Object *ob, Mesh *mesh)
{
  Mesh *result = mesh;
  const bool use_correct_order_on_merge = mmd->use_correct_order_on_merge;

  /* check which axes have been toggled and mirror accordingly */
  if (mmd->flag & MOD_MIR_AXIS_X) {
    result = BKE_mesh_mirror_apply_mirror_on_axis_for_modifier(
        mmd, ob, result, 0, use_correct_order_on_merge);
  }
  if (mmd->flag & MOD_MIR_AXIS_Y) {
    Mesh *tmp = result;
    result = BKE_mesh_mirror_apply_mirror_on_axis_for_modifier(
        mmd, ob, result, 1, use_correct_order_on_merge);
    if (tmp != mesh) {
      /* free intermediate results */
      BKE_id_free(NULL, tmp);
    }
  }
  if (mmd->flag & MOD_MIR_AXIS_Z) {
    Mesh *tmp = result;
    result = BKE_mesh_mirror_apply_mirror_on_axis_for_modifier(
        mmd, ob, result, 2, use_correct_order_on_merge);
    if (tmp != mesh) {
      /* free intermediate results */
      BKE_id_free(NULL, tmp);
    }
  }

  return result;
}

static Mesh *modifyMesh(ModifierData *md, const ModifierEvalContext *ctx, Mesh *mesh)
{
  Mesh *result;
  MirrorModifierData *mmd = (MirrorModifierData *)md;

  result = mirrorModifier__doMirror(mmd, ctx->object, mesh);

  if (result != mesh) {
    BKE_mesh_normals_tag_dirty(result);
  }
  return result;
}

static void panel_draw(const bContext *UNUSED(C), Panel *panel)
{
  uiLayout *row, *col; /*bfa - no *sub*/
  uiLayout *layout = panel->layout;
  int toggles_flag = UI_ITEM_R_TOGGLE | UI_ITEM_R_FORCE_BLANK_DECORATE;

  PropertyRNA *prop;
  PointerRNA ob_ptr;
  PointerRNA *ptr = modifier_panel_get_property_pointers(panel, &ob_ptr);

  col = uiLayoutColumn(layout, false);
  uiLayoutSetPropSep(col, true);

  prop = RNA_struct_find_property(ptr, "use_axis");
  row = uiLayoutRowWithHeading(col, true, IFACE_("Axis"));
  uiItemFullR(row, ptr, prop, 0, 0, toggles_flag, IFACE_("X"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 1, 0, toggles_flag, IFACE_("Y"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 2, 0, toggles_flag, IFACE_("Z"), ICON_NONE);

  prop = RNA_struct_find_property(ptr, "use_bisect_axis");
  row = uiLayoutRowWithHeading(col, true, IFACE_("Bisect"));
  uiItemFullR(row, ptr, prop, 0, 0, toggles_flag, IFACE_("X"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 1, 0, toggles_flag, IFACE_("Y"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 2, 0, toggles_flag, IFACE_("Z"), ICON_NONE);

  prop = RNA_struct_find_property(ptr, "use_bisect_flip_axis");
  row = uiLayoutRowWithHeading(col, true, IFACE_("Flip"));
  uiItemFullR(row, ptr, prop, 0, 0, toggles_flag, IFACE_("X"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 1, 0, toggles_flag, IFACE_("Y"), ICON_NONE);
  uiItemFullR(row, ptr, prop, 2, 0, toggles_flag, IFACE_("Z"), ICON_NONE);

  uiItemS(col);

  uiItemR(col, ptr, "mirror_object", 0, NULL, ICON_NONE);

  /*------------------- bfa - original props */
  // uiItemR(col, ptr, "use_clip", 0, IFACE_("Clipping"), ICON_NONE);

  col = uiLayoutColumn(layout, true);
  row = uiLayoutRow(col, true);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_clip", 0, IFACE_("Clipping"), ICON_NONE);
  uiItemDecoratorR(row, ptr, "use_clip", 0); /*bfa - decorator*/

  /* ------------ end bfa */

  /*------------------- bfa - original props */
  //row = uiLayoutRowWithHeading(col, true, IFACE_("Merge"));
  //uiItemR(row, ptr, "use_mirror_merge", 0, "", ICON_NONE);
  //sub = uiLayoutRow(row, true);
  //uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_mirror_merge"));
  //uiItemR(sub, ptr, "merge_threshold", 0, "", ICON_NONE);

  // ------------------ bfa new left aligned prop with triangle button to hide the slider

  /* NOTE: split amount here needs to be synced with normal labels */
  uiLayout *split = uiLayoutSplit(layout, 0.385f, true);

  /* FIRST PART ................................................ */
  row = uiLayoutRow(split, false);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_mirror_merge", 0, "Merge", ICON_NONE);
  uiItemDecoratorR(row, ptr, "use_mirror_merge", 0); /*bfa - decorator*/

  /* SECOND PART ................................................ */
  row = uiLayoutRow(split, false);
  if (RNA_boolean_get(ptr, "use_mirror_merge")) {
    uiLayoutSetPropSep(row, true);
    uiItemR(row, ptr, "merge_threshold", toggles_flag, "", ICON_NONE);
  }
  else {
    uiItemL(row, TIP_(""), ICON_DISCLOSURE_TRI_RIGHT);
  }
  // ------------------------------- end bfa

  bool is_bisect_set[3];
  RNA_boolean_get_array(ptr, "use_bisect_axis", is_bisect_set);

  col = uiLayoutRow(layout, true); /*bfa - col, not sub*/
  uiLayoutSetPropSep(col, true);  /* bfa - use_property_split = true */
  uiLayoutSetActive(col, is_bisect_set[0] || is_bisect_set[1] || is_bisect_set[2]);
  uiItemR(col, ptr, "bisect_threshold", 0, IFACE_("Bisect Distance"), ICON_NONE);

  modifier_panel_end(layout, ptr);
}

static void data_panel_draw(const bContext *UNUSED(C), Panel *panel)
{
  uiLayout *col, *row; /*bfa - no *sub*/
  uiLayout *layout = panel->layout;

  PointerRNA *ptr = modifier_panel_get_property_pointers(panel, NULL);

  uiLayoutSetPropSep(layout, true);

  col = uiLayoutColumn(layout, true);

/*------------------- bfa - original props */
  //row = uiLayoutRowWithHeading(col, true, IFACE_("Mirror U"));
  //uiLayoutSetPropDecorate(row, false);
  //sub = uiLayoutRow(row, true);
  //uiItemR(sub, ptr, "use_mirror_u", 0, "", ICON_NONE);
  //sub = uiLayoutRow(sub, true);
  //uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_mirror_u"));
  //uiItemR(sub, ptr, "mirror_offset_u", UI_ITEM_R_SLIDER, "", ICON_NONE);
  //uiItemDecoratorR(row, ptr, "mirror_offset_u", 0);

  // ------------------ bfa new left aligned prop with triangle button to hide the slider

  /* NOTE: split amount here needs to be synced with normal labels */
  uiLayout *split = uiLayoutSplit(col, 0.385f, true);

  /* FIRST PART ................................................ */
  row = uiLayoutRow(split, false);
  uiLayoutSetPropDecorate(row, false);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_mirror_u", 0, "Mirror U", ICON_NONE);

  /* SECOND PART ................................................ */
  row = uiLayoutRow(split, false);
  if (RNA_boolean_get(ptr, "use_mirror_u")) {
    uiItemR(row, ptr, "mirror_offset_u", UI_ITEM_R_SLIDER, "", ICON_NONE);
  }
  else {
    uiItemL(row, TIP_(""), ICON_DISCLOSURE_TRI_RIGHT);
  }

  // ------------------------------- end bfa

  /*------------------- bfa - original props */
  //row = uiLayoutRowWithHeading(col, true, IFACE_("V"));
  //uiLayoutSetPropDecorate(row, false);
  //sub = uiLayoutRow(row, true);
  //uiItemR(sub, ptr, "use_mirror_v", 0, "", ICON_NONE);
  //sub = uiLayoutRow(sub, true);
  //uiLayoutSetActive(sub, RNA_boolean_get(ptr, "use_mirror_v"));
  //uiItemR(sub, ptr, "mirror_offset_v", UI_ITEM_R_SLIDER, "", ICON_NONE);
  //uiItemDecoratorR(row, ptr, "mirror_offset_v", 0);

  // ------------------ bfa new left aligned prop with triangle button to hide the slider

  /* NOTE: split amount here needs to be synced with normal labels */
  split = uiLayoutSplit(col, 0.385f, true);

  /* FIRST PART ................................................ */
  row = uiLayoutRow(split, false);
  uiLayoutSetPropDecorate(row, false);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_mirror_v", 0, "V", ICON_NONE);

  /* SECOND PART ................................................ */
  row = uiLayoutRow(split, false);
  if (RNA_boolean_get(ptr, "use_mirror_v")) {
    uiItemR(row, ptr, "mirror_offset_v", UI_ITEM_R_SLIDER, "", ICON_NONE);
  }
  else {
    uiItemL(row, TIP_(""), ICON_DISCLOSURE_TRI_RIGHT);
  }

  // ------------------------------- end bfa

  col = uiLayoutColumn(layout, true);
  uiItemR(col, ptr, "offset_u", UI_ITEM_R_SLIDER, IFACE_("Offset U"), ICON_NONE);
  uiItemR(col, ptr, "offset_v", UI_ITEM_R_SLIDER, IFACE_("V"), ICON_NONE);

  /*------------------- bfa - original props */
  //uiItemR(layout, ptr, "use_mirror_vertex_groups", 0, IFACE_("Vertex Groups"), ICON_NONE);
  //uiItemR(layout, ptr, "use_mirror_udim", 0, IFACE_("Flip UDIM"), ICON_NONE);

  col = uiLayoutColumn(layout, true);
  row = uiLayoutRow(col, true);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_mirror_vertex_groups", 0, IFACE_("Vertex Groups"), ICON_NONE);
  uiItemDecoratorR(row, ptr, "use_mirror_vertex_groups", 0); /*bfa - decorator*/

  row = uiLayoutRow(col, true);
  uiLayoutSetPropSep(row, false); /* bfa - use_property_split = False */
  uiItemR(row, ptr, "use_mirror_udim", 0, IFACE_("Flip UDIM"), ICON_NONE);
  uiItemDecoratorR(row, ptr, "use_mirror_udim", 0); /*bfa - decorator*/

  /* ------------ end bfa */
}

static void panelRegister(ARegionType *region_type)
{
  PanelType *panel_type = modifier_panel_register(region_type, eModifierType_Mirror, panel_draw);
  modifier_subpanel_register(region_type, "data", "Data", NULL, data_panel_draw, panel_type);
}

ModifierTypeInfo modifierType_Mirror = {
    /* name */ "Mirror",
    /* structName */ "MirrorModifierData",
    /* structSize */ sizeof(MirrorModifierData),
    /* srna */ &RNA_MirrorModifier,
    /* type */ eModifierTypeType_Constructive,
    /* flags */ eModifierTypeFlag_AcceptsMesh | eModifierTypeFlag_SupportsMapping |
        eModifierTypeFlag_SupportsEditmode | eModifierTypeFlag_EnableInEditmode |
        eModifierTypeFlag_AcceptsCVs |
        /* this is only the case when 'MOD_MIR_VGROUP' is used */
        eModifierTypeFlag_UsesPreview,
    /* icon */ ICON_MOD_MIRROR,

    /* copyData */ BKE_modifier_copydata_generic,

    /* deformVerts */ NULL,
    /* deformMatrices */ NULL,
    /* deformVertsEM */ NULL,
    /* deformMatricesEM */ NULL,
    /* modifyMesh */ modifyMesh,
    /* modifyHair */ NULL,
    /* modifyGeometrySet */ NULL,

    /* initData */ initData,
    /* requiredDataMask */ NULL,
    /* freeData */ NULL,
    /* isDisabled */ NULL,
    /* updateDepsgraph */ updateDepsgraph,
    /* dependsOnTime */ NULL,
    /* dependsOnNormals */ NULL,
    /* foreachIDLink */ foreachIDLink,
    /* foreachTexLink */ NULL,
    /* freeRuntimeData */ NULL,
    /* panelRegister */ panelRegister,
    /* blendWrite */ NULL,
    /* blendRead */ NULL,
};
