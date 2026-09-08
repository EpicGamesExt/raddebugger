test:
{
  build:
  {
    windows:
    {
      outputs:
      {
        object: "test.obj"
        baseline_exe: "main.exe"
        baseline_pdb: "main.pdb"
      }

      // compile the test target (torture)
      compile:
      {
        output: "%outputs.object%"
        args: "/fsanitize=address /Z7 -DBUILD_GIT_HASH=Stringify(0000000000000000000000000000000000000000) -I%source_dir% /Zc:preprocessor %source_dir%/torture/torture_main.c"
      }

      // single-threaded link
      link:
      {
        output: "%outputs.baseline_exe%"
        args: "%outputs.object% /debug:full /rad_time_stamp:0 /rad_workers:1 /pdbaltpath:main.pdb /rad_log:-all /rad_ignore:74"
        produces: { "%outputs.baseline_pdb%" }
      }

      // read b

      // multi-threaded links
      link:
      {
        output: "%run%.exe"
        args: "%outputs.object% /debug:full /rad_time_stamp:0 /rad_imagealtpath:main.exe /pdbaltpath:main.pdb /rad_log:-all /rad_ignore:74"
        produces: { "%run%.pdb" }
        repeat: 25
        index: run
        parallel: true
      }

      // wait for linkers
    }
  }

  steps:
  {
    repeat:
    {
      count: 25
      index: run
      steps:
      {
        compare_file: { left: "%build.outputs.baseline_exe%", right: "%run%.exe" }
        compare_file: { left: "%build.outputs.baseline_pdb%", right: "%run%.pdb" }
      }
    }
  }
}
