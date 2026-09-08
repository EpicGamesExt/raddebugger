test:
{
  artifacts:
  {
    exports_obj:
    {
      file_name: "exports.obj"
      coff: { object: { machine: x64,
        sections: { text: {
          name: ".text", permissions: (read, execute), content: code,
          alignment: 1, data: { hex: "c3" }
        } }
        symbols: {
          dotted: { kind: external_function, name: "__RTFM.autortfm_is_context_status", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "exports.dll", pe: {} }
  }
  build: { link: {
    args: "/dll /entry:__RTFM.autortfm_is_context_status /nodefaultlib /out:exports.dll /export:__RTFM.autortfm_is_context_status /export:forwarded=other_dll.symbol exports.obj"
    artifact: image
  } }
  steps: { expect_pe: { artifact: image, expected: { pe: { exports: {
    count: 2
    entries: {
      export_0: { name: "__RTFM.autortfm_is_context_status", forwarder: "" }
      export_1: { name: "forwarded", forwarder: "other_dll.symbol" }
    }
  } } } } }
}
