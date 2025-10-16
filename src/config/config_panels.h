// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CONFIG_PANELS_H
#define CONFIG_PANELS_H

typedef struct CFG_PanelNode CFG_PanelNode;
struct CFG_PanelNode
{
  // rjf: links data
  CFG_PanelNode *first;
  CFG_PanelNode *last;
  CFG_PanelNode *next;
  CFG_PanelNode *prev;
  CFG_PanelNode *parent;
  U64 child_count;
  CFG_Node *cfg;
  
  // rjf: split data
  Axis2 split_axis;
  F32 pct_of_parent;
  
  // rjf: tab params
  Side tab_side;
  
  // rjf: which tabs are attached
  CFG_NodePtrList tabs;
  CFG_Node *selected_tab;
};

typedef struct CFG_PanelTree CFG_PanelTree;
struct CFG_PanelTree
{
  CFG_PanelNode *root;
  CFG_PanelNode *focused;
};

typedef struct CFG_PanelNodeRec CFG_PanelNodeRec;
struct CFG_PanelNodeRec
{
  CFG_PanelNode *next;
  S32 push_count;
  S32 pop_count;
};

read_only global CFG_PanelNode cfg_nil_panel_node =
{
  &cfg_nil_panel_node,
  &cfg_nil_panel_node,
  &cfg_nil_panel_node,
  &cfg_nil_panel_node,
  &cfg_nil_panel_node,
  0,
  &cfg_nil_node,
  .selected_tab = &cfg_nil_node,
};

internal CFG_Node *cfg_window_from_cfg(CFG_Node *cfg);
internal CFG_PanelTree cfg_panel_tree_from_cfg(Arena *arena, CFG_Node *cfg_root);
internal CFG_PanelNodeRec cfg_panel_node_rec__depth_first(CFG_PanelNode *root, CFG_PanelNode *panel, U64 sib_off, U64 child_off);
#define cfg_panel_node_rec__depth_first_pre(root, p)     cfg_panel_node_rec__depth_first((root), (p), OffsetOf(CFG_PanelNode, next), OffsetOf(CFG_PanelNode, first))
#define cfg_panel_node_rec__depth_first_pre_rev(root, p) cfg_panel_node_rec__depth_first((root), (p), OffsetOf(CFG_PanelNode, prev), OffsetOf(CFG_PanelNode, last))
internal CFG_PanelNode *cfg_panel_node_from_tree_cfg(CFG_PanelNode *root, CFG_Node *cfg);
internal Rng2F32 cfg_target_rect_from_panel_node_child(Rng2F32 parent_rect, CFG_PanelNode *parent, CFG_PanelNode *panel);
internal Rng2F32 cfg_target_rect_from_panel_node(Rng2F32 root_rect, CFG_PanelNode *root, CFG_PanelNode *panel);

#endif // CONFIG_PANELS_H
