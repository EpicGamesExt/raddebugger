test:
{
  // provider.lib is searched before target exists. seed.lib introduces a
  // no-library weak target; promote.lib then replaces it in-place with an
  // earlier search-library weak. No search-chunk entry is appended, so the
  // existing entry must be revisited when provider.lib is searched again.
  artifacts:
  {
    provider_lib:
    {
      file_name: "provider.lib"
      coff: { library: { second_linker_member: true, members: { provider: {
        path: "provider.obj"
        object: { machine: x64,
          sections: { target: {
            name: ".target", permissions: (read), content: initialized_data,
            alignment: 1, data: { text: "provider" }
          } }
          symbols: { target: { kind: external, name: "target", section: target, value: 0 } }
        }
      } } } }
    }
    promote_lib:
    {
      file_name: "promote.lib"
      coff: { library: { second_linker_member: true, members: { promote: {
        path: "promote.obj"
        object: { machine: x64, symbols: {
          load_promote: { kind: absolute, name: "load_promote", value: 1, storage: external }
          fallback: { kind: absolute, name: "promote_fallback", value: 2, storage: external }
          target: { kind: weak, name: "target", fallback: fallback, search: search_library }
        } }
      } } } }
    }
    seed_lib:
    {
      file_name: "seed.lib"
      coff: { library: { second_linker_member: true, members: { seed: {
        path: "seed.obj"
        object: { machine: x64, symbols: {
          load_seed: { kind: absolute, name: "load_seed", value: 1, storage: external }
          load_promote: { kind: undefined, name: "load_promote" }
          fallback: { kind: absolute, name: "seed_fallback", value: 3, storage: external }
          target: { kind: weak, name: "target", fallback: fallback, search: no_library }
        } }
      } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64,
        sections: { text: {
          name: ".text", permissions: (read, execute), content: code,
          alignment: 1, data: { hex: "c3" }
        } }
        symbols: {
          entry: { kind: external_function, name: "entry", section: text, value: 0 }
          load_seed: { kind: undefined, name: "load_seed" }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: {
    args: "/nodefaultlib /subsystem:console /entry:entry /out:a.exe provider.lib promote.lib seed.lib entry.obj"
    artifact: image
  } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: {
    ".target": { data: 70726f7669646572 }
  } } } } }
}
