/*
 * Format.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_FORMAT_H
#define LLGL_FORMAT_H


#include "Export.h"
#include <cstdint>


namespace LLGL
{


/* ----- Enumerations ----- */

/**
\brief Hardware vector and pixel format enumeration.
\remarks This enumeration is used for hardware texture formats and vertex attribute formats.
\see TextureDescriptor::format
\see RenderingCapabilities::textureFormats
\see Vulkan counterpart <code>VkFormat</code>: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkFormat.html
\see Direct3D counterpart <code>DXGI_FORMAT</code>: https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx
*/
enum class Format
{
    Undefined,          //!< Undefined format.

    /* --- Color formats --- */
    R8UNorm,            //!< Color format: red 8-bit normalized unsigned integer component.
    R8SNorm,            //!< Color format: red 8-bit normalized signed integer component.
    R8UInt,             //!< Color format: red 8-bit unsigned integer component.
    R8SInt,             //!< Color format: red 8-bit signed integer component.

    R16UNorm,           //!< Color format: red 16-bit normalized unsigned interger component.
    R16SNorm,           //!< Color format: red 16-bit normalized signed interger component.
    R16UInt,            //!< Color format: red 16-bit unsigned interger component.
    R16SInt,            //!< Color format: red 16-bit signed interger component.
    R16Float,           //!< Color format: red 16-bit floating point component.

    R32UInt,            //!< Color format: red 32-bit unsigned interger component.
    R32SInt,            //!< Color format: red 32-bit signed interger component.
    R32Float,           //!< Color format: red 32-bit floating point component.

    RG8UNorm,           //!< Color format: red, green 8-bit normalized unsigned integer components.
    RG8SNorm,           //!< Color format: red, green 8-bit normalized signed integer components.
    RG8UInt,            //!< Color format: red, green 8-bit unsigned integer components.
    RG8SInt,            //!< Color format: red, green 8-bit signed integer components.

    RG16UNorm,          //!< Color format: red, green 16-bit normalized unsigned interger components.
    RG16SNorm,          //!< Color format: red, green 16-bit normalized signed interger components.
    RG16UInt,           //!< Color format: red, green 16-bit unsigned interger components.
    RG16SInt,           //!< Color format: red, green 16-bit signed interger components.
    RG16Float,          //!< Color format: red, green 16-bit floating point components.

    RG32UInt,           //!< Color format: red, green 32-bit unsigned interger components.
    RG32SInt,           //!< Color format: red, green 32-bit signed interger components.
    RG32Float,          //!< Color format: red, green 32-bit floating point components.

    RGB8UNorm,          //!< Color format: red, green, blue 8-bit normalized unsigned integer components. \note Only supported with: OpenGL, Vulkan.
    RGB8SNorm,          //!< Color format: red, green, blue 8-bit normalized signed integer components. \note Only supported with: OpenGL, Vulkan.
    RGB8UInt,           //!< Color format: red, green, blue 8-bit unsigned integer components. \note Only supported with: OpenGL, Vulkan.
    RGB8SInt,           //!< Color format: red, green, blue 8-bit signed integer components. \note Only supported with: OpenGL, Vulkan.

    RGB16UNorm,         //!< Color format: red, green, blue 16-bit normalized unsigned interger components. \note Only supported with: OpenGL, Vulkan.
    RGB16SNorm,         //!< Color format: red, green, blue 16-bit normalized signed interger components. \note Only supported with: OpenGL, Vulkan.
    RGB16UInt,          //!< Color format: red, green, blue 16-bit unsigned interger components. \note Only supported with: OpenGL, Vulkan.
    RGB16SInt,          //!< Color format: red, green, blue 16-bit signed interger components. \note Only supported with: OpenGL, Vulkan.
    RGB16Float,         //!< Color format: red, green, blue 16-bit floating point components. \note Only supported with: OpenGL, Vulkan.

    RGB32UInt,          //!< Color format: red, green, blue 32-bit unsigned interger components.
    RGB32SInt,          //!< Color format: red, green, blue 32-bit signed interger components.
    RGB32Float,         //!< Color format: red, green, blue 32-bit floating point components.

    RGBA8UNorm,         //!< Color format: red, green, blue, alpha 8-bit normalized unsigned integer components.
    RGBA8SNorm,         //!< Color format: red, green, blue, alpha 8-bit normalized signed integer components.
    RGBA8UInt,          //!< Color format: red, green, blue, alpha 8-bit unsigned integer components.
    RGBA8SInt,          //!< Color format: red, green, blue, alpha 8-bit signed integer components.

    RGBA16UNorm,        //!< Color format: red, green, blue, alpha 16-bit normalized unsigned interger components.
    RGBA16SNorm,        //!< Color format: red, green, blue, alpha 16-bit normalized signed interger components.
    RGBA16UInt,         //!< Color format: red, green, blue, alpha 16-bit unsigned interger components.
    RGBA16SInt,         //!< Color format: red, green, blue, alpha 16-bit signed interger components.
    RGBA16Float,        //!< Color format: red, green, blue, alpha 16-bit floating point components.

    RGBA32UInt,         //!< Color format: red, green, blue, alpha 32-bit unsigned interger components.
    RGBA32SInt,         //!< Color format: red, green, blue, alpha 32-bit signed interger components.
    RGBA32Float,        //!< Color format: red, green, blue, alpha 32-bit floating point components.

    /* --- Extended color formats --- */
    R64Float,           //!< Color format: red 64-bit floating point component. \note Only supported with: OpenGL, Vulkan.
    RG64Float,          //!< Color format: red, green 64-bit floating point components. \note Only supported with: OpenGL, Vulkan.
    RGB64Float,         //!< Color format: red, green, blue 64-bit floating point components. \note Only supported with: OpenGL, Vulkan.
    RGBA64Float,        //!< Color format: red, green, blue, alpha 64-bit floating point components. \note Only supported with: OpenGL, Vulkan.

    /* --- Depth-stencil formats --- */
    D16UNorm,           //!< Depth-stencil format: depth 16-bit normalized unsigned integer component.
    D24UNormS8UInt,     //!< Depth-stencil format: depth 24-bit normalized unsigned integer component, and 8-bit unsigned integer stencil component.
    D32Float,           //!< Depth-stencil format: depth 32-bit floating point component.
    D32FloatS8X24UInt,  //!< Depth-stencil format: depth 32-bit floating point component, and 8-bit unsigned integer stencil components (where the remaining 24 bits are unused).

    /* --- Compressed color formats --- */
    BC1RGB,             //!< Compressed color format: RGB S3TC DXT1 with 8 bytes per 4x4 block. \note Only supported with: OpenGL, Vulkan.
    BC1RGBA,            //!< Compressed color format: RGBA S3TC DXT1 with 8 bytes per 4x4 block.
    BC2RGBA,            //!< Compressed color format: RGBA S3TC DXT3 with 16 bytes per 4x4 block.
    BC3RGBA,            //!< Compressed color format: RGBA S3TC DXT5 with 16 bytes per 4x4 block.
};

/**
\brief Renderer data types enumeration.
\see SrcImageDescriptor::dataType
*/
enum class DataType
{
    Int8,       //!< 8-bit signed integer (char).
    UInt8,      //!< 8-bit unsigned integer (unsigned char).

    Int16,      //!< 16-bit signed integer (short).
    UInt16,     //!< 16-bit unsigned integer (unsigned short).

    Int32,      //!< 32-bit signed integer (int).
    UInt32,     //!< 32-bit unsigned integer (unsiged int).

    #if 1//TODO: replace this by "Float16", etc.
    Float,      //!< 32-bit floating-point (float).
    Double,     //!< 64-bit real type (double).
    #else
    Float16,    //!< 16-bit floating-point (half).
    Float32,    //!< 32-bit floating-point (float).
    Float64,    //!< 32-bit floating-point (double).
    #endif
};

#if 0//TODO: replace by "Format" enum
/**
\brief Renderer vector types enumeration.
\see VertexAttribute::vectorType
\todo Replace by "Format" enum.
*/
enum class VectorType
{
    Float,      //!< 1-Dimensional single precision floating-point vector (float in GLSL, float in HLSL).
    Float2,     //!< 2-Dimensional single precision floating-point vector (vec2 in GLSL, float2 in HLSL).
    Float3,     //!< 3-Dimensional single precision floating-point vector (vec3 in GLSL, float3 in HLSL).
    Float4,     //!< 4-Dimensional single precision floating-point vector (vec4 in GLSL, float4 in HLSL).
    Double,     //!< 1-Dimensional double precision floating-point vector (double in GLSL, double in HLSL).
    Double2,    //!< 2-Dimensional double precision floating-point vector (dvec2 in GLSL, double2 in HLSL).
    Double3,    //!< 3-Dimensional double precision floating-point vector (dvec3 in GLSL, double3 in HLSL).
    Double4,    //!< 4-Dimensional double precision floating-point vector (dvec4 in GLSL, double4 in HLSL).
    Int,        //!< 1-Dimensional signed integer vector (int in GLSL, int in HLSL).
    Int2,       //!< 2-Dimensional signed integer vector (ivec2 in GLSL, int2 in HLSL).
    Int3,       //!< 3-Dimensional signed integer vector (ivec3 in GLSL, int3 in HLSL).
    Int4,       //!< 4-Dimensional signed integer vector (ivec4 in GLSL, int4 in HLSL).
    UInt,       //!< 1-Dimensional unsigned integer vector (uint in GLSL, uint in HLSL).
    UInt2,      //!< 2-Dimensional unsigned integer vector (uvec2 in GLSL, uint2 in HLSL).
    UInt3,      //!< 3-Dimensional unsigned integer vector (uvec3 in GLSL, uint3 in HLSL).
    UInt4,      //!< 4-Dimensional unsigned integer vector (uvec4 in GLSL, uint4 in HLSL).
};
#endif


/* ----- Functions ----- */

/**
\defgroup group_format_util Hardware format utility functions.
\addtogroup group_format_util
@{
*/

#if 0 // TODO: replace by "Format"-specific functions

//! Returns the size (in bytes) of the specified vector type.
LLGL_EXPORT std::uint32_t VectorTypeSize(const VectorType vectorType);

/**
\brief Retrieves the format of the specified vector type.
\param[in] vectorType Specifies the vector type whose format is to be retrieved.
\param[out] dataType Specifies the output parameter for the resulting data type.
\param[out] components Specifiefs the output parameter for the resulting number of vector components.
*/
LLGL_EXPORT void VectorTypeFormat(const VectorType vectorType, DataType& dataType, std::uint32_t& components);

#else

/**
\brief Returns the bit size of the specified hardware format.
\return Number of bits for one vector of the specified hardware format.
\remarks This function does not return the size in bytes because some compressed block formats require less than one byte for a single color vector.
\see Format
*/
LLGL_EXPORT std::uint32_t FormatBitSize(const Format format);

/**
\brief Splits the specified hardware format into a data type and the number of components.
\see DataType
\see Format
*/
LLGL_EXPORT bool SplitFormat(const Format format, DataType& dataType, std::uint32_t& components);

#endif // /TODO

/**
\brief Returns true if the specified hardware format is a compressed format,
i.e. either Format::BC1RGB, Format::BC1RGBA, Format::BC2RGBA, or Format::BC3RGBA.
\see Format
*/
LLGL_EXPORT bool IsCompressedFormat(const Format format);

/**
\brief Returns true if the specified hardware format is a depth or depth-stencil format,
i.e. either Format::DepthComponent, or Format::DepthStencil.
\see Format
*/
LLGL_EXPORT bool IsDepthStencilFormat(const Format format);

/**
\brief Returns true if the specified hardware format is a normalized format (like Format::RGBA8UNorm, Format::R8SNorm etc.).
\remarks This does not include depth-stencil formats or compressed formats.
\see IsDepthStencilFormat
\see IsCompressedFormat
\see Format
*/
LLGL_EXPORT bool IsNormalizedFormat(const Format format);

/**
\brief Returns true if the specified hardware format is an integral format (like Format::RGBA8UInt, Format::R8SInt etc.).
\remarks This also includes all normalized formats.
\see IsNormalizedFormat
\see Format
*/
LLGL_EXPORT bool IsIntegralFormat(const Format format);

/**
\brief Returns true if the specified hardware format is a floating-point format (like Format::RGBA32Float, Format::R32Float etc.).
\remarks This does not include depth-stencil formats or compressed formats.
\see IsDepthStencilFormat
\see IsCompressedFormat
\see Format
*/
LLGL_EXPORT bool IsFloatFormat(const Format format);

/** @} */

/**
\defgroup group_datatype_util Data type utility functions.
\addtogroup group_datatype_util
@{
*/

//! Returns the size (in bytes) of the specified data type.
LLGL_EXPORT std::uint32_t DataTypeSize(const DataType dataType);

/**
\brief Determines if the argument refers to a signed integer data type.
\return True if the specified data type equals one of the following enumeration entries: DataType::Int8, DataType::Int16, DataType::Int32.
*/
LLGL_EXPORT bool IsIntDataType(const DataType dataType);

/**
\brief Determines if the argument refers to an unsigned integer data type.
\return True if the specified data type equals one of the following enumeration entries: DataType::UInt8, DataType::UInt16, DataType::UInt32.
*/
LLGL_EXPORT bool IsUIntDataType(const DataType dataType);

/**
\brief Determines if the argument refers to a floating-pointer data type.
\return True if the specified data type equals one of the following enumeration entries: DataType::Float, DataType::Double.
*/
LLGL_EXPORT bool IsFloatDataType(const DataType dataType);

/** @} */


} // /namespace LLGL


#endif



// ================================================================================
