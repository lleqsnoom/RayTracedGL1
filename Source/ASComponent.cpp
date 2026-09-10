// Copyright (c) 2021 Sultim Tsyrendashiev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ASComponent.h"

#include <algorithm>

namespace
{
VkDeviceSize GetASBufferCapacity( VkDeviceSize requiredSize )
{
    return std::max( requiredSize, requiredSize + requiredSize / 4 + ( 1 << 16 ) );
}
}

RTGL1::ASComponent::ASComponent( VkDevice _device, const char* _debugName )
    : device( _device ), as( VK_NULL_HANDLE ), debugName( _debugName )
{
}

RTGL1::BLASComponent::BLASComponent( VkDevice _device, VertexCollectorFilterTypeFlags _filter )
    : ASComponent( _device, VertexCollectorFilterTypeFlags_GetNameForBLAS( _filter ) )
    , filter( _filter )
    , geomCount( 0 )
{
}

RTGL1::TLASComponent::TLASComponent( VkDevice _device, const char* _debugName )
    : ASComponent( _device, _debugName )
{
}

RTGL1::ASComponent::~ASComponent()
{
    Destroy();
}

void RTGL1::ASComponent::CreateBuffer( const std::shared_ptr< MemoryAllocator >& allocator,
                                       VkDeviceSize                              size )
{
    assert( !buffer.IsInitted() );

    buffer.Init( *allocator,
                 size,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 GetBufferDebugName() );
}

void RTGL1::ASComponent::Destroy()
{
    assert( device != VK_NULL_HANDLE );

    buffer.Destroy();

    if( as != VK_NULL_HANDLE )
    {
        svkDestroyAccelerationStructureKHR( device, as, nullptr );
        as = VK_NULL_HANDLE;
    }

    asAddress = 0;
}

bool RTGL1::ASComponent::RecreateIfNotValid(
    const VkAccelerationStructureBuildSizesInfoKHR& buildSizes,
    const std::shared_ptr< MemoryAllocator >&       allocator )
{
    if( IsValid( buildSizes ) )
    {
        return false;
    }

    // destroy
    Destroy();

    // create
    CreateBuffer( allocator, GetASBufferCapacity( buildSizes.accelerationStructureSize ) );
    CreateAS( buildSizes.accelerationStructureSize );
    return true;
}

void RTGL1::BLASComponent::CreateAS( VkDeviceSize size )
{
    assert( device != VK_NULL_HANDLE );

    VkAccelerationStructureCreateInfoKHR info = {
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = buffer.GetBuffer(),
        .size   = size,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };

    VkResult r = svkCreateAccelerationStructureKHR( device, &info, nullptr, &as );
    VK_CHECKERROR( r );

    SET_DEBUG_NAME( device, as, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, debugName );
}

void RTGL1::TLASComponent::CreateAS( VkDeviceSize size )
{
    assert( device != VK_NULL_HANDLE );

    VkAccelerationStructureCreateInfoKHR tlasInfo = {
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = buffer.GetBuffer(),
        .size   = size,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };

    VkResult r = svkCreateAccelerationStructureKHR( device, &tlasInfo, nullptr, &as );
    VK_CHECKERROR( r );

    SET_DEBUG_NAME( device, as, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, debugName );
}

bool RTGL1::ASComponent::IsValid( const VkAccelerationStructureBuildSizesInfoKHR& buildSizes ) const
{
    return buffer.IsInitted() && buffer.GetSize() >= buildSizes.accelerationStructureSize;
}

bool RTGL1::ASComponent::IsBuildSizesCached( uint64_t signature ) const
{
    return signature != 0 && cachedBuildSizesSignature == signature;
}

const VkAccelerationStructureBuildSizesInfoKHR& RTGL1::ASComponent::GetCachedBuildSizes() const
{
    return cachedBuildSizes;
}

void RTGL1::ASComponent::SetCachedBuildSizes(
    uint64_t signature, const VkAccelerationStructureBuildSizesInfoKHR& sizes )
{
    cachedBuildSizesSignature = signature;
    cachedBuildSizes          = sizes;
}

VkAccelerationStructureKHR RTGL1::ASComponent::GetAS() const
{
    return as;
}

VkDeviceAddress RTGL1::ASComponent::GetASAddress() const
{
    assert( buffer.IsInitted() );

    if( asAddress == 0 )
    {
        asAddress = GetASAddress( as );
    }

    return asAddress;
}

VkDeviceAddress RTGL1::ASComponent::GetASAddress( VkAccelerationStructureKHR as ) const
{
    assert( device != VK_NULL_HANDLE );
    assert( as != VK_NULL_HANDLE );

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = as;

    return svkGetAccelerationStructureDeviceAddressKHR( device, &addressInfo );
}

const char* RTGL1::BLASComponent::GetBufferDebugName() const
{
    return "BLAS buffer";
}

const char* RTGL1::TLASComponent::GetBufferDebugName() const
{
    return "TLAS buffer";
}

RTGL1::VertexCollectorFilterTypeFlags RTGL1::BLASComponent::GetFilter() const
{
    return filter;
}

void RTGL1::BLASComponent::SetGeometryCount( uint32_t geomCount )
{
    this->geomCount = geomCount;
}

bool RTGL1::BLASComponent::IsEmpty() const
{
    return geomCount == 0;
}

uint32_t RTGL1::BLASComponent::GetGeomCount() const
{
    return geomCount;
}

namespace
{
bool SameTriangleGeometry( const VkAccelerationStructureGeometryKHR& a,
                           const VkAccelerationStructureGeometryKHR& b )
{
    if( a.geometryType != b.geometryType || a.flags != b.flags )
    {
        return false;
    }
    if( a.geometryType != VK_GEOMETRY_TYPE_TRIANGLES_KHR )
    {
        return false;
    }

    const auto& ta = a.geometry.triangles;
    const auto& tb = b.geometry.triangles;

    return ta.vertexFormat == tb.vertexFormat &&
           ta.vertexData.deviceAddress == tb.vertexData.deviceAddress &&
           ta.vertexStride == tb.vertexStride && ta.maxVertex == tb.maxVertex &&
           ta.indexType == tb.indexType && ta.indexData.deviceAddress == tb.indexData.deviceAddress &&
           ta.transformData.deviceAddress == tb.transformData.deviceAddress;
}

bool SameRange( const VkAccelerationStructureBuildRangeInfoKHR& a,
                const VkAccelerationStructureBuildRangeInfoKHR& b )
{
    return a.primitiveCount == b.primitiveCount && a.primitiveOffset == b.primitiveOffset &&
           a.firstVertex == b.firstVertex && a.transformOffset == b.transformOffset;
}
}

bool RTGL1::BLASComponent::CanRefit(
    const std::vector< VkAccelerationStructureGeometryKHR >&      geometries,
    const std::vector< VkAccelerationStructureBuildRangeInfoKHR >& ranges ) const
{
    if( !hasLastBuild || geometries.size() != lastGeometries.size() ||
        ranges.size() != lastRanges.size() )
    {
        return false;
    }

    for( size_t i = 0; i < geometries.size(); i++ )
    {
        if( !SameTriangleGeometry( geometries[ i ], lastGeometries[ i ] ) )
        {
            return false;
        }
    }

    for( size_t i = 0; i < ranges.size(); i++ )
    {
        if( !SameRange( ranges[ i ], lastRanges[ i ] ) )
        {
            return false;
        }
    }

    return true;
}

uint32_t RTGL1::BLASComponent::GetFramesSinceFullBuild() const
{
    return framesSinceFullBuild;
}

void RTGL1::BLASComponent::RecordBuild(
    const std::vector< VkAccelerationStructureGeometryKHR >&      geometries,
    const std::vector< VkAccelerationStructureBuildRangeInfoKHR >& ranges,
    bool                                                           wasFullBuild )
{
    lastGeometries.assign( geometries.begin(), geometries.end() );
    lastRanges.assign( ranges.begin(), ranges.end() );
    hasLastBuild = true;

    if( wasFullBuild )
    {
        framesSinceFullBuild = 0;
    }
    else
    {
        framesSinceFullBuild++;
    }
}