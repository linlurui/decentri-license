{
  "targets": [
    {
      "target_name": "decenlicense_node",
      "sources": [
        "src/decenlicense_node.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../dl-core/include",
        "../../dl-core/third-party/asio"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ ],
      "cflags_cc!": [ ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ],
      "libraries": [
        "-L<(module_root_dir)/../../dl-core/build",
        "-ldecentrilicense"
      ],
      "link_settings": {
        "libraries": [
          "-L<(module_root_dir)/../../dl-core/build",
          "-ldecentrilicense"
        ]
      },
      "conditions": [
        ["OS==\"mac\"", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_RTTI": "YES",
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
            "LD_RUNPATH_SEARCH_PATHS": [
              "@loader_path",
              "@loader_path/../../../../dl-core/build",
              "<(module_root_dir)/../../dl-core/build"
            ]
          }
        }]
      ]
    }
  ]
}