test:
{
  artifacts:
  {
    override_entry_obj:
    {
      file_name: "override_entry.obj"
      coff: { object: { machine: x64,
        sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } } }
        symbols: {
          entry: { kind: external_function, name: "entry", section: text, value: 0 }
          early: { kind: undefined, name: "early$fo$" }
          caller: { kind: undefined, name: "late_caller" }
          ordinary: { kind: undefined, name: "foo" }
        }
      } }
    }
    override_caller_lib:
    {
      file_name: "override_caller.lib"
      coff: { library: { second_linker_member: true, members: { caller: {
        path: "late_caller.obj"
        object: { machine: x64,
          directives: { directive: "/alternatename:from_archive$fo$=test" }
          sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } } }
          symbols: {
            caller: { kind: external_function, name: "late_caller", section: text, value: 0 }
            late: { kind: undefined, name: "late$fo$" }
            from_archive: { kind: undefined, name: "from_archive$fo$" }
          }
        }
      } } } }
    }
    override_trap_lib:
    {
      file_name: "override_trap.lib"
      // These archive definitions must not be pulled in before the overrides.
      // Pulling this member exposes an unresolved symbol and fails the link.
      coff: { library: { second_linker_member: true, members: { trap: {
        path: "override_trap.obj"
        object: { machine: x64,
          sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } } }
          symbols: {
            early: { kind: external_function, name: "early$fo$", section: text, value: 0 }
            late: { kind: external_function, name: "late$fo$", section: text, value: 0 }
            from_archive: { kind: external_function, name: "from_archive$fo$", section: text, value: 0 }
            missing: { kind: undefined, name: "override_archive_must_not_be_loaded" }
          }
        }
      } } } }
    }
    test_obj:
    {
      file_name: "test.obj"
      coff: { object: { machine: x64, sections: { data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "test" } } }, symbols: { test: { kind: external, name: "test", section: data, value: 0 } } } }
    }
    foo_obj:
    {
      file_name: "foo.obj"
      coff: { object: { machine: x64, sections: { data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "foo" } } }, symbols: { foo: { kind: external, name: "foo", section: data, value: 0 } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text:
        {
          name: ".text"
          permissions: (read, execute)
          content: code
          // mov rax, $imm
          // ret
          data: { hex: "48c7c000000000c3" }
          relocations: { foo: { type: Addr32Nb, offset: 0, symbol: foo } }
        } }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          foo: { kind: undefined, name: "foo" }
        }
      } }
    }
  }
  build:
  {
    // Cover command-line overrides, duplicate directives, references appearing
    // in a later archive load, and new override directives from that archive.
    link: { args: "/subsystem:console /entry:entry /out:overrides.exe /alternatename:early$fo$=test /alternatename:early$fo$=test /alternatename:late$fo$=test /alternatename:foo=test override_entry.obj test.obj override_caller.lib override_trap.lib" }
    // basic alternate name test
    link: { args: "/subsystem:console /entry:entry /out:alternate_basic.exe /alternatename:foo=test test.obj entry.obj" }
    // linker should not chase alt name links
    link: { args: "/subsystem:console /entry:entry /out:alternate_chain.exe /alternatename:foo=bar /alternatename:bar=test test.obj entry.obj", expect_exit: nonzero }
    // alt name conflict
    link: { args: "/subsystem:console /entry:entry /out:alternate_conflict.exe /alternatename:foo=test /alternatename:foo=qwe test.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_missing_equal.exe /alternatename:foo foo.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_wrong_separator.exe /alternatename:foo-oof foo.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_extra_equal.exe /alternatename:foo=test=bar foo.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_empty_target.exe /alternatename:foo= foo.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_empty_source.exe /alternatename:= foo.obj entry.obj", expect_exit: nonzero }
    // syntax error
    link: { args: "/subsystem:console /entry:entry /out:alternate_empty_option.exe /alternatename: foo.obj entry.obj", expect_exit: nonzero }
    // TODO: check that RAD Linker prints these warnings
    // warn about alt name to self alt name?
    link: { args: "/subsystem:console /entry:entry /out:alternate_self.exe /alternatename:foo=foo foo.obj entry.obj" }
    // warn about alt name to unknown symbol?
    link: { args: "/subsystem:console /entry:entry /out:alternate_unknown.exe /alternatename:qwe=ewq foo.obj entry.obj" }
  }
  steps: {}
}
