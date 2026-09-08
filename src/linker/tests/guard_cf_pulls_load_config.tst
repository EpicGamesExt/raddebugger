test:
{
  artifacts:
  {
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
    loadcfg_lib:
    {
      file_name: "loadcfg.lib"
      coff: { library:
      {
        second_linker_member: true
        members: { loadcfg_obj:
        {
          path: "loadcfg.obj"
          object:
          {
            machine: x64
            sections: { loadcfg: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, data: { concat: { hex: "40000000", zero: 60 } } } }
            symbols: { load_config_used: { kind: external, name: "_load_config_used", section: loadcfg, value: 0 } }
          }
        } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/nodefaultlib /subsystem:console /entry:entry /out:a.exe /guard:cf entry.obj loadcfg.lib", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe:
  {
    data_directories: { load_config: { file_size: 64 } }
    optional: { @bits_clear(16384) dll_characteristics }
  } } } }
}
