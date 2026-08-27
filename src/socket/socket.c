// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal SOCK_Endpoint
sock_endpoint_from_string_port(String8 address, U16 port)
{
  //- rjf: detect kind
  SOCK_EndpointKind kind = SOCK_EndpointKind_IPv4;
  {
    if(str8_find_needle(address, 0, s(":"), 0) < address.size)
    {
      kind = SOCK_EndpointKind_IPv6;
    }
  }
  
  //- rjf: fill based on format
  SOCK_Endpoint ep = {0};
  ep.port = port;
  ep.kind = kind;
  switch(kind)
  {
    default:
    case SOCK_EndpointKind_IPv4:
    {
      U64 off = 0;
      U64 part_start_off = 0;
      for(;off < 4;)
      {
        U64 part_end_off = str8_find_needle(address, part_start_off+1, s("."), 0);
        String8 part = str8_substr(address, r1u64(part_start_off, part_end_off));
        U64 part_val = u64_from_str8(part, 10);
        ep.address_u8[off] = (U8)part_val;
        part_start_off = part_end_off+1;
        off += 1;
        if(part_end_off >= address.size)
        {
          break;
        }
      }
    }break;
    case SOCK_EndpointKind_IPv6:
    {
      U64 off = 0;
      U64 part_start_off = 0;
      U64 double_colon_off = 0;
      for(;off < 8;)
      {
        U64 part_end_off = str8_find_needle(address, part_start_off+1, s(":"), 0);
        String8 part = str8_substr(address, r1u64(part_start_off, part_end_off));
        U64 part_val = u64_from_str8(part, 16);
        ep.address_u16[off] = (U16)part_val;
        part_start_off = part_end_off+1;
        if(part_end_off+1 < address.size && address.str[part_end_off+1] == ':')
        {
          double_colon_off = off;
        }
        off += 1;
        if(part_end_off >= address.size)
        {
          break;
        }
      }
      if(off < 8 && double_colon_off < 8)
      {
        U64 shift_amt = 8 - off;
        MemoryCopy(&ep.address_u16[0] + double_colon_off + shift_amt, &ep.address_u16[0] + double_colon_off, sizeof(U16) * (off - double_colon_off));
      }
    }break;
  }
}

internal SOCK_Endpoint
sock_endpoint_from_string(String8 address_and_port)
{
  String8 addr = {0};
  U64 port_off = 0;
  if(address_and_port.size > 0 && address_and_port.str[0] == '[')
  {
    U64 addr_end_off = str8_find_needle(address_and_port, 1, s("]"), 0);
    addr = str8_substr(address_and_port, r1u64(1, addr_end_off));
    port_off = str8_find_needle(address_and_port, addr_end_off+1, s(":"), 0)+1;
  }
  else
  {
    U64 colon_off = str8_find_needle(address_and_port, 0, s(":"), 0);
    port_off = colon_off + 1;
    addr = str8_prefix(address_and_port, colon_off);
  }
  String8 port_string = str8_skip(address_and_port, port_off);
  U16 port = u64_from_str8(port_string, 10);
  SOCK_Endpoint ep = sock_endpoint_from_string_port(addr, port);
  return ep;
}
