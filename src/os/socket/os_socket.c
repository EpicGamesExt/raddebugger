// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): Helper

internal B32
os_socket_write(OS_Socket *socket, String8 data){
  String8Node node = {0};
  String8List list = {0};
  str8_list_push(&list, &node, data);
  B32 result = os_socket_write(socket, list);
  return(result);
}

#endif
