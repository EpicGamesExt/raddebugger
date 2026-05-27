// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

bias = (bias^x)&7;
x -= bias;
x *= 2;
x *= x;
x += bias;