test:
{

  // TODO: relocations against undefined section symbols
  artifacts:
  {
    entry_obj:
    {
      file_name: "relocate_undefined_section_entry.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { hex: "48c7c000000000c3" }
            relocations: { caller_ref: { type: Addr32Nb, offset: 3, symbol: caller } }
          }
        }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          caller: { kind: undefined, name: "caller" }
        }
      } }
    }
    caller_obj:
    {
      file_name: "relocate_undefined_section_caller.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          caller:
          {
            name: ".caller"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            flags: (link_comdat)
            data: { zero: 100 }
            relocations:
            {
              target0_ref: { type: Addr32Nb, offset: 0, symbol: target0_section }
              target1_ref: { type: Addr32Nb, offset: 4, symbol: target1_section }
              target2_ref: { type: Addr32Nb, offset: 8, symbol: target2_section }
              target3_ref: { type: Addr32Nb, offset: 12, symbol: target3_section }
              target4_ref: { type: Addr32Nb, offset: 16, symbol: target4_section }
              target5_ref: { type: Addr32Nb, offset: 20, symbol: target5_section }
              target6_ref: { type: Addr32Nb, offset: 24, symbol: target6_section }
              target7_ref: { type: Addr32Nb, offset: 28, symbol: target7_section }
              target8_ref: { type: Addr32Nb, offset: 32, symbol: target8_section }
              target9_ref: { type: Addr32Nb, offset: 36, symbol: target9_section }
              target10_ref: { type: Addr32Nb, offset: 40, symbol: target10_section }
              target11_ref: { type: Addr32Nb, offset: 44, symbol: target11_section }
              target12_ref: { type: Addr32Nb, offset: 48, symbol: target12_section }
              target13_ref: { type: Addr32Nb, offset: 52, symbol: target13_section }
              target14_ref: { type: Addr32Nb, offset: 56, symbol: target14_section }
              target15_ref: { type: Addr32Nb, offset: 60, symbol: target15_section }
              target16_ref: { type: Addr32Nb, offset: 64, symbol: target16_section }
              target17_ref: { type: Addr32Nb, offset: 68, symbol: target17_section }
              target18_ref: { type: Addr32Nb, offset: 72, symbol: target18_section }
              target19_ref: { type: Addr32Nb, offset: 76, symbol: target19_section }
              target20_ref: { type: Addr32Nb, offset: 80, symbol: target20_section }
              target21_ref: { type: Addr32Nb, offset: 84, symbol: target21_section }
              target22_ref: { type: Addr32Nb, offset: 88, symbol: target22_section }
              target23_ref: { type: Addr32Nb, offset: 92, symbol: target23_section }
              target24_ref: { type: Addr32Nb, offset: 96, symbol: target24_section }
            }
          }
        }
        symbols:
        {
          caller_def: { kind: section_definition, section: caller, selection: Any }
          caller: { kind: external, name: "caller", section: caller, value: 0 }
          target0_section: { kind: undefined_section, name: ".target0", value: 1073741888 }
          target1_section: { kind: undefined_section, name: ".target1", value: 1073741888 }
          target2_section: { kind: undefined_section, name: ".target2", value: 1073741888 }
          target3_section: { kind: undefined_section, name: ".target3", value: 1073741888 }
          target4_section: { kind: undefined_section, name: ".target4", value: 1073741888 }
          target5_section: { kind: undefined_section, name: ".target5", value: 1073741888 }
          target6_section: { kind: undefined_section, name: ".target6", value: 1073741888 }
          target7_section: { kind: undefined_section, name: ".target7", value: 1073741888 }
          target8_section: { kind: undefined_section, name: ".target8", value: 1073741888 }
          target9_section: { kind: undefined_section, name: ".target9", value: 1073741888 }
          target10_section: { kind: undefined_section, name: ".target10", value: 1073741888 }
          target11_section: { kind: undefined_section, name: ".target11", value: 1073741888 }
          target12_section: { kind: undefined_section, name: ".target12", value: 1073741888 }
          target13_section: { kind: undefined_section, name: ".target13", value: 1073741888 }
          target14_section: { kind: undefined_section, name: ".target14", value: 1073741888 }
          target15_section: { kind: undefined_section, name: ".target15", value: 1073741888 }
          target16_section: { kind: undefined_section, name: ".target16", value: 1073741888 }
          target17_section: { kind: undefined_section, name: ".target17", value: 1073741888 }
          target18_section: { kind: undefined_section, name: ".target18", value: 1073741888 }
          target19_section: { kind: undefined_section, name: ".target19", value: 1073741888 }
          target20_section: { kind: undefined_section, name: ".target20", value: 1073741888 }
          target21_section: { kind: undefined_section, name: ".target21", value: 1073741888 }
          target22_section: { kind: undefined_section, name: ".target22", value: 1073741888 }
          target23_section: { kind: undefined_section, name: ".target23", value: 1073741888 }
          target24_section: { kind: undefined_section, name: ".target24", value: 1073741888 }
        }
      } }
    }

    target0: { file_name: "relocate_undefined_section_target0.obj", coff: { object: { machine: x64, sections: { target: { name: ".target0", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target0" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target0", section: target, value: 0 } } } } }
    target1: { file_name: "relocate_undefined_section_target1.obj", coff: { object: { machine: x64, sections: { target: { name: ".target1", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target1" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target1", section: target, value: 0 } } } } }
    target2: { file_name: "relocate_undefined_section_target2.obj", coff: { object: { machine: x64, sections: { target: { name: ".target2", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target2" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target2", section: target, value: 0 } } } } }
    target3: { file_name: "relocate_undefined_section_target3.obj", coff: { object: { machine: x64, sections: { target: { name: ".target3", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target3" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target3", section: target, value: 0 } } } } }
    target4: { file_name: "relocate_undefined_section_target4.obj", coff: { object: { machine: x64, sections: { target: { name: ".target4", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target4" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target4", section: target, value: 0 } } } } }
    target5: { file_name: "relocate_undefined_section_target5.obj", coff: { object: { machine: x64, sections: { target: { name: ".target5", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target5" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target5", section: target, value: 0 } } } } }
    target6: { file_name: "relocate_undefined_section_target6.obj", coff: { object: { machine: x64, sections: { target: { name: ".target6", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target6" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target6", section: target, value: 0 } } } } }
    target7: { file_name: "relocate_undefined_section_target7.obj", coff: { object: { machine: x64, sections: { target: { name: ".target7", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target7" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target7", section: target, value: 0 } } } } }
    target8: { file_name: "relocate_undefined_section_target8.obj", coff: { object: { machine: x64, sections: { target: { name: ".target8", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target8" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target8", section: target, value: 0 } } } } }
    target9: { file_name: "relocate_undefined_section_target9.obj", coff: { object: { machine: x64, sections: { target: { name: ".target9", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target9" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target9", section: target, value: 0 } } } } }
    target10: { file_name: "relocate_undefined_section_target10.obj", coff: { object: { machine: x64, sections: { target: { name: ".target10", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target10" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target10", section: target, value: 0 } } } } }
    target11: { file_name: "relocate_undefined_section_target11.obj", coff: { object: { machine: x64, sections: { target: { name: ".target11", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target11" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target11", section: target, value: 0 } } } } }
    target12: { file_name: "relocate_undefined_section_target12.obj", coff: { object: { machine: x64, sections: { target: { name: ".target12", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target12" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target12", section: target, value: 0 } } } } }
    target13: { file_name: "relocate_undefined_section_target13.obj", coff: { object: { machine: x64, sections: { target: { name: ".target13", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target13" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target13", section: target, value: 0 } } } } }
    target14: { file_name: "relocate_undefined_section_target14.obj", coff: { object: { machine: x64, sections: { target: { name: ".target14", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target14" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target14", section: target, value: 0 } } } } }
    target15: { file_name: "relocate_undefined_section_target15.obj", coff: { object: { machine: x64, sections: { target: { name: ".target15", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target15" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target15", section: target, value: 0 } } } } }
    target16: { file_name: "relocate_undefined_section_target16.obj", coff: { object: { machine: x64, sections: { target: { name: ".target16", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target16" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target16", section: target, value: 0 } } } } }
    target17: { file_name: "relocate_undefined_section_target17.obj", coff: { object: { machine: x64, sections: { target: { name: ".target17", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target17" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target17", section: target, value: 0 } } } } }
    target18: { file_name: "relocate_undefined_section_target18.obj", coff: { object: { machine: x64, sections: { target: { name: ".target18", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target18" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target18", section: target, value: 0 } } } } }
    target19: { file_name: "relocate_undefined_section_target19.obj", coff: { object: { machine: x64, sections: { target: { name: ".target19", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target19" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target19", section: target, value: 0 } } } } }
    target20: { file_name: "relocate_undefined_section_target20.obj", coff: { object: { machine: x64, sections: { target: { name: ".target20", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target20" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target20", section: target, value: 0 } } } } }
    target21: { file_name: "relocate_undefined_section_target21.obj", coff: { object: { machine: x64, sections: { target: { name: ".target21", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target21" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target21", section: target, value: 0 } } } } }
    target22: { file_name: "relocate_undefined_section_target22.obj", coff: { object: { machine: x64, sections: { target: { name: ".target22", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target22" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target22", section: target, value: 0 } } } } }
    target23: { file_name: "relocate_undefined_section_target23.obj", coff: { object: { machine: x64, sections: { target: { name: ".target23", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target23" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target23", section: target, value: 0 } } } } }
    target24: { file_name: "relocate_undefined_section_target24.obj", coff: { object: { machine: x64, sections: { target: { name: ".target24", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target24" } } }, symbols: { target_def: { kind: section_definition, section: target, selection: Any }, target: { kind: external, name: "target24", section: target, value: 0 } } } } }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /opt:ref /out:relocate_undefined_section_symbol.exe relocate_undefined_section_entry.obj relocate_undefined_section_caller.obj relocate_undefined_section_target0.obj relocate_undefined_section_target1.obj relocate_undefined_section_target2.obj relocate_undefined_section_target3.obj relocate_undefined_section_target4.obj relocate_undefined_section_target5.obj relocate_undefined_section_target6.obj relocate_undefined_section_target7.obj relocate_undefined_section_target8.obj relocate_undefined_section_target9.obj relocate_undefined_section_target10.obj relocate_undefined_section_target11.obj relocate_undefined_section_target12.obj relocate_undefined_section_target13.obj relocate_undefined_section_target14.obj relocate_undefined_section_target15.obj relocate_undefined_section_target16.obj relocate_undefined_section_target17.obj relocate_undefined_section_target18.obj relocate_undefined_section_target19.obj relocate_undefined_section_target20.obj relocate_undefined_section_target21.obj relocate_undefined_section_target22.obj relocate_undefined_section_target23.obj relocate_undefined_section_target24.obj"
    }
  }

  steps:
  {
    // T_Ok(match_count > 0);
  }
}
