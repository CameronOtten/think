#pragma once

#include "types.h"

Rect operator+=(const Rect& A, const Rect& B)
{
    Rect rect;
    rect.bottom = A.bottom+B.bottom;
    rect.top = A.top+B.top;
    rect.left = A.left+B.left;
    rect.right = A.right+B.right;
    return rect;
}
