//
// AllocationScheme
// MQSim
//
// Created by 文盛業 on 2019-06-13.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#include "AllocationScheme.h"
#include "Address_Mapping_Unit_Base.h"
#include "AddressMappingUnitDefs.h"

using namespace SSD_Components;

force_inline uint16_t
__index(LPA_type lpn, uint32_t div, uint32_t mod)
{
  return (lpn / div) % mod;
}

// =============
// Channel First
// =============
PlaneAllocator::AddressInfo
__CWDP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel,                      in.chip),
    __index(lpn, in.channel * in.chip,            in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__CWPD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel,                      in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, in.channel * in.chip,            in.plane)
  };
}

PlaneAllocator::AddressInfo
__CDWP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel * in.die,             in.chip),
    __index(lpn, in.channel,                      in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__CDPW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, in.channel,                      in.die),
    __index(lpn, in.channel * in.die,             in.plane)
  };
}

PlaneAllocator::AddressInfo
__CPWD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{

  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel * in.plane,           in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, in.channel,                      in.plane)
  };
}

PlaneAllocator::AddressInfo
__CPDW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{

  return {
    __index(lpn, 1,                               in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, in.channel * in.plane,           in.die),
    __index(lpn, in.channel,                      in.plane)
  };
}

// =========
// Way First
// =========
PlaneAllocator::AddressInfo
__WCDP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{

  return {
    __index(lpn, in.chip,                         in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.channel * in.chip,            in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__WCPD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{

  return {
    __index(lpn, in.chip,                         in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, in.channel * in.chip,            in.plane)
  };
}

PlaneAllocator::AddressInfo
__WDCP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{

  return {
    __index(lpn, in.chip * in.die,                in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.chip,                         in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__WDPC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.chip,                         in.die),
    __index(lpn, in.chip * in.die,                in.plane)
  };
}

PlaneAllocator::AddressInfo
__WPCD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.plane,              in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, in.chip,                         in.plane)
  };
}

PlaneAllocator::AddressInfo
__WPDC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, 1,                               in.chip),
    __index(lpn, in.chip * in.plane,              in.die),
    __index(lpn, in.chip,                         in.plane)
  };
}

// =========
// Die First
// =========
PlaneAllocator::AddressInfo
__DCWP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.die,                          in.channel),
    __index(lpn, in.channel * in.die,             in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__DCPW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.die,                          in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.channel * in.die,             in.plane)
  };
}

PlaneAllocator::AddressInfo
__DWCP_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die,                in.channel),
    __index(lpn, in.die,                          in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.channel * in.chip * in.die,   in.plane)
  };
}

PlaneAllocator::AddressInfo
__DWPC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, in.die,                          in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.chip * in.die,                in.plane)
  };
}

PlaneAllocator::AddressInfo
__DPCW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.die * in.plane,               in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.die,                          in.plane)
  };
}

PlaneAllocator::AddressInfo
__DPWC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, in.die * in.plane,               in.chip),
    __index(lpn, 1,                               in.die),
    __index(lpn, in.die,                          in.plane)
  };
}

// ===========
// Plane First
// ===========
PlaneAllocator::AddressInfo
__PCWD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.plane,                        in.channel),
    __index(lpn, in.channel * in.plane,           in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, 1,                               in.plane)
  };
}

PlaneAllocator::AddressInfo
__PCDW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.plane,                        in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, in.channel * in.plane,           in.die),
    __index(lpn, 1,                               in.plane)
  };
}

PlaneAllocator::AddressInfo
__PWCD_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.plane,              in.channel),
    __index(lpn, in.plane,                        in.chip),
    __index(lpn, in.channel * in.chip * in.plane, in.die),
    __index(lpn, 1,                               in.plane)
  };
}

PlaneAllocator::AddressInfo
__PWDC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, in.plane,                        in.chip),
    __index(lpn, in.chip * in.plane,              in.die),
    __index(lpn, 1,                               in.plane)
  };
}

PlaneAllocator::AddressInfo
__PDCW_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.die * in.plane,               in.channel),
    __index(lpn, in.channel * in.die * in.plane,  in.chip),
    __index(lpn, in.plane,                        in.die),
    __index(lpn, 1,                               in.plane)
  };
}

PlaneAllocator::AddressInfo
__PDWC_map(LPA_type lpn, const PlaneAllocator::AddressInfo& in)
{
  return {
    __index(lpn, in.chip * in.die * in.plane,     in.channel),
    __index(lpn, in.die * in.plane,               in.chip),
    __index(lpn, in.plane,                        in.die),
    __index(lpn, 1,                               in.plane)
  };
}

// ===========================
// Plane Allocator Constructor
// ===========================
PlaneAllocator::PlaneAllocator(flash_channel_ID_type* channel_ids,
                               flash_chip_ID_type*    chip_ids,
                               flash_die_ID_type*     die_ids,
                               flash_plane_ID_type*   plane_ids,
                               uint16_t channel_no,
                               uint16_t chip_no,
                               uint16_t die_no,
                               uint16_t plane_no,
                               Flash_Plane_Allocation_Scheme_Type scheme)
  : __channel_ids(channel_ids, &channel_ids[channel_no]),
    __chip_ids(chip_ids, &chip_ids[chip_no]),
    __die_ids(die_ids, &die_ids[die_no]),
    __plane_ids(plane_ids, &plane_ids[plane_no]),
    __structure{channel_no, chip_no, die_no, plane_no}
{
  switch (scheme) {
    // Channel First
    case Flash_Plane_Allocation_Scheme_Type::CWDP: __alloc = __CWDP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::CWPD: __alloc = __CWPD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::CDWP: __alloc = __CDWP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::CDPW: __alloc = __CDPW_map; break;
    case Flash_Plane_Allocation_Scheme_Type::CPWD: __alloc = __CPWD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::CPDW: __alloc = __CPDW_map; break;

    // Way first
    case Flash_Plane_Allocation_Scheme_Type::WCDP: __alloc = __WCDP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::WCPD: __alloc = __WCPD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::WDCP: __alloc = __WDCP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::WDPC: __alloc = __WDPC_map; break;
    case Flash_Plane_Allocation_Scheme_Type::WPCD: __alloc = __WPCD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::WPDC: __alloc = __WPDC_map; break;

    // Die first
    case Flash_Plane_Allocation_Scheme_Type::DCWP: __alloc = __DCWP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::DCPW: __alloc = __DCPW_map; break;
    case Flash_Plane_Allocation_Scheme_Type::DWCP: __alloc = __DWCP_map; break;
    case Flash_Plane_Allocation_Scheme_Type::DWPC: __alloc = __DWPC_map; break;
    case Flash_Plane_Allocation_Scheme_Type::DPCW: __alloc = __DPCW_map; break;
    case Flash_Plane_Allocation_Scheme_Type::DPWC: __alloc = __DPWC_map; break;

    // Plane first
    case Flash_Plane_Allocation_Scheme_Type::PCWD: __alloc = __PCWD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::PCDW: __alloc = __PCDW_map; break;
    case Flash_Plane_Allocation_Scheme_Type::PWCD: __alloc = __PWCD_map; break;
    case Flash_Plane_Allocation_Scheme_Type::PWDC: __alloc = __PWDC_map; break;
    case Flash_Plane_Allocation_Scheme_Type::PDCW: __alloc = __PDCW_map; break;
    case Flash_Plane_Allocation_Scheme_Type::PDWC: __alloc = __PDWC_map; break;

    default: throw mqsim_error("Unknown plane allocation scheme type!");
  }
}