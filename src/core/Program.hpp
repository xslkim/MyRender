#pragma once
#include "Attributes.hpp"
#include "Varyings.hpp"

typedef void    VertexShader(const Attributes* attributes, Varyings* varyings);
typedef half4   FragmentShader(Varyings& varyings);
