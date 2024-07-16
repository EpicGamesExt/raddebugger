// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DRAW_META_H
#define DRAW_META_H

typedef struct D_Tex2DSampleKindNode D_Tex2DSampleKindNode; struct D_Tex2DSampleKindNode {D_Tex2DSampleKindNode *next; R_Tex2DSampleKind v;};
typedef struct D_XForm2DNode D_XForm2DNode; struct D_XForm2DNode {D_XForm2DNode *next; Mat3x3F32 v;};
typedef struct D_ClipNode D_ClipNode; struct D_ClipNode {D_ClipNode *next; Rng2F32 v;};
typedef struct D_TransparencyNode D_TransparencyNode; struct D_TransparencyNode {D_TransparencyNode *next; F32 v;};
#define D_BucketStackDecls struct{\
D_Tex2DSampleKindNode *top_tex2d_sample_kind;\
D_XForm2DNode *top_xform2d;\
D_ClipNode *top_clip;\
D_TransparencyNode *top_transparency;\
}
read_only global D_Tex2DSampleKindNode d_nil_tex2d_sample_kind = {0, R_Tex2DSampleKind_Nearest};
read_only global D_XForm2DNode d_nil_xform2d = {0, {1, 0, 0, 0, 1, 0, 0, 0, 1}};
read_only global D_ClipNode d_nil_clip = {0, {0}};
read_only global D_TransparencyNode d_nil_transparency = {0, 0};
#define D_BucketStackInits(b) do{\
(b)->top_tex2d_sample_kind = &d_nil_tex2d_sample_kind;\
(b)->top_xform2d = &d_nil_xform2d;\
(b)->top_clip = &d_nil_clip;\
(b)->top_transparency = &d_nil_transparency;\
}while(0)
#if 0
internal R_Tex2DSampleKind          d_push_tex2d_sample_kind(R_Tex2DSampleKind v);
internal Mat3x3F32                  d_push_xform2d(Mat3x3F32 v);
internal Rng2F32                    d_push_clip(Rng2F32 v);
internal F32                        d_push_transparency(F32 v);
internal R_Tex2DSampleKind          d_pop_tex2d_sample_kind(void);
internal Mat3x3F32                  d_pop_xform2d(void);
internal Rng2F32                    d_pop_clip(void);
internal F32                        d_pop_transparency(void);
internal R_Tex2DSampleKind          d_top_tex2d_sample_kind(void);
internal Mat3x3F32                  d_top_xform2d(void);
internal Rng2F32                    d_top_clip(void);
internal F32                        d_top_transparency(void);
#endif
#if 0
#define D_Tex2DSampleKindScope(v)   DeferLoop(d_push_tex2d_sample_kind(v), d_pop_tex2d_sample_kind())
#define D_XForm2DScope(v)           DeferLoop(d_push_xform2d(v), d_pop_xform2d())
#define D_ClipScope(v)              DeferLoop(d_push_clip(v), d_pop_clip())
#define D_TransparencyScope(v)      DeferLoop(d_push_transparency(v), d_pop_transparency())
#endif
#endif // DRAW_META_H
