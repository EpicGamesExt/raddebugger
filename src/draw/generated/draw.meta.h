// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DRAW_META_H
#define DRAW_META_H

typedef struct DR_Tex2DSampleKindNode DR_Tex2DSampleKindNode; struct DR_Tex2DSampleKindNode {DR_Tex2DSampleKindNode *next; R_Tex2DSampleKind v;};
typedef struct DR_XForm2DNode DR_XForm2DNode; struct DR_XForm2DNode {DR_XForm2DNode *next; Mat3x3F32 v;};
typedef struct DR_ClipNode DR_ClipNode; struct DR_ClipNode {DR_ClipNode *next; Rng2F32 v;};
typedef struct DR_TransparencyNode DR_TransparencyNode; struct DR_TransparencyNode {DR_TransparencyNode *next; F32 v;};
#define DR_BucketStackDecls struct{\
DR_Tex2DSampleKindNode *top_tex2d_sample_kind;\
DR_XForm2DNode *top_xform2d;\
DR_ClipNode *top_clip;\
DR_TransparencyNode *top_transparency;\
}
read_only global DR_Tex2DSampleKindNode dr_nil_tex2d_sample_kind = {0, R_Tex2DSampleKind_Nearest};
read_only global DR_XForm2DNode dr_nil_xform2d = {0, {1, 0, 0, 0, 1, 0, 0, 0, 1}};
read_only global DR_ClipNode dr_nil_clip = {0, {0}};
read_only global DR_TransparencyNode dr_nil_transparency = {0, 0};
#define DR_BucketStackInits(b) do{\
(b)->top_tex2d_sample_kind = &dr_nil_tex2d_sample_kind;\
(b)->top_xform2d = &dr_nil_xform2d;\
(b)->top_clip = &dr_nil_clip;\
(b)->top_transparency = &dr_nil_transparency;\
}while(0)
#if 0
internal R_Tex2DSampleKind          dr_push_tex2d_sample_kind(R_Tex2DSampleKind v);
internal Mat3x3F32                  dr_push_xform2d(Mat3x3F32 v);
internal Rng2F32                    dr_push_clip(Rng2F32 v);
internal F32                        dr_push_transparency(F32 v);
internal R_Tex2DSampleKind          dr_pop_tex2d_sample_kind(void);
internal Mat3x3F32                  dr_pop_xform2d(void);
internal Rng2F32                    dr_pop_clip(void);
internal F32                        dr_pop_transparency(void);
internal R_Tex2DSampleKind          dr_top_tex2d_sample_kind(void);
internal Mat3x3F32                  dr_top_xform2d(void);
internal Rng2F32                    dr_top_clip(void);
internal F32                        dr_top_transparency(void);
#endif
#if 0
#define DR_Tex2DSampleKindScope(v)  DeferLoop(dr_push_tex2d_sample_kind(v), dr_pop_tex2d_sample_kind())
#define DR_XForm2DScope(v)          DeferLoop(dr_push_xform2d(v), dr_pop_xform2d())
#define DR_ClipScope(v)             DeferLoop(dr_push_clip(v), dr_pop_clip())
#define DR_TransparencyScope(v)     DeferLoop(dr_push_transparency(v), dr_pop_transparency())
#endif
#endif // DRAW_META_H
