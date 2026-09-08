test:
{
  artifacts:
  {
    debug_obj:
    {
      file_name: "debug.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          debug_t:
          {
            name: ".debug$T"
            permissions: (read)
            content: initialized_data
            flags: (discardable)
            //str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
            data: { hex: "0400000012000515000080000000000000000000000000000a0006150000800000000000" }
          }
          debug_h: { name: ".debug$H", permissions: (read), content: initialized_data, flags: (discardable), data: { hex: "c5c9330100000200a5fdf48ae0caba6e9a76f994668785c388229ba829a5a87e" } }
        }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj"
      output: none
      stderr_matches: "*Warning(*): *: mismatched .debug$H hash count and type count: got 3 hashes for 2 types*"
    }
  }
  steps: {}
}
