{
  "targets": [
    {
      "target_name": "pulsar_weave",
      "sources": [
        "native/weave/weave.c",
        "native/weave/nxh.c",
        "native/weave/dagger.c",
        "native/weave/weave_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_compress",
      "sources": [
        "native/compress/compress_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "libraries": [
        "<(module_root_dir)/native/compress/lib/libzstd.a",
        "<(module_root_dir)/native/compress/lib/liblz4.a"
      ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_fileops",
      "sources": [
        "native/fileops/zorya_fileops.c",
        "native/fileops/fileops_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "_GNU_SOURCE"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_hash",
      "sources": [
        "native/hash/nxh.c",
        "native/hash/nxh_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_dagger",
      "sources": [
        "native/hash/nxh.c",
        "native/dagger/dagger.c",
        "native/dagger/dagger_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_pcm",
      "sources": [
        "native/pcm/pcm_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "pulsar_ini",
      "sources": [
        "native/weave/weave.c",
        "native/hash/nxh.c",
        "native/dagger/dagger.c",
        "native/ini/zorya_ini.c",
        "native/ini/ini_napi.c"
      ],
      "include_dirs": [
        "native/include",
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "defines": [
        "NAPI_VERSION=8",
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags": ["-Wall", "-Wextra", "-O3", "-std=c99"],
      "cflags!": ["-fno-exceptions"],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    },
    {
      "target_name": "copy_modules",
      "type": "none",
      "dependencies": [
        "pulsar_weave",
        "pulsar_compress",
        "pulsar_fileops",
        "pulsar_hash",
        "pulsar_dagger",
        "pulsar_pcm",
        "pulsar_ini"
      ],
      "copies": [
        {
          "destination": "<(module_root_dir)/lib/native",
          "files": [
            "<(PRODUCT_DIR)/pulsar_weave.node",
            "<(PRODUCT_DIR)/pulsar_compress.node",
            "<(PRODUCT_DIR)/pulsar_fileops.node",
            "<(PRODUCT_DIR)/pulsar_hash.node",
            "<(PRODUCT_DIR)/pulsar_dagger.node",
            "<(PRODUCT_DIR)/pulsar_pcm.node",
            "<(PRODUCT_DIR)/pulsar_ini.node"
          ]
        }
      ]
    }
  ]
}
